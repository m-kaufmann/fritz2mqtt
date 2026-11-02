#pragma once
#include <array>
#include <cstdint>
#include <string>

namespace esphome::fritzbox_aha::firmware {
// The stable channel intentionally accepts only complete numeric release versions.
inline bool version(const std::string &text, std::array<unsigned, 3> &out, bool current = false) {
  std::string s = text;
  const auto dash = s.find('-');
  if (dash != std::string::npos) {
    if (!current || s.substr(dash) != "-dev") return false;
    s.resize(dash);
  }
  size_t pos = 0;
  for (size_t i = 0; i < 3; ++i) {
    unsigned n = 0; const size_t start = pos;
    while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9') {
      n = n * 10 + unsigned(s[pos++] - '0');
      if (n > 9999) return false;
    }
    if (pos == start || (pos - start > 1 && s[start] == '0')) return false;
    out[i] = n;
    if (i < 2 && (pos >= s.size() || s[pos++] != '.')) return false;
  }
  return pos == s.size();
}
struct Candidate {
  std::string current, latest, url, md5;
  bool verified = false, available = false, failed = false, checking = false, installing = false;
  uint32_t checked_at = 0;
};
inline std::string reject(const Candidate &c, uint32_t now) {
  if (c.installing) return "Firmwareinstallation laeuft";
  if (c.checking) return "Updatepruefung laeuft";
  if (!c.verified || c.failed || uint32_t(now - c.checked_at) > 600000U) return "Bitte erneut nach Updates suchen";
  if (!c.available) return "Kein Update verfuegbar";
  std::array<unsigned,3> old{}, next{};
  if (!version(c.current, old, true) || !version(c.latest, next)) return "Versionsformat nicht freigegeben";
  if (!(next > old || (next == old && c.current != c.latest))) return "Gleiche oder aeltere Version gesperrt";
  const std::string expected = "https://m-kaufmann.github.io/fritz2mqtt/firmware/esp32-4mb-v1/" + c.latest + "/firmware.ota.bin";
  if (c.url != expected) return "Firmwarequelle oder Hardwareprofil nicht freigegeben";
  if (c.md5.size() != 32 || c.md5.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos) return "Firmwarepruefsumme fehlt oder ist ungueltig";
  return "";
}
}  // namespace esphome::fritzbox_aha::firmware
