Golioth Developer Training: Golioth-Demo
########################################

For in-person trainings, this demo comes pre-provisioned with both WiFi and
Golioth credentials to begin the workshop.

This demo includes:

* Light and sound reaction to button presses
* Button presses recorded on Golioth via network logging
* LED on/off status sent to Light DB state
* Accelerometer readings sent to LIghtDB stream every 5 seconds

Hardware: Adafruit MagTag
*************************

This demonstrates how to interact with the Golioth Cloud using Zephyr on the
`Adafruit MagTag board`_.

**NOTE:** Update your Zephyr toolchain to ensure the ePaper works.

Resources
*********

* `MagTag purchase link`_
* `MagTag stock firmware`_
* `MagTag schematic`_
* `MagTag high-level pinout`_
* `MagTag design files`_


Build instructions
******************

**Prerequisite:** Follow the README in the root of this repository to use ``west
init`` to clone this repo and install Zephyr

**Prerequisite:** Create a credentials file in the root directory of this
repository called ``credentials.conf`` that contains your Golioth device
psk-id/psk and your WiFi SSID/password. We have included an example called
``credentials.conf_example`` as a starting point.

Here is what the contents of that file should look like.

.. code-block::

   CONFIG_GOLIOTH_SYSTEM_CLIENT_PSK_ID="device-id"
   CONFIG_GOLIOTH_SYSTEM_CLIENT_PSK="psk"

   CONFIG_ESP32_WIFI_SSID="ssid"
   CONFIG_ESP32_WIFI_PASSWORD="pw"

**This app will not build without `credentials.conf` in the parent directory of
this folder**

Activate Virtual Environment
============================

.. code-block:: bash

   source ~/magtag-demo/.venv/bin/activate

Build and Flash
===============

.. code-block:: bash

   cd ~/magtag-demo/app
   west build -b esp32s2_saola golioth-demo -p
   west flash --esp-device=/dev/ttyACM0

Board must be manually put into DFU mode (hold boot, hit reset) before flashing
and manually reset after flashing.

Behavior
********

At boot time the MagTag will not visibly react until after the WiFi hardware is
initialized. That process can take several seconds, at which point the two
center LEDs will turn blue to indicate the board is trying to establish an
internet connection and connect with Golioth.

When a connection with Golioth is achieved, the LEDs will light up
red/green/blue/yellow. Pressing a button will toggle the LED on/off and play a
tone. This button press will be reported to the Logs on `the Golioth Console`_,
and the state of the LED will be updated in the LightDB state. Every 5 seconds,
accelerometer data will be recorded on LightDB Stream.

.. _Adafruit MagTag board: https://learn.adafruit.com/adafruit-magtag
.. _MagTag purchase link: https://www.adafruit.com/magtag
.. _MagTag stock firmware: https://learn.adafruit.com/adafruit-magtag/downloads#all-in-one-shipping-demo-3077979-2
.. _MagTag schematic: https://learn.adafruit.com/assets/96946
.. _MagTag high-level pinout: https://github.com/adafruit/Adafruit_MagTag_PCBs/blob/main/Adafruit%20MagTag%20ESP32-S2%20pinout.pdf
.. _MagTag design files: https://github.com/adafruit/Adafruit_MagTag_PCBs
.. _AdafruitAdafruit MagTag board: https://www.adafruit.com/magtag
.. _the Golioth console: https://console.golioth.io/
