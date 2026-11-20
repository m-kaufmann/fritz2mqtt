#pragma once

#include <deque>
#include "runtime_config.h"
#include "aha_protocol.h"

namespace esphome::fritzbox_aha::gateway {

// All access is serialized by the gateway's web_mutex_.
class CommandBuffer {
 public:
  static constexpr size_t CAPACITY = 8;
  static constexpr uint32_t MAX_AGE_MS = 180000;
  struct Command {
    char ain[257] = "";
    float target = NAN;
    bool outlet = false;
    uint32_t created_at = 0, revision = 0, id = 0;
  };
  struct Diagnostic {
    std::string ain, state, message;
    uint32_t accepted = 0, replaced = 0, ignored = 0, rejected = 0, overflow = 0;
    uint32_t expired = 0, attempted = 0, finished = 0, deferred = 0, superseded = 0;
    uint32_t at_ms = 0, id = 0;
    float last_attempted_target = NAN;
  };

  bool busy() const { return running_ || !pending_.empty(); }
  bool running() const { return running_; }
  bool was_deferred() const { return deferred_; }
  size_t pending() const { return pending_.size(); }
  const Command &active() const { return active_; }
  const Diagnostic *diagnostic(const std::string &ain) const {
    for (const auto &d : diagnostics_) if (d.ain == ain) return &d;
    return nullptr;
  }
  bool valid(const Command &c, const Config &config) const {
    const auto *d = find(config, c.ain);
    return d && d->enabled && d->outlet == c.outlet && c.revision == config.revision;
  }
  static bool expired(const Command &c, uint32_t now) { return uint32_t(now - c.created_at) >= MAX_AGE_MS; }

  void reject(const std::string &ain, const std::string &reason, uint32_t now, bool overflow = false) {
    if (ain.empty() || ain.size() > 256) return;
    auto &d = event(ain, "rejected", reason, now);
    ++d.rejected;
    if (overflow) ++d.overflow;
  }
  void ignore(const std::string &ain, uint32_t now) {
    // Keep the state/message of a live command, while counting the neutral input.
    for (auto &d : diagnostics_) if (d.ain == ain) { ++d.ignored; return; }
    ++event(ain, "ignored", "Kein Stellauftrag (9.9)", now).ignored;
  }
  bool enqueue(Command c, const Config &config, uint32_t now, std::string &error) {
    if (protocol::no_command(c.target)) {
      ignore(c.ain, now);
      return true;
    }
    uint32_t encoded;
    const bool value_ok = c.outlet ? c.target == 0 || c.target == 1 : protocol::encode_target(c.target, encoded);
    if (!valid(c, config) || !value_ok || expired(c, now)) {
      error = "Auftrag ungueltig, veraltet oder Einrichtung geaendert";
      reject(c.ain, error, now);
      return false;
    }
    c.id = ++next_id_;
    for (auto &waiting : pending_) if (std::string(waiting.ain) == c.ain) {
      // Keep this device's FIFO position, so repeated updates cannot starve it or its neighbours.
      waiting = c;
      auto &d = event(c.ain, "queued", "Wartender Sollwert ersetzt", now, c.id);
      ++d.accepted; ++d.replaced;
      return true;
    }
    if (pending_.size() == CAPACITY) {
      error = "Auftragsspeicher belegt";
      reject(c.ain, error, now, true);
      return false;
    }
    pending_.push_back(c);
    ++event(c.ain, "queued", "Auftrag gepuffert", now, c.id).accepted;
    return true;
  }
  bool start(const Config &config, uint32_t now, Command &out) {
    if (running_) return false;
    while (!pending_.empty()) {
      Command c = pending_.front();
      pending_.pop_front();
      if (expired(c, now)) {
        ++event(c.ain, "expired", "Wartender Auftrag abgelaufen", now, c.id).expired;
        continue;
      }
      if (!valid(c, config)) {
        reject(c.ain, "Einrichtung geaendert; Auftrag verworfen", now);
        continue;
      }
      active_ = c; running_ = true; deferred_ = false; out = c;
      event(c.ain, "preflight", "Verbindung und Geraet werden geprueft", now, c.id);
      return true;
    }
    return false;
  }
  bool superseded() const {
    for (const auto &c : pending_) if (std::string(c.ain) == active_.ain) return true;
    return false;
  }
  void defer(const std::string &reason, uint32_t now) {
    if (!running_) return;
    const bool replaced = superseded();
    if (!replaced) pending_.push_back(active_);
    auto &d = event(active_.ain, replaced ? "superseded" : "waiting", reason, now, active_.id);
    ++d.deferred;
    if (replaced) ++d.superseded;
    running_ = false; deferred_ = true;
  }
  void attempted(uint32_t now) {
    auto &d = event(active_.ain, "attempted", "HTTP-Schreibversuch gestartet", now, active_.id);
    ++d.attempted; d.last_attempted_target = active_.target;
  }
  void finish(const std::string &message, uint32_t now, const char *state = "finished") {
    auto &d = event(active_.ain, state, message, now, active_.id);
    ++d.finished;
    if (d.state == "expired") ++d.expired;
    if (d.state == "rejected") ++d.rejected;
    if (d.state == "superseded") ++d.superseded;
    running_ = false;
  }
  bool waiting(const std::string &ain) const {
    for (const auto &c : pending_) if (c.ain == ain) return true;
    return false;
  }

 private:
  std::deque<Command> pending_;
  std::deque<Diagnostic> diagnostics_;
  Command active_;
  bool running_ = false, deferred_ = false;
  uint32_t next_id_ = 0;
  Diagnostic &event(const std::string &ain, const std::string &state,
                    const std::string &message, uint32_t now, uint32_t id = 0) {
    Diagnostic *d = nullptr;
    for (auto &row : diagnostics_) if (row.ain == ain) { d = &row; break; }
    if (!d) {
      if (diagnostics_.size() == 16) diagnostics_.pop_front();
      diagnostics_.push_back({});
      d = &diagnostics_.back(); d->ain = ain;
    }
    d->state = state; d->message = message; d->at_ms = now; d->id = id;
    return *d;
  }
};
}  // namespace esphome::fritzbox_aha::gateway
