Golioth Demo on Adafruit MagTag
###############################

This demonstrates how to interact with the Golioth Cloud using Zephyr on the
`Adafruit MagTag board`_.

Resources
*********

* `MagTag purchase link`_
* `MagTag stock firmware`_ 
* `MagTag schematic`_
* `MagTag high-level pinout`_
* `MagTag design files`_


Build instructions
******************

Create a credentials file to use from commandline (wifi and golioth):

.. code-block::

   CONFIG_GOLIOTH_SYSTEM_CLIENT_PSK_ID="device-id"
   CONFIG_GOLIOTH_SYSTEM_CLIENT_PSK="psk"

   CONFIG_ESP32_WIFI_SSID="ssid"
   CONFIG_ESP32_WIFI_PASSWORD="pw"

Build
=====

``west build -b esp32s2_saola samples/magtag/magtag-demo/ -D OVERLAY_CONFIG=~/Desktop/esp32-devboard-one-golioth.conf``

Build for development server:
=============================

``west build -b esp32s2_saola samples/magtag/magtag-hello/ -D OVERLAY_CONFIG=~/Desktop/esp32-devboard-one-devserver.conf -D CONFIG_GOLIOTH_SYSTEM_SERVER_HOST=\"coap.golioth.net\" -p``

Flash
=====

``west flash --esp-device=/dev/ttyACM0``

Board must be manually put in DFU mode (hold boot, hit reset) and manually reset
after flashing)


Status
******

Started from the lightDB/observe sample.

* Added in JSON parsing. Add this to lightDB state in console:

  .. code-block:: json

     {
        "led_settings": {
           "led0_color": "red",
           "led0_state": 1,
           "led1_color": "green",
           "led1_state": 1,
           "led2_color": "blue",
           "led2_state": 1,
           "led3_color": "red",
           "led3_state": 0
        }
     }

* Added ws2812 hardware support
* Added lis3dh accelerometer support
* Added button input support
* Added state write to Golioth on button press

Fixme
*****

* The ESP32s2_soala will only compile for me if I set: ``CONFIG_NET_SHELL=n``
* USB-CDC for console
* This particular epaper display is not currently supported in Zephyr
* Add light sensor
* Add speaker
* Sort out overlay so all pins are mapped
* Button debounce

Fast Hash
*********

Instead of doing string comparison in c, we can do a weak hash to match strings.
This is just does an ``XOR`` on the ascii values for the letters and using the
result as the matching value.

A python function can be used to compute these easily:

.. code-block::

   >>> from operator import xor
   >>> wordlist = ["red","green","blue","off"]
   >>> def fasthash(word):
        count = 0
        for i in word:
           count = xor(count,ord(i))
        print(word,count)

      
   >>> for i in wordlist:
      	fasthash(i)

	
   red 115 green 123 blue 30 off 111

.. _Adafruit MagTag board: https://learn.adafruit.com/adafruit-magtag
.. _MagTag purchase link: https://www.adafruit.com/magtag
.. _MagTag stock firmware: https://learn.adafruit.com/adafruit-magtag/downloads#all-in-one-shipping-demo-3077979-2
.. _MagTag schematic: https://learn.adafruit.com/assets/96946
.. _MagTag high-level pinout: https://github.com/adafruit/Adafruit_MagTag_PCBs/blob/main/Adafruit%20MagTag%20ESP32-S2%20pinout.pdf
.. _MagTag design files: https://github.com/adafruit/Adafruit_MagTag_PCBs
.. _AdafruitAdafruit MagTag board: https://www.adafruit.com/magtag