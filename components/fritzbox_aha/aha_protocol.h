#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include "expat.h"
#include "mbedtls/md.h"
#include "mbedtls/platform_util.h"

namespace esphome::fritzbox_aha::protocol {

inline std::string trim(const std::string &s) {
  auto a = s.find_first_not_of(" \t\r\n");
  return a == std::string::npos ? "" : s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
}

inline bool number(const std::string &s, uint32_t &n, uint32_t limit) {
  n = 0;
  if (s.empty()) return false;
  for (unsigned char c : s) {
    if (c < '0' || c > '9' || n > limit / 10 || (n == limit / 10 && c - '0' > limit % 10)) return false;
    n = n * 10 + c - '0';
  }
  return true;
}

inline bool hex_decode(const std::string &s, std::vector<uint8_t> &bytes) {
  if (s.empty() || s.size() % 2 || s.size() > 128) return false;
  auto digit = [](char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
  };
  bytes.clear();
  for (size_t i = 0; i < s.size(); i += 2) {
    int a = digit(s[i]), b = digit(s[i + 1]);
    if (a < 0 || b < 0) return false;
    bytes.push_back((a << 4) | b);
  }
  return true;
}

inline std::string hex(const uint8_t *data, size_t size) {
  static const char *digits = "0123456789abcdef";
  std::string out;
  out.reserve(size * 2);
  for (size_t i = 0; i < size; i++) { out += digits[data[i] >> 4]; out += digits[data[i] & 15]; }
  return out;
}

inline bool valid_sid(const std::string &sid) {
  return sid.size() == 16 && sid != "0000000000000000" &&
         sid.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos;
}

inline std::string form_encode(const std::string &s) {
  static const char *digits = "0123456789ABCDEF";
  std::string out;
  for (unsigned char c : s) {
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') out += c;
    else { out += '%'; out += digits[c >> 4]; out += digits[c & 15]; }
  }
  return out;
}

// One-block PBKDF2-HMAC-SHA256. Periodic yielding keeps the ESP32 idle tasks alive.
inline bool pbkdf2(const uint8_t *password, size_t length, const std::vector<uint8_t> &salt,
                   uint32_t iterations, uint8_t out[32], void (*yield_fn)()) {
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  auto info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  uint8_t u[32] = {}, next[32] = {};
  const uint8_t block[4] = {0, 0, 0, 1};
  bool ok = info && iterations && mbedtls_md_setup(&ctx, info, 1) == 0 &&
            mbedtls_md_hmac_starts(&ctx, password, length) == 0 &&
            mbedtls_md_hmac_update(&ctx, salt.data(), salt.size()) == 0 &&
            mbedtls_md_hmac_update(&ctx, block, sizeof(block)) == 0 &&
            mbedtls_md_hmac_finish(&ctx, u) == 0;
  if (ok) std::memcpy(out, u, 32);
  for (uint32_t i = 1; ok && i < iterations; i++) {
    ok = mbedtls_md_hmac_reset(&ctx) == 0 && mbedtls_md_hmac_update(&ctx, u, 32) == 0 &&
         mbedtls_md_hmac_finish(&ctx, next) == 0;
    if (ok) {
      std::memcpy(u, next, 32);
      for (size_t j = 0; j < 32; j++) out[j] ^= u[j];
    }
    if (yield_fn && (i % 256 == 0)) yield_fn();
  }
  mbedtls_platform_zeroize(u, sizeof(u));
  mbedtls_platform_zeroize(next, sizeof(next));
  mbedtls_md_free(&ctx);
  return ok;
}

inline bool make_response(const std::string &challenge, const std::string &password,
                          std::string &response, std::string &method, void (*yield_fn)()) {
  response.clear();
  if (challenge.rfind("2$", 0) == 0) {
    method = "PBKDF2";
    std::vector<std::string> fields;
    size_t at = 0;
    while (true) {
      auto end = challenge.find('$', at);
      fields.push_back(challenge.substr(at, end == std::string::npos ? end : end - at));
      if (end == std::string::npos) break;
      at = end + 1;
    }
    uint32_t iter1, iter2;
    std::vector<uint8_t> salt1, salt2;
    if (fields.size() != 5 || !number(fields[1], iter1, 1000000) || !iter1 ||
        !number(fields[3], iter2, 1000000) || !iter2 ||
        !hex_decode(fields[2], salt1) || !hex_decode(fields[4], salt2)) return false;
    uint8_t hash1[32] = {}, hash2[32] = {};
    bool ok = pbkdf2(reinterpret_cast<const uint8_t *>(password.data()), password.size(), salt1, iter1, hash1, yield_fn) &&
              pbkdf2(hash1, sizeof(hash1), salt2, iter2, hash2, yield_fn);
    if (ok) response = fields[4] + "$" + hex(hash2, 32);
    mbedtls_platform_zeroize(hash1, sizeof(hash1));
    mbedtls_platform_zeroize(hash2, sizeof(hash2));
    return ok;
  }
  method = "MD5";
  if (challenge.empty() || challenge.size() > 64 ||
      challenge.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") != std::string::npos)
    return false;
  const std::string input = challenge + "-" + password;
  std::vector<uint8_t> utf16;
  for (size_t i = 0; i < input.size();) {
    uint32_t cp = static_cast<unsigned char>(input[i++]);
    if (cp >= 0x80) {
      int continuation = 0;
      uint32_t minimum = 0;
      if (cp >= 0xc2 && cp <= 0xdf) { cp &= 0x1f; continuation = 1; minimum = 0x80; }
      else if (cp >= 0xe0 && cp <= 0xef) { cp &= 0x0f; continuation = 2; minimum = 0x800; }
      else if (cp >= 0xf0 && cp <= 0xf4) { cp &= 7; continuation = 3; minimum = 0x10000; }
      else return false;
      for (int j = 0; j < continuation; j++) {
        if (i >= input.size() || (static_cast<unsigned char>(input[i]) & 0xc0) != 0x80) return false;
        cp = (cp << 6) | (static_cast<unsigned char>(input[i++]) & 0x3f);
      }
      if (cp < minimum || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff)) return false;
    }
    utf16.push_back(cp <= 255 ? cp : '.');
    utf16.push_back(0);
  }
  uint8_t digest[16] = {};
  auto info = mbedtls_md_info_from_type(MBEDTLS_MD_MD5);
  bool ok = info && mbedtls_md(info, utf16.data(), utf16.size(), digest) == 0;
  if (ok) response = challenge + "-" + hex(digest, sizeof(digest));
  mbedtls_platform_zeroize(utf16.data(), utf16.size());
  mbedtls_platform_zeroize(digest, sizeof(digest));
  return ok;
}

// MQTT command sentinel: explicitly means "no device command", never a target.
inline bool no_command(float value) { return std::isfinite(value) && std::fabs(value - 9.9f) < 0.0001f; }

// Only ordinary temperature targets; ON/OFF special codes are not writable here.
inline bool encode_target(float value, uint32_t &encoded) {
  if (no_command(value) || !std::isfinite(value) || value < 8.0f || value > 28.0f) return false;
  float doubled = value * 2.0f;
  if (std::fabs(doubled - std::round(doubled)) > 0.0001f) return false;
  encoded = static_cast<uint32_t>(std::round(doubled));
  return true;
}

struct Thermostat {
  std::string ain, name, product, present, celsius, tist, tsoll, battery, battery_low, error;
};

inline float decimal_temperature(const std::string &value) {
  auto v = trim(value);
  bool negative = !v.empty() && v[0] == '-';
  uint32_t n;
  if (!number(negative ? v.substr(1) : v, n, 1000)) return NAN;
  return (negative ? -1.0f : 1.0f) * n / 10.0f;
}
inline float actual_temperature(const Thermostat &t) {
  float value = decimal_temperature(t.celsius);
  uint32_t n;
  if (std::isnan(value) && number(trim(t.tist), n, 120)) value = n / 2.0f;
  return value;
}
inline float target_temperature(const Thermostat &t) {
  uint32_t n;
  return number(trim(t.tsoll), n, 56) && n >= 16 ? n / 2.0f : NAN;
}
inline float battery_percent(const Thermostat &t) {
  uint32_t n;
  return number(trim(t.battery), n, 100) ? static_cast<float>(n) : NAN;
}
inline std::string target_mode(const Thermostat &t) {
  auto v = trim(t.tsoll);
  return v == "253" ? "Aus" : v == "254" ? "Ein" :
      std::isnan(target_temperature(t)) ? "Unbekannt" : "Temperaturregelung";
}

struct XmlResult {
  std::string sid, challenge, block_time;
  int device_count = 0;
  std::vector<Thermostat> thermostats;
};

// Expat checks XML well-formedness; DTDs are forbidden. No raw XML is logged.
class XmlReader {
 public:
  bool parse(const std::string &body, const char *expected_root, XmlResult &out) {
    if (body.empty() || body.size() > 65536) return false;
    result_ = {};
    depth_ = 0;
    invalid_ = false;
    root_ = expected_root;
    field_.clear();
    path_.clear();
    current_ = {};
    has_hkr_ = false;
    leaf_.clear();
    seen_sid_ = seen_challenge_ = seen_block_ = false;
    parser_ = XML_ParserCreate(nullptr);
    if (!parser_) return false;
    XML_SetUserData(parser_, this);
    XML_SetElementHandler(parser_, start_, end_);
    XML_SetCharacterDataHandler(parser_, text_);
    XML_SetStartDoctypeDeclHandler(parser_, doctype_);
    bool ok = XML_Parse(parser_, body.data(), static_cast<int>(body.size()), XML_TRUE) == XML_STATUS_OK &&
              !invalid_ && depth_ == 0;
    XML_ParserFree(parser_);
    parser_ = nullptr;
    if (ok) out = result_;
    return ok;
  }
 private:
  XML_Parser parser_ = nullptr;
  XmlResult result_;
  std::string root_, field_, leaf_;
  std::vector<std::string> path_;
  Thermostat current_;
  bool has_hkr_ = false;
  int depth_ = 0;
  bool invalid_ = false, seen_sid_ = false, seen_challenge_ = false, seen_block_ = false;
  void reject_() { invalid_ = true; XML_StopParser(parser_, XML_FALSE); }
  static void XMLCALL start_(void *arg, const XML_Char *name, const XML_Char **attrs) {
    auto &s = *static_cast<XmlReader *>(arg);
    if (s.invalid_) return;
    if (++s.depth_ > 32 || (s.depth_ == 1 && s.root_ != name)) { s.reject_(); return; }
    s.path_.emplace_back(name);
    if (s.root_ == "devicelist") {
      s.leaf_.clear();
      if (s.depth_ == 2 && s.path_[1] == "device") {
        s.result_.device_count++;
        s.current_ = {};
        s.has_hkr_ = false;
        for (int i = 0; attrs && attrs[i]; i += 2) {
          if (std::strlen(attrs[i + 1]) > 256) { s.reject_(); return; }
          if (std::strcmp(attrs[i], "identifier") == 0) s.current_.ain = trim(attrs[i + 1]);
          if (std::strcmp(attrs[i], "productname") == 0) s.current_.product = attrs[i + 1];
        }
      }
      if (s.depth_ >= 3 && s.path_[1] == "device") {
        if (s.depth_ == 3 && s.path_[2] == "hkr") s.has_hkr_ = true;
        if (s.depth_ == 3 && (s.path_[2] == "name" || s.path_[2] == "present" ||
            s.path_[2] == "battery" || s.path_[2] == "batterylow")) s.leaf_ = s.path_[2];
        if (s.depth_ == 4 && s.path_[2] == "temperature" && s.path_[3] == "celsius") s.leaf_ = "celsius";
        if (s.depth_ == 4 && s.path_[2] == "hkr" && (s.path_[3] == "tist" || s.path_[3] == "tsoll" ||
            s.path_[3] == "battery" || s.path_[3] == "batterylow" || s.path_[3] == "errorcode")) s.leaf_ = s.path_[3];
      }
    }
    if (s.root_ == "SessionInfo" && s.depth_ == 2) {
      bool *seen = nullptr;
      if (std::strcmp(name, "SID") == 0) seen = &s.seen_sid_;
      if (std::strcmp(name, "Challenge") == 0) seen = &s.seen_challenge_;
      if (std::strcmp(name, "BlockTime") == 0) seen = &s.seen_block_;
      if (seen) {
        if (*seen) { s.reject_(); return; }
        *seen = true;
        s.field_ = name;
      }
    } else if (s.root_ == "SessionInfo" && s.depth_ > 2 && !s.field_.empty()) s.reject_();
  }
  static void XMLCALL end_(void *arg, const XML_Char *) {
    auto &s = *static_cast<XmlReader *>(arg);
    if (s.invalid_) return;
    if (s.root_ == "devicelist" && !s.leaf_.empty()) {
      auto value = trim(s.field_);
      auto &t = s.current_;
      if (s.leaf_ == "name") t.name = value;
      if (s.leaf_ == "present") t.present = value;
      if (s.leaf_ == "battery" && !value.empty()) t.battery = value;
      if (s.leaf_ == "batterylow" && !value.empty()) t.battery_low = value;
      if (s.leaf_ == "celsius") t.celsius = value;
      if (s.leaf_ == "tist") t.tist = value;
      if (s.leaf_ == "tsoll") t.tsoll = value;
      if (s.leaf_ == "errorcode") t.error = value;
      s.leaf_.clear();
      s.field_.clear();
    }
    if (s.root_ == "devicelist" && s.depth_ == 2 && s.path_[1] == "device" && s.has_hkr_) {
      if (s.current_.ain.empty() || s.result_.thermostats.size() >= 32) { s.reject_(); return; }
      s.result_.thermostats.push_back(s.current_);
    }
    if (s.depth_ == 2) s.field_.clear();
    s.path_.pop_back();
    --s.depth_;
  }
  static void XMLCALL text_(void *arg, const XML_Char *text, int length) {
    auto &s = *static_cast<XmlReader *>(arg);
    if (s.invalid_) return;
    if (s.root_ == "devicelist") {
      if (s.leaf_.empty()) return;
      if (s.field_.size() + length > 256) { s.reject_(); return; }
      s.field_.append(text, length);
      return;
    }
    if (s.depth_ != 2 || s.field_.empty()) return;
    auto &dest = s.field_ == "SID" ? s.result_.sid : s.field_ == "Challenge" ? s.result_.challenge : s.result_.block_time;
    if (dest.size() + length > 1024) { s.reject_(); return; }
    dest.append(text, length);
  }
  static void XMLCALL doctype_(void *arg, const XML_Char *, const XML_Char *, const XML_Char *, int) {
    static_cast<XmlReader *>(arg)->reject_();
  }
};

}  // namespace esphome::fritzbox_aha::protocol
