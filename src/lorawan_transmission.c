#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/lorawan/lorawan.h>

#include "messages.h"

#define LORAWAN_DEV_EUI			{ 0x8A, 0x39, 0x32, 0x35, 0x59, 0x37,\
					  0x91, 0x95 }
#define LORAWAN_JOIN_EUI		{ 0xAA, 0xBB, 0xCC, 0xDD, 0x00, 0x11,\
					  0x22, 0x33 }
#define LORAWAN_APP_KEY			{ 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE,\
					  0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88,\
					  0x09, 0xCF, 0x4F, 0x3E }

LOG_MODULE_REGISTER(lorawan_module, LOG_LEVEL_INF);

ZBUS_CHAN_DECLARE(payload_chan);

ZBUS_SUBSCRIBER_DEFINE(lora_sub, 8);

static void dl_callback(uint8_t port, uint8_t flags, int16_t rssi, int8_t snr, uint8_t len,
			const uint8_t *hex_data)
{
	LOG_INF("Port %d, Pending %d, RSSI %ddB, SNR %ddBm, Time %d", port,
		flags & LORAWAN_DATA_PENDING, rssi, snr, !!(flags & LORAWAN_TIME_UPDATED));
	if (hex_data) {
		LOG_HEXDUMP_INF(hex_data, len, "Payload: ");
	}
}

static void lorwan_datarate_changed(enum lorawan_datarate dr)
{
	uint8_t unused, max_size;

	lorawan_get_payload_sizes(&unused, &max_size);
	LOG_INF("New Datarate: DR_%d, Max Payload %d", dr, max_size);
}

void lorawan_thread (void) {
    const struct device *lora_dev;
	struct lorawan_join_config join_cfg;
	uint8_t dev_eui[] = LORAWAN_DEV_EUI;
	uint8_t join_eui[] = LORAWAN_JOIN_EUI;
	uint8_t app_key[] = LORAWAN_APP_KEY;
	int ret;

	struct lorawan_downlink_cb downlink_cb = {
		.port = LW_RECV_PORT_ANY,
		.cb = dl_callback
	};
    // ret = lorawan_set_region(LORAWAN_REGION_EU868);
	// if (ret < 0) {
	// 	LOG_ERR("lorawan_set_region failed: %d", ret);
	// 	return 0;
	// } else {
	// 	LOG_INF("Lorawan region set to LORAWAN_REGION_EU868");
	// }

    // ret = lorawan_start();
	// if (ret < 0) {
	// 	LOG_ERR("lorawan_start failed: %d", ret);
	// }

	// LOG_INF("Setting Class A device...");
	// if (lorawan_set_class(LORAWAN_CLASS_A) == 0) {
	// 	LOG_INF("Set class succesfull");
	// } else {
	// 	LOG_ERR("Fail in setting class a");
	// }

	lorawan_register_downlink_callback(&downlink_cb);
	lorawan_register_dr_changed_callback(lorwan_datarate_changed);
	
	join_cfg.mode = LORAWAN_ACT_OTAA;
	join_cfg.dev_eui = dev_eui;
	join_cfg.otaa.join_eui = join_eui;
	join_cfg.otaa.app_key = app_key;
	join_cfg.otaa.nwk_key = app_key;

	printf("\r\n DEV_EUI: ");
    for (int i = 0; i < sizeof(dev_eui); ++i) printf("%02x", dev_eui[i]);
    printf("\r\n APP_EUI: ");
    for (int i = 0; i < sizeof(join_eui); ++i) printf("%02x", join_eui[i]);
    printf("\r\n APP_KEY: ");
    for (int i = 0; i < sizeof(app_key); ++i) printf("%02x", app_key[i]);
    printf("\r\n");

    LOG_INF("Trying to join network over OTAA...");
     int err;
    // for (int attempt = 0; attempt < 3; attempt++) {
	//     LOG_INF("Join attempt %d", attempt + 1);
	//     err = lorawan_join(&join_cfg);
	//     if (err == 0) {
	// 	    LOG_INF("Join successful");
	// 	    break;
	//     }
	//     LOG_WRN("Join failed (err=%d), retrying in 5s...", err);
	//     k_sleep(K_SECONDS(5));
    // }

	// if (err < 0 ){
	// 	LOG_ERR("Not possible to join the network over OTAA");
	// }

    const struct zbus_channel *chan;

    while (!zbus_sub_wait(&lora_sub, &chan, K_FOREVER)) {
        struct payload_msg payload;
		zbus_chan_read(chan, &payload, K_MSEC(200));
        LOG_HEXDUMP_INF(&payload, sizeof(payload), "Payload sent: ");
        /* Send Payload Message*/
		// ret = lorawan_send(2, &payload, sizeof(payload), LORAWAN_MSG_UNCONFIRMED);
		// if (ret == -EAGAIN) {
		// 	LOG_ERR("lorawan_send failed: %d. Continuing...", ret);
		// }

		// if (ret < 0) {
		// 	LOG_ERR("lorawan_send failed: %d", ret);
		// }
		// if (ret == 0 ) {
		// 	LOG_INF("Data sent");
		// }
		k_sleep(K_SECONDS(18));
    }
}

K_THREAD_DEFINE(lorawan_thread_id, 2048, lorawan_thread, NULL, NULL, NULL, 4, 0, 0);