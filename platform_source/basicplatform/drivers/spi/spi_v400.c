/*
 * Synopsys DesignWare SPI adapter driver.
 *
 * Copyright (C) Huawei Technologies Co., Ltd. 2016-2019. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <securec.h>

#ifdef CONFIG_FMEA_FAULT_INJECTION
#include <debug_fault_injection.h>
#include <platform_include/basicplatform/linux/mfd/pmic_platform.h>
#include <pmic_interface.h>
#endif

#define GET_HARDWARE_TIMEOUT 10000
#define SPI_4G_PHYS_ADDR 0xFFFFFFFF

#define ssp_txfifocr(r) ((r) + 0x028)
#define ssp_rxfifocr(r) ((r) + 0x02C)

#define SSP_TXFIFOCR_MASK_DMA (0x07UL << 0)
#define SSP_TXFIFOCR_MASK_INT (0x3UL << 3)

/* SSP RX FIFO Register - SSP_TXFIFOCR */
#define SSP_RXFIFOCR_MASK_DMA (0x07UL << 0)
#define SSP_RXFIFOCR_MASK_INT (0x3UL << 3)

#define PERI_CTRL0_OFFSET 0x04
#define SPI_GATE_NUM 5

#define DEFAULT_SSP_REG_TXFIFOCR                                               \
	(GEN_MASK_BITS(                                                        \
		SSP_TX_64_OR_MORE_EMPTY_LOC, SSP_TXFIFOCR_MASK_DMA, 0) |       \
		GEN_MASK_BITS(SSP_TX_64_OR_MORE_EMPTY_LOC,                     \
		SSP_TXFIFOCR_MASK_INT, 3))

/* Default SSP Register Values */
#define DEFAULT_SSP_REG_RXFIFOCR                                               \
	(GEN_MASK_BITS(SSP_RX_16_OR_MORE_ELEM, SSP_RXFIFOCR_MASK_DMA, 0) |     \
		GEN_MASK_BITS(                                                 \
			SSP_RX_16_OR_MORE_ELEM, SSP_RXFIFOCR_MASK_INT, 3))

#define ENUM_SPI_HWSPIN_LOCK 27

static void pl022_cs_control(struct pl022 *pl022, u32 command);
static void *next_transfer(struct pl022 *pl022);
static void setup_dma_scatter(struct pl022 *pl022,
	void *buffer, unsigned int length, struct sg_table *sgtab);

#if defined(CONFIG_SPI_V400_CS_USE_PCTRL)
static inline bool pctrl_cs_is_valid(int cs)
{
	return (cs != ~0);
}

static void pl022_pctrl_cs_set(struct pl022 *pl022, u32 command)
{
	/* cmd bit0~3 mask bit16~19 cs0~3 */
	if (command == SSP_CHIP_SELECT)
		writel(pl022->cur_cs, pl022->pctrl_base + PERI_CTRL0_OFFSET);
	else
		writel(pl022->cur_cs & 0xFFFF0000,
			pl022->pctrl_base + PERI_CTRL0_OFFSET);
}
#endif

static int pl022_prepare_transfer_hardware(struct spi_master *master)
{
	struct pl022 *pl022 = spi_master_get_devdata(master);
	struct hwspinlock *hwlock = pl022->spi_hwspin_lock;
	unsigned long time, timeout;

	volatile int ret;

	/*
	 * Just make sure we have all we need to run the transfer by syncing
	 * with the runtime PM framework.
	 */
	timeout = jiffies + msecs_to_jiffies(GET_HARDWARE_TIMEOUT);
	if (!pl022->hardware_mutex) {
		dev_dbg(&pl022->adev->dev,
			"%s hardware_mutex is null\n", __func__);
		return 0;
	}

	do {
		ret = hwlock->bank->ops->trylock(hwlock);
		if (ret)
			break;

		time = jiffies;
		if (time_after(time, timeout)) {
			dev_err(&pl022->adev->dev,
				"Get hardware_mutex complete timeout\n");
			return -ETIME;
		}
		/* sleep 1000~2000 us */
		usleep_range(1000, 2000);
	} while (!ret);

	return 0;
}

static struct {
	struct mutex lock;
	int users;
} spi_secos_gates[SPI_GATE_NUM] = {
	[0] = {
		.lock = __MUTEX_INITIALIZER(spi_secos_gates[0].lock),
	},
	[1] = {
		.lock = __MUTEX_INITIALIZER(spi_secos_gates[1].lock),
	},
	[2] = {
		.lock = __MUTEX_INITIALIZER(spi_secos_gates[2].lock),
	},
	[3] = {
		.lock = __MUTEX_INITIALIZER(spi_secos_gates[3].lock),
	},
	[4] = {
		.lock = __MUTEX_INITIALIZER(spi_secos_gates[4].lock),
	},
};

int spi_init_secos(unsigned int spi_bus_id)
{
	int ret = 0;
	struct spi_master *master = NULL;
	struct pl022 *pl022 = NULL;

	if (spi_bus_id >= ARRAY_SIZE(spi_smc_flag) ||
		spi_bus_id >= ARRAY_SIZE(spi_secos_gates)) {
		pr_err("%s:spi_bus_id is illegal, %u:%lu:%lu\n",
			__func__, spi_bus_id, ARRAY_SIZE(spi_smc_flag),
			ARRAY_SIZE(spi_secos_gates));
		return -EINVAL;
	}

	if (spi_smc_flag[spi_bus_id]) {
		master = spi_busnum_to_master(spi_bus_id);
		if (!master) {
			pr_err("secos get spi master:%u wrong\n", spi_bus_id);
			return -EINVAL;
		}

		pl022 = spi_master_get_devdata(master);
		if (!pl022) {
			pr_err("secos pl022:%u is NULL!\n", spi_bus_id);
			return -EINVAL;
		}

		if (!pl022->clk) {
			dev_err(&pl022->adev->dev,
				"secos pl022->clk is NULL for spi%u!\n",
				spi_bus_id);
			return -EINVAL;
		}

		mutex_lock(&spi_secos_gates[spi_bus_id].lock);
		if (spi_secos_gates[spi_bus_id].users) {
			++spi_secos_gates[spi_bus_id].users;
			mutex_unlock(&spi_secos_gates[spi_bus_id].lock);
			return 0;
		}

		ret = clk_prepare_enable(pl022->clk);
		if (ret) {
			dev_err(&pl022->adev->dev,
				"secos can not enable SPI%u bus clock\n",
				spi_bus_id);
			ret = -EINVAL;
		} else {
			spi_secos_gates[spi_bus_id].users = 1;
		}
		mutex_unlock(&spi_secos_gates[spi_bus_id].lock);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(spi_init_secos);

int spi_exit_secos(unsigned int spi_bus_id)
{
	struct spi_master *master = NULL;
	struct pl022 *pl022 = NULL;

	if (spi_bus_id >= ARRAY_SIZE(spi_smc_flag) ||
		spi_bus_id >= ARRAY_SIZE(spi_secos_gates)) {
		pr_err("%s:spi_bus_id is illegal, %u:%lu:%lu\n",
			__func__, spi_bus_id, ARRAY_SIZE(spi_smc_flag),
			ARRAY_SIZE(spi_secos_gates));
		return -EINVAL;
	}

	if (spi_smc_flag[spi_bus_id]) {
		master = spi_busnum_to_master(spi_bus_id);
		if (!master) {
			pr_err("kernel get spi master:%u wrong\n", spi_bus_id);
			return -EINVAL;
		}

		pl022 = spi_master_get_devdata(master);
		if (!pl022) {
			pr_err("kernel pl022:%u is NULL!\n", spi_bus_id);
			return -EINVAL;
		}

		if (!pl022->clk) {
			dev_err(&pl022->adev->dev,
				"kernel pl022->clk is NULL for spi:%u\n",
				spi_bus_id);
			return -EINVAL;
		}

		mutex_lock(&spi_secos_gates[spi_bus_id].lock);
		if (--spi_secos_gates[spi_bus_id].users) {
			mutex_unlock(&spi_secos_gates[spi_bus_id].lock);
			return 0;
		}
		clk_disable_unprepare(pl022->clk);
		mutex_unlock(&spi_secos_gates[spi_bus_id].lock);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(spi_exit_secos);

#ifdef CONFIG_FMEA_FAULT_INJECTION
static int stub_hs_dw_spi_injection(void)
{
	if (pmic_read_reg(FAULT_INJECT_REG) == KERNEL_FAULT_SPI) {
		pr_err("%s start spi init fail fault injection.\n", __func__);
		return -ENODEV;
	}

	return 0;
}
#endif

static int spi_v400_get_pins_data(struct pl022 *pl022, struct device *dev)
{
	int status;

	pl022->pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(pl022->pinctrl)) {
		dev_err(dev, "%s pinctrl is error\n", __func__);
		return -EFAULT;
	}

	pl022->pins_default =
		pinctrl_lookup_state(pl022->pinctrl, PINCTRL_STATE_DEFAULT);
	/* enable pins to be muxed in and configured */
	if (!IS_ERR(pl022->pins_default)) {
		status = pinctrl_select_state(
			pl022->pinctrl, pl022->pins_default);
		if (status)
			dev_err(dev, "could not set default pins\n");
	} else {
		dev_err(dev, "could not get default pinstate\n");
	}

	pl022->pins_idle =
		pinctrl_lookup_state(pl022->pinctrl, PINCTRL_STATE_IDLE);
	if (IS_ERR(pl022->pins_idle))
		dev_dbg(dev, "could not get idle pinstate\n");

	pl022->pins_sleep =
		pinctrl_lookup_state(pl022->pinctrl, PINCTRL_STATE_SLEEP);
	if (IS_ERR(pl022->pins_sleep))
		dev_dbg(dev, "could not get sleep pinstate\n");

	pl022->pins.p = pl022->pinctrl;
	pl022->pins.default_state = pl022->pins_default;
#ifdef CONFIG_PM
	pl022->pins.idle_state = pl022->pins_idle;
	pl022->pins.sleep_state = pl022->pins_sleep;
#endif
	dev->pins = &pl022->pins;

	return 0;
}

static void spi_v400_txrx_buffer_check(
	struct pl022 *pl022, struct spi_transfer *transfer)
{
	errno_t ret;

	if (virt_to_phys(pl022->cur_transfer->tx_buf) > SPI_4G_PHYS_ADDR) {
		/*
		 * wrining! must be use dma buffer, and need the flag "GFP_DMA"
		 * when alloc memery
		 */
		WARN_ON(1);
		pl022->tx_buffer =
			kzalloc(pl022->cur_transfer->len, GFP_KERNEL | GFP_DMA);
		if (pl022->tx_buffer) {
			ret = memcpy_s(pl022->tx_buffer,
				pl022->cur_transfer->len,
				transfer->tx_buf,
				pl022->cur_transfer->len);
			if (ret != EOK) {
				dev_err(&pl022->adev->dev,
					"%s:memcpy_s failed\n", __func__);
				return;
			}

			pl022->tx = (void *)pl022->tx_buffer;
			dev_err(&pl022->adev->dev, "tx is not dma-buffer\n");
		} else {
			pl022->tx = (void *)transfer->tx_buf;
			dev_err(&pl022->adev->dev,
				"can not alloc dma-buffer for tx\n");
		}
	} else {
		pl022->tx = (void *)transfer->tx_buf;
	}
	pl022->tx_end = pl022->tx + pl022->cur_transfer->len;

	if (virt_to_phys(pl022->cur_transfer->rx_buf) > SPI_4G_PHYS_ADDR) {
		/*
		 * wrining! must be use dma buffer, and need the flag "GFP_DMA"
		 * when alloc memery
		 */
		WARN_ON(1);
		pl022->rx_buffer =
			kzalloc(pl022->cur_transfer->len, GFP_KERNEL | GFP_DMA);
		if (pl022->rx_buffer) {
			pl022->rx = (void *)pl022->rx_buffer;
			dev_err(&pl022->adev->dev, "rx is not dma-buffer\n");
		} else {
			pl022->rx = (void *)transfer->rx_buf;
			dev_err(&pl022->adev->dev,
				"can not alloc dma-buffer for rx\n");
		}
	} else {
		pl022->rx = (void *)transfer->rx_buf;
	}
	pl022->rx_end = pl022->rx + pl022->cur_transfer->len;
}

static void spi_v400_dma_buffer_free(struct pl022 *pl022)
{
	errno_t ret;

	if (pl022->rx_buffer) {
		ret = memcpy_s(pl022->cur_transfer->rx_buf,
			pl022->cur_transfer->len,
			pl022->rx,
			pl022->cur_transfer->len);
		if (ret != EOK) {
			dev_err(&pl022->adev->dev, "memcpy_s failed\n");
			return;
		}

		kfree(pl022->rx_buffer);
		pl022->rx_buffer = NULL;
	}

	kfree(pl022->tx_buffer);
	pl022->tx_buffer = NULL;
}

static void unmap_free_dma_tx_scatter(struct pl022 *pl022)
{
	/* Unmap and free the SG tables */
	dma_unmap_sg(pl022->dma_tx_channel->device->dev, pl022->sgt_tx.sgl,
		pl022->sgt_tx.nents, DMA_TO_DEVICE);
	sg_free_table(&pl022->sgt_tx);
}

static void dma_tx_callback(void *data)
{
	struct pl022 *pl022 = data;
	struct spi_message *msg = pl022->cur_msg;

	spi_v400_dma_buffer_free(pl022);
	unmap_free_dma_tx_scatter(pl022);

	/* Update total bytes transferred */
	msg->actual_length += pl022->cur_transfer->len;
	/* Move to next transfer */
	msg->state = next_transfer(pl022);

	if (pl022->cur_transfer->cs_change)
		pl022_cs_control(pl022, SSP_CHIP_DESELECT);

	tasklet_schedule(&pl022->pump_transfers);
}

static int spi_v400_configure_dma_tx(struct pl022 *pl022)
{
	struct dma_slave_config tx_conf = {
		.dst_addr = SSP_DR(pl022->phybase),
		.direction = DMA_MEM_TO_DEV,
		.device_fc = false,
	};

	unsigned int pages, txpages;
	int ret;
	int tx_sglen;
	struct dma_chan *txchan = pl022->dma_tx_channel;
	struct dma_async_tx_descriptor *txdesc;

	/* Check that the channels are available */
	if (!txchan)
		return -ENODEV;

	switch (pl022->tx_lev_trig) {
	case SSP_TX_1_OR_MORE_EMPTY_LOC:
		tx_conf.dst_maxburst = 1;
		break;
	case SSP_TX_4_OR_MORE_EMPTY_LOC:
		tx_conf.dst_maxburst = 4;
		break;
	case SSP_TX_8_OR_MORE_EMPTY_LOC:
		tx_conf.dst_maxburst = 8;
		break;
	case SSP_TX_16_OR_MORE_EMPTY_LOC:
		tx_conf.dst_maxburst = 16;
		break;
	case SSP_TX_32_OR_MORE_EMPTY_LOC:
		tx_conf.dst_maxburst = 32;
		break;
	default:
		tx_conf.dst_maxburst = pl022->vendor->fifodepth >> 1;
		break;
	}

	switch (pl022->write) {
	case WRITING_NULL:
		/* Use the same as for reading */
		tx_conf.dst_addr_width = DMA_SLAVE_BUSWIDTH_UNDEFINED;
		break;
	case WRITING_U8:
		tx_conf.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
		break;
	case WRITING_U16:
		tx_conf.dst_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
		break;
	case WRITING_U32:
		tx_conf.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		break;
	}

	dmaengine_slave_config(txchan, &tx_conf);

	/* Create sglists for the transfers */
	pages = DIV_ROUND_UP(pl022->cur_transfer->len, PAGE_SIZE);
	dev_dbg(&pl022->adev->dev, "using %d pages for transfer\n", pages);

	txpages = pages;
	if (pl022->cur_transfer->len >
		(pages * PAGE_SIZE -
			offset_in_page((unsigned long)(uintptr_t)pl022->tx)))
		txpages++;

	ret = sg_alloc_table(&pl022->sgt_tx, txpages, GFP_ATOMIC);
	if (ret)
		goto err_alloc_tx_sg;

	setup_dma_scatter(pl022, pl022->tx,
			  pl022->cur_transfer->len, &pl022->sgt_tx);

	/* Map DMA buffers */
	tx_sglen = dma_map_sg(txchan->device->dev, pl022->sgt_tx.sgl,
			   pl022->sgt_tx.nents, DMA_TO_DEVICE);
	if (!tx_sglen)
		goto err_tx_sgmap;

	txdesc = dmaengine_prep_slave_sg(txchan,
		pl022->sgt_tx.sgl,
		tx_sglen,
		DMA_MEM_TO_DEV,
		DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	if (!txdesc)
		goto err_txdesc;

	txdesc->callback = dma_tx_callback;
	txdesc->callback_param = pl022;

	dmaengine_submit(txdesc);
	dma_async_issue_pending(txchan);
	pl022->dma_running = true;

	return 0;

err_txdesc:
	dmaengine_terminate_all(txchan);
	dma_unmap_sg(txchan->device->dev, pl022->sgt_tx.sgl,
		pl022->sgt_tx.nents, DMA_TO_DEVICE);
err_tx_sgmap:
	sg_free_table(&pl022->sgt_tx);
err_alloc_tx_sg:
	return -ENOMEM;
}
