#include "messages.h"

#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(parser_module, LOG_LEVEL_INF);

ZBUS_CHAN_DEFINE(payload_chan,          /* Name */
                 struct payload_msg,              /* Message type */
                 NULL,                 /* Validator */
                 NULL,                 /* User data */
                 ZBUS_OBSERVERS(lora_sub), /* observers */
                 ZBUS_MSG_INIT(0)      /* Initial value 0*/
);

ZBUS_SUBSCRIBER_DEFINE(parser_sub, 4);
ZBUS_OBS_DECLARE(lora_sub);

static void parser_thread(void)
{
	const struct zbus_channel *chan;

	while (!zbus_sub_wait(&parser_sub, &chan, K_FOREVER)) {
		struct sensor_msg msg;

		zbus_chan_read(chan, &msg, K_MSEC(200));
        struct payload_msg payload;
        payload.flags = 0;
        if (msg.sos_status) {
            payload.flags = msg.sos_counter;
            payload.flags |= 0x80;
        }else {
            if (msg.ff_status) {
                payload.flags |= 0x40;
            }
            if (msg.fix_indicator) {
                payload.flags |= 0x20;
                if (msg.latDir == 'S') {
                    payload.flags |= 0x10;
                }
                if (msg.lonDir == 'W') {
                    payload.flags |= 0x08;
                }
            }

            LOG_INF("Fix: %d | FF: %d | SOS: %d",
                     msg.fix_indicator,
                     msg.ff_status,
                     msg.sos_status);
            LOG_INF("Accel Raw: ax=%d ay=%d az=%d", msg.ax, msg.ay, msg.az);
            LOG_INF("Gyro Raw:  gx=%d gy=%d gz=%d", msg.gx, msg.gy, msg.gz);
            LOG_INF("Temp Raw:  %d C", msg.temp);
            LOG_INF("GPS:   %.5f %c, %.5f %c, Alt: %.2f m",
                    msg.latitude, msg.latDir,
                    msg.longitude, msg.lonDir,
                    msg.altitude);
            LOG_INF("BPM Value: %d", msg.pulsiMeterValue);
            payload.pyld_ax = msg.ax;
            payload.pyld_ay = msg.ay;
            payload.pyld_az = msg.az;
            payload.pyld_gx = msg.gx;
            payload.pyld_gy =  msg.gy;
            payload.pyld_gz = msg.gz;
            payload.pyld_temp = msg.temp;
            payload.pyld_pulsimeter = msg.pulsiMeterValue;
            memcpy(&payload.pyld_lat, &msg.latitude, sizeof(float));
            memcpy(&payload.pyld_lon, &msg.longitude, sizeof(float));
            uint32_t alitude_int = msg.altitude*10;
            //uint32_t alitude_int = 12345;
            payload.pyld_alt1 = alitude_int >> 16;
            payload.pyld_alt2 = alitude_int >> 8;
            payload.pyld_alt3 = alitude_int;
        }

        zbus_chan_pub(&payload_chan, &payload, K_MSEC(250));
        
	}
}

K_THREAD_DEFINE(parser_thread_id, 512, parser_thread, NULL, NULL, NULL, 4, 0, 0);

