=============
Parser Module
=============

This module handles the parsing and packaging of sensor data into a compact payload format to be published over a Zbus channel for further processing (e.g., by LoRa communication subsystems).

Overview
---------------

The `parser_module` listens for incoming sensor messages on a Zbus subscription channel. It processes the received data and generates a payload structure (`payload_msg`) which is then published to the `payload_chan` channel.


Zbus Channel Definitions
------------------------

- **payload_chan**: A Zbus channel defined to transport `payload_msg` data. It is observed by the `lora_sub` subscriber.

.. code-block:: c

    ZBUS_CHAN_DEFINE(payload_chan, struct payload_msg, NULL, NULL,
                     ZBUS_OBSERVERS(lora_sub), ZBUS_MSG_INIT(0));

- **parser_sub**: A Zbus subscriber waiting for sensor messages.
- **lora_sub**: An observer that receives published payloads.

Thread Function
---------------

**parser_thread()**

This thread waits indefinitely (`K_FOREVER`) for a new message on the `parser_sub` subscription. When a message is received:

1. It reads a `sensor_msg` structure.
2. It creates and fills a `payload_msg` structure:
   - Sets appropriate flags based on SOS, free-fall (FF), and GPS fix status.
   - Logs raw sensor values and GPS data.
   - Populates payload fields including accelerometer, gyroscope, temperature, GPS, and pulsimeter values.
   - Encodes the altitude into three separate 8-bit payload fields.
3. It publishes the resulting `payload_msg` to the `payload_chan` channel.

.. code-block:: c

    K_THREAD_DEFINE(parser_thread_id, 512, parser_thread, NULL, NULL, NULL, 4, 0, 0);

Payload Construction Logic
--------------------------

- **Flags Field (payload.flags):**
  - Bit 7 (0x80): SOS active
  - Bit 6 (0x40): Free-fall detected
  - Bit 5 (0x20): GPS fix available
  - Bit 4 (0x10): South latitude
  - Bit 3 (0x08): West longitude

- **GPS Coordinates:**
  - Latitude and longitude are copied via `memcpy` to preserve floating point bit representation.

- **Altitude Encoding:**
  - Altitude is scaled by 10 and split across three 8-bit fields (`pyld_alt1`, `pyld_alt2`, `pyld_alt3`).

Dependencies
------------

- Zephyr RTOS
- Zbus IPC system
- Custom `messages.h` header defining `sensor_msg` and `payload_msg` structures

