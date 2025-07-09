Messages Definition
==========

This header file defines the data structures exchanged between system modules via Zbus channels. It provides two key message formats:

1. `sensor_msg`: Raw sensor data received from hardware.
2. `payload_msg`: Processed and packed data prepared for LoRaWAN transmission.

Data Structures
---------------

sensor_msg
~~~~~~~~~~

This structure contains raw sensor data and status flags. It is intended to represent the full state of the system based on sensor readings and computed GPS data.

.. code-block:: c

    struct sensor_msg {
        int16_t ax;           // Accelerometer X-axis
        int16_t ay;           // Accelerometer Y-axis
        int16_t az;           // Accelerometer Z-axis
        int16_t gx;           // Gyroscope X-axis
        int16_t gy;           // Gyroscope Y-axis
        int16_t gz;           // Gyroscope Z-axis
        int16_t temp;         // Raw temperature reading
        float latitude;       // GPS latitude in decimal degrees
        float longitude;      // GPS longitude in decimal degrees
        float altitude;       // Altitude in meters
        char latDir;          // Latitude direction ('N' or 'S')
        char lonDir;          // Longitude direction ('E' or 'W')
        uint16_t pulsiMeterValue; // Heartbeat or pulse meter reading
        uint8_t fix_indicator;    // GPS fix status
        uint8_t ff_status;        // Fall detection flag
        uint8_t sos_status;       // SOS activation status
        uint8_t sos_counter;      // Counter for SOS events
    };

payload_msg
~~~~~~~~~~~

This structure represents a compressed, LoRaWAN-friendly version of the sensor message. It minimizes payload size for efficient transmission.

.. code-block:: c

    struct payload_msg {
        uint8_t flags;            // Bitwise-encoded status (GPS, SOS, fall)
        uint8_t pyld_alt1;        // Altitude high byte (bits 16–23)
        uint8_t pyld_alt2;        // Altitude middle byte (bits 8–15)
        uint8_t pyld_alt3;        // Altitude low byte (bits 0–7)
        int16_t pyld_ax;          // Accelerometer X
        int16_t pyld_ay;          // Accelerometer Y
        int16_t pyld_az;          // Accelerometer Z
        int16_t pyld_gx;          // Gyroscope X
        int16_t pyld_gy;          // Gyroscope Y
        int16_t pyld_gz;          // Gyroscope Z
        int16_t pyld_temp;        // Temperature
        int16_t pyld_pulsimeter;  // Heartbeat value
        float pyld_lat;           // Latitude (float, 32-bit)
        float pyld_lon;           // Longitude (float, 32-bit)
    };


