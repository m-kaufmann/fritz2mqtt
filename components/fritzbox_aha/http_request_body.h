#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

namespace esphome::fritzbox_aha::gateway {
// Bounded assembler for ESPHome IDF's handleBody callbacks (2026.10+).
// Never parse or dispatch partial, reordered or oversized request bodies.
class HttpRequestBody {
 public:
  static constexpr size_t MAX_BYTES = 16384;
  void reset() { body_.clear(); total_ = 0; seen_ = false; valid_ = false; }
  void append(const uint8_t *data, size_t len, size_t index, size_t total) {
    if (index == 0) {
      reset();
      seen_ = true;
      valid_ = total <= MAX_BYTES;
      total_ = total;
    }
    if (!seen_ || !valid_ || total != total_ || index != body_.size() ||
        index > total || len > total - index || (len && !data)) {
      valid_ = false;
      body_.clear();
      return;
    }
    if (len) body_.append(reinterpret_cast<const char *>(data), len);
  }
  bool take(size_t expected, std::string &out) {
    const bool complete = seen_ && valid_ && total_ == expected && body_.size() == expected;
    out.clear();
    if (complete) out.swap(body_);
    reset();
    return complete;
  }
 private:
  std::string body_;
  size_t total_ = 0;
  bool seen_ = false, valid_ = false;
};
}  // namespace esphome::fritzbox_aha::gateway
