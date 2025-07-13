/** @file
 *
 * Copyright (c) 2019, Andrey Warkentin <andrey.warkentin@gmail.com>
 *
 * SPDX-License-Identifier:     GPL-2.0
 */

#include <Library/UefiLib.h>
#include <Rk3399/Rk3399.h>
#include <Rk3399/Rk3399Cru.h>
#include <Rk3399/Rk3399Grf.h>
#include <Library/CRULib.h>
/*
 * Order is important here.
 */
#include "common.h"
#include "dw_hdmi.h"
#include "edid.h"
#include "rk_vop.h"

static const struct hdmi_phy_config rockchip_phy_config[] = {
  {
    .mpixelclock = 74250000,
    .sym_ctr = 0x8009, .term = 0x0004, .vlev_ctr = 0x0272,
  }, {
    .mpixelclock = 148500000,
    .sym_ctr = 0x802b, .term = 0x0004, .vlev_ctr = 0x028d,
  }, {
    .mpixelclock = 297000000,
    .sym_ctr = 0x8039, .term = 0x0005, .vlev_ctr = 0x028d,
  }, {
    .mpixelclock = 584000000,
    .sym_ctr = 0x8039, .term = 0x0000, .vlev_ctr = 0x019d,
  }, {
    .mpixelclock = ~0ul,
    .sym_ctr = 0x0000, .term = 0x0000, .vlev_ctr = 0x0000,
  }
};

static const struct hdmi_mpll_config rockchip_mpll_cfg[] = {
  {
    .mpixelclock = 40000000,
    .cpce = 0x00b3, .gmp = 0x0000, .curr = 0x0018,
  }, {
    .mpixelclock = 65000000,
    .cpce = 0x0072, .gmp = 0x0001, .curr = 0x0028,
  }, {
    .mpixelclock = 66000000,
    .cpce = 0x013e, .gmp = 0x0003, .curr = 0x0038,
  }, {
    .mpixelclock = 83500000,
    .cpce = 0x0072, .gmp = 0x0001, .curr = 0x0028,
  }, {
    .mpixelclock = 146250000,
    .cpce = 0x0051, .gmp = 0x0002, .curr = 0x0038,
  }, {
    .mpixelclock = 148500000,
    .cpce = 0x0051, .gmp = 0x0003, .curr = 0x0000,
  }, {
    .mpixelclock = 272000000,
    .cpce = 0x0040, .gmp = 0x0003, .curr = 0x0000,
  }, {
    .mpixelclock = 340000000,
    .cpce = 0x0040, .gmp = 0x0003, .curr = 0x0000,
  }, {
    .mpixelclock = ~0ul,
    .cpce = 0x0051, .gmp = 0x0003, .curr = 0x0000,
  }
};

struct dw_hdmi my_hdmi = {
  .ioaddr = RK3399_HDMI,
  .mpll_cfg = rockchip_mpll_cfg,
  .phy_cfg = rockchip_phy_config,
  .i2c_clk_high = 0x7a,
  .i2c_clk_low = 0x8d,
  .reg_io_width = 4,
  .phy_set = dw_hdmi_phy_cfg,
};

struct vop my_vop = {
  .regs = (void *) RK3399_VOP0_BIG,
  .has_10_bit_output = TRUE,
  .set_pin_polarity = rk3399_set_pin_polarity,
};

STATIC VOID
DumpHex (
  IN UINTN Indent,
  IN UINTN Offset,
  IN UINTN DataSize,
  IN VOID  *UserData
  )
{
  UINT8 *Data;
  CHAR8 Val[50];
  CHAR8 Str[20];

  UINT8 TempByte;
  UINTN Size;
  UINTN Index;

  STATIC CONST CHAR8 Hex[] = {
    '0', '1', '2', '3', '4',
    '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E',
    'F'
  };

  Data = UserData;
  while (DataSize != 0) {
    Size = 16;
    if (Size > DataSize) {
      Size = DataSize;
    }

    for (Index = 0; Index < Size; Index += 1) {
      TempByte            = Data[Index];
      Val[Index * 3 + 0]  = Hex[TempByte >> 4];
      Val[Index * 3 + 1]  = Hex[TempByte & 0xF];
      Val[Index * 3 + 2]  = (CHAR8) ((Index == 7) ? '-' : ' ');
      Str[Index]          = (CHAR8) ((TempByte < ' ' || TempByte > '~') ? '.' : TempByte);
    }

    Val[Index * 3]  = 0;
    Str[Index]      = 0;
    DEBUG((EFI_D_ERROR, "%*a%08X: %-48a *%a*\r\n", Indent, "", Offset, Val, Str));

    Data += Size;
    Offset += Size;
    DataSize -= Size;
  }
}

unsigned char hardcoded_edid[] = {
  0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x4a, 0x8b, 0x54, 0x4c,
  0x01, 0x00, 0x00, 0x00, 0x0c, 0x11, 0x01, 0x03, 0x81, 0x46, 0x27, 0x78,
  0x8a, 0xa5, 0x8e, 0xa6, 0x54, 0x4a, 0x9c, 0x26, 0x12, 0x45, 0x46, 0xaf,
  0xcf, 0x00, 0x95, 0x00, 0x95, 0x0f, 0x95, 0x19, 0x01, 0x01, 0x01, 0x01,
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x1d, 0x00, 0x72, 0x51, 0xd0,
  0x1e, 0x20, 0x6e, 0x28, 0x55, 0x00, 0xb9, 0x88, 0x21, 0x00, 0x00, 0x1e,
  0x8c, 0x0a, 0xd0, 0x8a, 0x20, 0xe0, 0x2d, 0x10, 0x10, 0x3e, 0x96, 0x00,
  0xb9, 0x88, 0x21, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x32,
  0x4b, 0x18, 0x3c, 0x0b, 0x00, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
  0x00, 0x00, 0x00, 0xfc, 0x00, 0x33, 0x32, 0x56, 0x33, 0x48, 0x2d, 0x48,
  0x36, 0x41, 0x0a, 0x20, 0x20, 0x20, 0x01, 0x29, 0x02, 0x03, 0x21, 0x71,
  0x4e, 0x06, 0x07, 0x02, 0x03, 0x15, 0x96, 0x11, 0x12, 0x13, 0x04, 0x14,
  0x05, 0x1f, 0x90, 0x23, 0x09, 0x07, 0x07, 0x83, 0x01, 0x00, 0x00, 0x65,
  0x03, 0x0c, 0x00, 0x10, 0x00, 0x8c, 0x0a, 0xd0, 0x90, 0x20, 0x40, 0x31,
  0x20, 0x0c, 0x40, 0x55, 0x00, 0xb9, 0x88, 0x21, 0x00, 0x00, 0x18, 0x01,
  0x1d, 0x80, 0x18, 0x71, 0x1c, 0x16, 0x20, 0x58, 0x2c, 0x25, 0x00, 0xb9,
  0x88, 0x21, 0x00, 0x00, 0x9e, 0x01, 0x1d, 0x80, 0xd0, 0x72, 0x1c, 0x16,
  0x20, 0x10, 0x2c, 0x25, 0x80, 0xb9, 0x88, 0x21, 0x00, 0x00, 0x9e, 0x01,
  0x1d, 0x00, 0xbc, 0x52, 0xd0, 0x1e, 0x20, 0xb8, 0x28, 0x55, 0x40, 0xb9,
  0x88, 0x21, 0x00, 0x00, 0x1e, 0x02, 0x3a, 0x80, 0xd0, 0x72, 0x38, 0x2d,
  0x40, 0x10, 0x2c, 0x45, 0x80, 0xb9, 0x88, 0x21, 0x00, 0x00, 0x1e, 0x00,
  0x00, 0x00, 0x00, 0xd0
};

EFI_STATUS
EFIAPI
InitializeVopDxe (
  IN EFI_HANDLE            ImageHandle,
  IN EFI_SYSTEM_TABLE      *SystemTable
  )
{
  int ret;
  EFI_STATUS Status = EFI_SUCCESS;
  u8 buf[EDID_EXT_SIZE];
  struct display_timing timing;
  int bpc;

  /* Route internal HDMI I2C: 4 RK_PC0 3 and 4 RK_PC1 3 -> hdmii2c_sda and hdmii2c_scl */
  MmioWrite32(RK3399_GRF_BASE + GRF_GPIO4C_IOMUX, (0xF << 16) | 0xF);

  /* 4 RK_PC7 1 -> hdmi_cecinout */
  MmioWrite32(RK3399_GRF_BASE + GRF_GPIO4C_IOMUX, (0xC000 << 16) | 0x4000);

  /* Pick VOP0 */
  GrfClearSetl(SGRF_SOC_CON20, GRF_RK3399_HDMI_VOP_SEL_MASK, GRF_RK3399_HDMI_VOP_SEL_B);
  DEBUG((EFI_D_ERROR, "Routing reg = 0x%x\n", MmioRead32(RK3399_GRF_BASE + SGRF_SOC_CON20)));

  CruWritel((0x1 << ( 2 + 16)) | (1 << 2), CRU_SOFTRSTS_CON(16));
  CruWritel((0x1 << ( 4 + 16)) | (1 << 4), CRU_SOFTRSTS_CON(16));
  CruWritel((0x1 << ( 5 + 16)) | (1 << 5), CRU_SOFTRSTS_CON(16));
  CruWritel((0x1 << ( 0 + 16)) | (1 << 0), CRU_SOFTRSTS_CON(15));
  CruWritel((0x1 << ( 1 + 16)) | (1 << 1), CRU_SOFTRSTS_CON(15));
  CruWritel((0x1 << ( 2 + 16)) | (1 << 2), CRU_SOFTRSTS_CON(15));
  CruWritel((0x1 << ( 3 + 16)) | (1 << 3), CRU_SOFTRSTS_CON(15));
  CruWritel((0x1 << ( 4 + 16)) | (1 << 4), CRU_SOFTRSTS_CON(15));
  CruWritel((0x1 << ( 5 + 16)) | (1 << 5), CRU_SOFTRSTS_CON(15));
  CruWritel((0x1 << ( 6 + 16)) | (1 << 6), CRU_SOFTRSTS_CON(15));
  CruWritel((0x1 << ( 7 + 16)) | (1 << 7), CRU_SOFTRSTS_CON(15));
  udelay(1000);
  CruWritel((0x1 << ( 2 + 16)) | (0 << 2), CRU_SOFTRSTS_CON(16));
  CruWritel((0x1 << ( 4 + 16)) | (0 << 4), CRU_SOFTRSTS_CON(16));
  CruWritel((0x1 << ( 5 + 16)) | (0 << 5), CRU_SOFTRSTS_CON(16));
  CruWritel((0x1 << ( 0 + 16)) | (0 << 0), CRU_SOFTRSTS_CON(15));
  CruWritel((0x1 << ( 1 + 16)) | (0 << 1), CRU_SOFTRSTS_CON(15));
  CruWritel((0x1 << ( 2 + 16)) | (0 << 2), CRU_SOFTRSTS_CON(15));
  CruWritel((0x1 << ( 3 + 16)) | (0 << 3), CRU_SOFTRSTS_CON(15));
  CruWritel((0x1 << ( 4 + 16)) | (0 << 4), CRU_SOFTRSTS_CON(15));
  CruWritel((0x1 << ( 5 + 16)) | (0 << 5), CRU_SOFTRSTS_CON(15));
  CruWritel((0x1 << ( 6 + 16)) | (0 << 6), CRU_SOFTRSTS_CON(15));
  CruWritel((0x1 << ( 7 + 16)) | (0 << 7), CRU_SOFTRSTS_CON(15));

  rk3399_vio_set_clk(400*MHz);
  rk3399_hdcp_set_clk(400*MHz);

  ret = dw_hdmi_phy_wait_for_hpd(&my_hdmi);
  if (ret < 0) {
    debug("hdmi can not get hpd signal\n");
    return EFI_UNSUPPORTED;
  }

  dw_hdmi_init(&my_hdmi);
  dw_hdmi_phy_init(&my_hdmi);

  ret = dw_hdmi_read_edid(&my_hdmi, buf, sizeof(buf));
  if (ret > 0) {
    edid_print_info((void *) buf);
    debug("read_edid returned 0x%x bytes\n", ret);
    DumpHex(0, 0, ret, buf);

    ret = edid_get_timing(buf, ret, &timing, &bpc);
    if (ret < 0) {
      debug("can't get timing, trying hardcoded EDID\n");
      edid_print_info((void *) hardcoded_edid);
      ret = edid_get_timing(hardcoded_edid, ret, &timing, &bpc);
      if (ret < 0) {
        debug("can't get timing for hardcoded EDID\n");
      }
    }
  }

  CruWritel((0x1 << ( 0 + 16)) | (1 << 0), CRU_SOFTRSTS_CON(17));
  CruWritel((0x1 << ( 2 + 16)) | (1 << 2), CRU_SOFTRSTS_CON(17));
  CruWritel((0x1 << ( 4 + 16)) | (1 << 4), CRU_SOFTRSTS_CON(17));
  CruWritel((0x1 << ( 6 + 16)) | (1 << 6), CRU_SOFTRSTS_CON(17));
  CruWritel((0x1 << ( 8 + 16)) | (1 << 8), CRU_SOFTRSTS_CON(17));
  CruWritel((0x1 << ( 10 + 16)) | (1 << 10), CRU_SOFTRSTS_CON(17));
  udelay(1000);
  CruWritel((0x1 << ( 0 + 16)) | (0 << 0), CRU_SOFTRSTS_CON(17));
  CruWritel((0x1 << ( 2 + 16)) | (0 << 2), CRU_SOFTRSTS_CON(17));
  CruWritel((0x1 << ( 4 + 16)) | (0 << 4), CRU_SOFTRSTS_CON(17));
  CruWritel((0x1 << ( 6 + 16)) | (0 << 6), CRU_SOFTRSTS_CON(17));
  CruWritel((0x1 << ( 8 + 16)) | (0 << 8), CRU_SOFTRSTS_CON(17));
  CruWritel((0x1 << ( 10 + 16)) | (0 << 10), CRU_SOFTRSTS_CON(17));

  rk3399_vop_set_clk(DCLK_VOP0, timing.pixelclock.typ);

  rkvop_mode_set(&my_vop, &timing, VOP_MODE_HDMI);
  rkvop_enable(&my_vop, (void *) 0x200000, 32, &timing);

  ret = dw_hdmi_enable(&my_hdmi, &timing);
  if (ret < 0) {
    debug("can't configure timing\n");
  }


  return Status;
}
