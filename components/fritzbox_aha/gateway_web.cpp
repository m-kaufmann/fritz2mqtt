#include "fritzbox_aha.h"
#include "gateway_page.h"
#include "esphome/components/json/json_util.h"
#include "esphome/components/wifi/wifi_component.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "nvs.h"
#include <cstdio>
#include <cstring>
#include <ctime>
#include "esp_crt_bundle.h"
#include "esp_sntp.h"

namespace esphome::fritzbox_aha {
static uint32_t web_millis() { return static_cast<uint32_t>(esp_timer_get_time() / 1000); }
static constexpr size_t CONFIG_LIMIT = gateway::HttpRequestBody::MAX_BYTES;

std::shared_ptr<const gateway::Config> FritzboxAha::config_copy_() {
  xSemaphoreTake(web_mutex_, portMAX_DELAY);
  auto config = runtime_config_;
  xSemaphoreGive(web_mutex_);
  return config;
}

void FritzboxAha::init_web_() {
  web_mutex_ = xSemaphoreCreateMutex();
  if (!web_mutex_) { mark_failed(); return; }
  auto config = std::make_shared<gateway::Config>();
  config->host = host_; config->username = username_; config->password = password_;
  if (!thermostat_ain_.empty()) {
    gateway::Device device;
    device.ain = thermostat_ain_; device.name = "Thermostat"; device.base = "hkth_thermostat";
    device.enabled = allow_control_;
    config->devices.push_back(device);
  }
  nvs_handle_t handle;
  if (nvs_open(storage_namespace_.c_str(), NVS_READONLY, &handle) == ESP_OK) {
    size_t size = 0;
    if (nvs_get_blob(handle, "config_v1", nullptr, &size) == ESP_OK && size > 0 && size <= CONFIG_LIMIT) {
      std::string body(size, '\0');
      if (nvs_get_blob(handle, "config_v1", body.data(), &size) == ESP_OK) {
        auto restored = std::make_shared<gateway::Config>();
        std::string error;
        if (decode_config_(body, *config, *restored, error)) config = restored;
        else web_command_message_ = "Gespeicherte Einrichtung ungueltig; YAML-Grundwerte aktiv";
      }
    }
    nvs_close(handle);
  }
  runtime_config_ = config;
  auto *wifi = wifi::global_wifi_component;
  auto ap = wifi->get_ap();
  ap.set_ssid("FRITZ2MQTT-" + get_mac_address().substr(6));
  ap.set_password("");
  wifi->set_ap(ap);
  wifi->set_ap_timeout(60000);
  wifi->set_reboot_timeout(0);
  if (config->wifi_configured) set_timeout("wifi_boot", 1000, [this]() { apply_wifi_(); });
  host_ = config->host; username_ = config->username; password_ = config->password;
  applied_revision_ = config->revision;
  char token[33];
  std::snprintf(token, sizeof(token), "%08lx%08lx%08lx%08lx", static_cast<unsigned long>(esp_random()),
      static_cast<unsigned long>(esp_random()), static_cast<unsigned long>(esp_random()), static_cast<unsigned long>(esp_random()));
  csrf_token_ = token;
  // Set the open AP before WiFi starts, and register before the native web_server.
  web_server_base::global_web_server_base->add_handler(this);
}

// WiFi APIs run only in the ESPHome main loop, never in the HTTP task.
void FritzboxAha::apply_wifi_() {
  auto c = config_copy_();
  if (!c->wifi_configured) return;
  auto *wifi = wifi::global_wifi_component;
  // enable() may restore ESPHome's older single-network preferences. Replace
  // them afterwards with our atomically stored two-profile configuration.
  wifi->disable();
  wifi->enable();
  wifi::WiFiAP first;
  bool have_first = false;
  for (const auto &profile : c->wifi) {
    if (profile.ssid.empty()) continue;
    wifi::WiFiAP ap;
    ap.set_ssid(profile.ssid); ap.set_password(profile.password);
    if (!have_first) { first = ap; have_first = true; wifi->set_sta(ap); wifi->init_sta(2); }
    wifi->add_sta(ap);
  }
  if (have_first) wifi->start_connecting(first);
}

void FritzboxAha::wifi_loop_() {
  if (wifi_apply_requested_.exchange(false))
    set_timeout("wifi_apply", 1500, [this]() { apply_wifi_(); });
  // Migration: keep the already functioning network without reading it back
  // through HTTP. It becomes part of NVS on the next configuration save.
  auto *wifi = wifi::global_wifi_component;
  const bool connected = wifi->is_connected();
  const bool was_connected = network_ready_.exchange(connected);
  // Start only after association AND IP configuration. Do not wait for a poll
  // interval after boot/reconnect, and do not count missing WiFi as an AHA error.
  if (connected && !was_connected) update();
  if (!connected) return;
  xSemaphoreTake(web_mutex_, portMAX_DELAY);
  if (!runtime_config_->wifi_configured && runtime_config_->wifi[0].ssid.empty()) {
    auto sta = wifi->get_sta();
    if (!sta.get_ssid().empty()) {
      auto next = std::make_shared<gateway::Config>(*runtime_config_);
      next->wifi[0].ssid = sta.get_ssid().c_str();
      next->wifi[0].password = sta.get_password().c_str();
      runtime_config_ = next;
    }
  }
  xSemaphoreGive(web_mutex_);
}

void FritzboxAha::apply_runtime_config_() {
  auto c = config_copy_();
  if (c->revision == applied_revision_) return;
  if (c->host != host_ || c->username != username_ || c->password != password_) {
    host_ = c->host; username_ = c->username; password_ = c->password;
    sid_.clear(); failures_ = 0; retry_at_ = 0; discovered_.clear();
  }
  applied_revision_ = c->revision;
}

std::string FritzboxAha::config_json_(const gateway::Config &c, bool secrets) {
  return json::build_json([&](JsonObject r) {
    r["schema"] = 1; r["revision"] = c.revision; r["location"] = c.location;
    r["host"] = c.host; r["username"] = c.username;
    r["fritz_password_set"] = !c.password.empty();
    if (secrets) r["password"] = c.password;
    r["wifi_configured"] = c.wifi_configured;
    auto networks = r["wifi"].to<JsonArray>();
    for (const auto &w : c.wifi) {
      auto row = networks.add<JsonObject>();
      row["ssid"] = w.ssid; row["password_set"] = !w.password.empty();
      row["open"] = !w.ssid.empty() && w.password.empty();
      if (secrets) row["password"] = w.password;
    }
    auto mqtt = r["mqtt"].to<JsonObject>();
    mqtt["host"] = c.mqtt_host; mqtt["port"] = c.mqtt_port; mqtt["tls"] = c.mqtt_tls;
    mqtt["username"] = c.mqtt_username; mqtt["password_set"] = !c.mqtt_password.empty();
    if (secrets) mqtt["password"] = c.mqtt_password;
    auto rows = r["devices"].to<JsonArray>();
    for (const auto &d : c.devices) {
      auto row = rows.add<JsonObject>();
      row["ain"] = d.ain; row["name"] = d.name; row["enabled"] = d.enabled;
      row["unit"] = d.unit; row["base"] = d.base;
      row["actual_topic"] = d.actual_topic; row["target_topic"] = d.target_topic;
      row["command_topic"] = d.command_topic; row["battery_topic"] = d.battery_topic;
    }
  });
}

bool FritzboxAha::decode_config_(const std::string &body, const gateway::Config &previous,
                                gateway::Config &out, std::string &error) {
  if (body.size() > CONFIG_LIMIT) { error = "Einrichtung zu gross"; return false; }
  out = previous;
  bool parsed = json::parse_json(body, [&](JsonObject r) {
    auto string_field = [](JsonVariant v, std::string &dest) {
      if (!v.is<const char *>()) return false;
      dest = v.as<std::string>(); return true;
    };
    if (r["schema"] != 1 || !r["revision"].is<uint32_t>() || !string_field(r["location"], out.location) ||
        !string_field(r["host"], out.host) || !string_field(r["username"], out.username)) return false;
    out.revision = r["revision"].as<uint32_t>();
    if (!r["password"].isNull()) {
      std::string value;
      if (!string_field(r["password"], value)) return false;
      if (!value.empty()) out.password = value;
    }
    if (!r["wifi"].isNull()) {
      if (!r["wifi"].is<JsonArray>() || r["wifi"].size() != 2 || !r["wifi_configured"].is<bool>()) return false;
      out.wifi_configured = r["wifi_configured"].as<bool>();
      for (size_t i = 0; i < 2; i++) {
        auto row = r["wifi"][i]; auto &w = out.wifi[i];
        std::string ssid;
        if (!string_field(row["ssid"], ssid)) return false;
        // A replacement SSID must not silently inherit the old network password.
        if (ssid != w.ssid) {
          w.password.clear();
          for (const auto &old : previous.wifi) if (old.ssid == ssid) w.password = old.password;
        }
        if (ssid.empty()) w.password.clear();
        w.ssid = ssid;
        if (row["open"] == true) w.password.clear();
        else if (!row["password"].isNull()) {
          std::string value;
          if (!string_field(row["password"], value)) return false;
          if (!value.empty()) w.password = value;
        }
      }
    }
    auto mqtt = r["mqtt"].as<JsonObject>();
    if (mqtt.isNull() || !string_field(mqtt["host"], out.mqtt_host) || !string_field(mqtt["username"], out.mqtt_username) ||
        !mqtt["port"].is<unsigned>() || !mqtt["tls"].is<bool>()) return false;
    out.mqtt_port = mqtt["port"].as<unsigned>(); out.mqtt_tls = mqtt["tls"].as<bool>();
    if (!mqtt["password"].isNull()) {
      std::string value;
      if (!string_field(mqtt["password"], value)) return false;
      if (!value.empty()) out.mqtt_password = value;
    }
    if (!r["devices"].is<JsonArray>() || r["devices"].size() > gateway::MAX_DEVICES) return false;
    out.devices.clear();
    for (auto row : r["devices"].as<JsonArray>()) {
      gateway::Device d;
      if (!string_field(row["ain"], d.ain) || !string_field(row["name"], d.name) || !string_field(row["unit"], d.unit) ||
          !string_field(row["base"], d.base) || !row["enabled"].is<bool>() ||
          !string_field(row["actual_topic"], d.actual_topic) || !string_field(row["target_topic"], d.target_topic) ||
          !string_field(row["command_topic"], d.command_topic) || !string_field(row["battery_topic"], d.battery_topic)) return false;
      d.enabled = row["enabled"].as<bool>(); out.devices.push_back(d);
    }
    return true;
  });
  if (!parsed) { error = "Einrichtungsformat ungueltig oder unvollstaendig"; return false; }
  error = gateway::validate(out);
  return error.empty();
}

bool FritzboxAha::save_config_(const gateway::Config &c) {
  auto body = config_json_(c, true);
  if (body.size() > CONFIG_LIMIT) return false;
  nvs_handle_t handle;
  if (nvs_open(storage_namespace_.c_str(), NVS_READWRITE, &handle) != ESP_OK) return false;
  bool ok = nvs_set_blob(handle, "config_v1", body.data(), body.size()) == ESP_OK && nvs_commit(handle) == ESP_OK;
  nvs_close(handle);
  return ok;
}

bool FritzboxAha::queue_temperature_(const std::string &ain, float target, std::string &error, uint32_t expected_revision) {
  uint32_t encoded;
  if (!protocol::encode_target(target, encoded)) { error = "Nicht gesendet: Zielwert ausserhalb 8 bis 28 C"; return false; }
  xSemaphoreTake(web_mutex_, portMAX_DELAY);
  auto config = runtime_config_;
  const auto *mapping = gateway::find(*config, ain);
  bool ready = false;
  for (const auto &d : web_devices_) if (d.ain == ain && d.present == "1" && (d.error.empty() || d.error == "0")) ready = true;
  if (firmware_state_.installing || firmware_action_ == 2) {
    xSemaphoreGive(web_mutex_); error = "Firmwareinstallation aktiv"; return false;
  }
  if ((expected_revision != UINT32_MAX && config->revision != expected_revision) ||
      !mapping || !mapping->enabled || !commands_ || !web_last_.connected || !web_last_.authenticated ||
      web_last_.device_count < 0 || web_revision_ != config->revision || web_millis() - web_observation_at_ > 120000 || !ready) {
    xSemaphoreGive(web_mutex_); error = "Nicht gesendet: Thermostat nicht freigegeben oder keine aktuellen Daten"; return false;
  }
  bool expected = false;
  if (!command_busy_.compare_exchange_strong(expected, true)) {
    xSemaphoreGive(web_mutex_); error = "Ein Auftrag laeuft bereits; bitte Rueckmeldung abwarten"; return false;
  }
  Command command;
  std::snprintf(command.ain, sizeof(command.ain), "%s", ain.c_str());
  command.target = target; command.created_at = web_millis(); command.revision = config->revision;
  if (xQueueSend(commands_, &command, 0) != pdTRUE) {
    command_busy_ = false; xSemaphoreGive(web_mutex_); error = "Auftragsspeicher belegt"; return false;
  }
  web_command_ain_ = ain; web_command_message_ = "Auftrag angenommen; Rueckmeldung wird geprueft";
  xSemaphoreGive(web_mutex_);
  xSemaphoreGive(requests_);
  return true;
}

bool FritzboxAha::status_led_ready() {
  if (is_failed() || !web_mutex_ || xSemaphoreTake(web_mutex_, 0) != pdTRUE) return false;
  unsigned expected = 0;
  if (runtime_config_) {
    for (const auto &device : runtime_config_->devices)
      if (device.enabled && !device.command_topic.empty()) expected++;
  }
  const bool ready = runtime_config_ && mqtt_config_ &&
      runtime_config_->revision == mqtt_config_->revision &&
      web_revision_ == runtime_config_->revision &&
      web_last_.connected && web_last_.authenticated && web_last_.device_count >= 0 &&
      web_millis() - web_observation_at_ <= 120000 &&
      mqtt_connected_.load() && mqtt_seen_generation_ == mqtt_generation_.load() &&
      expected > 0 && mqtt_subscriptions_ == expected && mqtt_subacks_.load() >= expected;
  xSemaphoreGive(web_mutex_);
  return ready;
}

std::string FritzboxAha::status_json_() {
  xSemaphoreTake(web_mutex_, portMAX_DELAY);
  std::string body = json::build_json([&](JsonObject r) {
    r["csrf"] = csrf_token_; r["revision"] = runtime_config_->revision;
    r["location"] = runtime_config_->location;
    bool fresh = web_revision_ == runtime_config_->revision && web_last_.device_count >= 0 && web_millis() - web_observation_at_ <= 120000;
    r["connected"] = web_last_.connected; r["authenticated"] = web_last_.authenticated;
    r["fresh"] = fresh; r["diagnostic"] = web_last_.diagnostic; r["method"] = web_last_.method;
    r["device_count"] = web_last_.device_count; r["uptime"] = web_millis()/1000;
    auto http = r["http"].to<JsonObject>();
    http["requests"] = web_http_requests_; http["failures"] = web_http_failures_;
    auto observation = [&](JsonObject row, const HttpObservation &h) {
      row["operation"] = h.operation; row["phase"] = h.phase;
      row["at_ms"] = h.at_ms; row["elapsed_ms"] = h.elapsed_ms;
      row["status"] = h.status; row["complete"] = h.complete;
      row["bytes"] = h.bytes; row["error"] = h.error; row["errno"] = h.socket_error;
      row["read"] = h.last_read; row["wifi_connected"] = h.wifi_connected;
      row["rssi"] = h.rssi; row["free_heap"] = h.heap;
    };
    observation(http["last"].to<JsonObject>(), web_http_last_);
    if (web_http_failures_) observation(http["last_failure"].to<JsonObject>(), web_http_failure_);
    else http["last_failure"] = nullptr;
    r["mqtt"] = mqtt_status_text_;
    r["mqtt_connected"] = mqtt_connected_.load();
    r["mqtt_rx"] = mqtt_rx_.load(); r["mqtt_ignored"] = mqtt_ignored_.load();
    r["mqtt_rejected"] = mqtt_rejected_.load(); r["mqtt_accepted"] = mqtt_accepted_.load();
    r["mqtt_pubacks"] = mqtt_acks_.load(); r["mqtt_subacks"] = mqtt_subacks_.load();
    r["mqtt_last_input"] = mqtt_last_input_;
    auto *wifi = wifi::global_wifi_component;
    r["wifi_connected"] = wifi->is_connected();
    r["wifi_ssid"] = wifi->is_connected() ? std::string(wifi->get_sta().get_ssid().c_str()) : std::string();
    r["ap_ssid"] = std::string(wifi->get_ap().get_ssid().c_str());
    r["ap_address"] = "192.168.4.1";
    r["busy"] = command_busy_.load(); r["command_ain"] = web_command_ain_; r["command_message"] = web_command_message_;
    auto rows = r["devices"].to<JsonArray>();
    std::set<std::string> emitted;
    auto append = [&](const protocol::Thermostat *t, const gateway::Device *cfg) {
      std::string ain = cfg ? cfg->ain : t->ain;
      if (!emitted.insert(ain).second) return;
      auto row = rows.add<JsonObject>(); row["ain"] = ain;
      row["name"] = cfg && cfg->name != "Thermostat" ? cfg->name : t && !t->name.empty() ? t->name : cfg ? cfg->name : ain;
      row["model"] = t ? t->product : ""; row["configured"] = cfg != nullptr;
      row["enabled"] = cfg && cfg->enabled;
      bool available = fresh && t && t->present == "1";
      row["available"] = available;
      row["ready"] = available && web_last_.connected && web_last_.authenticated && (t->error.empty() || t->error == "0");
      row["actual"] = nullptr; row["target"] = nullptr; row["battery"] = nullptr;
      if (available) {
        float actual = protocol::actual_temperature(*t), target = protocol::target_temperature(*t), battery = protocol::battery_percent(*t);
        if (std::isfinite(actual)) row["actual"] = actual;
        if (std::isfinite(target)) row["target"] = target;
        if (std::isfinite(battery)) row["battery"] = battery;
        row["mode"] = protocol::target_mode(*t); row["battery_low"] = t->battery_low;
        row["error"] = t->error;
      }
      row["valve"] = nullptr; row["battery_voltage"] = nullptr;
    };
    for (const auto &cfg : runtime_config_->devices) {
      const protocol::Thermostat *found = nullptr;
      for (const auto &t : web_devices_) if (t.ain == cfg.ain) found = &t;
      append(found, &cfg);
    }
    for (const auto &t : web_devices_) if (rows.size() < 32) append(&t, gateway::find(*runtime_config_, t.ain));
  });
  xSemaphoreGive(web_mutex_);
  return body;
}

bool FritzboxAha::canHandle(AsyncWebServerRequest *request) const {
  char buffer[AsyncWebServerRequest::URL_BUF_SIZE];
  auto url = request->url_to(buffer);
  if (url == "/") return !request->hasParam("esphome");
  return url == "/gateway/" || url == "/gateway" || url == "/gateway/api/status" ||
         url == "/gateway/api/firmware" || url == "/gateway/api/firmware/check" || url == "/gateway/api/firmware/install" ||
         url == "/gateway/api/mqtt-test" || url == "/gateway/api/config" || url == "/gateway/api/target" || url == "/gateway/api/poll";
}

void FritzboxAha::serve_json_(AsyncWebServerRequest *request, int code, const std::string &body) {
  auto *response = request->beginResponse(code, "application/json; charset=utf-8", body);
  // ESPHome IDF 2026.6.4 maps most status codes (including 202) to 500.
  // Set the actual IDF status after beginResponse(), which initializes it.
  const char *status = code == 202 ? "202 Accepted" : code == 400 ? "400 Bad Request" :
      code == 409 ? "409 Conflict" : code == 404 ? "404 Not Found" : code == 403 ? "403 Forbidden" : code == 405 ? "405 Method Not Allowed" : nullptr;
  if (status) httpd_resp_set_status(*request, status);
  response->addHeader("Cache-Control", "no-store"); request->send(response);
}

void FritzboxAha::handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (index == 0) body_request_ = request;
  request_body_.append(data, len, index, total);
}

void FritzboxAha::handleRequest(AsyncWebServerRequest *request) {
  // Consume callback state even when header validation below rejects the POST.
  // No request payloads (which can contain passwords) are logged.
  std::string body;
  const bool delivered = body_request_ == request;
  const bool body_complete = delivered && request_body_.take(request->contentLength(), body);
  body_request_ = nullptr;
  request_body_.reset();
  char buffer[AsyncWebServerRequest::URL_BUF_SIZE];
  auto url = request->url_to(buffer);
  auto fail = [&](int code, const std::string &message) {
    std::string body = json::build_json([&](JsonObject r) { r["error"] = message; }); serve_json_(request, code, body);
  };
  if (request->method() == HTTP_GET) {
    if (url == "/gateway/api/firmware") { serve_json_(request, 200, firmware_json_()); return; }
    if (url == "/gateway/api/status") { serve_json_(request, 200, status_json_()); return; }
    if (url == "/gateway/api/config") { serve_json_(request, 200, config_json_(*config_copy_(), false)); return; }
    if (url == "/" || url == "/gateway" || url == "/gateway/") {
      auto *response = request->beginResponse(200, "text/html; charset=utf-8",
          reinterpret_cast<const uint8_t *>(GATEWAY_PAGE), sizeof(GATEWAY_PAGE)-1);
      response->addHeader("Cache-Control", "no-store"); request->send(response); return;
    }
    fail(405, "POST erforderlich"); return;
  }
  if (request->method() != HTTP_POST) { fail(405, "Methode nicht erlaubt"); return; }
  auto origin = request->get_header("Origin"), host = request->get_header("Host"), token = request->get_header("X-Gateway-Token");
  if (!origin.has_value() || !host.has_value() || *origin != "http://" + *host || !token.has_value() || *token != csrf_token_) {
    fail(403, "Seite neu laden und erneut versuchen"); return;
  }
  auto content_type = request->get_header("Content-Type");
  if (!content_type.has_value() || content_type->find("application/json") != 0 || request->contentLength() > CONFIG_LIMIT) {
    fail(400, "JSON erforderlich oder Anfrage zu gross"); return;
  }
  if (delivered) {
    if (!body_complete) { fail(400, "Anfrage unvollstaendig"); return; }
  } else {
    // ESPHome 2026.6 leaves JSON unread; 2026.10 supplies it through handleBody.
    // Never read the same payload twice. The legacy path remains supported.
    body.resize(request->contentLength()); size_t offset = 0;
    httpd_req_t *raw = *request;
    while (offset < body.size()) {
      int n = httpd_req_recv(raw, body.data()+offset, body.size()-offset);
      if (n <= 0) { fail(400, "Anfrage unvollstaendig"); return; }
      offset += n;
    }
  }
  if (url == "/gateway/api/firmware/check" || url == "/gateway/api/firmware/install") {
    const bool install = url == "/gateway/api/firmware/install";
    std::string version, error;
    bool valid = json::parse_json(body, [&](JsonObject r) {
      if (!install) return r.size() == 0;
      if (r.size() != 2 || !r["confirm"].is<bool>() || !r["confirm"].as<bool>() || !r["version"].is<const char *>()) return false;
      version = r["version"].as<std::string>(); return version.size() <= 32;
    });
    if (!valid) { fail(400, "Bestaetigung und angezeigte Version erforderlich"); return; }
    if (!firmware_request_(install, version, error)) { fail(409, error); return; }
    serve_json_(request, 202, "{\"accepted\":true}"); return;
  }
  if (url == "/gateway/api/mqtt-test") {
    if (!mqtt_connected_) { fail(409, "MQTT nicht verbunden"); return; }
    mqtt_test_requested_ = true;
    serve_json_(request, 202, "{\"accepted\":true,\"test_value\":9.9}"); return;
  }
  if (url == "/gateway/api/poll") { update(); serve_json_(request, 202, "{\"accepted\":true}"); return; }
  if (url == "/gateway/api/target") {
    std::string ain; float target = NAN;
    bool parsed = json::parse_json(body, [&](JsonObject r) {
      if (!r["ain"].is<const char *>() || !r["target"].is<float>()) return false;
      ain = r["ain"].as<std::string>(); target = r["target"].as<float>(); return true;
    });
    if (parsed && protocol::no_command(target)) {
      serve_json_(request, 200, "{\"ignored\":true,\"reason\":\"Kein Stellauftrag (9.9); nichts gesendet\"}"); return;
    }
    std::string error;
    if (!parsed || !queue_temperature_(ain, target, error)) { fail(409, parsed ? error : "Auftrag ungueltig"); return; }
    serve_json_(request, 202, "{\"accepted\":true}"); return;
  }
  if (url == "/gateway/api/config") {
    auto old = config_copy_(); auto next = std::make_shared<gateway::Config>(); std::string error;
    if (!decode_config_(body, *old, *next, error)) { fail(400, error); return; }
    xSemaphoreTake(web_mutex_, portMAX_DELAY);
    if (firmware_state_.installing || firmware_action_ == 2 || command_busy_ || next->revision != runtime_config_->revision) {
      xSemaphoreGive(web_mutex_); fail(409, "Auftrag aktiv oder Einrichtung inzwischen geaendert; neu laden"); return;
    }
    bool wifi_changed = next->wifi != old->wifi || next->wifi_configured != old->wifi_configured;
    next->revision++;
    if (!save_config_(*next)) { xSemaphoreGive(web_mutex_); fail(500, "Speichern fehlgeschlagen; bisheriger Stand bleibt aktiv"); return; }
    runtime_config_ = next;
    xSemaphoreGive(web_mutex_);
    if (wifi_changed) wifi_apply_requested_ = true;
    update(); serve_json_(request, 200, wifi_changed ? "{\"saved\":true,\"wifi_reconnecting\":true}" : "{\"saved\":true}"); return;
  }
  fail(404, "Nicht gefunden");
}
void FritzboxAha::mqtt_message_(const std::string &message) {
  xSemaphoreTake(web_mutex_, portMAX_DELAY);
  mqtt_status_text_ = message;
  xSemaphoreGive(web_mutex_);
}

void FritzboxAha::mqtt_event_(void *arg, esp_event_base_t, int32_t id, void *data) {
  auto *self = static_cast<FritzboxAha *>(arg);
  auto *e = static_cast<esp_mqtt_event_t *>(data);
  switch (id) {
    case MQTT_EVENT_BEFORE_CONNECT: self->mqtt_message_("Verbindung wird aufgebaut"); break;
    case MQTT_EVENT_CONNECTED:
      self->mqtt_connected_ = true;
      self->mqtt_generation_++;
      self->mqtt_message_("Verbunden");
      break;
    case MQTT_EVENT_DISCONNECTED:
      if (self->mqtt_connected_.exchange(false)) self->mqtt_message_("Verbindung unterbrochen; erneuter Versuch automatisch");
      self->mqtt_subacks_ = 0;
      break;
    case MQTT_EVENT_SUBSCRIBED: self->mqtt_subacks_++; break;
    case MQTT_EVENT_PUBLISHED: self->mqtt_acks_++; break;
    case MQTT_EVENT_ERROR:
      self->mqtt_message_(e->error_handle && e->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED ?
          "Broker lehnt Anmeldung ab; Zugang und Berechtigung pruefen" : "Netzwerk/TLS-Fehler; Broker und Port pruefen");
      break;
    case MQTT_EVENT_DATA: {
      self->mqtt_rx_++;
      if (!self->mqtt_inputs_ || e->current_data_offset != 0 || e->total_data_len != e->data_len ||
          e->data_len < 1 || e->data_len > 32 || e->topic_len < 1 || e->topic_len > 160 ||
          std::memchr(e->data, '\0', e->data_len) || std::memchr(e->topic, '\0', e->topic_len)) {
        self->mqtt_rejected_++; break;
      }
      MqttInput input;
      std::memcpy(input.topic, e->topic, e->topic_len);
      std::memcpy(input.payload, e->data, e->data_len);
      input.retained = e->retain; input.duplicate = e->dup;
      input.revision = self->mqtt_active_revision_.load(); input.created_at = web_millis();
      if (xQueueSend(self->mqtt_inputs_, &input, 0) != pdTRUE) self->mqtt_rejected_++;
      break;
    }
    default: break;
  }
}

void FritzboxAha::mqtt_loop_() {
  if (!mqtt_inputs_) {
    mqtt_inputs_ = xQueueCreate(8, sizeof(MqttInput));
    if (!mqtt_inputs_) { mqtt_message_("MQTT-Empfangsspeicher fehlt"); return; }
  }
  auto c = config_copy_();
  if (mqtt_test_requested_.exchange(false)) mqtt_test_();
  MqttInput input;
  if (xQueueReceive(mqtt_inputs_, &input, 0) == pdTRUE) {
    std::string result;
    float target = NAN;
    auto parsed = gateway::parse_target_payload(input.payload, target);
    const auto *device = gateway::command_device(*c, input.topic);
    if (input.retained || input.duplicate || input.revision != c->revision ||
        web_millis() - input.created_at > 10000 || !mqtt_connected_ || !device) {
      mqtt_rejected_++; result = "Nicht gesendet: gespeicherte/doppelte/veraltete Nachricht oder keine Freigabe";
    } else if (parsed == gateway::TargetPayload::IGNORE) {
      mqtt_ignored_++; result = "Kein Stellauftrag (9.9); nichts an FRITZ!Box gesendet";
    } else if (parsed == gateway::TargetPayload::INVALID) {
      mqtt_rejected_++; result = "Nicht gesendet: ungueltiger MQTT-Sollwert";
    } else {
      std::string error;
      if (queue_temperature_(device->ain, target, error, input.revision)) {
        mqtt_accepted_++; result = "MQTT-Auftrag angenommen; FRITZ!Box-Rueckmeldung wird geprueft";
      } else { mqtt_rejected_++; result = error; }
    }
    xSemaphoreTake(web_mutex_, portMAX_DELAY); mqtt_last_input_ = result; xSemaphoreGive(web_mutex_);
  }
  uint32_t now = web_millis();
  if (mqtt_next_check_ && static_cast<int32_t>(now - mqtt_next_check_) < 0) return;
  mqtt_next_check_ = now + 1000;
  if (!mqtt_config_ || c->revision != mqtt_config_->revision) {
    if (mqtt_client_) {
      mqtt_connected_ = false;
      esp_mqtt_client_stop(mqtt_client_);
      esp_mqtt_client_destroy(mqtt_client_);
      mqtt_client_ = nullptr;
    }
    mqtt_config_ = c; mqtt_active_revision_ = c->revision;
    mqtt_seen_generation_ = mqtt_generation_.load(); mqtt_last_observation_ = 0;
    mqtt_subscriptions_ = 0; mqtt_subacks_ = 0;
  }
  if (c->mqtt_host.empty()) { mqtt_message_("Nicht eingerichtet"); return; }
  if (!wifi::global_wifi_component->is_connected()) { mqtt_message_("Warte auf WLAN"); return; }
  if (!mqtt_client_) {
    if (c->mqtt_tls && std::time(nullptr) < 1700000000) {
      if (!esp_sntp_enabled()) {
        esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
        static char ntp_server[] = "pool.ntp.org";
        esp_sntp_setservername(0, ntp_server);
        esp_sntp_init();
      }
      mqtt_message_("TLS: Warte auf NTP-Uhrzeit"); return;
    }
    mqtt_client_id_ = "fritz2mqtt-" + get_mac_address();
    esp_mqtt_client_config_t cfg{};
    cfg.broker.address.hostname = mqtt_config_->mqtt_host.c_str();
    cfg.broker.address.port = mqtt_config_->mqtt_port;
    cfg.broker.address.transport = c->mqtt_tls ? MQTT_TRANSPORT_OVER_SSL : MQTT_TRANSPORT_OVER_TCP;
    if (c->mqtt_tls) cfg.broker.verification.crt_bundle_attach = esp_crt_bundle_attach;
    cfg.credentials.client_id = mqtt_client_id_.c_str();
    if (!c->mqtt_username.empty()) cfg.credentials.username = mqtt_config_->mqtt_username.c_str();
    if (!c->mqtt_password.empty()) cfg.credentials.authentication.password = mqtt_config_->mqtt_password.c_str();
    cfg.session.keepalive = 30;
    cfg.session.disable_clean_session = false;
    cfg.session.protocol_ver = MQTT_PROTOCOL_V_3_1_1;
    cfg.network.timeout_ms = 1500;
    cfg.network.reconnect_timeout_ms = 10000;
    cfg.buffer.size = 1024;
    cfg.task.stack_size = 6144;
    cfg.outbox.limit = 8192;
    mqtt_client_ = esp_mqtt_client_init(&cfg);
    if (!mqtt_client_ || esp_mqtt_client_register_event(mqtt_client_, MQTT_EVENT_ANY, mqtt_event_, this) != ESP_OK ||
        esp_mqtt_client_start(mqtt_client_) != ESP_OK) {
      if (mqtt_client_) { esp_mqtt_client_destroy(mqtt_client_); mqtt_client_ = nullptr; }
      mqtt_message_("MQTT konnte nicht gestartet werden"); mqtt_next_check_ = now + 10000;
    }
    return;
  }
  if (!mqtt_connected_) return;
  auto generation = mqtt_generation_.load();
  if (generation != mqtt_seen_generation_) {
    mqtt_seen_generation_ = generation; mqtt_subscriptions_ = 0; mqtt_subacks_ = 0;
    for (const auto &d : c->devices) if (d.enabled && !d.command_topic.empty()) {
      // QoS 0 and a clean session: no queued or retransmitted actuator commands.
      if (esp_mqtt_client_subscribe(mqtt_client_, d.command_topic.c_str(), 0) >= 0) mqtt_subscriptions_++;
      else mqtt_message_("Verbunden; Topic-Abonnement fehlgeschlagen");
    }
    mqtt_last_observation_ = 0;
  }
  mqtt_publish_state_();
}

void FritzboxAha::mqtt_publish_state_() {
  std::vector<protocol::Thermostat> devices;
  uint32_t observed;
  xSemaphoreTake(web_mutex_, portMAX_DELAY);
  observed = web_observation_at_;
  bool fresh = runtime_config_->revision == mqtt_config_->revision && web_revision_ == mqtt_config_->revision && web_last_.device_count >= 0 &&
      web_last_.authenticated && web_last_.connected && web_millis() - observed <= 120000;
  if (fresh && observed != mqtt_last_observation_) devices = web_devices_;
  xSemaphoreGive(web_mutex_);
  if (!fresh || observed == mqtt_last_observation_) return;
  bool sent = true;
  for (const auto &d : mqtt_config_->devices) {
    for (const auto &t : devices) if (t.ain == d.ain && t.present == "1") {
      auto publish = [&](const std::string &topic, float value, bool integer = false) {
        if (topic.empty() || !std::isfinite(value)) return;
        char payload[24]; std::snprintf(payload, sizeof(payload), integer ? "%.0f" : "%.1f", value);
        // Retained measurements, with bounded outbox and broker PUBACK evidence.
        if (esp_mqtt_client_enqueue(mqtt_client_, topic.c_str(), payload, 0, 1, true, false) < 0) sent = false;
      };
      publish(d.actual_topic, protocol::actual_temperature(t));
      publish(d.target_topic, protocol::target_temperature(t));
      publish(d.battery_topic, protocol::battery_percent(t), true);
    }
  }
  if (sent) mqtt_last_observation_ = observed;
}

bool FritzboxAha::mqtt_test_() {
  // Broker round trip uses only the reserved sentinel, never a thermostat value.
  if (!mqtt_client_ || !mqtt_connected_ || !mqtt_config_) return false;
  auto c = config_copy_();
  if (c->revision != mqtt_config_->revision || !mqtt_subacks_) return false;
  for (const auto &d : c->devices) if (d.enabled && !d.command_topic.empty())
    return esp_mqtt_client_enqueue(mqtt_client_, d.command_topic.c_str(), "9.9", 3, 0, false, true) >= 0;
  return false;
}

}  // namespace esphome::fritzbox_aha
