#pragma once

#include <zephyr/device.h>

int usart1_capture(const struct device *uart_dev);
float readLatitude(void);
char readLatitudeDirection(void);
float readLongitude(void);
float readLLongitudeDirection(void);
uint8_t readFixIndicator(void);
float readAltitude (void);