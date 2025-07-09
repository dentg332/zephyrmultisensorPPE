GPS UART Listener
======================================

This section describes the behavior of the `uart_cb` callback function implemented in ``gps.c``. This function is responsible for processing incoming NMEA sentences via UART and extracting GPS data from `$GPGGA` messages.

Overview
--------

The GPS module communicates using the UART interface and sends NMEA sentences, such as:

.. code-block::

   $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47

The `uart_cb` function filters and parses these sentences in real time to extract:

- Latitude and direction
- Longitude and direction
- Fix status
- Altitude

Function Signature
------------------

.. code-block:: c

   static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)


Event Handling Logic
--------------------

The callback responds to several UART events defined by Zephyr's UART asynchronous API:

### UART_RX_RDY

- Triggered when new data is received.
- The function scans the incoming buffer byte by byte.
- It searches for the `$GPGGA` prefix to identify the start of a GPS sentence.
- Once detected, it begins capturing the sentence into a dedicated buffer until a newline (`\\n`) is received.
- Upon full sentence capture:
  
  - The sentence is passed to `parse_gpgga()` for field extraction.
  - The state is reset to prepare for the next sentence.

### UART_RX_DISABLED

- Triggered when RX is stopped.
- The buffer is cleared using `memset`.

### UART_RX_BUF_REQUEST

- Triggered when a new RX buffer is requested.
- The driver responds with a buffer via `uart_rx_buf_rsp`.

Default:
- Any unhandled UART events are ignored silently.

GPS Sentence Parser
-------------------

The function `parse_gpgga()` is responsible for extracting useful GPS data from the sentence, storing it in a global `gps_data_t` structure.

Fields extracted:
- `latitude`, `lat_dir`
- `longitude`, `lon_dir`
- `fix_indicator`
- `altitude`

All other fields are ignored.

Supporting Functions
--------------------

Getter functions are exposed to other modules for accessing GPS values:

.. code-block:: c

   float readLatitude(void);
   char readLatitudeDirection(void);
   float readLongitude(void);
   char readLLongitudeDirection(void);
   uint8_t readFixIndicator(void);
   float readAltitude(void);

Initialization Function
-----------------------

The UART listener is initialized with:

.. code-block:: c

   int usart1_capture(const struct device *uart_dev);

This function:
- Checks if the UART device is ready.
- Registers the callback (`uart_cb`).
- Enables UART RX.
- Initializes `current_gps_data` with zero/default values.

Summary
-------

The `uart_cb` function, together with `parse_gpgga()`, provides an real-time method for extracting key GPS data from an NMEA stream. It uses a sliding window to detect sentences and manages state transitions with minimal memory, suitable for embedded systems with constrained resources.
