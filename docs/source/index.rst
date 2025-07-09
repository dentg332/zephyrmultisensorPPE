.. Zephyr Multisensor PPE documentation master file, created by
   sphinx-quickstart on Wed Jul  9 18:30:01 2025.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Zephyr Multisensor PPE documentation
====================================

This documentation provides a comprehensive overview of the project's components, architecture, and key functionalities.

The primary goal of this project is to develop a modular and efficient embedded system that integrates sensor data acquisition, data parsing, and wireless communication using LoRaWAN technology.

Scope
-----

This documentation covers:
- A guide to setting up the development environment for Zephyr OS with VSCode.
- The overall system architecture, including inter-module communication.
- Detailed descriptions of each software module.
- Data structures and message formats.


.. toctree::
   :maxdepth: 1
   :caption: Contents:

   getting_started
   architecture
   events_module
   gps_cb
   sensor_module
   parser_module
   lora_module
   messages_file

