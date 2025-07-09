===============================================
Events Module
===============================================

Overview
--------

This module is responsible for managing all time- and event-driven operations within the embedded system. It forms the backbone of the system's responsiveness by coordinating sensor measurement cycles, user interaction through buttons, and emergency signals. It is implemented using Zephyr OS facilities such as GPIO interrupts, kernel work queues, and timers.

The system operates in three modes:
- **NORMAL_MODE**: Periodically gathers data from all sensors and transmits it over LoRaWAN every 30 seconds.
- **LP_MODE**: Only GPS data is collected to minimize power consumption.
- **SOS_MODE**: Immediately transmits four emergency messages through LoRaWAN.

This module handles the logic for switching between these modes and initiating periodic or event-based actions.

Responsibilities
----------------

- **Event Detection**: Detects and processes input from:
  - A button for toggling between `NORMAL_MODE` and `LP_MODE`.
  - A dedicated SOS button for entering `SOS_MODE`.
  - A free-fall interrupt pin for fall detection.
- **Timer-Based Triggering**: Starts periodic timers to initiate sensor data acquisition and message transmission.
- **Event Publishing**: Publishes events to the `zbus` message bus, to be consumed by other system modules.

Key Components
--------------

- **Timers**:
  - `event_timer`: Triggers every 30 seconds to initiate sensor data capture.
  - `button_timer` and `sos_button_timer`: Detect long-press behavior for mode changes.

- **GPIO Interrupts**:
  - Configured on three pins: `button`, `sos_button`, and `freefall`.
  - Each interrupt triggers a work item submitted to a Zephyr kernel workqueue.

- **Work Items**:
  - Four `k_work` items represent different types of events (button press, SOS button, free fall, and timer tick).
  - When triggered, each work item publishes a corresponding event identifier to the `events_chan` channel on the Zephyr `zbus`.

Event Workflow
--------------

1. A user action or sensor event triggers a GPIO interrupt.
2. The associated interrupt handler sets a flag and starts a timer if needed.
3. Upon timer expiration (e.g., long press), a kernel work item is submitted.
4. The work item invokes the callback `wq_eh_cb`, which publishes the corresponding event code to the `events_chan`.

Event Codes
-----------

Each event published to `events_chan` is represented by a numeric code:

- `1`: Mode change button press (NORMAL ↔ LP)
- `2`: SOS button activation
- `3`: Free-fall detection
- `4`: Periodic timer event

Dependencies
------------

- **Zephyr OS**: Kernel, GPIO, DeviceTree, Timers, Logging, Zbus
- **Custom Headers**: `messages.h`, `gps_sensor.h`
- **Hardware Requirements**:
  - GPIO-connected buttons for mode switching
  - A free-fall sensor with interrupt capabilities
  - USART interface for GPS or other peripherals

Limitations
------------------------------

- This module assumes the presence of properly configured aliases in the Device Tree (e.g., `sw1`, `sw2`, `ff0`, `usart1`).


