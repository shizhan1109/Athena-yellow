/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include "blescan.h"
#include "blecon.h"
#include "hci.h"

#define NAME_LEN 30

LOG_MODULE_REGISTER(observer);

#define IDX_OFFSET 5
#define IDX_MajorL 20 + IDX_OFFSET
#define IDX_MajorH 21 + IDX_OFFSET
#define IDX_MinorL 22 + IDX_OFFSET
#define IDX_MinorH 23 + IDX_OFFSET

static void value_send(int device_id, float16_t value)
{
	struct gcu_display data;

	data.device_id = device_id;
	data.value = value;
	/* send data to consumers */
	while (k_msgq_put(&gcu_display_msgq, &data, K_NO_WAIT) != 0)
	{
		/* message queue is full: purge old data & try again */
		k_msgq_purge(&gcu_display_msgq);
	}
}

void blecon_device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
						 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t *filt_addr = &global_rece_addr;

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	if (!ad)
	{
		LOG_ERR("ad is null");
		return;
	}

	if (bt_addr_eq(&(filt_addr->a), &(addr->a)))
	{
		float16_t value = 0;
		LOG_DBG("Received: %s (RSSI %d), type %u, AD data len %u",
				addr_str, rssi, type, ad->len);
		if (ad->len < 30)
		{
			LOG_ERR("ad->len < 30");
			return;
		}
		memcpy(&value, &(ad->data[IDX_MinorL]), sizeof(float16_t));
		LOG_DBG("dev: %X value: %X %X %.1f", ad->data[IDX_MajorL],
				ad->data[IDX_MinorL], ad->data[IDX_MinorH], (double)value);
		value_send(ad->data[IDX_MajorL], value);
	}
}

int blecon_observer_start(void)
{
	struct bt_le_scan_param scan_param = {
		.type = BT_LE_SCAN_TYPE_PASSIVE,
		.options = BT_LE_SCAN_OPT_NONE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
	};
	int err;

	err = bt_le_scan_start(&scan_param, blecon_device_found);
	if (err)
	{
		LOG_ERR("Start scanning failed (err %d)", err);
		return err;
	}
	LOG_INF("Started scanning...");

	return 0;
}

int blecon_receive_stop(void)
{
	int err;

	err = bt_le_scan_stop();
	if (err)
	{
		LOG_ERR("Stoped scanning failed (err %d)", err);
		return err;
	}
	LOG_INF("Stoped scanning...");
	return 0;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
						 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t *filt_addr = &global_filt_addr;

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	if (global_filt_enable)
	{
		if (bt_addr_eq(&(filt_addr->a), &(addr->a)))
		{
			LOG_INF("Device found(filt): %s (RSSI %d), type %u, AD data len %u",
					addr_str, rssi, type, ad->len);
		}
	}
	else
	{
		LOG_INF("Device found: %s (RSSI %d), type %u, AD data len %u",
				addr_str, rssi, type, ad->len);
	}
}

#if defined(CONFIG_BT_EXT_ADV)
static bool data_cb(struct bt_data *data, void *user_data)
{
	char *name = user_data;
	uint8_t len;

	switch (data->type)
	{
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
		len = MIN(data->data_len, NAME_LEN - 1);
		(void)memcpy(name, data->data, len);
		name[len] = '\0';
		return false;
	default:
		return true;
	}
}

static const char *phy2str(uint8_t phy)
{
	switch (phy)
	{
	case BT_GAP_LE_PHY_NONE:
		return "No packets";
	case BT_GAP_LE_PHY_1M:
		return "LE 1M";
	case BT_GAP_LE_PHY_2M:
		return "LE 2M";
	case BT_GAP_LE_PHY_CODED:
		return "LE Coded";
	default:
		return "Unknown";
	}
}

static void scan_recv(const struct bt_le_scan_recv_info *info,
					  struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char name[NAME_LEN];
	uint8_t data_status;
	uint16_t data_len;

	(void)memset(name, 0, sizeof(name));

	data_len = buf->len;
	bt_data_parse(buf, data_cb, name);

	data_status = BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS(info->adv_props);

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	printk("[DEVICE]: %s, AD evt type %u, Tx Pwr: %i, RSSI %i "
		   "Data status: %u, AD data len: %u Name: %s "
		   "C:%u S:%u D:%u SR:%u E:%u Pri PHY: %s, Sec PHY: %s, "
		   "Interval: 0x%04x (%u ms), SID: %u\n",
		   le_addr, info->adv_type, info->tx_power, info->rssi,
		   data_status, data_len, name,
		   (info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) != 0,
		   (info->adv_props & BT_GAP_ADV_PROP_SCANNABLE) != 0,
		   (info->adv_props & BT_GAP_ADV_PROP_DIRECTED) != 0,
		   (info->adv_props & BT_GAP_ADV_PROP_SCAN_RESPONSE) != 0,
		   (info->adv_props & BT_GAP_ADV_PROP_EXT_ADV) != 0,
		   phy2str(info->primary_phy), phy2str(info->secondary_phy),
		   info->interval, info->interval * 5 / 4, info->sid);
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
};
#endif /* CONFIG_BT_EXT_ADV */

int observer_start(void)
{
	struct bt_le_scan_param scan_param = {
		.type = BT_LE_SCAN_TYPE_PASSIVE,
		.options = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
	};
	int err;

#if defined(CONFIG_BT_EXT_ADV)
	bt_le_scan_cb_register(&scan_callbacks);
	printk("Registered scan callbacks\n");
#endif /* CONFIG_BT_EXT_ADV */

	err = bt_le_scan_start(&scan_param, device_found);
	if (err)
	{
		LOG_ERR("Start scanning failed (err %d)", err);
		return err;
	}
	LOG_INF("Started scanning...");

	return 0;
}

int observer_stop(void)
{
	int err;

	err = bt_le_scan_stop();
	if (err)
	{
		LOG_ERR("Stoped scanning failed (err %d)", err);
		return err;
	}
	LOG_INF("Stoped scanning...");
	return 0;
}