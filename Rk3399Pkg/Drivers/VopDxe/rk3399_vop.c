// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2017 Theobroma Systems Design und Consulting GmbH
 * Copyright (c) 2015 Google, Inc
 * Copyright 2014 Rockchip Inc.
 */

#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
/*
 * Order is important here.
 */
#include "common.h"
#include "rk_vop.h"


void
rk3399_set_pin_polarity(struct rk3288_vop *regs,
                        enum vop_modes mode,
                        u32 polarity)
{
  /*
   * The RK3399 VOPs (v3.5 and v3.6) require a per-mode setting of
   * the polarity configuration (in ctrl1).
   */
  switch (mode) {
  case VOP_MODE_HDMI:
    clrsetbits_le32(&regs->dsp_ctrl1,
                    M_RK3399_DSP_HDMI_POL,
                    V_RK3399_DSP_HDMI_POL(polarity));
    break;

  case VOP_MODE_EDP:
    clrsetbits_le32(&regs->dsp_ctrl1,
                    M_RK3399_DSP_EDP_POL,
                    V_RK3399_DSP_EDP_POL(polarity));
    break;

  case VOP_MODE_MIPI:
    clrsetbits_le32(&regs->dsp_ctrl1,
                    M_RK3399_DSP_MIPI_POL,
                    V_RK3399_DSP_MIPI_POL(polarity));
    break;

  case VOP_MODE_LVDS:
    /* The RK3399 has neither parallel RGB nor LVDS output. */
  default:
    debug("%s: unsupported output mode %x\n", __func__, mode);
  }
}
