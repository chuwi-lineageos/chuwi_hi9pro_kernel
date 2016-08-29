/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2015. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

#include "lcm_drv.h"

#ifdef BUILD_LK
#include <platform/upmu_common.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <string.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
#include <mach/mt_pm_ldo.h>
#include <mach/mt_gpio.h>
#endif
#include <cust_gpio_usage.h>
#include <cust_i2c.h>

#define LCD_DEBUG(fmt)  dprintf(CRITICAL, fmt)

/* static unsigned char lcd_id_pins_value = 0xFF; */
static const unsigned char LCD_MODULE_ID = 0x01;
/* --------------------------------------------------------------------------- */
/* Local Constants */
/* --------------------------------------------------------------------------- */
#define LCM_DSI_CMD_MODE 0
#define FRAME_WIDTH (1440)
#define FRAME_HEIGHT (2560)
#define GPIO_65133_EN GPIO_LCD_BIAS_ENP_PIN

#define REGFLAG_PORT_SWAP 0xFFFA
#define REGFLAG_DELAY 0xFFFC
/* END OF REGISTERS MARKER */
#define REGFLAG_END_OF_TABLE 0xFFFD

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif
/* static unsigned int lcm_esd_test = FALSE;      ///only for ESD test */
/* --------------------------------------------------------------------------- */
/* Local Variables */
/* --------------------------------------------------------------------------- */

static const unsigned int BL_MIN_LEVEL = 20;
static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v) (lcm_util.set_reset_pin((v)))
#define MDELAY(n) (lcm_util.mdelay(n))
static unsigned int lcm_compare_id(void);
/* --------------------------------------------------------------------------- */
/* Local Functions */
/* --------------------------------------------------------------------------- */
#define dsi_set_cmd_by_cmdq_dual(handle, cmd, count, ppara, force_update) \
    lcm_util.dsi_set_cmdq_V23(handle, cmd, count, ppara, force_update)

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
    lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) \
    lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) \
    lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd) \
    lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) \
    lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)
#define dsi_swap_port(swap) \
    lcm_util.dsi_swap_port(swap)

#ifdef BUILD_LK
void __attribute__((weak)) mt6331_upmu_set_rg_vgp1_en(kal_uint32 en)
{
	return;
}
#endif

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] = {
	{0xB0,  1, {0x00}},
	{0xD6,  1, {0x01}},
#if (LCM_DSI_CMD_MODE)
	{0xB3,  3, {0x04,0x00,0x00}},
#else
	{0xB3,  3, {0x14,0x00,0x00}},
#endif
	{0xB4,  1, {0x00}},
	{0xB6,  2, {0x3A,0xC3}},
	{0xBE,  1, {0x04}},
	{0xC3,  3, {0x00,0x00,0x00}},
	{0xC5,  1, {0x00}},
	{0xC0,  4, {0x00,0x00,0x00,0x00}},
	{0xC1, 35, {0x00,0x61,0x00,0x30,0x29,0x10,0x19,0x63,0x61,0xB4,0xE6,0xDC,0x7B,0xEF,0x39,0xD7,0xDA,0x08,0x8C,0xB1,0x08,0x54,0x82,0x00,0x00,0x00,0x00,0x00,0x02,0x60,0x23,0x03,0x00,0xFF,0x11}},
	{0xC2,  8, {0x08,0x0A,0x01,0x07,0x07,0xF0,0x00,0x04}},
	{0xC4, 14, {0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x05,0x01}},
	{0xC6, 21, {0x5A,0x2D,0x2D,0x02,0x01,0x02,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x15,0x08,0x5A}},
	{0xCB, 15, {0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x54,0xE0,0x07,0x2A,0xE0,0x00,0x00}},
	{0xCC,  2, {0x32}},
	{0xD7, 13, {0x82,0xFF,0x21,0x8E,0x8C,0xF1,0x87,0x3F,0x7E,0x10,0x00,0x00,0x8F}},
	{0xD9,  2, {0x00,0x00}},
	{0xD0,  4, {0x11,0x17,0x17,0xFD}},
	{0xD2, 16, {0xCD,0x2B,0x2B,0x33,0x12,0x33,0x33,0x33,0x77,0x77,0x33,0x33,0x33,0x00,0x00,0x00}},
	{0xD5,  7, {0x06,0x00,0x00,0x01,0x40,0x01,0x40}},
	{0xC7, 30, {0x01,0x0C,0x13,0x1D,0x2C,0x3C,0x47,0x58,0x3C,0x44,0x4F,0x5C,0x65,0x6C,0x75,0x01,0x0C,0x13,0x1D,0x2B,0x39,0x43,0x54,0x38,0x41,0x4D,0x5A,0x63,0x6A,0x74}},
	{0xC8, 19, {0x01,0x00,0xFF,0x00,0x00,0xFC,0x00,0x00,0xFF,0x00,0x00,0xFC,0x00,0x00,0xFF,0x00,0x00,0xFC,0x0F}},  
	{0xBC,  2, {0x50,0x32}},
	{0xCA,  6, {0x00,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0}},
	{0xB0,  1, {0x03}},
        {0x36,  1, {0x42}},

	{0x29, 0, {} },
	{REGFLAG_DELAY, 20, {} },
	{0x11, 0, {} },
	{REGFLAG_DELAY, 120, {} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

#if 0
static struct LCM_setting_table lcm_set_window[] = {
	{0x2A, 4, {0x00, 0x00, (FRAME_WIDTH >> 8), (FRAME_WIDTH & 0xFF)} },
	{0x2B, 4, {0x00, 0x00, (FRAME_HEIGHT >> 8), (FRAME_HEIGHT & 0xFF)} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

static struct LCM_setting_table lcm_sleep_out_setting[] = {
	{0x29, 1, {0x00} },
	{REGFLAG_DELAY, 120, {} },

	{0x11, 1, {0x00} },
	{REGFLAG_DELAY, 20, {} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

static struct LCM_setting_table lcm_normal_sleep_mode_in_setting[] = {
	/* Display off sequence */
	{0x28, 1, {0x00} },
	{REGFLAG_DELAY, 20, {} },

	/* Sleep Mode On */
	{0x10, 1, {0x00} },
	{REGFLAG_DELAY, 120, {} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

#endif

static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	/* Display off sequence */
	{0x28, 1, {0x00} },
	{REGFLAG_DELAY, 20, {} },

	/* Sleep Mode On */
	{0x10, 1, {0x00} },
	{REGFLAG_DELAY, 120, {} },

	{0xB0, 1, {0x00} },
	{0xB1, 1, {0x01} },
	{REGFLAG_DELAY, 20, {} },

	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	{0x28, 0, {} },
	{0x10, 0, {} },
	{REGFLAG_DELAY, 120, {} }
};

static struct LCM_setting_table lcm_backlight_level_setting[] = {
	{0x51, 1, {0xFF} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

	for (i = 0; i < count; i++) {
		unsigned cmd;
		cmd = table[i].cmd;

		switch (cmd) {
			case REGFLAG_DELAY:
#ifdef BUILD_LK
				dprintf(0, "[LK]REGFLAG_DELAY\n");
#endif
				if (table[i].count <= 10)
					MDELAY(table[i].count);
				else
					MDELAY(table[i].count);
				break;

			case REGFLAG_END_OF_TABLE:
				break;
			case REGFLAG_PORT_SWAP:
#ifdef BUILD_LK
				dprintf(0, "[LK]push_table end\n");
#endif
				dsi_swap_port(1);
				break;
			default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}

/* --------------------------------------------------------------------------- */
/* LCM Driver Implementations */
/* --------------------------------------------------------------------------- */

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type = LCM_TYPE_DSI;

	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	params->lcm_if = LCM_INTERFACE_DSI_DUAL;
	params->lcm_cmd_if = LCM_INTERFACE_DSI0;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
#else
	params->dsi.mode = BURST_VDO_MODE;  /* have no SYNC_PULSE_VDO_MODE; */
#endif
	params->dsi.dual_dsi_type = DUAL_DSI_CMD;
	/* DSI */
	/* Command mode setting */
	params->dsi.LANE_NUM = LCM_FOUR_LANE;
	/* The following defined the fomat for data coming from LCD engine. */
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	/* Highly depends on LCD driver capability. */
	params->dsi.packet_size = 256;
	params->dsi.ssc_disable = 0;
	params->dsi.ssc_range = 3;
	/* video mode timing */

	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active = 4;
	params->dsi.vertical_backporch = 6;
	params->dsi.vertical_frontporch = 20;
	params->dsi.vertical_frontporch_for_low_power = 600;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 10;
	params->dsi.horizontal_backporch = 30;
	params->dsi.horizontal_frontporch = 60;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
#if (LCM_DSI_CMD_MODE)
	params->dsi.PLL_CLOCK = 423;
#else
	params->dsi.PLL_CLOCK = 423;
#endif
	params->dsi.ufoe_enable = 1;
	params->dsi.ufoe_params.lr_mode_en = 1;

	params->dsi.esd_check_enable = 0;//1;
	params->dsi.customization_esd_check_enable = 0;
	params->dsi.lcm_esd_check_table[0].cmd = 0x53;  /* 0x0A; */
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x2C; /* 0x1C; */

	/* for R63419A Lane Swap */
#if 0
	params->dsi.lane_swap_en = 1;
	params->dsi.lane_swap[MIPITX_PHY_PORT_1][MIPITX_PHY_LANE_0] = MIPITX_PHY_LANE_2;
	params->dsi.lane_swap[MIPITX_PHY_PORT_1][MIPITX_PHY_LANE_1] = MIPITX_PHY_LANE_CK;
	params->dsi.lane_swap[MIPITX_PHY_PORT_1][MIPITX_PHY_LANE_2] = MIPITX_PHY_LANE_3;
	params->dsi.lane_swap[MIPITX_PHY_PORT_1][MIPITX_PHY_LANE_3] = MIPITX_PHY_LANE_1;
	params->dsi.lane_swap[MIPITX_PHY_PORT_1][MIPITX_PHY_LANE_CK] = MIPITX_PHY_LANE_0;
	params->dsi.lane_swap[MIPITX_PHY_PORT_1][MIPITX_PHY_LANE_RX] = MIPITX_PHY_LANE_CK;

	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_0] = MIPITX_PHY_LANE_CK;
	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_1] = MIPITX_PHY_LANE_2;
	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_2] = MIPITX_PHY_LANE_1;
	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_3] = MIPITX_PHY_LANE_0;
	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_CK] = MIPITX_PHY_LANE_3;
	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_RX] = MIPITX_PHY_LANE_3;
#endif
}
 
#ifdef BUILD_LK

#define TPS65133_SLAVE_ADDR_WRITE  0x7C
static struct mt_i2c_t TPS65133_i2c;

static int TPS65133_write_byte(kal_uint8 addr, kal_uint8 value)
{
	kal_uint32 ret_code = I2C_OK;
	kal_uint8 write_data[2];
	kal_uint16 len;

	write_data[0] = addr;
	write_data[1] = value;

	TPS65133_i2c.id = I2C_I2C_LCD_BIAS_CHANNEL; /* I2C2; */
	/* Since i2c will left shift 1 bit, we need to set FAN5405 I2C address to >>1 */
	TPS65133_i2c.addr = (TPS65133_SLAVE_ADDR_WRITE >> 1);
	TPS65133_i2c.mode = ST_MODE;
	TPS65133_i2c.speed = 100;
	len = 2;

	ret_code = i2c_write(&TPS65133_i2c, write_data, len);
	/* printf("%s: i2c_write: ret_code: %d\n", __func__, ret_code); */

	return ret_code;
}

#else

/* extern int mt8193_i2c_write(u16 addr, u32 data); */
/* extern int mt8193_i2c_read(u16 addr, u32 *data); */

/* #define TPS65133_write_byte(add, data)  mt8193_i2c_write(add, data) */
/* #define TPS65133_read_byte(add)  mt8193_i2c_read(add) */


#endif

static void lcm_init_power(void)
{
	mt6331_upmu_set_rg_vgp1_en(1);
}


static void lcm_resume_power(void)
{
	mt6331_upmu_set_rg_vgp1_en(1);
}


static void lcm_suspend_power(void)
{
	mt6331_upmu_set_rg_vgp1_en(0);
}


static void lcm_init(void)
{
	unsigned char cmd = 0x0;
	unsigned char data = 0xFF;
	int ret = 0;
	cmd = 0x00;
	data = 0x0E;

#if 0
	SET_RESET_PIN(0);
	MDELAY(10);
#endif

	mt_set_gpio_mode(GPIO_65133_EN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_65133_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_65133_EN, GPIO_OUT_ONE);
	MDELAY(5);

	ret = TPS65133_write_byte(cmd, data);
	if (ret)
		dprintf(0, "[LK]r63419----tps6132----cmd=%0x--i2c write error----\n", cmd);
	else
		dprintf(0, "[LK]r63419----tps6132----cmd=%0x--i2c write success----\n", cmd);

	cmd = 0x01;
	data = 0x0E;

	ret = TPS65133_write_byte(cmd, data);
	if (ret)
		dprintf(0, "[LK]r63419----tps6132----cmd=%0x--i2c write error----\n", cmd);
	else
		dprintf(0, "[LK]r63419----tps6132----cmd=%0x--i2c write success----\n", cmd);

	SET_RESET_PIN(1);
	MDELAY(1);
	/* MDELAY(10); */
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(10);

	/* when phone initial , config output high, enable backlight drv chip */
	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table),
	           1);

#ifdef BUILD_LK
	dprintf(0, "[LK]push_table end\n");
#endif
}

static void lcm_suspend(void)
{
	mt_set_gpio_mode(GPIO_65133_EN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_65133_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_65133_EN, GPIO_OUT_ZERO);
	push_table(lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
	SET_RESET_PIN(1);
        MDELAY(10);
}

static void lcm_resume(void)
{
	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(10);
	lcm_init();
}

static void lcm_update(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0 >> 8) & 0xFF);
	unsigned char x0_LSB = (x0 & 0xFF);
	unsigned char x1_MSB = ((x1 >> 8) & 0xFF);
	unsigned char x1_LSB = (x1 & 0xFF);
	unsigned char y0_MSB = ((y0 >> 8) & 0xFF);
	unsigned char y0_LSB = (y0 & 0xFF);
	unsigned char y1_MSB = ((y1 >> 8) & 0xFF);
	unsigned char y1_LSB = (y1 & 0xFF);

	unsigned int data_array[16];

	data_array[0] = 0x00053902;
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00053902;
	data_array[1] = (y1_MSB << 24) | (y0_LSB << 16) | (y0_MSB << 8) | 0x2b;
	data_array[2] = (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
	/*BEGIN PN:DTS2013013101431 modified by s00179437 , 2013-01-31 */
	/* delete high speed packet */
	/* data_array[0]=0x00290508; */
	/* dsi_set_cmdq(data_array, 1, 1); */
	/*END PN:DTS2013013101431 modified by s00179437 , 2013-01-31 */

	data_array[0] = 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);
}

#define LCM_ID_R63419_by (0x0003)

static unsigned int lcm_compare_id(void)
{

	unsigned char buffer[5];
	unsigned int array[16];
	int i;
	unsigned int lcd_id = 0;
        unsigned char cmd = 0x0;
	unsigned char data = 0xFF;
	int ret = 0;
	cmd = 0x00;
	data = 0x0E;

	mt_set_gpio_mode(GPIO_65133_EN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_65133_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_65133_EN, GPIO_OUT_ONE);
	MDELAY(5);

	ret = TPS65133_write_byte(cmd, data);
	if (ret)
		dprintf(0, "[LK]r63419----tps6132----cmd=%0x--i2c write error----\n", cmd);
	else
		dprintf(0, "[LK]r63419----tps6132----cmd=%0x--i2c write success----\n", cmd);

	cmd = 0x01;
	data = 0x0E;

	ret = TPS65133_write_byte(cmd, data);
	if (ret)
		dprintf(0, "[LK]r63419----tps6132----cmd=%0x--i2c write error----\n", cmd);
	else
		dprintf(0, "[LK]r63419----tps6132----cmd=%0x--i2c write success----\n", cmd);



	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(10);
//	array[0] = 0x00053700;  /* read id return two byte,version and id */
//	dsi_set_cmdq(array, 1, 1);
//array[0] = 0x0029b004;  /* read id return two byte,version and id */
       // array[1] = 0x0000370a;
       // array[2] = 0x000006a1;

        //dsi_set_cmdq(array, 3, 1);
        array[0] = 0x00023700;	 
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0xA1, buffer, 2);
	MDELAY(20);
	lcd_id = buffer[1];

	dprintf(0, "%s, LK r63419 debug: r63419 id = 0x%08x\n", __func__, lcd_id);
dprintf(0, "%s, LK r63419 debug: r63419 id0 = 0x%08x\n", __func__, buffer[0]);
dprintf(0, "%s, LK r63419 debug: r63419 id1 = 0x%08x\n", __func__, buffer[1]);
 

	if (lcd_id != LCM_ID_R63419_by)
		return 1;
	else
                return 0;


}

static unsigned int lcm_ata_check(unsigned char *buffer)
{
#ifndef BUILD_LK
	return 1;
#endif
}

static void lcm_setbacklight_cmdq(void *handle, unsigned int level)
{
	dprintf(0, "%s,lk R63419 backlight: level = %d\n", __func__, level);

	/* Refresh value of backlight level. */

	unsigned int cmd = 0x51;
	unsigned int count = 1;
	unsigned int value = level;
	dsi_set_cmd_by_cmdq_dual(handle, cmd, count, &value, 1);
	/* lcm_backlight_level_setting[0].para_list[0] = level; */
	/* push_table(lcm_backlight_level_setting,
	  sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1); */
}


LCM_DRIVER r63419_wqhd_lide_vdo_lcm_drv = {
	.name = "r63419_wqhd_lide_vdo",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.compare_id = lcm_compare_id,
	.init_power = lcm_init_power,
	.resume_power = lcm_resume_power,
	.suspend_power = lcm_suspend_power,
	.ata_check = lcm_ata_check,
	.set_backlight_cmdq = lcm_setbacklight_cmdq,
#if (LCM_DSI_CMD_MODE)
	.update = lcm_update,
#endif

};

/* END PN:DTS2013053103858 , Added by d00238048, 2013.05.31*/
