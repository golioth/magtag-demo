Golioth Developer Training: Blinky
###################################

This demo blinks the red LED on the bottom of the board once per second.

We have copied the `samples/basic/blinky`_ demo from the Zephyr repository. We
have included a board overlay file that sets up the pin assignment.

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

Activate Virtual Environment
============================

.. code-block:: bash

   source ~/magtag-demo/.venv/bin/activate

Build and Flash
===============

.. code-block:: bash

   cd ~/magtag-demo/app
   west build -b esp32s2_saola blinky -p
   west flash --esp-device=/dev/ttyACM0

Board must be manually put into DFU mode (hold boot, hit reset) before flashing
and manually reset after flashing.

.. _samples/basic/blinky: https://github.com/zephyrproject-rtos/zephyr/tree/main/samples/basic/blinky
.. _Adafruit MagTag board: https://learn.adafruit.com/adafruit-magtag
.. _MagTag purchase link: https://www.adafruit.com/magtag
.. _MagTag stock firmware: https://learn.adafruit.com/adafruit-magtag/downloads#all-in-one-shipping-demo-3077979-2
.. _MagTag schematic: https://learn.adafruit.com/assets/96946
.. _MagTag high-level pinout: https://github.com/adafruit/Adafruit_MagTag_PCBs/blob/main/Adafruit%20MagTag%20ESP32-S2%20pinout.pdf
.. _MagTag design files: https://github.com/adafruit/Adafruit_MagTag_PCBs
