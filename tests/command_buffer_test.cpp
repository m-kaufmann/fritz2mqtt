#include "../components/fritzbox_aha/command_buffer.h"
#include <cassert>
#include <cstdio>
#include <iostream>
using namespace esphome::fritzbox_aha::gateway;
CommandBuffer::Command command(const char *ain, float value, uint32_t now = 10) {
  CommandBuffer::Command c;
  std::snprintf(c.ain, sizeof(c.ain), "%s", ain);
  c.target = value; c.created_at = now; c.revision = 5;
  return c;
}
int main() {
  Config config; config.revision = 5;
  for (int i = 0; i < 9; ++i) {
    Device d; d.ain = std::to_string(i); d.enabled = true;
    config.devices.push_back(d);
  }
  CommandBuffer b; std::string error;
  for (int i = 0; i < 8; ++i) assert(b.enqueue(command(std::to_string(i).c_str(), 20), config, 10, error));
  assert(!b.enqueue(command("8", 20), config, 10, error));
  assert(b.diagnostic("8")->overflow == 1);
  assert(b.enqueue(command("0", 22), config, 11, error));
  assert(b.enqueue(command("0", 9.9), config, 12, error));
  assert(b.pending() == 8 && b.diagnostic("0")->replaced == 1 && b.diagnostic("0")->ignored == 1);
  CommandBuffer::Command c;
  assert(b.start(config, 13, c) && c.target == 22);
  assert(!b.start(config, 14, c));
  b.attempted(15); b.finish("confirmed", 16);
  assert(b.busy() && b.diagnostic("0")->attempted == 1);
  for (int i = 1; i < 8; ++i) {
    assert(b.start(config, 17, c) && c.ain == std::to_string(i));
    b.finish("confirmed", 18);
  }
  assert(!b.busy());
  assert(b.enqueue(command("0", 20, UINT32_MAX - 100), config, UINT32_MAX - 100, error));
  assert(!b.start(config, CommandBuffer::MAX_AGE_MS, c)); // uptime wrap
  assert(b.diagnostic("0")->expired == 1);
  assert(b.enqueue(command("0", 20), config, 10, error));
  ++config.revision;
  assert(!b.start(config, 11, c) && !b.busy());
  --config.revision;
  assert(!b.enqueue(command("0", 20.1), config, 10, error));
  assert(!b.enqueue(command("missing", 20), config, 10, error));
  assert(!b.enqueue(command("0", NAN), config, 10, error));
  assert(b.enqueue(command("0", 20), config, 10, error));
  assert(b.start(config, 10, c));
  assert(b.enqueue(command("0", 21), config, 10, error));
  assert(b.superseded());
  b.defer("session", 10);
  assert(b.pending() == 1);
  assert(b.start(config, 10, c) && c.target == 21);
  b.finish("confirmed", 10);
  std::cout << "PASS: bounded command buffer, FIFO replacement, neutral input, overflow, revision, age and uptime wrap\n";
}
