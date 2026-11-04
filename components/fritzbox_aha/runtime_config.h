#pragma once
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <set>
#include <string>
#include <vector>

namespace esphome::fritzbox_aha::gateway {
constexpr size_t MAX_DEVICES = 8;
struct Device {
  std::string ain, name, unit, base;
  bool enabled = false;
  std::string actual_topic, target_topic, command_topic, battery_topic;
};
struct WifiProfile {
  std::string ssid, password;
  bool operator==(const WifiProfile &o) const { return ssid == o.ssid && password == o.password; }
};
struct Config {
  std::array<WifiProfile, 2> wifi;
  bool wifi_configured = false;
  uint32_t revision = 0;
  std::string location = "site", host, username, password;
  std::string mqtt_host, mqtt_username, mqtt_password;
  unsigned mqtt_port = 8883;
  bool mqtt_tls = true;
  std::vector<Device> devices;
};
inline bool identifier(const std::string &s, bool dash = false) {
  if (s.empty() || s.size() > 64) return false;
  for (unsigned char c : s)
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || (dash && c == '-'))) return false;
  return true;
}
inline bool printable(const std::string &s, size_t max) {
  return s.size() <= max && std::none_of(s.begin(), s.end(), [](unsigned char c) { return c < 32 || c == 127; });
}
inline bool ipv4(const std::string &s) {
  unsigned value = 0, digits = 0, parts = 0;
  for (char c : s + ".") {
    if (c == '.') {
      if (!digits || value > 255 || ++parts > 4) return false;
      digits = value = 0;
    } else if (c >= '0' && c <= '9' && ++digits <= 3) value = value * 10 + c - '0';
    else return false;
  }
  return parts == 4;
}
inline bool topic(const std::string &s) {
  if (s.empty()) return true;  // Incomplete MQTT setup is allowed before MQTT is implemented.
  if (!printable(s, 160) || s.front() == '/' || s.back() == '/' || s.find("//") != std::string::npos) return false;
  return s.find_first_of("+# \t\r\n") == std::string::npos;
}
inline std::string validate(const Config &c) {
  std::set<std::string> ssids;
  for (const auto &w : c.wifi) {
    if (!printable(w.ssid, 32) || (!w.password.empty() && (w.password.size() < 8 || !printable(w.password, 63)))) return "WLAN: SSID maximal 32 Zeichen, Passwort leer oder 8 bis 63 Zeichen";
    if (w.ssid.empty() && !w.password.empty()) return "WLAN: Passwort ohne SSID";
    if (!w.ssid.empty() && !ssids.insert(w.ssid).second) return "WLAN-Kennungen muessen unterschiedlich sein";
  }
  if (c.wifi_configured && ssids.empty()) return "Mindestens eine WLAN-Kennung erforderlich";
  if (!identifier(c.location, true)) return "Location: nur Buchstaben, Ziffern, _ und -";
  if (!ipv4(c.host)) return "FRITZ!Box: gueltige IPv4-Adresse erforderlich";
  if (!printable(c.username, 128) || !printable(c.password, 256)) return "FRITZ!Box-Zugang ungueltig";
  if (!printable(c.mqtt_host, 128) || c.mqtt_host.find_first_of(" /:#?@") != std::string::npos ||
      !c.mqtt_port || c.mqtt_port > 65535 || !printable(c.mqtt_username, 128) || !printable(c.mqtt_password, 256)) return "MQTT-Verbindungsdaten ungueltig";
  if (c.devices.size() > MAX_DEVICES) return "Hoechstens 8 Thermostate konfigurierbar";
  std::set<std::string> ains, topics;
  for (const auto &d : c.devices) {
    if (d.ain.empty() || !printable(d.ain, 64) || !ains.insert(d.ain).second) return "AIN fehlt, ist zu lang oder doppelt";
    if (d.name.empty() || !printable(d.name, 96)) return "Geraetename fehlt oder ist zu lang";
    if (!d.unit.empty() && (d.unit.size() > 5 || d.unit.find_first_not_of("0123456789") != std::string::npos)) return "Unit: nur Ziffern";
    if (!d.base.empty() && !identifier(d.base)) return "Datenpunktbasis: nur Buchstaben, Ziffern und _";
    for (const auto *t : {&d.actual_topic, &d.target_topic, &d.command_topic, &d.battery_topic}) {
      if (!topic(*t)) return "Topic ungueltig: keine Platzhalter, Leerzeichen oder leeren Segmente";
      if (!t->empty() && !topics.insert(*t).second) return "Topics muessen je Datenpunkt eindeutig sein";
    }
  }
  return "";
}
// MQTT numeric payloads are deliberately not interpreted as JSON or expressions.
enum class TargetPayload { INVALID, IGNORE, TARGET };
inline TargetPayload parse_target_payload(const std::string &payload, float &out) {
  if (payload.empty() || payload.size() > 32) return TargetPayload::INVALID;
  size_t start = payload.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) return TargetPayload::INVALID;
  size_t end = payload.find_last_not_of(" \t\r\n") + 1;
  unsigned whole = 0, digits = 0, fraction_digits = 0;
  float fraction = 0, scale = 1;
  bool dot = false;
  for (size_t i = start; i < end; i++) {
    char ch = payload[i];
    if (ch == '.' && !dot && digits) { dot = true; continue; }
    if (ch < '0' || ch > '9') return TargetPayload::INVALID;
    if (!dot) { if (++digits > 2) return TargetPayload::INVALID; whole = whole * 10 + ch - '0'; }
    else { if (++fraction_digits > 6) return TargetPayload::INVALID; scale *= 10; fraction += (ch - '0') / scale; }
  }
  if (!digits || (dot && !fraction_digits)) return TargetPayload::INVALID;
  out = whole + fraction;
  if (std::fabs(out - 9.9f) < 0.0001f) return TargetPayload::IGNORE;
  if (out < 8 || out > 28 || std::fabs(out * 2 - std::round(out * 2)) > 0.0001f) return TargetPayload::INVALID;
  return TargetPayload::TARGET;
}
inline const Device *command_device(const Config &c, const std::string &topic) {
  if (topic.empty()) return nullptr;
  for (const auto &d : c.devices) if (d.enabled && d.command_topic == topic) return &d;
  return nullptr;
}
inline const Device *find(const Config &c, const std::string &ain) {
  for (const auto &d : c.devices) if (d.ain == ain) return &d;
  return nullptr;
}
}  // namespace esphome::fritzbox_aha::gateway
