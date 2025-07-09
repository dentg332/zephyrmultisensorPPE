System Architecture
============================

This section provides a generic overview of the architecture of the system, focusing on the inter-module communication using channels and subscribers.

Design Paradigm
--------------

The system is designed using a **publish-subscribe (pub-sub)** communication model. This pattern facilitates asynchronous and decoupled data exchange between different components (modules) of the system.

Key Concepts
------------

- **Channels**: 
  - Channels act as named message buses through which data packets (messages) are published.
  - Each channel is strongly typed, ensuring that only specific types of messages are sent over it.
  - Channels hold the latest message state and notify subscribers when new data is available.

- **Subscribers**:
  - Subscribers are entities that listen to one or more channels.
  - They wait for new messages and react accordingly.
  - Subscribers can block and wait for data, allowing event-driven processing.
  - Multiple subscribers can listen to the same channel independently.

Zephyr ZBUS Framework
---------------------

This system leverages the Zephyr Project's **ZBUS** framework for inter-thread communication using channels and subscribers.

- **Channel Definition**: Channels are defined using macros, specifying the message type and observers.
  
  Example:

  .. code-block:: c

     ZBUS_CHAN_DEFINE(payload_chan,          /* Channel name */
                      struct payload_msg,   /* Message type */
                      NULL,                 /* Validator */
                      NULL,                 /* User data */
                      ZBUS_OBSERVERS(lora_sub), /* Observers */
                      ZBUS_MSG_INIT(0)      /* Initial value */
     );

- **Subscriber Definition**: Subscribers are defined to wait and process data from channels.
  
  Example:

  .. code-block:: c

     ZBUS_SUBSCRIBER_DEFINE(parser_sub, 4);

  Here, `parser_sub` is a subscriber with a queue size of 4.

Advantages of This Architecture
--------------------------------

- **Loose Coupling**: Modules do not depend directly on each other but communicate via well-defined message types.
- **Scalability**: Additional modules can subscribe to existing channels without modifying publishers.
- **Concurrency**: Multiple threads safely exchange data with synchronization handled by the framework.
- **Maintainability**: Clear separation of concerns improves code clarity and ease of debugging.

Summary
-------

The system architecture harnesses the power of pub-sub via Zephyr's ZBUS framework to enable efficient, modular, and reactive communication between sensors, parsers, and communication modules like LoRaWAN. Channels and subscribers are central to this design, promoting flexibility and robustness in embedded applications.
