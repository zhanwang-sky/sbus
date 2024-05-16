//
//  sbus.h
//  sbus
//
//  Created by zhanwang-sky on 2024/5/16.
//

#ifndef sbus_h
#define sbus_h

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SBUS_FRAME_LEN 25
#define SBUS_NR_CHANNELS 16
#define SBUS_NR_SWITCHES 2
#define SBUS_MIN_VALUE 172
#define SBUS_MAX_VALUE 1811

typedef struct sbus_frame {
  uint16_t channels[SBUS_NR_CHANNELS];
  bool switches[SBUS_NR_SWITCHES];
  bool frame_lost;
  bool failsafe;
} sbus_frame_t;

typedef struct sbus_context {
  uint8_t state;
  uint8_t nbytes;
  uint8_t buf[SBUS_FRAME_LEN];
} sbus_context_t;

void sbus_reset_context(sbus_context_t* ctx);

int sbus_receive_data(sbus_context_t* ctx, uint8_t data);

void sbus_unpack_frame(const sbus_context_t* ctx, sbus_frame_t* frame);

void sbus_pack_frame(const sbus_frame_t* frame, uint8_t buf[SBUS_FRAME_LEN]);

#ifdef __cplusplus
}
#endif

#endif /* sbus_h */
