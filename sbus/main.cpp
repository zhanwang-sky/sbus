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
#include "sbus.h"

using std::cout;
using std::endl;

std::array<uint8_t, SBUS_FRAME_LEN> buf;

int main(int argc, const char* argv[]) {
  sbus_frame_t frame1, frame2;
  sbus_context_t ctx;

  // make frame
  for (int i = 0; i < SBUS_NR_CHANNELS; ++i) {
    frame1.channels[i] = SBUS_MIN_VALUE + (SBUS_MAX_VALUE - SBUS_MIN_VALUE) * i / 16;
  }
  frame1.switches[0] = false;
  frame1.switches[1] = true;
  frame1.frame_lost = true;
  frame1.failsafe = false;

  // pack
  sbus_pack_frame(&frame1, buf.data());

  // prepare to receive
  sbus_reset_context(&ctx);

  // test incomplete frame
  assert(sbus_receive_data(&ctx, 8) == 0);
  assert(sbus_receive_data(&ctx, 0) == 0);

  // receive
  for (int i = 0; i < SBUS_FRAME_LEN - 1; ++i) {
    assert(sbus_receive_data(&ctx, buf[i]) == 0);
  }
  assert(sbus_receive_data(&ctx, buf.back()) == 1);

  // unpack
  sbus_unpack_frame(&ctx, &frame2);

  // validate
  assert(memcmp(&frame1, &frame2, sizeof(sbus_frame_t)) == 0);

  // continue to receive next frame
  for (int i = 0; i < SBUS_FRAME_LEN - 1; ++i) {
    assert(sbus_receive_data(&ctx, buf[i]) == 0);
  }
  assert(sbus_receive_data(&ctx, buf.back()) == 1);
  sbus_unpack_frame(&ctx, &frame2);
  assert(memcmp(&frame1, &frame2, sizeof(sbus_frame_t)) == 0);

  // print
  for (int i = 0; i < SBUS_NR_CHANNELS; ++i) {
    cout << "ch" << i + 1 << "=" << frame2.channels[i] << endl;
  }
  cout << "sw1=" << frame2.switches[0] << endl;
  cout << "sw2=" << frame2.switches[1] << endl;
  cout << "frame_lost=" << frame2.frame_lost << endl;
  cout << "failsafe=" << frame2.failsafe << endl;

  return 0;
}
