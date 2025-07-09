==============
Sensors Module
==============

This module is responsible for handling various onboard sensors using I2C and ADC interfaces in a Zephyr-based embedded system. It manages sensor initialization, data acquisition, event-driven state transitions, and data publishing via `zbus`.

Overview
--------

The module includes support for:

- **MPU6050** (Accelerometer, Gyroscope, Temperature)
- **MMA8451** (Accelerometer with Free-Fall detection)
- **ADC**-based heartbeat sensor
- **GPS sensor interface** (external)
- **LED control** for normal and SOS states
- **Event-driven state machine** via `zbus` communication

Threads
-------

- **sensors_th**: Main thread handling sensor initialization and periodic sensor reading, triggered by events.
- **bpm_detection_thread**: Background thread that monitors ADC input for pulse detection and calculates BPM every 30 seconds.

States
------

The system operates in three main states:

- `NORMAL_MODE`: All sensors active; full data collected and transmitted.
- `LP_MODE` (Low Power): Only GPS data is collected; sensors are inactive to save power.
- `SOS_MODE`: Triggered by an SOS event; data is transmitted 3 times before reverting to the previous state.

Events
------

Handled events through `zbus` channel `events_chan`:

- `BUTTON_EVENT`: Toggles between `NORMAL_MODE` and `LP_MODE`
- `SOS_EVENT`: Triggers `SOS_MODE` and starts SOS counter
- `FF_EVENT`: Activates on free-fall detection from MMA8451
- `SEN_EVENT`: Periodic trigger to read sensor values based on current state

Data Flow
---------

Sensor data is collected and structured into a `sensor_msg` type, then published via the `sensor_data_chan` channel to other components such as parsers or network modules.

Functions
---------

- `mpu6050_init`, `mpu6050_read_accel`, `mpu6050_read_gyro`, `mpu6050_read_temperature`: Manage MPU6050 sensor.
- `mma8451_init`, `mma8451_read_accel`, `mma8451_read_freefall`: Handle initialization and data reading from MMA8451.
- `read_adc_u16`: Reads heartbeat signal from ADC.
- `leds_init`: Configures LED indicators.
- `sensors_th`: Main thread managing event-driven transitions and sensor reading.
- `bpm_detection_thread`: Calculates BPM using a threshold-based ADC signal.

Configuration & Dependencies
----------------------------

- **I2C node**: Uses the device labeled `i2c2` in the device tree.
- **ADC channels**: Pulled from `zephyr_user` node using `io_channels`.
- **GPIOs**: LEDs are defined through device tree aliases: `led0` and `blue_led_1`.

Zbus Channels
-------------

- `events_chan`: Receives events such as button press, SOS, etc.
- `sensor_data_chan`: Publishes `sensor_msg` containing data from all sensors.

Thread Definitions
------------------

.. code-block:: c

   K_THREAD_DEFINE(sensors_th_id, 2024, sensors_th, NULL, NULL, NULL, 3, 0, 0);
   K_THREAD_DEFINE(bpm_tid, 1024, bpm_detection_thread, NULL, NULL, NULL, 5, 0, 0);

Notes
-----

- The `mma8451_read_freefall` function clears internal interrupt bits by reading `FF_MT_SRC`.
- `generate_payload` flag controls whether sensor data should be published.
- GPS interface functions (`readLatitude`, `readFixIndicator`, etc.) are declared externally.
