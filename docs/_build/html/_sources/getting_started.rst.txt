Getting Started
================================================================

This guide explains how to prepare your development environment for building and debugging Zephyr OS projects using Visual Studio Code and a **STM32 Nucleo-WL55JC** board. It covers installing Zephyr OS, setting up the necessary tools, and integrating build and debug tasks into VSCode with provided JSON configurations.

The official **Getting Started Guide** provided by Zephyr has been used as a reference for this documentation:  
https://docs.zephyrproject.org/latest/getting_started/index.html

Prerequisites
-------------

- A supported operating system (Linux, Windows, or macOS)
- CMake 3.20.5
- Python 3.10 or later
- STM32CubeProgrammer
- VSCode installed
- Git installed
- `wget` installed
- 7-Zip installed

Step 1: Install Zephyr OS and Python Dependencies
-------------------------------------------------

Follow the steps detailed in the official documentation:

https://docs.zephyrproject.org/latest/getting_started/index.html

Step 2: Integrate Zephyr OS with VSCode
---------------------------------------

To integrate the build and debugging process in VSCode, place the following configuration files inside the project's `.vscode` folder.

### tasks.json

Defines tasks for building and flashing the firmware using **West**:

- **West Build**: Compiles the project for the specified board (`nucleo_wl55jc` by default).
- **West Flash**: Flashes the compiled firmware to the device.
- **West Build and Flash**: Runs build and flash sequentially.

**Example:**

.. code-block:: json

   {
     "version": "2.0.0",
     "tasks": [
       {
         "label": "West Build",
         "type": "shell",
         "group": {
           "kind": "build",
           "isDefault": true
         },
         "linux": {
           "command": "${userHome}/zephyrTFMproject/.venv/bin/west"
         },
         "windows": {
           "command": "${userHome}\\zephyrTFMproject\\.venv\\Scripts\\west.exe"
         },
         "osx": {
           "command": "${userHome}/zephyrTFMproject/.venv/bin/west"
         },
         "args": [
           "build",
           "-p",
           "auto",
           "-b",
           "nucleo_wl55jc",
           "./MultiSensorPPE/"
         ],
         "problemMatcher": ["$gcc"]
       },
       {
         "label": "West Flash",
         "type": "shell",
         "args": ["flash"],
         "linux": {
           "command": "${userHome}/zephyrTFMproject/.venv/bin/west"
         },
         "windows": {
           "command": "${userHome}\\zephyrTFMproject\\.venv\\Scripts\\west.exe"
         },
         "osx": {
           "command": "${userHome}/zephyrTFMproject/.venv/bin/west"
         },
         "problemMatcher": ["$gcc"]
       },
       {
         "label": "West Build and Flash",
         "dependsOn": [
           "West Build",
           "West Flash"
         ],
         "dependsOrder": "sequence",
         "group": {
           "kind": "build",
           "isDefault": false
         }
       }
     ],
     "inputs": [
       {
         "id": "board",
         "type": "promptString",
         "default": "nucleo_wl55jc",
         "description": "See https://docs.zephyrproject.org/latest/boards/index.html"
       }
     ]
   }

### launch.json

Configures the debugging environment using **Cortex-Debug** and **OpenOCD**:

- Specifies the device type (`STM32WL55JC`)
- Points to the compiled ELF executable
- Defines the GDB path from the Zephyr SDK
- Uses `West Build` as the pre-launch task
- Includes OpenOCD config files for ST-Link and STM32WL target

**Example:**

.. code-block:: json

   {
     "version": "0.2.0",
     "configurations": [
       {
         "name": "Launch",
         "device": "STM32WL55JC",
         "cwd": "${workspaceFolder}",
         "executable": "build/zephyr/zephyr.elf",
         "request": "launch",
         "type": "cortex-debug",
         "runToEntryPoint": "main",
         "servertype": "openocd",
         "gdbPath": "${userHome}/zephyr-sdk-0.17.1/arm-zephyr-eabi/bin/arm-zephyr-eabi-gdb",
         "preLaunchTask": "West Build",
         "configFiles": [
           "interface/stlink.cfg",
           "target/stm32wlx.cfg"
         ]
       }
     ]
   }

Step 3: Using the Environment
-----------------------------

- Open the project folder in VSCode.
- Use the **Terminal → Run Task** menu to execute `West Build` or `West Flash`.
- Start debugging by pressing **F5** or selecting the **Launch** configuration.
- The system will:
  
  1. Build the project.
  2. Flash the firmware to the board.
  3. Attach the debugger automatically.

----

