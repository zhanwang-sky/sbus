//
//  main.cpp
//  sbus
//
//  Created by zhanwang-sky on 2024/5/16.
//

#include <cassert>
#include <cstring>
#include <array>
#include <functional>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

#include <boost/asio.hpp>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include "sbus/sbus.h"

class SbusTransceiver {
 public:
  using callback_type = std::function<void(const sbus_frame_t&)>;

  SbusTransceiver(boost::asio::io_context& io,
                  const std::string& dev,
                  const callback_type& user_cb)
      : io_(io),
        port_(io),
        prev_tp_(clock_type::now()),
        user_cb_(user_cb) {
    // serial port
    port_.open(dev);
    port_.set_option(serial_type::baud_rate(100000));
    port_.set_option(serial_type::flow_control(serial_type::flow_control::none));
    port_.set_option(serial_type::parity(serial_type::parity::even));
    port_.set_option(serial_type::stop_bits(serial_type::stop_bits::two));
    port_.set_option(serial_type::character_size(8));
    // sbus context
    sbus_reset_context(&sbus_ctx_);
  }

  virtual ~SbusTransceiver() { }

  void start_receiving() {
    port_.async_read_some(boost::asio::buffer(buf_),
                          std::bind(&SbusTransceiver::do_read, this,
                                    std::placeholders::_1,
                                    std::placeholders::_2));
  }

  void send_frame(const sbus_frame_t& frame) {
    uint8_t buf[SBUS_FRAME_LEN];
    sbus_pack_frame(&frame, buf);
    boost::asio::write(port_, boost::asio::buffer(buf, sizeof(buf)));
  }

 private:
  using clock_type = boost::asio::chrono::steady_clock;
  using duration_type = boost::asio::chrono::milliseconds;
  using serial_type = boost::asio::serial_port;

  void do_read(const boost::system::error_code& ec, std::size_t nbytes) {
    if (ec) {
      // discard damaged frame
      sbus_reset_context(&sbus_ctx_);
    } else if (nbytes) {
      // frame synchronization
      clock_type::time_point curr_tp = clock_type::now();
      duration_type::rep interval = boost::asio::chrono::duration_cast<duration_type>(curr_tp - prev_tp_).count();
      if (interval > max_interval_) {
        // discard outdated frame
        sbus_reset_context(&sbus_ctx_);
      }
      // parse frame
      for (std::size_t i = 0; i < nbytes; ++i) {
        if (sbus_receive_data(&sbus_ctx_, buf_[i])) {
          sbus_frame_t frame = {0};
          sbus_unpack_frame(&sbus_ctx_, &frame);
          user_cb_(frame);
        }
      }
    }
    start_receiving();
  }

  static constexpr duration_type::rep max_interval_ = 3;

  boost::asio::io_context& io_;
  serial_type port_;
  clock_type::time_point prev_tp_;
  callback_type user_cb_;
  sbus_context_t sbus_ctx_;
  std::array<uint8_t, 256> buf_;
};

static void dump_frame(const sbus_frame_t& frame) {
  std::ostringstream oss;
  for (int i = 0; i < SBUS_NR_CHANNELS; ++i) {
    oss << std::setw(4) << frame.channels[i] << " ";
  }
  oss << "| "
      << frame.switches[0] << " "
      << frame.switches[1] << " "
      << frame.frame_lost << " "
      << frame.failsafe << "\n";
  LOG(INFO) << oss.str();
}

void unit_test() {
  std::array<uint8_t, SBUS_FRAME_LEN> buf;
  sbus_frame_t frame1, frame2;
  sbus_context_t ctx;

  LOG(INFO) << "**********";
  LOG(INFO) << "UNIT TEST";

  // make frame
  std::random_device rd;
  constexpr decltype(rd()) scale = SBUS_MAX_VALUE - SBUS_MIN_VALUE + 1;

  memset(&frame1, 0, sizeof(sbus_frame_t));
  for (int i = 0; i < SBUS_NR_CHANNELS; ++i) {
    auto num = rd();
    frame1.channels[i] = num % scale + SBUS_MIN_VALUE;
  }
  frame1.switches[0] = frame1.channels[0] % 2;
  frame1.switches[1] = frame1.channels[1] % 2;
  frame1.frame_lost = frame1.channels[2] % 2;
  frame1.failsafe = frame1.channels[3] % 2;

  dump_frame(frame1);

  // serialize
  sbus_pack_frame(&frame1, buf.data());

  // TEST
  sbus_reset_context(&ctx);

  // frame synchronization
  assert(sbus_receive_data(&ctx, 0x08) == 0);
  assert(sbus_receive_data(&ctx, 0) == 0);
  LOG(INFO) << "[PASS] frame synchronization";

  // parse frame (round 1)
  for (int i = 0; i < SBUS_FRAME_LEN - 1; ++i) {
    assert(sbus_receive_data(&ctx, buf[i]) == 0);
  }
  assert(sbus_receive_data(&ctx, buf.back()) == 1);
  memset(&frame2, 0, sizeof(sbus_frame_t));
  sbus_unpack_frame(&ctx, &frame2);
  assert(memcmp(&frame1, &frame2, sizeof(sbus_frame_t)) == 0);
  LOG(INFO) << "[PASS] parse frame (round 1)";

  // parse frame (round 2)
  for (int i = 0; i < SBUS_FRAME_LEN - 1; ++i) {
    assert(sbus_receive_data(&ctx, buf[i]) == 0);
  }
  assert(sbus_receive_data(&ctx, buf.back()) == 1);
  memset(&frame2, 0, sizeof(sbus_frame_t));
  sbus_unpack_frame(&ctx, &frame2);
  assert(memcmp(&frame1, &frame2, sizeof(sbus_frame_t)) == 0);
  LOG(INFO) << "[PASS] parse frame (round 2)";

  LOG(INFO) << "UNIT TEST success";
}

int main(int argc, char* argv[]) {
  FLAGS_logtostdout = true;
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  unit_test();

  if (argc < 2) {
    exit(EXIT_SUCCESS);
  }

  LOG(INFO) << "**********";
  LOG(INFO) << "start receiving";
  boost::asio::io_context io;
  sbus_frame_t prev_frame = {0};
  auto user_cb = [&prev_frame](const sbus_frame_t& frame) {
    if (memcmp(&prev_frame, &frame, sizeof(sbus_frame_t)) == 0) {
      return;
    }
    memcpy(&prev_frame, &frame, sizeof(sbus_frame_t));
    dump_frame(frame);
  };
  SbusTransceiver(io, argv[1], user_cb);
  io.run();

  return 0;
}
