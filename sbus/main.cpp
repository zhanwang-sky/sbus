//
//  main.cpp
//  sbus
//
//  Created by zhanwang-sky on 2024/5/16.
//

#include <cassert>
#include <cstring>
#include <array>
#include <iostream>
#include <random>
#include "sbus/sbus.h"

using std::cout;
using std::endl;

void unit_test() {
  std::array<uint8_t, SBUS_FRAME_LEN> frame_buf;
  sbus_frame_t frame1, frame2;
  sbus_context_t ctx;

  // make frame
  std::random_device rd;
  decltype(rd()) scale = SBUS_MAX_VALUE - SBUS_MIN_VALUE + 1;
  for (int i = 0; i < SBUS_NR_CHANNELS; ++i) {
    auto num = rd();
    frame1.channels[i] = num % scale + SBUS_MIN_VALUE;
  }
  frame1.switches[0] = frame1.channels[0] % 2;
  frame1.switches[1] = frame1.channels[1] % 2;
  frame1.frame_lost = frame1.channels[2] % 2;
  frame1.failsafe = frame1.channels[3] % 2;

  // log
  for (int i = 0; i < SBUS_NR_CHANNELS; ++i) {
    cout << "ch" << i + 1 << "=" << frame1.channels[i] << endl;
  }
  cout << "sw1=" << frame1.switches[0] << endl;
  cout << "sw2=" << frame1.switches[1] << endl;
  cout << "frame_lost=" << frame1.frame_lost << endl;
  cout << "failsafe=" << frame1.failsafe << endl;

  // pack
  sbus_pack_frame(&frame1, frame_buf.data());

  // unpack
  sbus_reset_context(&ctx);

  // test invalid data
  assert(sbus_receive_data(&ctx, 0x08) == 0);
  assert(sbus_receive_data(&ctx, 0) == 0);
  cout << "[PASS] invalid data\n";

  // round 1
  for (int i = 0; i < SBUS_FRAME_LEN - 1; ++i) {
    assert(sbus_receive_data(&ctx, frame_buf[i]) == 0);
  }
  assert(sbus_receive_data(&ctx, frame_buf.back()) == 1);
  sbus_unpack_frame(&ctx, &frame2);
  assert(memcmp(&frame1, &frame2, sizeof(sbus_frame_t)) == 0);
  cout << "[PASS] round 1\n";

  // round 2
  for (int i = 0; i < SBUS_FRAME_LEN - 1; ++i) {
    assert(sbus_receive_data(&ctx, frame_buf[i]) == 0);
  }
  assert(sbus_receive_data(&ctx, frame_buf.back()) == 1);
  sbus_unpack_frame(&ctx, &frame2);
  assert(memcmp(&frame1, &frame2, sizeof(sbus_frame_t)) == 0);
  cout << "[PASS] round 2\n";
}

int main(int argc, const char* argv[]) {
  unit_test();
  return 0;
}
