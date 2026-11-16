#include "domain/lift/decoded_instruction_cache.h"
#include "domain/lift/engine.h"
#include "domain/lift/runtime.h"
#include <cassert>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>
using namespace hexlith::domain;
void same(const arm::CpuState &a, const arm::CpuState &b) {
  assert(a.registers == b.registers && arm::PackCpsr(a) == arm::PackCpsr(b));
  assert(a.float_registers == b.float_registers && a.fpscr == b.fpscr);
  assert(a.exclusive_open == b.exclusive_open &&
         a.exclusive_address == b.exclusive_address);
  assert(a.thread_id_read_only == b.thread_id_read_only &&
         a.thread_id_read_write == b.thread_id_read_write);
}
int main() {
  memory::ProcessMemory a, b;
  assert(a.Map(0x1000, 0x1000, memory::Backing::Allocate(0x1000), 0,
               memory::Access::ReadWrite));
  assert(b.Map(0x1000, 0x1000, memory::Backing::Allocate(0x1000), 0,
               memory::Access::ReadWrite));
  lift::DecodedInstructionCache cache;
  auto step = [&](arm::CpuState state) {
    auto expected = state, actual = state;
    auto x = arm::Step(expected, a), y = cache.Step(actual, b);
    assert(x.reason == y.reason && x.detail == y.detail);
    same(expected, actual);
    for (unsigned at = 0x1000; at < 0x2000; at += 4)
      assert(a.Read32(at) == b.Read32(at));
    return y;
  };
  auto write = [&](unsigned at, unsigned word) {
    assert(a.Write32(at, word) && b.Write32(at, word));
  };
  arm::CpuState s;
  s.registers[15] = 0x1000;
  write(0x1000, 0xe2800001);
  step(s);
  step(s);
  assert(cache.Counted().hits > 0);
  write(0x1000, 0xe2800002);
  step(s); // changed code at the same address
  write(0x1000, 0x02800001);
  s.zero = false;
  step(s);
  s.zero = true;
  step(s); // cached condition must use live flags
  write(0x1000, 0xea000000);
  step(s);
  write(0x1004, 0xea000000);
  s.registers[15] = 0x1004;
  step(s); // same encoding, different PC
  s = {};
  s.registers[15] = 0x1ffe;
  s.thumb = true;
  assert(a.Write16(0x1ffe, 0x2007) && b.Write16(0x1ffe, 0x2007));
  assert(step(s).reason == arm::StopReason::None);
  step(s);
  assert(a.Write16(0x1ffe, 0xf000) && b.Write16(0x1ffe, 0xf000));
  auto stop = step(s);
  assert(stop.reason == arm::StopReason::FetchFault && stop.detail == 0x2000);
  assert(a.Map(0x2000, 0x1000, memory::Backing::Allocate(0x1000), 0,
               memory::Access::ReadWrite));
  assert(b.Map(0x2000, 0x1000, memory::Backing::Allocate(0x1000), 0,
               memory::Access::ReadWrite));
  assert(a.Write16(0x2000, 0xf800) && b.Write16(0x2000, 0xf800));
  step(s);
  step(s);
  assert(a.Unmap(0x2000, 0x1000) && b.Unmap(0x2000, 0x1000));
  assert(step(s).reason == arm::StopReason::FetchFault);
  // An instruction-only invalid operand decision is cached, but its condition
  // must still be evaluated first against current flags on every hit.
  write(0x1000, 0x0290f001); // ADDEQS pc,r0,#1: unpredictable only when EQ passes
  s = {}; s.registers[15] = 0x1000; s.registers[0] = 0x1800;
  assert(step(s).reason == arm::StopReason::None);
  s.zero = true;
  assert(step(s).reason == arm::StopReason::Unpredictable);
  s.zero = false;
  assert(step(s).reason == arm::StopReason::None);
  // Differential instruction execution under changing registers and flags.
  std::mt19937 random(0x12556);
  for (int mode = 0; mode < 2; mode++)
    for (int i = 0; i < 20000; i++) {
      unsigned word = random();
      write(0x1000, word);
      for (int repeat = 0; repeat < 2; repeat++) {
        s = {};
        s.thumb = mode;
        s.zero = random() & 1;
        s.carry = random() & 1;
        s.negative = random() & 1;
        s.overflow = random() & 1;
        for (unsigned r = 0; r < 15; r++)
          s.registers[r] = 0x1800 + (random() % 16) * 4;
        s.registers[15] = 0x1000;
        step(s);
      }
    }
  // Real engine fallback retains instruction budgets and SVC stops.
  lift::runtime::EntryTable entries;
  lift::Engine engine(entries);
  write(0x1000, 0xe2800001);
  write(0x1004, 0xeafffffd);
  arm::CpuState expected, actual;
  expected.registers[15] = actual.registers[15] = 0x1000;
  std::uint64_t xcount = 0, ycount = 0;
  auto x = arm::Run(expected, a, 10000, xcount),
       y = engine.Run(actual, b, 10000, ycount);
  same(expected, actual);
  assert(x.reason == y.reason && xcount == ycount && ycount == 10000);
  write(0x1000, 0xef000009);
  expected = {};
  actual = {};
  expected.registers[15] = actual.registers[15] = 0x1000;
  x = arm::Run(expected, a, 10, xcount);
  y = engine.Run(actual, b, 10, ycount);
  same(expected, actual);
  assert(x.reason == y.reason && x.detail == y.detail && xcount == ycount &&
         ycount == 1);
  // Alternating encodings that collide must not repeatedly decode each other.
  lift::DecodedInstructionCache collision_cache;
  std::vector<unsigned> colliding;
  for (unsigned immediate = 0; immediate < 4096 && colliding.size() < 2;
       ++immediate) {
    unsigned word = 0xe2800000 | immediate;
    if (((word ^ (word >> 11) ^ (word >> 21)) & 511) == 0)
      colliding.push_back(word);
  }
  assert(colliding.size() == 2);
  for (unsigned i = 0; i < 1000; ++i) {
    write(0x1000, colliding[i % 2]);
    arm::CpuState state;
    state.registers[15] = 0x1000;
    assert(collision_cache.Step(state, b).reason == arm::StopReason::None);
  }
  assert(collision_cache.Counted().misses == 2);
  assert(collision_cache.Counted().hits == 998);
  // Benchmark the production engine against the previous uncached interpreter.
  write(0x1000, 0xe2800001);
  write(0x1004, 0xeafffffd);
  constexpr unsigned iterations = 1000000;
  double plain = 0, cached = 0;
  for (int round = 0; round < 3; round++) {
    expected = {};
    actual = {};
    expected.registers[15] = actual.registers[15] = 0x1000;
    auto begin = std::chrono::steady_clock::now();
    arm::Run(expected, a, iterations, xcount);
    auto middle = std::chrono::steady_clock::now();
    engine.Run(actual, b, iterations, ycount);
    auto end = std::chrono::steady_clock::now();
    same(expected, actual);
    assert(xcount == ycount);
    plain += std::chrono::duration<double>(middle - begin).count();
    cached += std::chrono::duration<double>(end - middle).count();
  }
  assert(engine.DecodeStatistics().hits > iterations * 2);
  std::cout << "PASS cached/uncached ARM+Thumb, live flags/PC, changed code, "
               "Thumb boundary, missing second halfword, remapping, CPU state, "
               "engine budget and SVC\n";
  std::cout << "Host synthetic fallback benchmark: interpreter_ms="
            << plain * 1000 << " engine_cache_ms=" << cached * 1000
            << " hits=" << engine.DecodeStatistics().hits
            << " decodes=" << engine.DecodeStatistics().misses
            << " (not phone FPS)\n";
}
