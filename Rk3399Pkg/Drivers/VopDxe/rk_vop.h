/** @file
 *
 * Copyright (c) 2019, Andrey Warkentin <andrey.warkentin@gmail.com>
 *
 * SPDX-License-Identifier:     GPL-2.0
 */

#include "vop_rk3288.h"

#ifndef RK_VOP_H
#define RK_VOP_H

struct display_timing;

struct vop {
  struct rk3288_vop *regs;
  BOOLEAN has_10_bit_output;
  void (*set_pin_polarity)(struct rk3288_vop *regs,
                           enum vop_modes mode,
                           u32 pin_polarity);
};

void rk3399_set_pin_polarity(struct rk3288_vop *regs,
                             enum vop_modes mode,
                             u32 pin_polarity);

void rkvop_mode_set(struct vop *vop,
                    const struct display_timing *edid,
                    enum vop_modes mode);

void rkvop_enable(struct vop *vop,
                  void *fbbase,
                  int fb_bits_per_pixel,
                  const struct display_timing *edid);

#endif /* RK_VOP_H */
