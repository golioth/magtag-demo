Golioth Developer Training: Observe
###################################

This demo toggles four on-board LEDs on and off based on an observed data value
on your Golioth Cloud.

* Expected LightDB state key: ``leds``
* Valid values: 0..15

The decimal number entered on the console will be used as a binary bitmask. So
entering the number 14 is the same as binary 0b1110 and would turn the right LED
off and the other three on.

Don't forget to click the "Submit" button when you change LightDB state values
in the Golioth console. Each valid received value will be printed to the ePaper
display. If you need help with debugging, check the logs in the console.

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

Clone this repository into your Golioth folder within the Zephyr install
directory

**NOTE:** Your zephyr location may be different than below, check where
`zephyrproject` has been installed.

.. code-block:: bash

   ~/zephyrproject/modules/lib/golioth/samples
   git clone git@github.com:golioth/magtag-demo.git

Ensure that you have activated your virtual environment and set up the
espressif toolchain environment variables. These can be in different places
depending on your operating system but should look something like this:

.. code-block:: bash

   source ~/zephyrproject/.venv/bin/activate
   export ESPRESSIF_TOOLCHAIN_PATH="${HOME}/.espressif/tools/zephyr/"
   export ZEPHYR_TOOLCHAIN_VARIANT="espressif"

Create a credentials file
=========================

Create a credentials file called ``credentials.overlay`` that contains your
Golioth device psk-id/psk and your WiFi SSID/password. We have included an
example called ``credentials.overlay_example`` as a starting point.

Here is what the contents of that file should look like.

.. code-block::

   CONFIG_GOLIOTH_SYSTEM_CLIENT_PSK_ID="device-id"
   CONFIG_GOLIOTH_SYSTEM_CLIENT_PSK="psk"

   CONFIG_ESP32_WIFI_SSID="ssid"
   CONFIG_ESP32_WIFI_PASSWORD="pw"

Build
=====

``west build -b esp32s2_saola . -D OVERLAY_CONFIG=credentials.overlay -p``

Flash
=====

``west flash --esp-device=/dev/ttyACM0``

Board must be manually put into DFU mode (hold boot, hit reset) and manually
reset after flashing.

Behavior
********

At boot time the MagTag will not visibly react until after the WiFi hardware is
initialized. That process can take several seconds, at which point the two center
LEDs will turn blue to indicate the board is trying to establish an internet
connection and connect with Golioth.

When a connection with Golioth is achieve, all three LEDs will turn green. The
board will then begin sending "Hello!" messages to the Golioth cloud. You should
see this indicated on on the ePaper screen, and can confirm the messages are
being received by visiting `the Golioth console`_.

.. _Adafruit MagTag board: https://learn.adafruit.com/adafruit-magtag
.. _MagTag purchase link: https://www.adafruit.com/magtag
.. _MagTag stock firmware: https://learn.adafruit.com/adafruit-magtag/downloads#all-in-one-shipping-demo-3077979-2
.. _MagTag schematic: https://learn.adafruit.com/assets/96946
.. _MagTag high-level pinout: https://github.com/adafruit/Adafruit_MagTag_PCBs/blob/main/Adafruit%20MagTag%20ESP32-S2%20pinout.pdf
.. _MagTag design files: https://github.com/adafruit/Adafruit_MagTag_PCBs
.. _AdafruitAdafruit MagTag board: https://www.adafruit.com/magtag
.. _the Golioth console: https://console.golioth.io/
