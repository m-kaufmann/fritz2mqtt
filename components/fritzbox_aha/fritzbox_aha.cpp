#include "fritzbox_aha.h"
#include "aha_protocol.h"
#include <cmath>
#include <cstdio>
#include <strings.h>
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esphome/core/log.h"

namespace esphome::fritzbox_aha {
static const char *const TAG = "fritzbox_aha";
static uint32_t worker_millis() { return static_cast<uint32_t>(esp_timer_get_time() / 1000); }

void FritzboxAha::setup() {
  // The native HTTP library must never print URLs containing session parameters.
  esp_log_level_set("HTTP_CLIENT", ESP_LOG_NONE);
  init_web_();
  if (is_failed()) return;
  firmware_setup_();
  results_ = xQueueCreate(1, sizeof(Snapshot));
  requests_ = xSemaphoreCreateBinary();
  commands_ = xQueueCreate(1, sizeof(Command));
  command_results_ = xQueueCreate(1, sizeof(CommandResult));
  if (!results_ || !requests_ || !commands_ || !command_results_ ||
      xTaskCreate(worker_entry_, "fritz_aha", 12288, this, 1, &worker_) != pdPASS) {
    if (commands_) { vQueueDelete(commands_); commands_ = nullptr; }
    if (command_results_) { vQueueDelete(command_results_); command_results_ = nullptr; }
    if (results_) { vQueueDelete(results_); results_ = nullptr; }
    if (requests_) { vSemaphoreDelete(requests_); requests_ = nullptr; }
    ESP_LOGE(TAG, "Worker konnte nicht gestartet werden");
    mark_failed();
    return;
  }
  if (command_status_) command_status_->publish_state("Bereit: Temperatur waehlen, dann uebernehmen");
  if (diagnostic_) diagnostic_->publish_state("Warte auf erste Abfrage");
  set_timeout("first_poll", 10000, [this]() { this->update(); });
}

void FritzboxAha::update() {
  if (requests_) xSemaphoreGive(requests_);
}

void FritzboxAha::loop() {
  wifi_loop_();
  mqtt_loop_();
  CommandResult result;
  if (command_results_ && xQueueReceive(command_results_, &result, 0) == pdTRUE) {
    command_busy_ = false;
    if (command_status_) command_status_->publish_state(result.message);
  }
  Snapshot next;
  if (!results_ || xQueueReceive(results_, &next, 0) != pdTRUE) return;
  if (!have_last_ || std::strcmp(next.diagnostic, last_.diagnostic)) {
    ESP_LOGI(TAG, "%s", next.diagnostic);
    if (diagnostic_) diagnostic_->publish_state(next.diagnostic);
  }
  if (method_sensor_ && (!have_last_ || std::strcmp(next.method, last_.method))) method_sensor_->publish_state(next.method);
  if (connected_sensor_) connected_sensor_->publish_state(next.connected);
  if (authenticated_sensor_) authenticated_sensor_->publish_state(next.authenticated);
  if (count_sensor_) count_sensor_->publish_state(next.device_count < 0 ? NAN : next.device_count);
  if (thermostat_name_) thermostat_name_->publish_state(next.name);
  if (thermostat_identifier_) thermostat_identifier_->publish_state(next.ain);
  if (thermostat_model_) thermostat_model_->publish_state(next.product);
  if (thermostat_status_) thermostat_status_->publish_state(next.thermostat_status);
  if (thermostat_mode_) thermostat_mode_->publish_state(next.mode);
  if (thermostat_battery_status_) thermostat_battery_status_->publish_state(next.battery_status);
  if (thermostat_temperature_) thermostat_temperature_->publish_state(next.temperature);
  if (thermostat_target_temperature_) thermostat_target_temperature_->publish_state(next.target);
  if (thermostat_battery_) thermostat_battery_->publish_state(next.battery);
  observation_at_ = worker_millis();
  last_ = next;
  have_last_ = true;
}

void FritzboxAha::dump_config() {
  ESP_LOGCONFIG(TAG, "FRITZ2MQTT AHA: Login und Geraeteabfrage; HTTP-Worker");
  ESP_LOGCONFIG(TAG, "Keine Zugangsdaten, Antworten oder Session-IDs im Log");
}

void FritzboxAha::worker_entry_(void *arg) { static_cast<FritzboxAha *>(arg)->worker_run_(); }
void FritzboxAha::worker_run_() {
  while (true) {
    if (xSemaphoreTake(requests_, portMAX_DELAY) == pdTRUE) {
      apply_runtime_config_();
      Command command;
      if (xQueueReceive(commands_, &command, 0) == pdTRUE) execute_command_(command);
      else cycle_();
    }
  }
}

void FritzboxAha::request_target_temperature(float value) {
  if (protocol::no_command(value)) {
    if (command_status_) command_status_->publish_state("Kein Stellauftrag (9.9); nichts gesendet");
    return;
  }
  std::string error;
  if (!queue_temperature_(thermostat_ain_, value, error)) {
    if (command_status_) command_status_->publish_state(error);
  } else if (command_status_) {
    command_status_->publish_state("Auftrag angenommen; FRITZ!Box-Rueckmeldung wird geprueft");
  }
}

void FritzboxAha::command_result_(const std::string &message) {
  xSemaphoreTake(web_mutex_, portMAX_DELAY);
  web_command_message_ = message;
  xSemaphoreGive(web_mutex_);
  CommandResult result;
  std::snprintf(result.message, sizeof(result.message), "%s", message.c_str());
  xQueueOverwrite(command_results_, &result);
}

void FritzboxAha::execute_command_(const Command &command) {
  active_command_ain_ = command.ain;
  auto config = config_copy_();
  const auto *mapping = gateway::find(*config, command.ain);
  uint32_t encoded;
  if (!network_ready_.load() || !mapping || !mapping->enabled || config->revision != command.revision ||
      !protocol::encode_target(command.target, encoded) || worker_millis() - command.created_at > 15000 ||
      (retry_at_ && static_cast<int32_t>(worker_millis() - retry_at_) < 0)) {
    command_result_("Nicht gesendet: Auftrag ungueltig, veraltet oder Verbindung in Wartezeit"); return;
  }
  if (sid_.empty() || !discover_devices_()) {
    command_result_("Nicht gesendet: Session nicht bereit; bitte erneut versuchen"); return;
  }
  const protocol::Thermostat *selected = nullptr;
  for (const auto &t : discovered_) if (t.ain == command.ain) {
    if (selected) { command_result_("Nicht gesendet: AIN nicht eindeutig"); return; }
    selected = &t;
  }
  if (!selected || selected->present != "1" || (!selected->error.empty() && selected->error != "0") ||
      config_copy_()->revision != command.revision || worker_millis() - command.created_at > 15000) {
    command_result_("Nicht gesendet: Thermostat nicht bereit oder Einrichtung geaendert"); return;
  }
  // Exactly one write. Ambiguous network outcomes must never trigger an automatic replay.
  auto reply = http_("/webservices/homeautoswitch.lua?switchcmd=sethkrtsoll&sid=" + sid_ +
                     "&ain=" + protocol::form_encode(command.ain) + "&param=" + std::to_string(encoded));
  bool accepted = reply.complete && reply.status == 200 && protocol::trim(reply.body) != "inval";
  bool observed = discover_devices_();
  char message[160];
  float reported = NAN;
  if (observed) for (const auto &t : discovered_) if (t.ain == command.ain && t.present == "1") reported = protocol::target_temperature(t);
  if (std::isfinite(reported) && std::fabs(reported - command.target) < 0.01f) {
    std::snprintf(message, sizeof(message), accepted ? "FRITZ!Box bestaetigt: %.1f C" :
        "FRITZ!Box meldet %.1f C; Sendeantwort unklar", command.target);
  } else {
    std::snprintf(message, sizeof(message), accepted ?
        "Gesendet: %.1f C; Rueckmeldung noch nicht bestaetigt" :
        "Nicht bestaetigt: %.1f C; Verbindung/Rechte pruefen, kein automatisches Wiederholen", command.target);
  }
  command_result_(message);
}

void FritzboxAha::report_(const std::string &message, bool connected, bool authenticated, int count) {
  Snapshot s = thermostat_;
  if (count < 0) {
    s.temperature = s.target = s.battery = NAN;
    std::snprintf(s.thermostat_status, sizeof(s.thermostat_status), "Keine aktuellen Daten");
    std::snprintf(s.mode, sizeof(s.mode), "Unbekannt");
    std::snprintf(s.battery_status, sizeof(s.battery_status), "Unbekannt");
  }
  std::snprintf(s.diagnostic, sizeof(s.diagnostic), "%s", message.c_str());
  std::snprintf(s.method, sizeof(s.method), "%s", method_.c_str());
  s.connected = connected;
  s.authenticated = authenticated;
  s.device_count = count;
  xSemaphoreTake(web_mutex_, portMAX_DELAY);
  web_last_ = s;
  web_revision_ = applied_revision_;
  web_devices_ = discovered_;
  web_observation_at_ = worker_millis();
  xSemaphoreGive(web_mutex_);
  xQueueOverwrite(results_, &s);
}

void FritzboxAha::backoff_(uint32_t minimum_seconds) {
  if (failures_ < 5) ++failures_;
  uint32_t seconds = std::max<uint32_t>(minimum_seconds, std::min<uint32_t>(900U, 30U << failures_));
  retry_at_ = worker_millis() + seconds * 1000U;
}

FritzboxAha::HttpResult FritzboxAha::http_(const std::string &path, const std::string &post) {
  HttpResult result;
  struct Framing { bool has_length = false; bool chunked = false; } framing;
  const auto url = "http://" + host_ + path;
  esp_http_client_config_t config = {};
  config.url = url.c_str();
  // The router can take more than four seconds to start its login response.
  // Keep the network wait bounded inside the existing 20-second request budget.
  config.timeout_ms = 8000;
  config.disable_auto_redirect = true;
  config.buffer_size = 1024;
  config.buffer_size_tx = 1024;
  config.user_agent = "FRITZ2MQTT/0.1";
  config.user_data = &framing;
  config.event_handler = [](esp_http_client_event_t *event) -> esp_err_t {
    if (event->event_id == HTTP_EVENT_ON_HEADER && event->header_key && event->header_value) {
      auto *f = static_cast<Framing *>(event->user_data);
      if (strcasecmp(event->header_key, "Content-Length") == 0) f->has_length = true;
      if (strcasecmp(event->header_key, "Transfer-Encoding") == 0) {
        std::string value = event->header_value;
        for (auto &c : value) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
        if (value.find("chunked") != std::string::npos) f->chunked = true;
      }
    }
    return ESP_OK;
  };
  auto client = esp_http_client_init(&config);
  if (!client) { result.error = ESP_ERR_NO_MEM; return result; }
  const bool is_post = !post.empty();
  esp_http_client_set_method(client, is_post ? HTTP_METHOD_POST : HTTP_METHOD_GET);
  if (is_post) esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");
  const auto started = worker_millis();
  int last_read = 0;
  auto remaining_timeout = [&]() {
    const uint32_t elapsed = worker_millis() - started;
    if (elapsed >= 20000) { result.error = ESP_ERR_TIMEOUT; return false; }
    esp_http_client_set_timeout_ms(client, std::min<uint32_t>(8000, 20000 - elapsed));
    return true;
  };
  result.stage = "connect";
  result.error = esp_http_client_open(client, post.size());
  bool ok = result.error == ESP_OK;
  if (ok && is_post) {
    result.stage = "write";
    size_t sent = 0;
    while (sent < post.size()) {
      if (!remaining_timeout()) { ok = false; break; }
      int n = esp_http_client_write(client, post.data() + sent, post.size() - sent);
      if (n <= 0) { result.error = n < 0 ? n : ESP_FAIL; ok = false; break; }
      sent += n;
    }
    ok = ok && sent == post.size();
  }
  int64_t length = 0;
  if (ok && !remaining_timeout()) ok = false;
  if (ok) {
    result.stage = "headers";
    length = esp_http_client_fetch_headers(client);
    result.status = esp_http_client_get_status_code(client);
    ok = length >= 0 && length <= 65536;
    if (!ok) result.error = static_cast<int>(length);
    if (ok && length > 0) result.body.reserve(static_cast<size_t>(length));
  }
  char buffer[1024];
  // Drain the internal response buffer even when IDF already reports completion.
  // The 7490 also uses close-delimited XML (neither length nor chunked header).
  while (ok) {
    result.stage = "read";
    if (framing.has_length && !framing.chunked && result.body.size() == static_cast<size_t>(length)) {
      result.complete = true;
      break;
    }
    if (!remaining_timeout()) { ok = false; break; }
    int n = esp_http_client_read(client, buffer, sizeof(buffer));
    last_read = n;
    if (n <= 0) {
      if (framing.chunked) result.complete = n >= 0 && esp_http_client_is_complete_data_received(client);
      else if (framing.has_length) result.complete = result.body.size() == static_cast<size_t>(length);
      else result.complete = n != -ESP_ERR_HTTP_EAGAIN;
      // For close-delimited responses the caller additionally requires complete,
      // well-formed XML, so a truncated/partial body can never become a success.
      break;
    }
    if (result.body.size() + n > 65536) { result.error = ESP_ERR_INVALID_SIZE; ok = false; break; }
    result.body.append(buffer, n);
    vTaskDelay(1);
  }
  result.complete = ok && result.complete;
  if (!result.complete && result.error == ESP_OK) result.error = last_read < 0 ? last_read : ESP_FAIL;
  // Never log path, headers, response bodies, credentials or session identifiers.
  // A close-delimited response can finish with a native negative read/errno;
  // completeness and XML validation, not that raw code alone, decide success.
  const char *operation = path.find("switchcmd=sethkrtsoll") != std::string::npos ? "set-target" :
      path.find("switchcmd=getdevicelistinfos") != std::string::npos ? "devices" :
      is_post ? "login/session" : "challenge";
  wifi_ap_record_t ap{};
  const bool wifi_connected = esp_wifi_sta_get_ap_info(&ap) == ESP_OK;
  const int socket_error = esp_http_client_get_errno(client);
  const uint32_t elapsed = worker_millis() - started;
  HttpObservation observation;
  observation.operation = operation; observation.phase = result.stage;
  observation.at_ms = worker_millis(); observation.elapsed_ms = elapsed;
  observation.bytes = result.body.size(); observation.heap = esp_get_free_heap_size();
  observation.status = result.status; observation.error = result.error;
  observation.socket_error = socket_error; observation.last_read = last_read;
  observation.rssi = wifi_connected ? ap.rssi : 0;
  observation.complete = result.complete; observation.wifi_connected = wifi_connected;
  const bool failed = !result.complete || result.status != 200;
  xSemaphoreTake(web_mutex_, portMAX_DELAY);
  ++web_http_requests_; web_http_last_ = observation;
  if (failed) { ++web_http_failures_; web_http_failure_ = observation; }
  xSemaphoreGive(web_mutex_);
  if (failed) {
    ESP_LOGW(TAG, "HTTP %s: phase=%s status=%d complete=%d bytes=%u elapsed=%u ms error=%d errno=%d read=%d wifi=%d rssi=%d heap=%u",
             operation, result.stage, result.status, result.complete, static_cast<unsigned>(result.body.size()),
             static_cast<unsigned>(elapsed), result.error, socket_error, last_read, wifi_connected,
             wifi_connected ? ap.rssi : 0, static_cast<unsigned>(esp_get_free_heap_size()));
  } else {
    ESP_LOGD(TAG, "HTTP %s: status=200 complete=1 bytes=%u elapsed=%u ms read=%d errno=%d",
             operation, static_cast<unsigned>(result.body.size()), static_cast<unsigned>(elapsed), last_read, socket_error);
  }
  esp_http_client_close(client);
  esp_http_client_cleanup(client);
  return result;
}

bool FritzboxAha::login_() {
  report_("FRITZ!Box-Anmeldung laeuft", false, false);
  auto reply = http_("/login_sid.lua?version=2");
  if (!reply.complete || reply.status != 200) {
    report_("Challenge " + std::string(reply.stage) + ": Fehler " + std::to_string(reply.error) +
            "; HTTP " + std::to_string(reply.status), reply.complete, false);
    backoff_();
    return false;
  }
  protocol::XmlReader xml;
  protocol::XmlResult info;
  uint32_t block_time;
  if (!xml.parse(reply.body, "SessionInfo", info) ||
      !protocol::number(protocol::trim(info.block_time), block_time, 3600) || info.challenge.empty()) {
    report_("Ungueltige Login-XML-Antwort", true, false);
    backoff_();
    return false;
  }
  if (block_time > 0) {
    report_("FRITZ!Box-Sperrzeit; Anmeldung wird spaeter wiederholt", true, false);
    backoff_(block_time + 1);
    return false;
  }
  std::string response;
  if (!protocol::make_response(protocol::trim(info.challenge), password_, response, method_, crypto_yield_)) {
    report_("Challenge oder Kryptografie nicht unterstuetzt", true, false);
    backoff_(300);
    return false;
  }
  std::string post = "username=" + protocol::form_encode(username_) + "&response=" + protocol::form_encode(response);
  reply = http_("/login_sid.lua?version=2", post);
  mbedtls_platform_zeroize(response.data(), response.size());
  mbedtls_platform_zeroize(post.data(), post.size());
  if (!reply.complete || reply.status != 200 || !xml.parse(reply.body, "SessionInfo", info)) {
    report_("Anmeldeantwort fehlt oder ist ungueltig", reply.complete, false);
    backoff_();
    return false;
  }
  auto sid = protocol::trim(info.sid);
  if (!protocol::valid_sid(sid)) {
    uint32_t wait = 0;
    protocol::number(protocol::trim(info.block_time), wait, 3600);
    report_("Anmeldung abgelehnt; Benutzer und Passwort pruefen", true, false);
    backoff_(std::max<uint32_t>(300U, wait + 1));
    return false;
  }
  sid_ = sid;
  report_("Anmeldung erfolgreich; Geraeteabfrage laeuft", true, true);
  return true;
}

bool FritzboxAha::refresh_session_() {
  auto reply = http_("/login_sid.lua?version=2", "sid=" + protocol::form_encode(sid_));
  protocol::XmlReader xml;
  protocol::XmlResult info;
  if (reply.complete && reply.status == 200 && xml.parse(reply.body, "SessionInfo", info) &&
      protocol::trim(info.sid) == sid_) return true;
  sid_.clear();
  return false;
}

bool FritzboxAha::discover_devices_() {
  auto reply = http_("/webservices/homeautoswitch.lua?switchcmd=getdevicelistinfos&sid=" + sid_);
  if (!reply.complete) {
    report_("HTTP-Geraeteabfrage fehlgeschlagen", false, !sid_.empty());
    backoff_();
    return false;
  }
  if (reply.status == 401 || reply.status == 403 || protocol::trim(reply.body) == "inval") {
    // AHA errors can mean missing Smart Home rights, not just an expired session.
    if (refresh_session_()) {
      report_("AHA-Zugriff abgelehnt; Smart-Home-Rechte pruefen", true, true);
      backoff_(300);
    } else {
      report_("Session ungueltig; erneute Anmeldung folgt", true, false);
      backoff_();
    }
    return false;
  }
  if (reply.status != 200) {
    report_("AHA-HTTP-Fehler " + std::to_string(reply.status), true, true);
    backoff_();
    return false;
  }
  protocol::XmlReader xml;
  protocol::XmlResult devices;
  if (!xml.parse(reply.body, "devicelist", devices)) {
    report_("Ungueltige oder unvollstaendige Geraeteliste", true, true);
    backoff_();
    return false;
  }
  discovered_ = devices.thermostats;
  thermostat_ = {};
  const protocol::Thermostat *selected = nullptr;
  if (thermostat_ain_.empty()) {
    if (devices.thermostats.size() == 1) selected = &devices.thermostats.front();
  } else {
    for (const auto &t : devices.thermostats) {
      if (t.ain == thermostat_ain_) {
        if (selected) { selected = nullptr; break; }
        selected = &t;
      }
    }
  }
  auto &s = thermostat_;
  if (selected) {
    const auto &t = *selected;
    std::snprintf(s.name, sizeof(s.name), "%s", t.name.c_str());
    std::snprintf(s.ain, sizeof(s.ain), "%s", t.ain.c_str());
    std::snprintf(s.product, sizeof(s.product), "%s", t.product.c_str());
    bool present = t.present == "1";
    std::string status = present ? "Erreichbar" : t.present == "0" ? "Nicht erreichbar" : "Erreichbarkeit unbekannt";
    if (present && !t.error.empty() && t.error != "0") status += "; HKR-Fehlercode " + t.error;
    std::snprintf(s.thermostat_status, sizeof(s.thermostat_status), "%s", status.c_str());
    if (present) {
      s.temperature = protocol::actual_temperature(t);
      s.target = protocol::target_temperature(t);
      s.battery = protocol::battery_percent(t);
      std::snprintf(s.mode, sizeof(s.mode), "%s", protocol::target_mode(t).c_str());
      std::snprintf(s.battery_status, sizeof(s.battery_status), "%s",
          t.battery_low == "1" ? "Batterie wechseln" : t.battery_low == "0" ? "OK" : "Unbekannt");
    }
  } else {
    std::snprintf(s.thermostat_status, sizeof(s.thermostat_status), "%s",
        !thermostat_ain_.empty() ? "Konfigurierte AIN nicht eindeutig gefunden" :
        devices.thermostats.empty() ? "Kein Thermostat gefunden" : "Mehrere Thermostate: AIN konfigurieren");
  }
  report_("Anmeldung erfolgreich; " + std::to_string(devices.device_count) + " Geraete gefunden",
          true, true, devices.device_count);
  failures_ = 0;
  retry_at_ = 0;
  return true;
}

void FritzboxAha::cycle_() {
  if (!network_ready_.load()) return;
  if (retry_at_ && static_cast<int32_t>(worker_millis() - retry_at_) < 0) return;
  if (password_.empty() || username_.empty()) {
    report_("FRITZ!Box-Zugangsdaten fehlen", false, false);
    backoff_(300);
    return;
  }
  if (sid_.empty() && !login_()) return;
  discover_devices_();
}

}  // namespace esphome::fritzbox_aha
