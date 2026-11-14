#include "../components/fritzbox_aha/http_request_body.h"
#include <cassert>
#include <iostream>
#include <limits>
using esphome::fritzbox_aha::gateway::HttpRequestBody;
int main() {
  HttpRequestBody b;
  const std::string input = R"({"ain":"16000 0200077","target":9.9})";
  const auto *bytes = reinterpret_cast<const uint8_t *>(input.data());
  std::string out;
  b.append(bytes, input.size(), 0, input.size());
  assert(b.take(input.size(), out) && out == input);
  assert(!b.take(input.size(), out) && out.empty()); // no replay
  for (size_t split = 1; split < input.size(); ++split) {
    b.append(bytes, split, 0, input.size());
    b.append(bytes + split, input.size() - split, split, input.size());
    assert(b.take(input.size(), out) && out == input);
  }
  b.append(bytes, 3, 0, input.size());
  assert(!b.take(input.size(), out)); // truncated
  b.append(bytes, 3, 0, input.size());
  b.append(bytes + 4, 2, 4, input.size());
  assert(!b.take(input.size(), out)); // gap
  b.append(bytes, 3, 0, input.size());
  b.append(bytes + 1, 2, 1, input.size());
  assert(!b.take(input.size(), out)); // overlap
  b.append(bytes, 3, 0, input.size());
  b.append(bytes + 3, 2, 3, input.size() + 1);
  assert(!b.take(input.size(), out)); // total changed
  std::string limit(HttpRequestBody::MAX_BYTES, 'x');
  b.append(reinterpret_cast<const uint8_t *>(limit.data()), limit.size(), 0, limit.size());
  assert(b.take(limit.size(), out) && out == limit);
  b.append(bytes, 1, 0, HttpRequestBody::MAX_BYTES + 1);
  assert(!b.take(HttpRequestBody::MAX_BYTES + 1, out));
  b.append(bytes, 1, std::numeric_limits<size_t>::max(), 2);
  assert(!b.take(2, out));
  b.append(bytes, 3, 0, input.size());
  assert(!b.take(0, out)); // aborted body cannot become next empty request
  b.append(nullptr, 0, 0, 0);
  assert(b.take(0, out) && out.empty());
  b.append(nullptr, 1, 0, 1);
  assert(!b.take(1, out));
  b.append(bytes, input.size(), 0, input.size());
  assert(b.take(input.size(), out) && out == input); // recovery
  std::cout << "HTTP body: complete, fragmented, rejected, bounded, no replay: PASS\n";
}
