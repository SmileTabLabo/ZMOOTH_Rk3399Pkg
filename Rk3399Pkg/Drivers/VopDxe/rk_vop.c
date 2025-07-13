// SPDX-License-Identifier: GPL-2.0+
/*
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
#include "display_timing.h"

enum vop_pol {
  HSYNC_POSITIVE = 0,
  VSYNC_POSITIVE = 1,
  DEN_NEGATIVE   = 2,
  DCLK_INVERT    = 3
};

void
rkvop_enable(struct vop *vop,
             void *fbbase,
             int fb_bits_per_pixel,
             const struct display_timing *edid)
{
  u32 lb_mode;
  u32 rgb_mode;
  u32 hactive = edid->hactive.typ;
  u32 vactive = edid->vactive.typ;
  struct rk3288_vop *regs = vop->regs;

  writel(V_ACT_WIDTH(hactive - 1) | V_ACT_HEIGHT(vactive - 1),
         &regs->win0_act_info);

  writel(V_DSP_XST(edid->hsync_len.typ + edid->hback_porch.typ) |
         V_DSP_YST(edid->vsync_len.typ + edid->vback_porch.typ),
         &regs->win0_dsp_st);

  writel(V_DSP_WIDTH(hactive - 1) |
         V_DSP_HEIGHT(vactive - 1),
         &regs->win0_dsp_info);

  clrsetbits_le32(&regs->win0_color_key, M_WIN0_KEY_EN | M_WIN0_KEY_COLOR,
                  V_WIN0_KEY_EN(0) | V_WIN0_KEY_COLOR(0));

  switch (fb_bits_per_pixel) {
  case 16:
    rgb_mode = RGB565;
    writel(V_RGB565_VIRWIDTH(hactive), &regs->win0_vir);
    break;
  case 24:
    rgb_mode = RGB888;
    writel(V_RGB888_VIRWIDTH(hactive), &regs->win0_vir);
    break;
  case 32:
  default:
    rgb_mode = ARGB8888;
    writel(V_ARGB888_VIRWIDTH(hactive), &regs->win0_vir);
    break;
  }
  
  if (hactive > 2560)
    lb_mode = LB_RGB_3840X2;
  else if (hactive > 1920)
    lb_mode = LB_RGB_2560X4;
  else if (hactive > 1280)
    lb_mode = LB_RGB_1920X5;
  else
    lb_mode = LB_RGB_1280X8;

  clrsetbits_le32(&regs->win0_ctrl0,
                  M_WIN0_LB_MODE | M_WIN0_DATA_FMT | M_WIN0_EN,
                  V_WIN0_LB_MODE(lb_mode) | V_WIN0_DATA_FMT(rgb_mode) |
                  V_WIN0_EN(1));
  
  writel((UINT32) (UINTN) fbbase, &regs->win0_yrgb_mst);
  writel(0x01, &regs->reg_cfg_done); /* enable reg config */
}

static void
rkvop_enable_output(struct vop *vop,
                    enum vop_modes mode)
{
  struct rk3288_vop *regs = vop->regs;

  /* remove from standby */
  clrbits_le32(&regs->sys_ctrl, V_STANDBY_EN(1));

  switch (mode) {
  case VOP_MODE_HDMI:
    clrsetbits_le32(&regs->sys_ctrl, M_ALL_OUT_EN,
                    V_HDMI_OUT_EN(1));
    break;

  case VOP_MODE_EDP:
    clrsetbits_le32(&regs->sys_ctrl, M_ALL_OUT_EN,
                    V_EDP_OUT_EN(1));
    break;

  case VOP_MODE_LVDS:
    clrsetbits_le32(&regs->sys_ctrl, M_ALL_OUT_EN,
                    V_RGB_OUT_EN(1));
    break;

  case VOP_MODE_MIPI:
    clrsetbits_le32(&regs->sys_ctrl, M_ALL_OUT_EN,
                    V_MIPI_OUT_EN(1));
    break;

  default:
    debug(STR_FMT": unsupported output mode %x\n", __func__, mode);
  }
}

void rkvop_mode_set(struct vop *vop,
                    const struct display_timing *edid,
                    enum vop_modes mode)
{
  struct rk3288_vop *regs = vop->regs;
  u32 hactive = edid->hactive.typ;
  u32 vactive = edid->vactive.typ;
  u32 hsync_len = edid->hsync_len.typ;
  u32 hback_porch = edid->hback_porch.typ;
  u32 vsync_len = edid->vsync_len.typ;
  u32 vback_porch = edid->vback_porch.typ;
  u32 hfront_porch = edid->hfront_porch.typ;
  u32 vfront_porch = edid->vfront_porch.typ;
  int mode_flags;
  u32 pin_polarity;

  pin_polarity = BIT(DCLK_INVERT);
  if (edid->flags & DISPLAY_FLAGS_HSYNC_HIGH)
    pin_polarity |= BIT(HSYNC_POSITIVE);
  if (edid->flags & DISPLAY_FLAGS_VSYNC_HIGH)
    pin_polarity |= BIT(VSYNC_POSITIVE);

  if (vop->set_pin_polarity != NULL) {
    vop->set_pin_polarity(regs, mode, pin_polarity);
  }
  rkvop_enable_output(vop, mode);

  mode_flags = 0;  /* RGB888 */
  if (vop->has_10_bit_output &&
      (mode == VOP_MODE_HDMI || mode == VOP_MODE_EDP))
    mode_flags = 15;  /* RGBaaa */

  clrsetbits_le32(&regs->dsp_ctrl0, M_DSP_OUT_MODE,
                  V_DSP_OUT_MODE(mode_flags));

  writel(V_HSYNC(hsync_len) |
         V_HORPRD(hsync_len + hback_porch + hactive + hfront_porch),
         &regs->dsp_htotal_hs_end);

  writel(V_HEAP(hsync_len + hback_porch + hactive) |
         V_HASP(hsync_len + hback_porch),
         &regs->dsp_hact_st_end);

  writel(V_VSYNC(vsync_len) |
         V_VERPRD(vsync_len + vback_porch + vactive + vfront_porch),
         &regs->dsp_vtotal_vs_end);

  writel(V_VAEP(vsync_len + vback_porch + vactive)|
         V_VASP(vsync_len + vback_porch),
         &regs->dsp_vact_st_end);

  writel(V_HEAP(hsync_len + hback_porch + hactive) |
         V_HASP(hsync_len + hback_porch),
         &regs->post_dsp_hact_info);

  writel(V_VAEP(vsync_len + vback_porch + vactive)|
         V_VASP(vsync_len + vback_porch),
         &regs->post_dsp_vact_info);

  writel(0x01, &regs->reg_cfg_done); /* enable reg config */
}
