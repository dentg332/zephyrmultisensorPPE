LoRaWAN Module
==============

This module manages the LoRaWAN communication interface for transmitting payloads over a low-power wide-area network (LPWAN) using the Zephyr LoRaWAN stack.

It handles the initialization, configuration, and joining of the network via OTAA (Over-The-Air Activation), and sends messages received from a Zbus channel.

Overview
---------------

The `lorawan_module`:
- Sets up LoRaWAN credentials (DevEUI, JoinEUI, AppKey).
- Initializes and joins the LoRaWAN network using OTAA.
- Registers callbacks for downlink reception and data rate changes.
- Waits for incoming messages on a Zbus channel (`payload_chan`) and sends them through the LoRaWAN network.

Zbus Configuration
------------------

- **ZBUS_CHAN_DECLARE(payload_chan)**: Declares the channel used for incoming payloads to be sent via LoRaWAN.
- **ZBUS_SUBSCRIBER_DEFINE(lora_sub, 8)**: Defines a Zbus subscriber with priority 8 that listens for published payload messages.

LoRaWAN Keys and Identifiers
----------------------------

.. code-block:: c

    #define LORAWAN_DEV_EUI     { ... }
    #define LORAWAN_JOIN_EUI    { ... }
    #define LORAWAN_APP_KEY     { ... }

These constants define the device's identity and security credentials used for OTAA.

Callback Functions
------------------

**dl_callback(...)**

Handles incoming downlink messages and logs metadata such as RSSI, SNR, and whether time synchronization was updated.

**lorwan_datarate_changed(...)**

Logs the new data rate and available maximum payload size after a change in LoRaWAN data rate.

Thread Function
---------------

**lorawan_thread()**

This thread performs the following tasks:

1. **Device & Join Configuration:**
   - Loads credentials into a `lorawan_join_config` structure.
   - Prints the DevEUI, JoinEUI, and AppKey for verification.

2. **Initialization & Join:**
   - Sets the region (e.g., `LORAWAN_REGION_EU868`).
   - Starts the LoRaWAN stack with `lorawan_start()`.
   - Sets device class to Class A (`lorawan_set_class()`).
   - Registers downlink and datarate change callbacks.
   - Attempts to join the network via OTAA with up to 3 retries.

3. **Payload Transmission Loop:**
   - Waits indefinitely for payloads published to the `payload_chan`.
   - Reads the payload, logs its raw content.
   - Sends the payload over LoRaWAN using `lorawan_send()` on port 2.
   - Waits for 18 seconds before processing the next message.

.. code-block:: c

    while (!zbus_sub_wait(&lora_sub, &chan, K_FOREVER)) {
        struct payload_msg payload;
        zbus_chan_read(chan, &payload, K_MSEC(200));
        LOG_HEXDUMP_INF(&payload, sizeof(payload), "Payload sent: ");
        ret = lorawan_send(2, &payload, sizeof(payload), LORAWAN_MSG_UNCONFIRMED);
        ...
        k_sleep(K_SECONDS(18));
    }

Thread Declaration
------------------

.. code-block:: c

    K_THREAD_DEFINE(lorawan_thread_id, 2048, lorawan_thread, NULL, NULL, NULL, 4, 0, 0);

Starts the `lorawan_thread` with a 2048-byte stack and priority 4.

Dependencies
------------

- Zephyr RTOS
- Zephyr LoRaWAN subsystem
- Zephyr Zbus IPC
- Custom `messages.h` header for `payload_msg`


