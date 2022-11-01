Golioth Developer Training: Name Tag
########################################

This demo provides four different ways to display your name on the MagTag ePaper
screen

This demo includes:

* Option at boot to connect to Golioth and download name/title/handle
* Store updated information from Golioth in flash
* Button control for four different display styles

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
   west build -b esp32s2_saola nametag -p
   west flash --esp-device=/dev/ttyACM0

Board must be manually put into DFU mode (hold boot, hit reset) before flashing
and manually reset after flashing.

Behavior
********

At boot time, two blue lights will be shown and you will be given the option to
connect to WiFi and download information from Golioth. Rainbow lights will be
shown while the device is trying to connect to WiFi/Golioth. As new data is
downloaded a message will be printed to the screen. When the download process is
complete, a name tag display will be shown. Press any of the buttons to change
name tag displays.


Use the LightDB State interafce for your device on `the Golioth Console`_ to
enter your name/title/handle. Add three unique endpoints:

- ``name``
- ``title``
- ``handle``

.. _Adafruit MagTag board: https://learn.adafruit.com/adafruit-magtag
.. _MagTag purchase link: https://www.adafruit.com/magtag
.. _MagTag stock firmware: https://learn.adafruit.com/adafruit-magtag/downloads#all-in-one-shipping-demo-3077979-2
.. _MagTag schematic: https://learn.adafruit.com/assets/96946
.. _MagTag high-level pinout: https://github.com/adafruit/Adafruit_MagTag_PCBs/blob/main/Adafruit%20MagTag%20ESP32-S2%20pinout.pdf
.. _MagTag design files: https://github.com/adafruit/Adafruit_MagTag_PCBs
.. _AdafruitAdafruit MagTag board: https://www.adafruit.com/magtag
.. _the Golioth console: https://console.golioth.io/
