# Golioth MagTag Demo

This code repository is used for Golioth's Developer Training. Based on Zephyr
RTOS, the applications are designed for self-guided learning that teaches how to
use the Zephyr build system and how to connect to and interact with Golioth's
device management platform. As you move through the lessons, you will become
familiar with Zephyr GPIO, Devicetree, and working with sensor drivers.

## Self-Guided Developer Training

Please visit our Developer Training site at
[training.Golioth.io](https://training.golioth.io). The companion materials
include an in-depth explanation of how to use the code in this repository.

## Installing this repository

This repository includes a Zephyr manifest file that indicates that last tested
version of Golioth (and in turn, version of Zephyr and all supporting modules).
We recommend that you install the Zephyr tree using this manifest.

### Recommended: use a Python virtual environment

We recommend using a Python virtual environment which will keep all installed
Python files inside of a subdirectory and separate from the rest of your system.

```bash
cd ~
mkdir magtag-demo
python3 -m venv magtag-demo/.venv
source magtag-demo/.venv/bin/activate
pip install wheel
pip install west
```

* **Note:** when using a Python virtual environment, you need to re-enable it with each new
terminal session: `source ~/magtag-demo/.venv/bin/activate`

### Clone and Install

This repository can be cloned using the `west` meta tool which will also install
Zephyr and all necessary modules.

```bash
cd ~
west init -m https://github.com/golioth/magtag-demo.git magtag-demo
cd magtag-demo
west update
west zephyr-export
pip install -r ~/magtag-demo/deps/zephyr/scripts/requirements.txt
west blobs fetch hal_espressif
```
