/*
 * usbaudio_dsp_client.c
 *
 * usbaudio dsp client
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2015-2021. All rights reserved.
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
#include "usbaudio_dsp_client.h"

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/usb.h>
#include <linux/usb/audio.h>
#include <linux/usb/audio-v2.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
#include <linux/usb/audio-v3.h>
#endif
#include <linux/slab.h>
#include <linux/platform_drivers/usb/chip_usb.h>
#include <sound/control.h>
#include <sound/core.h>
#include <sound/info.h>
#include <sound/initval.h>
#include <huawei_platform/usb/usb_extra_modem.h>
#include <linux/of_platform.h>

#include "audio_log.h"
#include "usbaudio_ioctl.h"
#include "card.h"
#include "helper.h"
#include "format.h"
#include "clock.h"

#ifndef LOG_TAG
#define LOG_TAG "usbaudio_dsp_client"
#endif

#define USB_REQ_SET_VOLUME 0x2
#define TIMEOUT_MS 3000

struct usbaudio_pcm_cfg {
	u64 formats;    /* ALSA format bits */
	unsigned int channels;
	unsigned int rates;
};

struct usb_audio_format {
	unsigned int format;
	unsigned int num_channels;
	int iface;
	unsigned char clock;
	unsigned char altno;
	unsigned char protocol;
	unsigned char altset_idx;
};

u32 g_forced_match_arm_usb_ids[] = {
	USB_ID(0x041e, 0x3000),
	USB_ID(0x041e, 0x3010),
	USB_ID(0x041e, 0x3020),
	USB_ID(0x041e, 0x3040),
	USB_ID(0x041e, 0x3042),
	USB_ID(0x041e, 0x3048),
	USB_ID(0x041e, 0x3061),
	USB_ID(0x041e, 0x30df),
	USB_ID(0x041e, 0x3237),
	USB_ID(0x041e, 0x3f02),
	USB_ID(0x041e, 0x3f04),
	USB_ID(0x041e, 0x3f0a),
	USB_ID(0x041e, 0x3f19),
	USB_ID(0x041e, 0x4064),
	USB_ID(0x041e, 0x4068),
	USB_ID(0x041e, 0x4080),
	USB_ID(0x045e, 0x075d),
	USB_ID(0x045e, 0x076d),
	USB_ID(0x045e, 0x076e),
	USB_ID(0x045e, 0x076f),
	USB_ID(0x045e, 0x0772),
	USB_ID(0x045e, 0x0779),
	USB_ID(0x046d, 0x09a4),
	USB_ID(0x046d, 0x0807),
	USB_ID(0x046d, 0x0808),
	USB_ID(0x046d, 0x0809),
	USB_ID(0x046d, 0x0819),
	USB_ID(0x046d, 0x081b),
	USB_ID(0x046d, 0x081d),
	USB_ID(0x046d, 0x0825),
	USB_ID(0x046d, 0x0826),
	USB_ID(0x046d, 0x08ca),
	USB_ID(0x046d, 0x0991),
	USB_ID(0x046d, 0x09a4),
	USB_ID(0x0471, 0x0101),
	USB_ID(0x0471, 0x0104),
	USB_ID(0x0471, 0x0105),
	USB_ID(0x047f, 0x0ca1),
	USB_ID(0x047f, 0xc010),
	USB_ID(0x047f, 0x0415),
	USB_ID(0x047f, 0xaa05),
	USB_ID(0x04d8, 0xfeea),
	USB_ID(0x04fa, 0x4201),
	USB_ID(0x0556, 0x0014),
	USB_ID(0x0582, 0x000c),
	USB_ID(0x0582, 0x0016),
	USB_ID(0x0582, 0x002b),
	USB_ID(0x05a3, 0x9420),
	USB_ID(0x05a7, 0x1020),
	USB_ID(0x0644, 0x8038),
	USB_ID(0x0672, 0x1041),
	USB_ID(0x06f8, 0xb000),
	USB_ID(0x06f8, 0xc000),
	USB_ID(0x06f8, 0xd002),
	USB_ID(0x074d, 0x3553),
	USB_ID(0x0763, 0x0150),
	USB_ID(0x0763, 0x2001),
	USB_ID(0x0763, 0x2003),
	USB_ID(0x0763, 0x2012),
	USB_ID(0x0763, 0x2030),
	USB_ID(0x0763, 0x2031),
	USB_ID(0x0763, 0x2080),
	USB_ID(0x0763, 0x2081),
	USB_ID(0x077d, 0x07af),
	USB_ID(0x07fd, 0x0001),
	USB_ID(0x08bb, 0x2702),
	USB_ID(0x0a92, 0x0053),
	USB_ID(0x0a92, 0x0091),
	USB_ID(0x0b05, 0x1739),
	USB_ID(0x0b05, 0x1743),
	USB_ID(0x0b05, 0x17a0),
	USB_ID(0x0bda, 0x4014),
	USB_ID(0x0c45, 0x1158),
	USB_ID(0x0ccd, 0x00b1),
	USB_ID(0x0d8c, 0x0102),
	USB_ID(0x0d8c, 0x0103),
	USB_ID(0x0d8c, 0x0201),
	USB_ID(0x0dba, 0x1000),
	USB_ID(0x0dba, 0x3000),
	USB_ID(0x10f5, 0x0200),
	USB_ID(0x1130, 0xf211),
	USB_ID(0x1235, 0x0010),
	USB_ID(0x1235, 0x0018),
	USB_ID(0x1235, 0x8002),
	USB_ID(0x1235, 0x8004),
	USB_ID(0x1235, 0x800c),
	USB_ID(0x1235, 0x8012),
	USB_ID(0x1235, 0x8014),
	USB_ID(0x133e, 0x0815),
	USB_ID(0x13e5, 0x0001),
	USB_ID(0x154e, 0x1003),
	USB_ID(0x154e, 0x3005),
	USB_ID(0x154e, 0x3006),
	USB_ID(0x1686, 0x00dd),
	USB_ID(0x17cc, 0x1000),
	USB_ID(0x17cc, 0x1010),
	USB_ID(0x17cc, 0x1011),
	USB_ID(0x17cc, 0x1020),
	USB_ID(0x17cc, 0x1021),
	USB_ID(0x18d1, 0x2d04),
	USB_ID(0x18d1, 0x2d05),
	USB_ID(0x1901, 0x0191),
	USB_ID(0x1de7, 0x0013),
	USB_ID(0x1de7, 0x0014),
	USB_ID(0x1de7, 0x0114),
	USB_ID(0x200c, 0x1018),
	USB_ID(0x20b1, 0x3008),
	USB_ID(0x20b1, 0x2008),
	USB_ID(0x20b1, 0x300a),
	USB_ID(0x20b1, 0x000a),
	USB_ID(0x20b1, 0x2009),
	USB_ID(0x20b1, 0x2023),
	USB_ID(0x20b1, 0x3023),
	USB_ID(0x21b4, 0x0081),
	USB_ID(0x22d9, 0x0416),
	USB_ID(0x2573, 0x0008),
	USB_ID(0x25c4, 0x0003),
	USB_ID(0x2616, 0x0106),
	USB_ID(0x27ac, 0x1000),
};

u32 g_customsized_usb_ids[] = {
	USB_ID(0x12d1, 0x3a07),
};

u32 g_forced_match_hifi_usb_ids[] = {
	USB_ID(0x262a, 0x9302), /* iBasso-DC-Series */
};

static bool need_reset_vbus(u32 usb_id)
{
	int i;
	static const u32 reset_vbus_usb_ids[] = {
		/* Letv Headset */
		USB_ID(0x262a, 0x1534),
		/* OnePlus ED117 */
		USB_ID(0x2a70, 0x1881),
	};

	for (i = 0; i < ARRAY_SIZE(reset_vbus_usb_ids); i++) {
		if (reset_vbus_usb_ids[i] == usb_id)
			return true;
	}

	return false;
}

/*
 * find an input terminal descriptor (either UAC1 or UAC2)
 * with the given terminal id
 */
static void *snd_usb_find_input_terminal_descriptor(
	struct usb_host_interface *ctrl_iface, int terminal_id)
{
	struct uac2_input_terminal_descriptor *term = NULL;

	while ((term = snd_usb_find_csint_desc(ctrl_iface->extra,
		ctrl_iface->extralen, term, UAC_INPUT_TERMINAL))) {
		if (term->bTerminalID == terminal_id)
			return term;
	}

	return NULL;
}

static struct uac2_output_terminal_descriptor *snd_usb_find_output_terminal_descriptor(
	struct usb_host_interface *ctrl_iface, int terminal_id)
{
	struct uac2_output_terminal_descriptor *term = NULL;

	while ((term = snd_usb_find_csint_desc(ctrl_iface->extra,
		ctrl_iface->extralen, term, UAC_OUTPUT_TERMINAL))) {
		if (term->bTerminalID == terminal_id)
			return term;
	}

	return NULL;
}

static unsigned int parse_uac_endpoint_attributes(struct snd_usb_audio *chip,
	struct usb_host_interface *alts, unsigned char protocol, int iface_no)
{
	/*
	 * parsed with a v1 header here. that's ok as we only look at the
	 * header first which is the same for both versions
	 */
	struct uac_iso_endpoint_descriptor *csep = NULL;
	struct usb_interface_descriptor *altsd = get_iface_desc(alts);
	unsigned int attributes = 0;

	csep = snd_usb_find_desc(alts->endpoint[0].extra,
		alts->endpoint[0].extralen, NULL, USB_DT_CS_ENDPOINT);
	/* Creamware Noah has this descriptor after the 2nd endpoint */
	if (!csep && altsd->bNumEndpoints >= 2)
		csep = snd_usb_find_desc(alts->endpoint[1].extra,
			alts->endpoint[1].extralen, NULL, USB_DT_CS_ENDPOINT);

	/*
	 * If we can't locate the USB_DT_CS_ENDPOINT descriptor in the extra
	 * bytes after the first endpoint, go search the entire interface.
	 * Some devices have it directly *before* the standard endpoint.
	 */
	if (!csep)
		csep = snd_usb_find_desc(alts->extra, alts->extralen, NULL, USB_DT_CS_ENDPOINT);

	if (!csep || csep->bLength < 7 ||
		csep->bDescriptorSubtype != UAC_EP_GENERAL) {
		usb_audio_warn(chip,
			"%d: %d: no or invalid class specific endpoint descriptor\n",
			iface_no, altsd->bAlternateSetting);
		return 0;
	}

	if (protocol == UAC_VERSION_1) {
		attributes = csep->bmAttributes;
	} else {
		struct uac2_iso_endpoint_descriptor *csep2 =
			(struct uac2_iso_endpoint_descriptor *) csep;

		attributes = csep->bmAttributes & UAC_EP_CS_ATTR_FILL_MAX;

		/* emulate the endpoint attributes of a v1 device */
		if (csep2->bmControls & UAC2_CONTROL_PITCH)
			attributes |= UAC_EP_CS_ATTR_PITCH_CONTROL;
	}

	return attributes;
}

static int check_interface(struct usb_host_interface *alts, struct usb_interface_descriptor *altsd)
{
	/* skip invalid one */
	if (((altsd->bInterfaceClass != USB_CLASS_AUDIO ||
		(altsd->bInterfaceSubClass != USB_SUBCLASS_AUDIOSTREAMING &&
		altsd->bInterfaceSubClass != USB_SUBCLASS_VENDOR_SPEC)) &&
		altsd->bInterfaceClass != USB_CLASS_VENDOR_SPEC) ||
		altsd->bNumEndpoints < 1 ||
		le16_to_cpu(get_endpoint(alts, 0)->wMaxPacketSize) == 0)
		return -1;

	/* must be isochronous */
	if ((get_endpoint(alts, 0)->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) !=
		USB_ENDPOINT_XFER_ISOC)
		return -1;

	return 0;
}

static int get_audio_formats(struct snd_usb_audio *chip, struct usb_device *dev,
	struct usb_audio_format *audio_fmt, struct usb_host_interface *alts)
{
	switch (audio_fmt->protocol) {
	default:
		dev_dbg(&dev->dev, "%d: %d: unknown interface protocol %#02x, assuming v1\n",
			audio_fmt->iface, audio_fmt->altno, audio_fmt->protocol);
		audio_fmt->protocol = UAC_VERSION_1;
		/* fall through */

	case UAC_VERSION_1: {
		struct uac1_as_header_descriptor *as =
			snd_usb_find_csint_desc(alts->extra, alts->extralen, NULL, UAC_AS_GENERAL);
		struct uac_input_terminal_descriptor *iterm = NULL;

		if (!as) {
			dev_err(&dev->dev,
				"%d: %d: UAC_AS_GENERAL descriptor not found\n",
				audio_fmt->iface, audio_fmt->altno);
			return -1;
		}

		if (as->bLength < sizeof(*as)) {
			dev_err(&dev->dev,
				"%d: %d: invalid UAC_AS_GENERAL desc\n",
				audio_fmt->iface, audio_fmt->altno);
			return -1;
		}

		/* remember the format value */
		audio_fmt->format = le16_to_cpu(as->wFormatTag);

		iterm = snd_usb_find_input_terminal_descriptor(chip->ctrl_intf,
			as->bTerminalLink);
		if (iterm)
			audio_fmt->num_channels = iterm->bNrChannels;

		break;
	}

	case UAC_VERSION_2: {
		struct uac2_input_terminal_descriptor *input_term = NULL;
		struct uac2_output_terminal_descriptor *output_term = NULL;
		struct uac2_as_header_descriptor *as =
			snd_usb_find_csint_desc(alts->extra, alts->extralen, NULL, UAC_AS_GENERAL);

		if (!as) {
			dev_err(&dev->dev,
				"%d: %d: UAC_AS_GENERAL descriptor not found\n",
				audio_fmt->iface, audio_fmt->altno);
			return -1;
		}

		if (as->bLength < sizeof(*as)) {
			dev_err(&dev->dev,
				"%d: %d: invalid UAC_AS_GENERAL desc\n",
				audio_fmt->iface, audio_fmt->altno);
			return -1;
		}

		audio_fmt->num_channels = as->bNrChannels;
		audio_fmt->format = le32_to_cpu(as->bmFormats);

		/*
		 * lookup the terminal associated to this interface
		 * to extract the clock
		 */
		input_term = snd_usb_find_input_terminal_descriptor(chip->ctrl_intf,
			as->bTerminalLink);
		if (input_term) {
			audio_fmt->clock = input_term->bCSourceID;
			break;
		}

		output_term = snd_usb_find_output_terminal_descriptor(chip->ctrl_intf,
			as->bTerminalLink);
		if (output_term) {
			audio_fmt->clock = output_term->bCSourceID;
			break;
		}

		dev_err(&dev->dev, "%d: %d: bogus bTerminalLink %d\n",
			audio_fmt->iface, audio_fmt->altno, as->bTerminalLink);
		return -1;
	}
	}

	return 0;
}

/*
 * Blue Microphones workaround: The last altsetting is identical
 * with the previous one, except for a larger packet size, but
 * is actually a mislabeled two-channel setting; ignore it.
 */
static bool need_blue_microphones_workaround(const struct audioformat *fp,
	const struct uac_format_type_i_continuous_descriptor *fmt,
	struct usb_audio_format audio_fmt,
	struct usb_host_interface *alts, int num)
{
	if (fmt->bNrChannels == 1 && fmt->bSubframeSize == 2 &&
		audio_fmt.altno == 2 && num == 3 &&
		fp && fp->altsetting == 1 && fp->channels == 1 &&
		fp->formats == SNDRV_PCM_FMTBIT_S16_LE &&
		audio_fmt.protocol == UAC_VERSION_1 &&
		le16_to_cpu(get_endpoint(alts, 0)->wMaxPacketSize) ==
		fp->maxpacksize * 2)
			return true;

	return false;
}

static void set_format_para(struct audioformat *fp,
	const struct usb_audio_format audio_fmt, struct usb_host_interface *alts,
	struct snd_usb_audio *chip, struct usb_device *dev)
{
	fp->iface = audio_fmt.iface;
	fp->altsetting = audio_fmt.altno;
	fp->altset_idx = audio_fmt.altset_idx;
	fp->endpoint = get_endpoint(alts, 0)->bEndpointAddress;
	fp->ep_attr = get_endpoint(alts, 0)->bmAttributes;
	fp->datainterval = snd_usb_parse_datainterval(chip, alts);
	fp->protocol = audio_fmt.protocol;
	fp->maxpacksize = le16_to_cpu(get_endpoint(alts, 0)->wMaxPacketSize);
	fp->channels = audio_fmt.num_channels;
	if (snd_usb_get_speed(dev) == USB_SPEED_HIGH)
		fp->maxpacksize = (((fp->maxpacksize >> 11) & 3) + 1) *
			(fp->maxpacksize & 0x7ff);

	fp->attributes = parse_uac_endpoint_attributes(chip, alts,
		audio_fmt.protocol, audio_fmt.iface);
	fp->clock = audio_fmt.clock;
}

static int snd_usb_parse_audio_interface(struct snd_usb_audio *chip,
	int iface_no, struct list_head *fp_list)
{
	struct usb_device *dev = NULL;
	struct usb_interface *iface = NULL;
	struct usb_host_interface *alts = NULL;
	struct usb_interface_descriptor *altsd = NULL;
	struct audioformat *fp = NULL;
	struct uac_format_type_i_continuous_descriptor *fmt = NULL;
	struct usb_audio_format audio_fmt = {0};
	int i, num, stream;
	int ret;

	dev = chip->dev;

	/* parse the interface's altsettings */
	iface = usb_ifnum_to_if(dev, iface_no);
	if (!iface) {
		dev_err(&dev->dev, "%d: does not exist\n", iface_no);
		return -EINVAL;
	}

	audio_fmt.iface = iface_no;
	num = iface->num_altsetting;

	for (i = 0; i < num; i++) {
		alts = &iface->altsetting[i];
		altsd = get_iface_desc(alts);
		audio_fmt.protocol = altsd->bInterfaceProtocol;
		audio_fmt.altset_idx = (unsigned char)i;

		ret = check_interface(alts, altsd);
		if (ret != 0)
			continue;

		/* check direction */
		stream = (get_endpoint(alts, 0)->bEndpointAddress & USB_DIR_IN) ?
			SNDRV_PCM_STREAM_CAPTURE : SNDRV_PCM_STREAM_PLAYBACK;
		audio_fmt.altno = altsd->bAlternateSetting;

		ret = get_audio_formats(chip, dev, &audio_fmt, alts);
		if (ret != 0)
			continue;

		/* get format type */
		fmt = snd_usb_find_csint_desc(alts->extra, alts->extralen, NULL, UAC_FORMAT_TYPE);
		if (!fmt) {
			dev_err(&dev->dev,
				"%d: %d: no UAC_FORMAT_TYPE desc\n",
				audio_fmt.iface, audio_fmt.altno);
			continue;
		}

		if (((audio_fmt.protocol == UAC_VERSION_1) && (fmt->bLength < 8)) ||
			((audio_fmt.protocol == UAC_VERSION_2) && (fmt->bLength < 6))) {
			dev_err(&dev->dev,
				"%d: %d: invalid UAC_FORMAT_TYPE desc\n",
				audio_fmt.iface, audio_fmt.altno);
			continue;
		}

		ret = need_blue_microphones_workaround(fp, fmt, audio_fmt, alts, num);
		if (ret)
			continue;

		fp = kzalloc(sizeof(*fp), GFP_KERNEL);
		if (!fp) {
			dev_err(&dev->dev, "cannot malloc\n");
			return -ENOMEM;
		}

		set_format_para(fp, audio_fmt, alts, chip, dev);
		INIT_LIST_HEAD(&fp->list);

		/* ok, let's parse further... */
		if (snd_usb_parse_audio_format(chip, fp, audio_fmt.format, fmt, stream) < 0) {
			kfree(fp->rate_table);
			kfree(fp);
			fp = NULL;
			continue;
		}

		list_add_tail(&fp->list, fp_list);
	}

	return 0;
}

static int snd_usb_create_stream(struct snd_usb_audio *chip, int ctrlif,
	int interface, struct list_head *fp_list)
{
	struct usb_device *dev = chip->dev;
	struct usb_host_interface *alts = NULL;
	struct usb_interface_descriptor *altsd = NULL;
	struct usb_interface *iface = usb_ifnum_to_if(dev, interface);

	if (!iface) {
		dev_err(&dev->dev, "%d: %d: does not exist\n",
			ctrlif, interface);
		return -EINVAL;
	}

	alts = &iface->altsetting[0];
	altsd = get_iface_desc(alts);

	if (usb_interface_claimed(iface)) {
		dev_dbg(&dev->dev, "%d: %d: skipping, already claimed\n",
			ctrlif, interface);
		return -EINVAL;
	}

	if ((altsd->bInterfaceClass == USB_CLASS_AUDIO ||
		altsd->bInterfaceClass == USB_CLASS_VENDOR_SPEC) &&
		altsd->bInterfaceSubClass == USB_SUBCLASS_MIDISTREAMING)
		return -EINVAL;

	if ((altsd->bInterfaceClass != USB_CLASS_AUDIO &&
		altsd->bInterfaceClass != USB_CLASS_VENDOR_SPEC) ||
		altsd->bInterfaceSubClass != USB_SUBCLASS_AUDIOSTREAMING) {
		dev_dbg(&dev->dev,
			"%d: %d: skipping non-supported interface %d\n",
			ctrlif, interface, altsd->bInterfaceClass);
		/* skip non-supported classes */
		return -EINVAL;
	}

	if (snd_usb_get_speed(dev) == USB_SPEED_LOW) {
		dev_err(&dev->dev, "low speed audio streaming not supported\n");
		return -EINVAL;
	}

	return snd_usb_parse_audio_interface(chip, interface, fp_list);
}

static int snd_usb_create_streams(struct snd_usb_audio *chip,
	int ctrlif, struct list_head *fp_list)
{
	struct usb_device *dev = chip->dev;
	struct usb_host_interface *host_iface = NULL;
	struct usb_interface_descriptor *altsd = NULL;
	void *control_header = NULL;
	unsigned char i;

	/* find audiocontrol interface */
	host_iface = &usb_ifnum_to_if(dev, ctrlif)->altsetting[0];
	control_header = snd_usb_find_csint_desc(host_iface->extra,
		host_iface->extralen, NULL, UAC_HEADER);
	if (!control_header) {
		dev_err(&dev->dev, "cannot find UAC_HEADER\n");
		return -EINVAL;
	}

	altsd = get_iface_desc(host_iface);

	switch (altsd->bInterfaceProtocol) {
	default:
		dev_warn(&dev->dev,
			"unknown interface protocol %#02x, assuming v1\n",
			altsd->bInterfaceProtocol);
		/* fall through */
	case UAC_VERSION_1: {
		struct uac1_ac_header_descriptor *h1 = control_header;

		if (!h1->bInCollection) {
			dev_info(&dev->dev, "skipping empty audio interface v1\n");
			return -EINVAL;
		}

		if (h1->bLength < sizeof(*h1) + h1->bInCollection) {
			dev_err(&dev->dev, "invalid UAC_HEADER v1\n");
			return -EINVAL;
		}

		for (i = 0; i < h1->bInCollection; i++)
			snd_usb_create_stream(chip, ctrlif, h1->baInterfaceNr[i], fp_list);

		break;
	}
	case UAC_VERSION_2: {
		struct usb_interface_assoc_descriptor *assoc =
			usb_ifnum_to_if(dev, ctrlif)->intf_assoc;

		if (!assoc) {
			/*
			 * Firmware writers cannot count to three.  So to find
			 * the IAD on the NuForce UDH-100, also check the next interface
			 */
			struct usb_interface *iface = usb_ifnum_to_if(dev, ctrlif + 1);

			if (iface &&
				iface->intf_assoc &&
				(iface->intf_assoc->bFunctionClass == USB_CLASS_AUDIO) &&
				(iface->intf_assoc->bFunctionProtocol == UAC_VERSION_2))
				assoc = iface->intf_assoc;
		}

		if (!assoc) {
			dev_err(&dev->dev,
				"audio class v2 interfaces need an interface association\n");
			return -EINVAL;
		}

		for (i = 0; i < assoc->bInterfaceCount; i++) {
			int intf = assoc->bFirstInterface + i;

			if (intf != ctrlif)
				snd_usb_create_stream(chip, ctrlif, intf, fp_list);
		}

		break;
	}
	}

	return 0;
}

bool is_customized_headset(u32 usb_id)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(g_customsized_usb_ids); i++) {
		if (usb_id == g_customsized_usb_ids[i]) {
			AUDIO_LOGI("customized usbid %u", usb_id);
			return true;
		}
	}

	return false;
}

bool is_forced_match_hifi_headset(u32 usb_id)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(g_forced_match_hifi_usb_ids); i++) {
		if (usb_id == g_forced_match_hifi_usb_ids[i]) {
			AUDIO_LOGI("forced match hifi usbid %u", usb_id);
			return true;
		}
	}

	return false;
}


static void set_capture_rate(const struct audioformat *fp,
	struct usbaudio_pcm_cfg *pcm_cfg, unsigned int *capture_rate, bool *found, int idx)
{
	unsigned int i;
	unsigned int hifi_capture_rates[] = { 96000, 48000 };
	unsigned int rate;

	rate = *capture_rate;
	for (i = 0; i < ARRAY_SIZE(hifi_capture_rates); i++) {
		if (hifi_capture_rates[i] != fp->rate_table[idx])
			continue;

		*found = true;
		if (rate >= fp->rate_table[idx]) {
			rate = fp->rate_table[idx];
			pcm_cfg[SNDRV_PCM_STREAM_CAPTURE].rates = rate;
			*capture_rate = rate;
		}
	}
}

static void set_playback_rate(const struct audioformat *fp,
	struct usbaudio_pcm_cfg *pcm_cfg, unsigned int *playback_rate, bool *found, int idx)
{
	unsigned int i;
	unsigned int hifi_playback_rates[] = { 192000, 96000, 48000 };
	unsigned int rate;

	rate = *playback_rate;
	for (i = 0; i < ARRAY_SIZE(hifi_playback_rates); i++) {
		if (hifi_playback_rates[i] != fp->rate_table[idx])
			continue;

		*found = true;
		if (rate <= fp->rate_table[idx]) {
			rate = fp->rate_table[idx];
			pcm_cfg[SNDRV_PCM_STREAM_PLAYBACK].rates = rate;
			*playback_rate = rate;
		}
	}
}

bool hifi_samplerate_compare(struct audioformat *fp, struct usbaudio_pcm_cfg *pcm_cfg)
{
	int i;
	bool found = false;
	unsigned int playback_rate = 48000;
	unsigned int capture_rate = 96000;

	for (i = 0; i < fp->nr_rates; i++) {
		if (fp->endpoint & USB_DIR_IN)
			set_capture_rate(fp, pcm_cfg, &capture_rate, &found, i);
		else
			set_playback_rate(fp, pcm_cfg, &playback_rate, &found, i);
	}

	return found;
}

bool hifi_formats_compare(struct audioformat *fp, struct usbaudio_pcm_cfg *pcm_cfg)
{
	unsigned int i;
	bool found = false;
	unsigned long long hifi_playback_formats[] = {
		SNDRV_PCM_FMTBIT_S32_LE,
		SNDRV_PCM_FMTBIT_S24_LE,
		SNDRV_PCM_FMTBIT_S24_3LE,
		SNDRV_PCM_FMTBIT_S16_LE
	};
	unsigned long long hifi_capture_formats[] = {SNDRV_PCM_FMTBIT_S16_LE};

	if (fp->endpoint & USB_DIR_IN) {
		for (i = 0; i < ARRAY_SIZE(hifi_capture_formats); i++) {
			if (fp->formats == hifi_capture_formats[i]) {
				found = true;
				pcm_cfg[SNDRV_PCM_STREAM_CAPTURE].formats = fp->formats;
			}
		}
	} else {
		for (i = 0; i < ARRAY_SIZE(hifi_playback_formats); i++) {
			if (fp->formats == hifi_playback_formats[i]) {
				found = true;
				pcm_cfg[SNDRV_PCM_STREAM_PLAYBACK].formats = fp->formats;
			}
		}
	}

	return found;
}

bool hifi_channel_compare(struct audioformat *fp, struct usbaudio_pcm_cfg *pcm_cfg)
{
	unsigned int i;
	bool found = false;
	unsigned int hifi_playback_channels[] = {2};
	unsigned int hifi_capture_channels[] = {2, 1};

	if (fp->endpoint & USB_DIR_IN) {
		for (i = 0; i < ARRAY_SIZE(hifi_capture_channels); i++) {
			if (fp->channels == hifi_capture_channels[i]) {
				found = true;
				pcm_cfg[SNDRV_PCM_STREAM_CAPTURE].channels = fp->channels;
			}
		}
	} else {
		for (i = 0; i < ARRAY_SIZE(hifi_playback_channels); i++) {
			if (fp->channels == hifi_playback_channels[i]) {
				found = true;
				pcm_cfg[SNDRV_PCM_STREAM_PLAYBACK].channels = fp->channels;
			}
		}
	}

	return found;
}

bool hifi_usbaudio_protocal_compare(struct usb_device *dev, struct audioformat *fp, u32 usb_id)
{
	struct usb_host_interface *alts = NULL;
	struct usb_interface_descriptor *altsd = NULL;
	struct usb_interface *iface = NULL;

	iface = usb_ifnum_to_if(dev, fp->iface);
	if (WARN_ON(!iface))
		return false;

	alts = &iface->altsetting[fp->altset_idx];
	altsd = get_iface_desc(alts);
	if (altsd->bNumEndpoints != 1 && !is_forced_match_hifi_headset(usb_id)) {
		AUDIO_LOGE("endpoint num is not match, bNumEndpoints %u", altsd->bNumEndpoints);
		return false;
	}

	if ((fp->protocol == UAC_VERSION_1 || fp->protocol == UAC_VERSION_2) &&
		(fp->fmt_type == UAC_FORMAT_TYPE_I) &&
		(fp->datainterval == 0))
		return true;

	return false;
}

snd_pcm_format_t pcm_fomat_convert(unsigned long long format)
{
	snd_pcm_format_t pcm_format = SNDRV_PCM_FORMAT_S16_LE;

	switch (format) {
	case SNDRV_PCM_FMTBIT_S16_LE:
		pcm_format = SNDRV_PCM_FORMAT_S16_LE;
		break;
	case SNDRV_PCM_FMTBIT_S24_LE:
		pcm_format = SNDRV_PCM_FORMAT_S24_LE;
		break;
	case SNDRV_PCM_FMTBIT_S24_3LE:
		pcm_format = SNDRV_PCM_FORMAT_S24_3LE;
		break;
	case SNDRV_PCM_FMTBIT_S32_LE:
		pcm_format = SNDRV_PCM_FORMAT_S32_LE;
		break;
	default:
		AUDIO_LOGE("format not support %llu", format);
		break;
	}

	return pcm_format;
}

static void hifi_samplerate_table_filled(struct audioformat *fp, struct usbaudio_formats *fmt)
{
	unsigned int i;
	unsigned int rate = 0;

	for (i = 0; i < ARRAY_SIZE(fmt->rate_table); i++)
		fmt->rate_table[i] = 0;

	for (i = 0; i < fp->nr_rates; i++) {
		rate = fp->rate_table[i];
		switch (rate) {
		case 44100:
			fmt->rate_table[USBAUDIO_TABLE_SAMPLERATE_44100] = 44100;
			break;
		case 48000:
			fmt->rate_table[USBAUDIO_TABLE_SAMPLERATE_48000] = 48000;
			break;
		case 88200:
			fmt->rate_table[USBAUDIO_TABLE_SAMPLERATE_88200] = 88200;
			break;
		case 96000:
			fmt->rate_table[USBAUDIO_TABLE_SAMPLERATE_96000] = 96000;
			break;
		case 176400:
			fmt->rate_table[USBAUDIO_TABLE_SAMPLERATE_176400] = 176400;
			break;
		case 192000:
			fmt->rate_table[USBAUDIO_TABLE_SAMPLERATE_192000] = 192000;
			break;
		case 384000:
			fmt->rate_table[USBAUDIO_TABLE_SAMPLERATE_384000] = 384000;
			break;
		default:
			AUDIO_LOGE("rata table do not contain this rate, %u", rate);
			break;
		}
	}
}

bool is_headphone(struct list_head *fp_list)
{
	struct audioformat *fp = NULL;
	bool is_hp = true;

	list_for_each_entry(fp, fp_list, list) {
		if (fp->endpoint & USB_DIR_IN)
			is_hp = false;
	}

	return is_hp;
}

static void print_audio_format(const struct audioformat *fp)
{
	unsigned int i;

	AUDIO_LOGE("begin");
	AUDIO_LOGE("formats %llu channels %u fmt_type %u frame_size %u iface %d altsetting %u altset_idx %u attributes %u endpoint %u ep_attr %u datainterval %u protocol %u maxpacksize %u clock %u",
		fp->formats,
		fp->channels,
		fp->fmt_type,
		fp->frame_size,
		fp->iface,
		fp->altsetting,
		fp->altset_idx,
		fp->attributes,
		fp->endpoint,
		fp->ep_attr,
		fp->datainterval,
		fp->protocol,
		fp->maxpacksize,
		fp->clock);

	for (i = 0; i < fp->nr_rates; i++)
		AUDIO_LOGE("rate[%u]: %u", i, fp->rate_table[i]);

	AUDIO_LOGE("end");
}

static void set_pcms_para(struct usbaudio_pcms *pcms, struct audioformat *fp, u32 usb_id,
	const struct usbaudio_pcm_cfg *pcm_cfg, const struct usb_interface *iface)
{
	struct usb_host_interface *alts = NULL;
	struct usb_interface_descriptor *altsd = NULL;
	struct usb_endpoint_descriptor *epd = NULL;
	int dir = 0;

	alts = &iface->altsetting[fp->altset_idx];
	altsd = get_iface_desc(alts);
	epd = get_endpoint(alts, 0);

	if (fp->endpoint & USB_DIR_IN)
		dir = SNDRV_PCM_STREAM_CAPTURE;
	else
		dir = SNDRV_PCM_STREAM_PLAYBACK;

	hifi_samplerate_table_filled(fp, &pcms->fmts[dir]);

	pcms->fmts[dir].formats = pcm_fomat_convert(fp->formats);
	pcms->fmts[dir].channels = fp->channels;
	pcms->fmts[dir].fmt_type = fp->fmt_type;
	pcms->fmts[dir].frame_size = fp->frame_size;
	pcms->fmts[dir].iface = fp->iface;
	pcms->fmts[dir].altsetting = fp->altsetting;
	pcms->fmts[dir].altset_idx = fp->altset_idx;
	pcms->fmts[dir].attributes = fp->attributes;
	pcms->fmts[dir].endpoint = fp->endpoint;
	pcms->fmts[dir].ep_attr = fp->ep_attr;
	pcms->fmts[dir].data_interval = fp->datainterval;
	pcms->fmts[dir].protocol = fp->protocol;
	pcms->fmts[dir].max_pack_size = fp->maxpacksize;
	pcms->fmts[dir].rates = pcm_cfg[dir].rates;
	pcms->fmts[dir].clock = fp->clock;

	if (is_customized_headset(usb_id))
		pcms->customsized = true;

	pcms->ifdesc[dir].length = altsd->bLength;
	pcms->ifdesc[dir].descriptor_type = altsd->bDescriptorType;
	pcms->ifdesc[dir].interface_number = altsd->bInterfaceNumber;
	pcms->ifdesc[dir].alternate_setting = altsd->bAlternateSetting;
	pcms->ifdesc[dir].num_endpoints = altsd->bNumEndpoints;
	pcms->ifdesc[dir].interface_class = altsd->bInterfaceClass;
	pcms->ifdesc[dir].interface_sub_class = altsd->bInterfaceSubClass;
	pcms->ifdesc[dir].interface_protocol = altsd->bInterfaceProtocol;
	pcms->ifdesc[dir].interface = altsd->iInterface;

	pcms->epdesc[dir].length = epd->bLength;
	pcms->epdesc[dir].descriptor_type = epd->bDescriptorType;
	pcms->epdesc[dir].endpoint_address = epd->bEndpointAddress;
	pcms->epdesc[dir].attributes = epd->bmAttributes;
	pcms->epdesc[dir].max_packet_size = epd->wMaxPacketSize;
	pcms->epdesc[dir].interval = epd->bInterval;
	pcms->epdesc[dir].refresh = epd->bRefresh;
	pcms->epdesc[dir].synch_address = epd->bSynchAddress;
}

bool match_hifi_format(struct usb_device *dev, struct list_head *fp_list,
	struct usbaudio_pcms *pcms, u32 usb_id)
{
	struct audioformat *fp = NULL;
	struct audioformat *n = NULL;
	struct usb_interface *iface = NULL;
	struct usbaudio_pcm_cfg pcm_cfg[2];
	bool playback_match = false;
	bool capture_match = is_headphone(fp_list);

	if (!pcms) {
		AUDIO_LOGE("params error, pcms is null");
		return false;
	}

	memset(pcms, 0, sizeof(*pcms)); /* unsafe_function_ignore: memset */

	list_for_each_entry(fp, fp_list, list) {
		print_audio_format(fp);

		if (hifi_usbaudio_protocal_compare(dev, fp, usb_id) &&
			hifi_samplerate_compare(fp, pcm_cfg) &&
			hifi_formats_compare(fp, pcm_cfg) &&
			hifi_channel_compare(fp, pcm_cfg)) {
			if (fp->endpoint & USB_DIR_IN)
				capture_match = true;
			else
				playback_match = true;

			iface = usb_ifnum_to_if(dev, fp->iface);
			if (WARN_ON(!iface))
				return false;

			set_pcms_para(pcms, fp, usb_id, pcm_cfg, iface);
		}
	}

	list_for_each_entry_safe(fp, n, fp_list, list) {
		kfree(fp->rate_table);
		kfree(fp->chmap);
		kfree(fp);
	}

	return (playback_match && capture_match);
}

bool is_match_hifi_format(struct usb_device *dev, u32 usb_id,
	struct usb_host_interface *ctrl_intf, int ctrlif, struct usbaudio_pcms *pcms)
{
	struct snd_usb_audio chip;
	struct list_head fp_list;
	int ret;

	chip.dev = dev;
	chip.ctrl_intf = ctrl_intf;
	chip.usb_id = usb_id;

	INIT_LIST_HEAD(&fp_list);

	ret = snd_usb_create_streams(&chip, ctrlif, &fp_list);
	if (ret != 0)
		return false;

	return match_hifi_format(dev, &fp_list, pcms, usb_id);
}

bool is_usbaudio_device(struct usb_device *udev)
{
	struct usb_host_config *config = NULL;
	int hid_intf_num = 0;
	int audio_intf_num = 0;
	int other_intf_num = 0;
	int nintf;
	int i;

	/* root hub */
	if (udev->parent == NULL)
		return false;

	/* not connect to roothub */
	if (udev->parent->parent != NULL)
		return false;

	config = udev->actconfig;
	if (!config) {
		AUDIO_LOGE("config is null");
		return false;
	}

	nintf = config->desc.bNumInterfaces;
	if ((nintf < 0) || (nintf > USB_MAXINTERFACES)) {
		AUDIO_LOGE("nintf invalid %d", nintf);
		return false;
	}

	for (i = 0; i < nintf; ++i) {
		struct usb_interface_cache *intfc = NULL;
		struct usb_host_interface *alt = NULL;

		intfc = config->intf_cache[i];
		alt = &intfc->altsetting[0];

		if (alt->desc.bInterfaceClass == USB_CLASS_AUDIO) {
			if (alt->desc.bInterfaceSubClass == USB_SUBCLASS_AUDIOCONTROL)
				audio_intf_num++;
		} else if (alt->desc.bInterfaceClass == USB_CLASS_HID) {
			hid_intf_num++;
		} else {
			other_intf_num++;
		}
	}

	AUDIO_LOGI("audio intf num %d, hid intf num %d, other intf num %d",
		audio_intf_num, hid_intf_num, other_intf_num);

	if ((audio_intf_num == 1) && (hid_intf_num <= 1) && (other_intf_num == 0)) {
		AUDIO_LOGI("to usb start hifi usb");
		return true;
	}

	return false;
}

bool is_forced_match_arm_usbid(u32 usb_id)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(g_forced_match_arm_usb_ids); i++) {
		if (usb_id == g_forced_match_arm_usb_ids[i]) {
			AUDIO_LOGI("device's usb id %u", usb_id);
			return true;
		}
	}

	return false;
}

static void set_customsized_headset_volume(struct usb_device *dev, u32 usb_id)
{
	unsigned int i;
	int ret;
	char result = 0;

	for (i = 0; i < ARRAY_SIZE(g_customsized_usb_ids); i++) {
		if (usb_id != g_customsized_usb_ids[i])
			continue;

		AUDIO_LOGI("audio headset %u", usb_id);
		ret = usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
			USB_REQ_SET_VOLUME,
			USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
			0, 0, &result, 1, TIMEOUT_MS);
		if (ret > 0) {
			AUDIO_LOGI("result 0x%02x", result);
			if (result == 1)
				AUDIO_LOGI("volume set success");
			else
				AUDIO_LOGI("do not set volume");
		} else {
			AUDIO_LOGE("control msg send failed, ret: %d", ret);
		}
		break;
	}
}

static bool get_suport_usbvoice(void)
{
	struct device_node *node = NULL;
	const char *value = NULL;

	node = of_find_node_by_path("/audio_hw_config");
	if ((node) && (of_property_read_string(node, "usbvoice_suport_switch", &value) == 0)) {
		AUDIO_LOGI("%s: usbvoice_suport_switch: %s", __func__, value);
		return (strncmp(value, "true", strlen(value)) == 0);
	}
	return false;
}

bool controller_switch(struct usb_device *dev, u32 usb_id,
	struct usb_host_interface *ctrl_intf, int ctrlif, struct usbaudio_pcms *pcms)
{
	int ret = 0;
	bool suport_usbvoice_status = false;

	if (get_suport_usbvoice() && uem_check_online_status())
		suport_usbvoice_status = true;

	AUDIO_LOGI("suport_usbvoice_status = %d\n", suport_usbvoice_status);
	if (is_usbaudio_device(dev) && !is_forced_match_arm_usbid(usb_id) &&
		is_match_hifi_format(dev, usb_id, ctrl_intf, ctrlif, pcms) &&
		!suport_usbvoice_status) {
		if (usb_using_hifi_usb(dev)) {
			/* set volume for audio headset */
			set_customsized_headset_volume(dev, usb_id);
			AUDIO_LOGI("already using hifiusb");
			return false;
		}

		ret = usbaudio_nv_check();
		if (ret != 0)
			return true;

		if (need_reset_vbus(usb_id)) {
			if (!usb_start_hifi_usb_reset_power())
				return true;
		} else if (!usb_start_hifi_usb()) {
			return true;
		} else {
			AUDIO_LOGE("start hifi usb fail");
		}

		return false;
	}

	if (usb_using_hifi_usb(dev)) {
		AUDIO_LOGI("using hifi usb, switch to arm usb");
		usb_stop_hifi_usb();
		return true;
	}

	return false;
}

bool send_usbaudioinfo2hifi(struct snd_usb_audio *chip, struct usbaudio_pcms *pcms)
{
	int i = 0;

	if (usb_using_hifi_usb(chip->dev)) {
		if (usbaudio_probe_msg(pcms) != 0) {
			AUDIO_LOGE("send 2 hifi fail");
			return false;
		}

		for (i = 0; i < USBAUDIO_PCM_NUM; i++) {
			AUDIO_LOGE("begin");
			AUDIO_LOGE("formats %llx channels %u fmt_type %u frame_size %u iface %d altsetting %u altset_idx %u attributes %u endpoint %u ep_attr %u datainterval %u protocol %u maxpacksize %u rates %u clock %u",
			pcms->fmts[i].formats,
			pcms->fmts[i].channels,
			pcms->fmts[i].fmt_type,
			pcms->fmts[i].frame_size,
			pcms->fmts[i].iface,
			pcms->fmts[i].altsetting,
			pcms->fmts[i].altset_idx,
			pcms->fmts[i].attributes,
			pcms->fmts[i].endpoint,
			pcms->fmts[i].ep_attr,
			pcms->fmts[i].data_interval,
			pcms->fmts[i].protocol,
			pcms->fmts[i].max_pack_size,
			pcms->fmts[i].rates,
			pcms->fmts[i].clock);

			AUDIO_LOGE("bLength %u bDescriptorType %u bInterfaceNumber %u bAlternateSetting %u bNumEndpoints %u bInterfaceClass  %u bInterfaceSubClass  %u bInterfaceProtocol %u iInterface %u",
			pcms->ifdesc[i].length,
			pcms->ifdesc[i].descriptor_type,
			pcms->ifdesc[i].interface_number,
			pcms->ifdesc[i].alternate_setting,
			pcms->ifdesc[i].num_endpoints,
			pcms->ifdesc[i].interface_class,
			pcms->ifdesc[i].interface_sub_class,
			pcms->ifdesc[i].interface_protocol,
			pcms->ifdesc[i].interface);

			AUDIO_LOGE("bLength %u bDescriptorType %u bEndpointAddress %u bmAttributes %u wMaxPacketSize %u bInterval %u bRefresh %u bSynchAddress %u",
			pcms->epdesc[i].length,
			pcms->epdesc[i].descriptor_type,
			pcms->epdesc[i].endpoint_address,
			pcms->epdesc[i].attributes,
			pcms->epdesc[i].max_packet_size,
			pcms->epdesc[i].interval,
			pcms->epdesc[i].refresh,
			pcms->epdesc[i].synch_address);
			AUDIO_LOGE("end");
		}
	}

	return true;
}

void usbaudio_nv_check_complete(unsigned int usb_id)
{
	if (need_reset_vbus(usb_id)) {
		AUDIO_LOGI("start hifi usb reset power");
		if (usb_start_hifi_usb_reset_power() != 0)
			AUDIO_LOGE("start hifi usb reset power err");
	} else {
		AUDIO_LOGI("do not start hifi usb reset power");
		if (usb_start_hifi_usb() != 0)
			AUDIO_LOGE("start hifi usb err");
	}
}

static int usbaudio_set_interface(int direction, struct snd_usb_audio *chip,
	struct usbaudio_pcms *pcms, unsigned int running, unsigned int rate)
{
	int ret;
	struct usb_host_interface *alts = NULL;
	struct usb_interface *iface = NULL;
	struct usbaudio_formats *fmt = NULL;
	struct audioformat cur_audiofmt;

	fmt = &pcms->fmts[direction];
	AUDIO_LOGI("direction %d running %u iface %d altsetting %u",
		direction, running, fmt->iface, fmt->altsetting);

	if (running == START_STREAM) {
		AUDIO_LOGI("call usb API to check usb power status");
		ret = check_hifi_usb_status(HIFI_USB_AUDIO);
		if (ret != 0)
			AUDIO_LOGE("call check_hifi_usb_status return fail %d", ret);
	}

	ret = usb_set_interface(chip->dev, fmt->iface, 0);
	if (ret < 0) {
		AUDIO_LOGE("%d: %d: usb_set_interface failed %d", fmt->iface, 0, ret);
		if (ret == -ETIMEDOUT)
			usb_stop_hifi_usb_reset_power();
		return ret;
	}

	if (running == START_STREAM) {
		ret = usb_set_interface(chip->dev, fmt->iface, fmt->altsetting);
		if (ret < 0) {
			AUDIO_LOGE("%d: %u: usb_set_interface failed %d",
				fmt->iface, fmt->altsetting, ret);
			if (ret == -ETIMEDOUT)
				usb_stop_hifi_usb_reset_power();
			return ret;
		}

		cur_audiofmt.attributes = fmt->attributes;
		cur_audiofmt.altsetting = fmt->altsetting;
		cur_audiofmt.protocol = fmt->protocol;
		cur_audiofmt.clock = fmt->clock;
		iface = usb_ifnum_to_if(chip->dev, fmt->iface);
		if (!iface) {
			AUDIO_LOGE("usb_set_interface failed, iface: %d", fmt->iface);
			return -EINVAL;
		}

		alts = &iface->altsetting[fmt->altset_idx];
		ret = snd_usb_init_sample_rate(chip, fmt->iface, alts, &cur_audiofmt, rate);
		AUDIO_LOGI("rate: %u", rate);
		if (ret < 0) {
			AUDIO_LOGE("%d: %u: init_sample_rate failed %d",
				fmt->iface, fmt->rates, ret);
			return ret;
		}
	}

	return ret;
}

int set_pipeout_interface(struct snd_usb_audio *chip,
	struct usbaudio_pcms *pcms, unsigned int running, unsigned int rate)
{
	return usbaudio_set_interface(SNDRV_PCM_STREAM_PLAYBACK, chip, pcms, running, rate);
}

int set_pipein_interface(struct snd_usb_audio *chip,
	struct usbaudio_pcms *pcms, unsigned int running, unsigned int rate)
{
	return usbaudio_set_interface(SNDRV_PCM_STREAM_CAPTURE, chip, pcms, running, rate);
}

void setinterface_complete(unsigned int dir, unsigned int running, int ret, unsigned int rate)
{
	usbaudio_setinterface_complete_msg(dir, running, ret, rate);
}

int usb_power_resume(void)
{
	int ret;

	AUDIO_LOGI("call usb API to check usb power status");
	ret = check_hifi_usb_status(HIFI_USB_AUDIO);
	if (ret != 0)
		AUDIO_LOGE("call check hifi usb status return fail %d", ret);

	return ret;
}

bool controller_query(struct snd_usb_audio *chip)
{
	return usb_using_hifi_usb(chip->dev);
}

int disconnect(struct snd_usb_audio *chip, unsigned int dsp_reset_flag)
{
	if (usb_using_hifi_usb(chip->dev))
		return usbaudio_disconnect_msg(dsp_reset_flag);
	else
		return 0;
}
