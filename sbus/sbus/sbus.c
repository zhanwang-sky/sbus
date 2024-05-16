//
//  sbus.c
//  sbus
//
//  Created by zhanwang-sky on 2024/5/16.
//

#include <string.h>
#include "sbus.h"

void sbus_reset_context(sbus_context_t* ctx) {
  memset(ctx, 0, sizeof(sbus_context_t));
}

int sbus_receive_data(sbus_context_t* ctx, uint8_t data) {
  if (ctx->nbytes >= SBUS_FRAME_LEN) {
    sbus_reset_context(ctx);
  }

  if (ctx->state == 0) {
    if (data == 0x0f) {
      ctx->state = 1;
      ctx->nbytes = 1;
      ctx->buf[0] = data;
    }
    return 0;
  }

  ctx->buf[ctx->nbytes++] = data;
  if ((ctx->nbytes >= SBUS_FRAME_LEN) && (data == 0)) {
    return 1;
  }

  return 0;
}

void sbus_unpack_frame(const sbus_context_t* ctx, sbus_frame_t* frame) {
  frame->channels[0] = ((uint16_t) ctx->buf[1]) |
                       ((((uint16_t) ctx->buf[2]) << 8) & 0x7ff);
  frame->channels[1] = (((uint16_t) ctx->buf[2]) >> 3) |
                       ((((uint16_t) ctx->buf[3]) << 5) & 0x7ff);
  frame->channels[2] = (((uint16_t) ctx->buf[3]) >> 6) |
                       (((uint16_t) ctx->buf[4]) << 2) |
                       ((((uint16_t) ctx->buf[5]) << 10) & 0x7ff);
  frame->channels[3] = (((uint16_t) ctx->buf[5]) >> 1) |
                       ((((uint16_t) ctx->buf[6]) << 7) & 0x7ff);
  frame->channels[4] = (((uint16_t) ctx->buf[6]) >> 4) |
                       ((((uint16_t) ctx->buf[7]) << 4) & 0x7ff);
  frame->channels[5] = (((uint16_t) ctx->buf[7]) >> 7) |
                       (((uint16_t) ctx->buf[8]) << 1) |
                       ((((uint16_t) ctx->buf[9]) << 9) & 0x7ff);
  frame->channels[6] = (((uint16_t) ctx->buf[9]) >> 2) |
                       ((((uint16_t) ctx->buf[10]) << 6) & 0x7ff);
  frame->channels[7] = (((uint16_t) ctx->buf[10]) >> 5) |
                       ((((uint16_t) ctx->buf[11]) << 3) & 0x7ff);
  frame->channels[8] = ((uint16_t) ctx->buf[12]) |
                       ((((uint16_t) ctx->buf[13]) << 8) & 0x7ff);
  frame->channels[9] = (((uint16_t) ctx->buf[13]) >> 3) |
                       ((((uint16_t) ctx->buf[14]) << 5) & 0x7ff);
  frame->channels[10] = (((uint16_t) ctx->buf[14]) >> 6) |
                        (((uint16_t) ctx->buf[15]) << 2) |
                        ((((uint16_t) ctx->buf[16]) << 10) & 0x7ff);
  frame->channels[11] = (((uint16_t) ctx->buf[16]) >> 1) |
                        ((((uint16_t) ctx->buf[17]) << 7) & 0x7ff);
  frame->channels[12] = (((uint16_t) ctx->buf[17]) >> 4) |
                        ((((uint16_t) ctx->buf[18]) << 4) & 0x7ff);
  frame->channels[13] = (((uint16_t) ctx->buf[18]) >> 7) |
                        (((uint16_t) ctx->buf[19]) << 1) |
                        ((((uint16_t) ctx->buf[20]) << 9) & 0x7ff);
  frame->channels[14] = (((uint16_t) ctx->buf[20]) >> 2) |
                        ((((uint16_t) ctx->buf[21]) << 6) & 0x7ff);
  frame->channels[15] = (((uint16_t) ctx->buf[21]) >> 5) |
                        ((((uint16_t) ctx->buf[22]) << 3) & 0x7ff);
  frame->switches[0] = (ctx->buf[23] & 0x01) ? true : false;
  frame->switches[1] = (ctx->buf[23] & 0x02) ? true : false;
  frame->frame_lost = (ctx->buf[23] & 0x04) ? true : false;
  frame->failsafe = (ctx->buf[23] & 0x08) ? true : false;
}

void sbus_pack_frame(const sbus_frame_t* frame, uint8_t buf[SBUS_FRAME_LEN]) {
  buf[0] = 0x0f;
  buf[1] = frame->channels[0];
  buf[2] = (frame->channels[0] >> 8) | (frame->channels[1] << 3);
  buf[3] = (frame->channels[1] >> 5) | (frame->channels[2] << 6);
  buf[4] = frame->channels[2] >> 2;
  buf[5] = (frame->channels[2] >> 10) | (frame->channels[3] << 1);
  buf[6] = (frame->channels[3] >> 7) | (frame->channels[4] << 4);
  buf[7] = (frame->channels[4] >> 4) | (frame->channels[5] << 7);
  buf[8] = frame->channels[5] >> 1;
  buf[9] = (frame->channels[5] >> 9) | (frame->channels[6] << 2);
  buf[10] = (frame->channels[6] >> 6) | (frame->channels[7] << 5);
  buf[11] = frame->channels[7] >> 3;
  buf[12] = frame->channels[8];
  buf[13] = (frame->channels[8] >> 8) | (frame->channels[9] << 3);
  buf[14] = (frame->channels[9] >> 5) | (frame->channels[10] << 6);
  buf[15] = frame->channels[10] >> 2;
  buf[16] = (frame->channels[10] >> 10) | (frame->channels[11] << 1);
  buf[17] = (frame->channels[11] >> 7) | (frame->channels[12] << 4);
  buf[18] = (frame->channels[12] >> 4) | (frame->channels[13] << 7);
  buf[19] = frame->channels[13] >> 1;
  buf[20] = (frame->channels[13] >> 9) | (frame->channels[14] << 2);
  buf[21] = (frame->channels[14] >> 6) | (frame->channels[15] << 5);
  buf[22] = frame->channels[15] >> 3;
  buf[23] = (frame->switches[0] ? 0x01 : 0) |
            (frame->switches[1] ? 0x02 : 0) |
            (frame->frame_lost ? 0x04 : 0) |
            (frame->failsafe ? 0x08 : 0);
  buf[24] = 0;
}
