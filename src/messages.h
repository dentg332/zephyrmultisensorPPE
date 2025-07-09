#ifndef _MESSAGES_H_
#define _MESSAGES_H_
#include <stdint.h>

struct sensor_msg {
	int16_t ax;
	int16_t ay;
	int16_t az;
	int16_t gx;
	int16_t gy;
	int16_t gz;
	int16_t temp;
	float latitude;
	float longitude;
	float altitude;
	char latDir;
	char lonDir;
	uint16_t pulsiMeterValue;
	uint8_t fix_indicator;
	uint8_t ff_status;
	uint8_t sos_status;
	uint8_t sos_counter;
};

struct payload_msg {
	uint8_t flags;
	uint8_t pyld_alt1;
	uint8_t pyld_alt2;
	uint8_t pyld_alt3;
	int16_t pyld_ax;
	int16_t pyld_ay;
	int16_t pyld_az;
	int16_t pyld_gx;
	int16_t pyld_gy;
	int16_t pyld_gz;
	int16_t pyld_temp;
	int16_t pyld_pulsimeter;
	float pyld_lat;
	float pyld_lon;
	
	
	
};

#endif /* _MESSAGES_H_ */