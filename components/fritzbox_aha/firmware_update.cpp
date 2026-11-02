#include "fritzbox_aha.h"
#include "esphome/components/json/json_util.h"
#include "esphome/components/wifi/wifi_component.h"
#include "esp_timer.h"
#include <ctime>

namespace esphome::fritzbox_aha {
static uint32_t firmware_millis() { return static_cast<uint32_t>(esp_timer_get_time() / 1000); }

void FritzboxAha::firmware_setup_() {
#ifdef ESPHOME_PROJECT_VERSION
  firmware_state_.current = ESPHOME_PROJECT_VERSION;
#else
  firmware_state_.current = "custom";
#endif
#ifdef USE_FRITZ2MQTT_UPDATE
  if (firmware_update_) {
    firmware_message_ = "Noch keine erfolgreiche Updatepruefung";
    // ESPHome publishes these callbacks on its main loop, including OTA progress.
    firmware_update_->add_on_state_callback([this]() { firmware_changed_(); });
    set_interval("firmware_state", 100, [this]() { firmware_tick_(); });
  }
#endif
}

void FritzboxAha::firmware_changed_() {
#ifdef USE_FRITZ2MQTT_UPDATE
  if (!firmware_update_) return;
  xSemaphoreTake(web_mutex_, portMAX_DELAY);
  const auto &info = firmware_update_->update_info;
  firmware_progress_ = info.has_progress ? info.progress : 0;
  firmware_state_.latest = info.latest_version;
  firmware_state_.url = info.firmware_url;
  firmware_state_.md5 = info.md5;
  firmware_state_.available = firmware_update_->state == update::UPDATE_STATE_AVAILABLE;
  firmware_state_.installing = firmware_update_->state == update::UPDATE_STATE_INSTALLING;
  firmware_state_.failed = firmware_update_->status_has_error();
  if (!firmware_state_.installing) {
    firmware_state_.checking = false;
    firmware_state_.verified = !firmware_state_.failed;
    if (firmware_state_.verified) firmware_state_.checked_at = firmware_millis();
  }
  firmware_message_ = firmware_state_.failed ? "Update fehlgeschlagen; bitte erneut pruefen" :
      firmware_state_.installing ? "Firmware wird installiert; Neustart folgt" :
      firmware_state_.available ? "Neue Firmware gefunden" : "Firmware ist aktuell";
  xSemaphoreGive(web_mutex_);
#endif
}

void FritzboxAha::firmware_tick_() {
#ifdef USE_FRITZ2MQTT_UPDATE
  if (!firmware_update_) return;
  int action = 0;
  xSemaphoreTake(web_mutex_, portMAX_DELAY);
  if (firmware_state_.checking && uint32_t(firmware_millis() - firmware_check_started_) > 30000U) {
    firmware_state_.checking = false; firmware_state_.verified = false;
    firmware_message_ = "Updatepruefung ohne Ergebnis; bitte erneut versuchen";
  }
  if (firmware_update_->status_has_error()) {
    firmware_state_.verified = false; firmware_state_.failed = true;
    firmware_state_.checking = false;
    firmware_message_ = "Updatequelle nicht erreichbar oder Update fehlgeschlagen";
  }
  action = firmware_action_.load();
  if (action == 1) {
    firmware_state_.checking = true; firmware_state_.verified = false; firmware_state_.failed = false;
    firmware_check_started_ = firmware_millis(); firmware_message_ = "Updatepruefung laeuft";
  } else if (action == 2) {
    const auto error = firmware::reject(firmware_state_, firmware_millis());
    if (!error.empty() || command_busy_ || firmware_expected_ != firmware_state_.latest || !wifi::global_wifi_component->is_connected()) {
      firmware_message_ = error.empty() ? "Installation verworfen; bitte neu pruefen" : error;
      action = 0;
    } else {
      firmware_state_.installing = true; firmware_message_ = "Installation angenommen; Firmware wird geladen";
    }
  }
  firmware_action_ = 0;
  xSemaphoreGive(web_mutex_);
  if (action == 1) { firmware_update_->status_clear_error(); firmware_update_->check(); }
  if (action == 2) firmware_update_->perform(false);
#endif
}

bool FritzboxAha::firmware_request_(bool install, const std::string &version, std::string &error) {
#ifdef USE_FRITZ2MQTT_UPDATE
  if (!firmware_update_) { error = "Updatefunktion nicht eingerichtet"; return false; }
  xSemaphoreTake(web_mutex_, portMAX_DELAY);
  if (std::time(nullptr) < 1700000000) error = "Uhrzeit noch nicht synchronisiert; HTTPS benoetigt eine gueltige Uhrzeit";
  else if (!wifi::global_wifi_component->is_connected()) error = "Standort-WLAN nicht verbunden";
  else if (firmware_action_ || firmware_state_.checking || firmware_state_.installing) error = "Updatevorgang bereits aktiv";
  else if (install && command_busy_) error = "Bitte laufenden Thermostat-Auftrag abwarten";
  else if (install && version != firmware_state_.latest) error = "Versionsangebot geaendert; erneut pruefen";
  else if (install) error = firmware::reject(firmware_state_, firmware_millis());
  if (error.empty()) { firmware_expected_ = version; firmware_action_ = install ? 2 : 1; }
  xSemaphoreGive(web_mutex_);
  return error.empty();
#else
  error = "Updatefunktion nicht eingerichtet"; return false;
#endif
}

std::string FritzboxAha::firmware_json_() {
  xSemaphoreTake(web_mutex_, portMAX_DELAY);
  const std::string body = json::build_json([&](JsonObject r) {
    r["enabled"] = false;
#ifdef USE_FRITZ2MQTT_UPDATE
    r["enabled"] = firmware_update_ != nullptr;
#endif
    r["current"] = firmware_state_.current; r["latest"] = firmware_state_.latest;
    r["profile"] = "esp32-4mb-v1"; r["device"] = get_mac_address();
    r["failed"] = firmware_state_.failed;
    r["checking"] = firmware_state_.checking || firmware_action_ == 1;
    r["installing"] = firmware_state_.installing || firmware_action_ == 2;
    r["can_install"] = firmware::reject(firmware_state_, firmware_millis()).empty() && !firmware_action_ && !command_busy_;
    r["reason"] = firmware::reject(firmware_state_, firmware_millis());
    r["message"] = firmware_message_;
    // HTTP reads cached state only; native update objects belong to the main loop.
    r["progress"] = firmware_progress_;
  });
  xSemaphoreGive(web_mutex_);
  return body;
}
}  // namespace esphome::fritzbox_aha
