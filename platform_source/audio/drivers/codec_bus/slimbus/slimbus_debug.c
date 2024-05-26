/*
 * slimbus is a kernel driver which is used to manager slimbus devices
 *
 * Copyright (c) 2012-2018 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/sysfs.h>

#include "audio_log.h"
#include "platform_base_addr_info.h"
#include "slimbus_drv.h"
#include "slimbus.h"

#ifdef CONFIG_SND_SOC_DA_COMBINE_V5
#include "slimbus_da_combine_v5.h"
#endif

/*lint -e838 -e737 -e715 -e778 -e846 -e866 -e528 -e753 -e514 -e84 -e64 -e747 -e732 */

#define LOG_TAG "slimbus_debug"

#define CLASS_NAME "slimbus_debug"

#ifdef CONFIG_AUDIO_COMMON_IMAGE
#undef SOC_ACPU_ASP_DMAC_BASE_ADDR
#undef SOC_ACPU_SLIMBUS_BASE_ADDR
#define SOC_ACPU_ASP_DMAC_BASE_ADDR 0
#define SOC_ACPU_SLIMBUS_BASE_ADDR 0
#endif

#define CONFIGURE_ASP_DMA 1
#define CONFIGURE_DA_COMBINE_DMA 2
#define CONFIGURE_AND_ACTIVE_SLIMBUS_CHANNEL 3
#define DEACTIVATE_AND_REMOVE_SLIMBUS_CHANNEL 4

#define REG_4_BYTE_START1 0x10000000
#define REG_4_BYTE_END1 0x10001000

#define REG_4_BYTE_START2 0x20000700
#define REG_4_BYTE_END2 0x20007000

#define REG_1_BYTE_START 0x20007000
#define REG_1_BYTE_END 0x20008000

#define REG_ACCESS_TIMES 1000

#ifdef __IMAGE_DOWN_TEST
#define IMAGE_ARRAY_SIZE 2000
static uint32_t simgdown;
static uint64_t gdmabaseaddr;
static uint16_t gimagearray[IMAGE_ARRAY_SIZE] = {0};
#endif

static struct class *sclass;

static uint32_t sregpagerd;
static uint32_t sregrd;
static uint32_t sregwr;
static uint32_t sswitchframer;
static uint32_t sclockgear = 10;
static uint32_t spauseclock;
static uint32_t schannelctrl;
static uint32_t stracktype;
static uint32_t sregtest;
static uint32_t sbusreset;
static uint32_t smixtest;
static uint32_t srequestinfo;
static uint32_t scopyright_error;
static uint32_t sreg8_rd_error;
static uint32_t sreg32_rd_error;

static uint32_t srdwrerr_logcount;
static uint32_t srdwrerr_logtimes;
static uint32_t slostms_times;

static uint32_t gregvalue;
static uint32_t sdevice_type;

#ifdef __IMAGE_DOWN_TEST
static ssize_t imgdown_show(struct class *class, struct class_attribute *attr, char *buf)
{
	uint32_t ret = snprintf(buf, PAGE_SIZE, "usage: echo param>simgdown\n");

	ret += snprintf(buf + ret, (PAGE_SIZE - ret), "para:\n1: configure asp dma;\n");
	ret += snprintf(buf + ret, (PAGE_SIZE - ret), "2: start and active slimbus image download channel;\n");
	ret += snprintf(buf + ret, (PAGE_SIZE - ret), "3: deactive and remove slimbus image download channel;\n");
	ret += snprintf(buf + ret, (PAGE_SIZE - ret), "else: set startvalue write to da_combine ocram;\n");

	return ret;
}

static ssize_t imgdown_store(struct class *class, struct class_attribute *attr,
	const char *buf, size_t size)
{
	uint32_t startvalue = 0;
	uint32_t ret = 0;
	uint32_t i;
	uint32_t val = 0;

	if (sdevice_type != SLIMBUS_DEVICE_DA_COMBINE_V3)
		return -EFAULT;

	// cppcheck-suppress *
	sscanf(buf, "0x%x", &simgdown);

	AUDIO_LOGE("simgdown: %x", simgdown);

	switch (simgdown) {
	case CONFIGURE_ASP_DMA:
		AUDIO_LOGE("configure asp dma");
		gdmabaseaddr = ioremap(get_phy_addr(SOC_ACPU_ASP_DMAC_BASE_ADDR, PLT_DMAC_MEM),
			0xa00);
		if (!gdmabaseaddr) {
			AUDIO_LOGE("ioremap fail");
			break;
		}

		writel(0x400, gdmabaseaddr + 0x810);
		writel(gimagearray, gdmabaseaddr + 0x814);
		writel(get_phy_addr(SOC_ACPU_SLIMBUS_BASE_ADDR + 0x1000, PLT_SLIMBUS_MEM),
			gdmabaseaddr + 0x818);
		writel(0x83322007, gdmabaseaddr + 0x81c);

		iounmap(gdmabaseaddr);
		gdmabaseaddr = NULL;
		break;
	case CONFIGURE_DA_COMBINE_DMA:
		AUDIO_LOGE("configure dma");
		slimbus_write_1byte(0x20007040, 0x3);
		slimbus_write_1byte(0x20007214, 0xff);
		slimbus_write_1byte(0x200072f2, 0x3);
		slimbus_write_1byte(0x20007045, 0xff);
		slimbus_write_1byte(0x20007052, 0x50);
		slimbus_write_4byte(0x20000810, 0x200);
		slimbus_write_4byte(0x20000814, 0x20010000);
		slimbus_write_4byte(0x20000818, 0x10000000);
		slimbus_write_4byte(0x2000081c, 0x47711007);
		slimbus_write_1byte(0x200073d8, 0x1);
		slimbus_write_1byte(0x20007031, 0x11);

		ret = slimbus_bus_configure(SLIMBUS_BUS_CONFIG_IMGDOWN);

		break;
	case CONFIGURE_AND_ACTIVE_SLIMBUS_CHANNEL:
		AUDIO_LOGE("configure and active slimbus channel");
		ret = slimbus_activate_track(DA_COMBINE_V3_BUS_TRACK_IMAGE_LOAD, NULL);

		break;
	case DEACTIVATE_AND_REMOVE_SLIMBUS_CHANNEL:
		AUDIO_LOGE("deactivate and remove slimbus channel");

		slimbus_deactivate_track(DA_COMBINE_V3_BUS_TRACK_IMAGE_LOAD, NULL);
		ret = slimbus_bus_configure(SLIMBUS_BUS_CONFIG_NORMAL);

		break;
	default:
		startvalue = simgdown;
		for (i = 0; i < IMAGE_ARRAY_SIZE; i += 2) {
			gimagearray[i] = startvalue + i / 2;
			gimagearray[i + 1] = startvalue + i / 2;
		}
		AUDIO_LOGE("startvalue: %x", startvalue);
		break;
	}

	if (ret)
		AUDIO_LOGE("ret: %x", ret);

	return size;
}
#endif

static ssize_t regpagerd_show(struct class *class, struct class_attribute *attr, char *buf)
{
	uint32_t i;
	uint32_t j;
	uint32_t ret = 0;

	if (sregpagerd >= REG_4_BYTE_START1 && sregpagerd <= REG_1_BYTE_END) {
		ret = snprintf(buf, PAGE_SIZE, "addr:%x:\n", sregpagerd - REG_4_BYTE_START1);

		if ((sregpagerd >= REG_4_BYTE_START1 && sregpagerd <= REG_4_BYTE_END1) ||
			(sregpagerd >= REG_4_BYTE_START2 && sregpagerd < REG_4_BYTE_END2)) {
			for (i = 0; i < 240; i += 16) {
				ret += snprintf(buf + ret, (PAGE_SIZE - ret), " %4x %4x %4x %4x\n",
							slimbus_read_4byte(sregpagerd + i), slimbus_read_4byte(sregpagerd + i + 4),
							slimbus_read_4byte(sregpagerd + i + 8), slimbus_read_4byte(sregpagerd + i + 12));
				msleep(20);
			}
		}

		if (sregpagerd >= REG_1_BYTE_START && sregpagerd <= REG_1_BYTE_END) {
			ret += snprintf(buf + ret, (PAGE_SIZE - ret), "begin\n");
			for (j = 0; j <= 0x1ff; j += 16) {
				for (i = j; i < j + 16; i++)
					ret += snprintf(buf + ret, (PAGE_SIZE - ret), " %3x ", slimbus_read_1byte(sregpagerd + i));
				ret += snprintf(buf + ret, (PAGE_SIZE - ret), "\n");
			}
		}
		ret += snprintf(buf + ret, (PAGE_SIZE - ret), "end\n");
	} else {
		ret += snprintf(buf, PAGE_SIZE, "usage: echo regaddr>sregpagerd\n");
		ret += snprintf(buf + ret, (PAGE_SIZE - ret), "sregpagerd:%x!\n", sregpagerd);
	}

	return ret;
}

static ssize_t regpagerd_store(struct class *class,
	struct class_attribute *attr, const char *buf, size_t size)
{
	// cppcheck-suppress *
	if (sscanf(buf, "0x%x", &sregpagerd) != 1)
		return -EINVAL;

	return (ssize_t)size;
}

static ssize_t regrd_show(struct class *class,
	struct class_attribute *attr, char *buf)
{
	uint32_t ret;

	if (sregrd >= REG_1_BYTE_START && sregrd < REG_1_BYTE_END) {
		gregvalue = slimbus_read_1byte(sregrd);
		slimbus_read_pageaddr();

		ret = snprintf(buf, PAGE_SIZE, "sregrd:0x%x, value:%x!\n", sregrd, gregvalue);
	} else {
		ret = snprintf(buf, PAGE_SIZE, "usage: echo param>sregrd\n");
		ret += snprintf(buf + ret, (PAGE_SIZE - ret), "param: 0xregaddr\n");
	}

	AUDIO_LOGE("sregrd (%x %x)", sregrd, gregvalue);

	return ret;
}

static ssize_t regrd_store(struct class *class,
	struct class_attribute *attr, const char *buf, size_t size)
{
	// cppcheck-suppress *
	if (sscanf(buf, "0x%x", &sregrd) != 1)
		return -EINVAL;

	return (ssize_t)size;
}
static ssize_t regwr_show(struct class *class,
	struct class_attribute *attr, char *buf)
{
	uint32_t ret = snprintf(buf, PAGE_SIZE, "usage: echo param>sregrd\n");

	ret += snprintf(buf + ret, (PAGE_SIZE - ret), "param: 0xregaddr 0xvalue\n");

	return ret;
}

static ssize_t regwr_store(struct class *class,
	struct class_attribute *attr, const char *buf, size_t size)
{
	// cppcheck-suppress *
	if (sscanf(buf, "0x%x 0x%x", &sregwr, &gregvalue) != 1)
		return -EINVAL;

	if (sregwr >= REG_1_BYTE_START && sregwr <= REG_1_BYTE_END) {
		slimbus_write_1byte(sregwr, gregvalue);

		AUDIO_LOGE("sregwr (%pK %x)", (void *)(uintptr_t)sregwr, gregvalue);
	} else {
		AUDIO_LOGE("param error, sregwr (%pK %x)", (void *)(uintptr_t)sregwr, gregvalue);
	}

	return (ssize_t)size;
}


static ssize_t switchframer_show(struct class *class,
	struct class_attribute *attr, char *buf)
{
	uint32_t ret = snprintf(buf, PAGE_SIZE, "sswitchframer:0x%x!\n", sswitchframer);

	ret += snprintf(buf + ret, (PAGE_SIZE - ret), "usage: echo param>sswitchframer\n");
	ret += snprintf(buf + ret, (PAGE_SIZE - ret), "param:\n");
	ret += snprintf(buf + ret, (PAGE_SIZE - ret), "0x1: switch to soc\n");
	ret += snprintf(buf + ret, (PAGE_SIZE - ret), "0x2: switch to codec\n");

	return ret;
}

static ssize_t switchframer_store(struct class *class,
	struct class_attribute *attr, const char *buf, size_t size)
{
	int32_t ret;
	// cppcheck-suppress *
	if (sscanf(buf, "0x%x", &sswitchframer) != 1)
		return -EINVAL;

	AUDIO_LOGE("sswitchframer: 0x%x", sswitchframer);

	switch (sswitchframer) {
	case 1:
		ret = slimbus_switch_framer(FRAMER_SOC);
		if (ret)
			AUDIO_LOGE("slimbus switch framer to soc failed %d", ret);
		break;
	case 2:
		ret = slimbus_switch_framer(FRAMER_CODEC);
		if (ret)
			AUDIO_LOGE("slimbus switch framer failed %d", ret);
		break;
	default:
		AUDIO_LOGE("default");
		break;
	}

	return (ssize_t)size;
}

static ssize_t devtype_show(struct class *class,
	struct class_attribute *attr, char *buf)
{
	uint32_t ret = snprintf(buf, PAGE_SIZE, "sdevicetype:0x%x!\n", sdevice_type);

	ret += snprintf(buf + ret, (PAGE_SIZE - ret), "usage: echo param>sdevice_type\n");

	return ret;
}

static ssize_t devtype_store(struct class *class,
	struct class_attribute *attr, const char *buf, size_t size)
{
	// cppcheck-suppress *
	uint32_t type;

	if (sscanf(buf, "0x%x", &type) != 1)
		return -EINVAL;

	if (type >= SLIMBUS_DEVICE_NUM) {
		AUDIO_LOGE("device type is invalid: %d", type);
		return -EINVAL;
	}

	sdevice_type = type;
	AUDIO_LOGE("sdevice_type: 0x%x", sdevice_type);

	return (ssize_t)size;
}

static ssize_t clockgear_show(struct class *class,
	struct class_attribute *attr, char *buf)
{
	uint32_t ret = snprintf(buf, PAGE_SIZE, "sclockgear:0x%x!\n", sclockgear);

	ret += snprintf(buf + ret, (PAGE_SIZE - ret), "usage: echo param>sclockgear\n");

	return ret;
}

static ssize_t clockgear_store(struct class *class,
	struct class_attribute *attr, const char *buf, size_t size)
{
	struct slimbus_bus_config *bus_cfg = slimbus_get_bus_config(SLIMBUS_BUS_CONFIG_SWITCH_FRAMER);

	if (bus_cfg == NULL) {
		AUDIO_LOGE("bus cfg is NULL");
		return -EINVAL;
	}

	// cppcheck-suppress *
	if (sscanf(buf, "0x%x", &sclockgear) != 1)
		return -EINVAL;

	if (sclockgear > SLIMBUS_CG_10) {
		AUDIO_LOGE("sclockgear: %x", sclockgear);
		return -EINVAL;
	}

	bus_cfg->cg = (enum slimbus_clock_gear)sclockgear;

	AUDIO_LOGE("sclockgear: %x", sclockgear);
	slimbus_bus_configure(SLIMBUS_BUS_CONFIG_SWITCH_FRAMER);

	return (ssize_t)size;
}

static ssize_t channelctrl_show(struct class *class,
	struct class_attribute *attr, char *buf)
{
	uint32_t ret = snprintf(buf, PAGE_SIZE, "schannelctrl:0x%x 0x%x!\n", schannelctrl, stracktype);

	ret += snprintf(buf + ret, (PAGE_SIZE - ret), "usage: echo param>schannelctrl\n");
	ret += snprintf(buf + ret, (PAGE_SIZE - ret), "param: schannelctrl\n");
	ret += snprintf(buf + ret, (PAGE_SIZE - ret), "0x1: deactive channel!\n");
	ret += snprintf(buf + ret, (PAGE_SIZE - ret), "0x2: active channel!\n");
	ret += snprintf(buf + ret, (PAGE_SIZE - ret), "param: stracktype\n");
	ret += snprintf(buf + ret, (PAGE_SIZE - ret), "da_combine_v3:0x0:audio play; 0x1:record; 0x2:voice down; 0x3:voice up; 0x4:image; 0x5:ec; 0x6:sound trigger; 0x7:debug; 0x8:direct; 0x9:fast\n");
	ret += snprintf(buf + ret, (PAGE_SIZE - ret),
		"da_combine_v5:0x0:audio play; 0x1:record; 0x2:voice down; 0x3:voice up; 0x4:ec; 0x5:sound trigger; 0x6:debug; 0x7:direct;\n");

	return ret;
}

static ssize_t channelctrl_store(struct class *class,
	struct class_attribute *attr, const char *buf, size_t size)
{
	int32_t ret = 0;
	int32_t i;

	// cppcheck-suppress *
	if (sscanf(buf, "0x%x 0x%x", &schannelctrl, &stracktype) != 2)
		return -EINVAL;

	AUDIO_LOGE("schannelctrl: %x, stracktype: %x, sdevice_type: %x", schannelctrl, stracktype, sdevice_type);

	if (sdevice_type == SLIMBUS_DEVICE_DA_COMBINE_V3 &&
		stracktype >= (uint32_t)DA_COMBINE_V3_BUS_TRACK_TYPE_MAX) {
		return -EFAULT;
	}
#ifdef CONFIG_SND_SOC_DA_COMBINE_V5
	if (sdevice_type == SLIMBUS_DEVICE_DA_COMBINE_V5 &&
		stracktype >= (uint32_t)DA_COMBINE_V5_BUS_TRACK_TYPE_MAX) {
		return -EFAULT;
	}
#endif
	switch (schannelctrl) {
	case 1:
		ret = slimbus_deactivate_track(stracktype, NULL);
		break;
	case 2:
		ret = slimbus_activate_track(stracktype, NULL);
		break;
	case 3:
		for (i = 0; i < 10000; i++) {
			ret += slimbus_activate_track(stracktype, NULL);
			ret += slimbus_deactivate_track(stracktype, NULL);
		}
		break;
	default:
		break;
	}

	if (ret)
		AUDIO_LOGE("ret: %d", ret);

	return (ssize_t)size;
}

static ssize_t pauseclock_show(struct class *class,
	struct class_attribute *attr, char *buf)
{
	uint32_t ret = snprintf(buf, PAGE_SIZE, "spauseclock:0x%x!\n", spauseclock);

	ret += snprintf(buf + ret, (PAGE_SIZE - ret), "0x2: slimbus clk pause!\n");
	ret += snprintf(buf + ret, (PAGE_SIZE - ret), "himm 0xe8050020 0x8 to wakeup clk!\n");

	return ret;
}

static ssize_t pauseclock_store(struct class *class,
	struct class_attribute *attr, const char *buf, size_t size)
{
	// cppcheck-suppress *
	if (sscanf(buf, "0x%x", &spauseclock) != 1)
		return -EINVAL;

	AUDIO_LOGE("g_pause_clock: %x", spauseclock);

	switch (spauseclock) {
	case 0:
		(void)slimbus_pause_clock(SLIMBUS_RT_FAST_RECOVERY);
		break;
	case 1:
		(void)slimbus_pause_clock(SLIMBUS_RT_CONSTANT_PHASE_RECOVERY);
		break;
	case 2:
		(void)slimbus_pause_clock(SLIMBUS_RT_UNSPECIFIED_DELAY);
		break;
	default:
		AUDIO_LOGE("unsupport restarttime: %x", spauseclock);
		break;
	}

	return (ssize_t)size;
}

static ssize_t regtest_show(struct class *class,
	struct class_attribute *attr, char *buf)
{
	uint32_t ret = snprintf(buf, PAGE_SIZE, "sregtest:0x%x!\n", sregtest);

	ret += snprintf(buf + ret, (PAGE_SIZE - ret), "scopyright_error:0x%x!\n", scopyright_error);
	ret += snprintf(buf + ret, (PAGE_SIZE - ret), "sreg8_rd_error:0x%x!\n", sreg8_rd_error);
	ret += snprintf(buf + ret, (PAGE_SIZE - ret), "sreg32_rd_error:0x%x!\n", sreg32_rd_error);

	return ret;
}

static void process_reg_1byte(void)
{
	uint32_t i;
	uint32_t val;
	int64_t start_time, end_time;

	start_time = get_timeus();
	for (i = 0; i < REG_ACCESS_TIMES; i++) {
		val = slimbus_read_1byte(REG_1_BYTE_START);
		if (i <= 4)
			AUDIO_LOGE("%x", val);

		if (val != 0x11) {
			scopyright_error++;
			if (scopyright_error <= 4)
				AUDIO_LOGE("val: %x i: %x", val, i);
		}
	}
	end_time = get_timeus();
	AUDIO_LOGE("read_1byte 1000s use %lld us (%lld, %lld)", end_time-start_time, start_time, end_time);

	start_time = get_timeus();
	for (i = 0; i < REG_ACCESS_TIMES; i++) {
		slimbus_write_1byte(0x2000703a, i);
		val = slimbus_read_1byte(0x2000703a);
		if (i <= 4)
			AUDIO_LOGE("val: %x", val);

		if ((i & 0xff) != val) {
			sreg8_rd_error++;
			if (sreg8_rd_error <= 4)
				AUDIO_LOGE("val: %x i: %x", val, i);
		}

	}
	end_time = get_timeus();
	AUDIO_LOGE("write_1byte 1000s use %lld us (%lld, %lld)", end_time-start_time, start_time, end_time);
}

static void process_reg_4byte(void)
{
	uint32_t i = 0;
	uint32_t val = 0;
	int64_t start_time, end_time;

	start_time = get_timeus();
	for (i = 0; i < REG_ACCESS_TIMES; i++) {
		val = slimbus_read_4byte(sregtest);
		if (i <= 4)
			AUDIO_LOGE("val: %x", val);
	}
	end_time = get_timeus();
	AUDIO_LOGE("read_4byte 1000s use %lld us  (%lld, %lld)", end_time-start_time, start_time, end_time);

	start_time = get_timeus();
	for (i = 0; i < REG_ACCESS_TIMES; i++) {
		slimbus_write_4byte(sregtest, i);
		val = slimbus_read_4byte(sregtest);
		if (i <= 4)
			AUDIO_LOGE("val: %x ", val);

		if (val != i) {
			sreg32_rd_error++;
			if (sreg32_rd_error <= 4)
				AUDIO_LOGE("val: %x i: %x !", val, i);
		}
	}
	end_time = get_timeus();
	AUDIO_LOGE("write_4byte 1000s use %lld us (%lld, %lld)", end_time-start_time, start_time, end_time);
}

static void process_reg(void)
{
	process_reg_1byte();
	process_reg_4byte();
}

static ssize_t regtest_store(struct class *class,
	struct class_attribute *attr, const char *buf, size_t size)
{
	scopyright_error = 0;
	sreg8_rd_error = 0;
	sreg32_rd_error = 0;

	// cppcheck-suppress *
	if (sscanf(buf, "0x%x", &sregtest) != 1)
		return -EINVAL;

	process_reg();

	return (ssize_t)size;
}

static ssize_t busreset_show(struct class *class,
	struct class_attribute *attr, char *buf)
{
	uint32_t ret = snprintf(buf, PAGE_SIZE, "sbusreset:0x%x!\n", sbusreset);

	return ret;
}

static ssize_t busreset_store(struct class *class,
	struct class_attribute *attr, const char *buf, size_t size)
{
	// cppcheck-suppress *
	if (sscanf(buf, "0x%x", &sbusreset) != 1)
		return -EINVAL;

	AUDIO_LOGE("sbusreset: 0x%x", sbusreset);

	if (sbusreset == 1)
		slimbus_drv_reset_bus();

	return (ssize_t)size;
}

static ssize_t mixtest_show(struct class *class,
	struct class_attribute *attr, char *buf)
{
	uint32_t ret = snprintf(buf, PAGE_SIZE, "smixtest:0x%x sregtest:0x%x stracktype:0x%x!\n",
					smixtest, sregtest, stracktype);

	return ret;
}

static ssize_t mixtest_store(struct class *class,
	struct class_attribute *attr, const char *buf, size_t size)
{
	int32_t ret;

	// cppcheck-suppress *
	if (sscanf(buf, "0x%x 0x%x 0x%x", &smixtest, &sregtest, &stracktype) != 3)
		return -EINVAL;

	AUDIO_LOGE("smixtest: 0x%x sregtest: 0x%x stracktype: 0x%x", smixtest, sregtest, stracktype);

	if (sdevice_type == SLIMBUS_DEVICE_DA_COMBINE_V3 && stracktype >= (uint32_t)DA_COMBINE_V3_BUS_TRACK_TYPE_MAX)
		return -EFAULT;
#ifdef CONFIG_SND_SOC_DA_COMBINE_V5
	else if (sdevice_type == SLIMBUS_DEVICE_DA_COMBINE_V5 && stracktype >= (uint32_t)DA_COMBINE_V5_BUS_TRACK_TYPE_MAX)
		return -EFAULT;
#endif
	if (smixtest == 1) {
		scopyright_error = 0;
		sreg8_rd_error = 0;
		sreg32_rd_error = 0;

		process_reg();

		AUDIO_LOGE("8rderr: %d, 8wrerr: %d, 32rdwrerr: %d", scopyright_error, sreg8_rd_error, sreg32_rd_error);

		slimbus_drv_reset_bus();
		usleep_range(4000, 4400);
		ret = slimbus_activate_track(stracktype, NULL);
		AUDIO_LOGE("ret %d", ret);
	}

	return (ssize_t)size;
}

static int32_t request_info_val;

static ssize_t requestinfo_show(struct class *class,
	struct class_attribute *attr, char *buf)
{
	uint32_t ret = snprintf(buf, PAGE_SIZE, "srequestinfo:0x%x!, val:%x!\n", srequestinfo, request_info_val);

	return ret;
}

static ssize_t requestinfo_store(struct class *class,
	struct class_attribute *attr, const char *buf, size_t size)
{
	// cppcheck-suppress *
	if (sscanf(buf, "0x%x", &srequestinfo) != 1)
		return -EINVAL;

	AUDIO_LOGE("srequestinfo: 0x%x", srequestinfo);

	switch (srequestinfo) {
	case 0x1:
		slimbus_drv_request_info(0x21, SLIMBUS_DRV_ADDRESS, SLIMBUS_SS_1_BYTE, &request_info_val);
		break;
	case 0x2:
		slimbus_drv_request_info(0x40, SLIMBUS_DRV_ADDRESS, SLIMBUS_SS_1_BYTE, &request_info_val);
		break;
	case 0x3:
		slimbus_drv_request_info(0x20, SLIMBUS_DRV_ADDRESS, SLIMBUS_SS_1_BYTE, &request_info_val);
		break;
	case 0x4:
		slimbus_drv_request_info(0x41, SLIMBUS_DRV_ADDRESS, SLIMBUS_SS_1_BYTE, &request_info_val);
		break;
	default:
		break;
	}
	AUDIO_LOGE("srequestinfo: 0x%x info: %x, -----", srequestinfo, request_info_val);

	return (ssize_t)size;
}

static ssize_t logcount_show(struct class *class,
	struct class_attribute *attr, char *buf)
{
	uint32_t ret;

	srdwrerr_logcount = slimbus_logcount_get();

	ret = snprintf(buf, PAGE_SIZE, "srdwrerr_logcount:0x%x!\n", srdwrerr_logcount);

	return ret;
}

static ssize_t logcount_store(struct class *class,
	struct class_attribute *attr, const char *buf, size_t size)
{
	// cppcheck-suppress *
	if (sscanf(buf, "0x%x", &srdwrerr_logcount) != 1)
		return -EINVAL;

	slimbus_logcount_set(srdwrerr_logcount);

	return (ssize_t)size;
}

static ssize_t logtimes_show(struct class *class,
	struct class_attribute *attr, char *buf)
{
	uint32_t ret;

	srdwrerr_logtimes = slimbus_logtimes_get();

	ret = snprintf(buf, PAGE_SIZE, "srdwrerr_logtimes:0x%x!\n", srdwrerr_logtimes); /*[false alarm]*/

	return ret;
}

static ssize_t logtimes_store(struct class *class,
	struct class_attribute *attr, const char *buf, size_t size)
{
	// cppcheck-suppress *
	if (sscanf(buf, "0x%x", &srdwrerr_logtimes) != 1)
		return -EINVAL;

	slimbus_logtimes_set(srdwrerr_logtimes);

	return (ssize_t)size;
}

static ssize_t lostmstimes_show(struct class *class,
	struct class_attribute *attr, char *buf)
{
	uint32_t ret;

	slostms_times = slimbus_drv_lostms_get();

	ret = snprintf(buf, PAGE_SIZE, "slostms_times:0x%x!\n", slostms_times);

	return ret;
}

static ssize_t lostmstimes_store(struct class *class,
	struct class_attribute *attr, const char *buf, size_t size)
{
	// cppcheck-suppress *
	if (sscanf(buf, "0x%x", &slostms_times) != 1)
		return -EINVAL;

	slimbus_drv_lostms_set(slostms_times);

	return (ssize_t)size;
}

static const struct class_attribute slimbus_attrs[] = {
#ifdef __IMAGE_DOWN_TEST
	__ATTR(simgdown,            0640, imgdown_show,        imgdown_store),
#endif
	__ATTR(sregpagerd,          0640, regpagerd_show,      regpagerd_store),
	__ATTR(sregrd,              0640, regrd_show,          regrd_store),
	__ATTR(sregwr,              0640, regwr_show,          regwr_store),
	__ATTR(sswitchframer,       0660, switchframer_show,   switchframer_store),
	__ATTR(sclockgear,          0660, clockgear_show,      clockgear_store),
	__ATTR(schannelctrl,        0660, channelctrl_show,    channelctrl_store),
	__ATTR(spauseclock,         0660, pauseclock_show,     pauseclock_store),
	__ATTR(sregtest,            0660, regtest_show,        regtest_store),
	__ATTR(sbusreset,           0660, busreset_show,       busreset_store),
	__ATTR(smixtest,            0660, mixtest_show,        mixtest_store),
	__ATTR(srequestinfo,        0660, requestinfo_show,    requestinfo_store),
	__ATTR(srdwrerr_logcount,   0660, logcount_show,       logcount_store),
	__ATTR(srdwrerr_logtimes,   0660, logtimes_show,       logtimes_store),
	__ATTR(slostms_times,       0660, lostmstimes_show,    lostmstimes_store),
	__ATTR(sdevice_type,        0660, devtype_show,        devtype_store),
};

static int32_t create_slimbus_attrs(struct class *class)
{
	int32_t i;
	int32_t ret;

	for (i = 0; (unsigned int)i < (sizeof(slimbus_attrs)/sizeof(struct class_attribute)); i++) {
		ret = class_create_file(class, &slimbus_attrs[i]);
		if (ret)
			goto error;
	}

	return ret;
error:
	while (--i >= 0)
		class_remove_file(class, &slimbus_attrs[i]);
	return ret;
}

static void remove_slimbus_attrs(struct class *class)
{
	uint32_t i;

	for (i = 0; i < (sizeof(slimbus_attrs) / sizeof(struct class_attribute)); i++)
		class_remove_file(class, &slimbus_attrs[i]);
}

static int32_t __init slimbus_debug_init(void)
{
	int32_t ret;

	sdevice_type = slimbus_debug_get_device_type();
	if (sdevice_type >= SLIMBUS_DEVICE_NUM)
		return -EFAULT;

	sclass = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(sclass)) {
		AUDIO_LOGE("class create error");
		return -EFAULT;
	}

	ret = create_slimbus_attrs(sclass);
	if (ret < 0) {
		class_destroy(sclass);
		sclass = NULL;
		AUDIO_LOGE("create_slimbus_attrs error");
		return -EFAULT;
	}

	return 0;
}

static void __exit slimbus_debug_exit(void)
{
	if (!sclass)
		return;

	remove_slimbus_attrs(sclass);
	class_destroy(sclass);
	sclass = NULL;
}

module_init(slimbus_debug_init);
module_exit(slimbus_debug_exit);

MODULE_LICENSE("GPL");

