import subprocess
from subprocess import PIPE
import os
from secrets import token_hex
import random
import shutil
import sys

wifi_ssid = "Golioth"
wifi_pw = "training"
project_id = "developer-training"

build_dir = "/tmp/magtag-build"
bin_output_dir = "/home/mike/Desktop/generated_binaries"

venv = 'source /home/mike/compile/zephyrproject/.venv/bin/activate'
outoftree = '''source /home/mike/compile/zephyrproject/zephyr/zephyr-env.sh'''
exports = "export ESPRESSIF_TOOLCHAIN_PATH=\"${HOME}/.espressif/tools/zephyr/\"; export ZEPHYR_TOOLCHAIN_VARIANT=\"espressif\""
build = '''/home/mike/compile/zephyrproject/.venv/bin/west build -b esp32s2_saola /home/mike/golioth-compile/magtag-demo -d %s -DCONFIG_MAGTAG_NAME=\\\"{}\\\" -DCONFIG_GOLIOTH_SYSTEM_CLIENT_PSK_ID=\\\"{}\\\" -DCONFIG_GOLIOTH_SYSTEM_CLIENT_PSK=\\\"{}\\\" -DCONFIG_ESP32_WIFI_SSID=\\\"%s\\\" -DCONFIG_ESP32_WIFI_PASSWORD=\\\"%s\\\"''' % (build_dir, wifi_ssid, wifi_pw)


#Random names from color/flower lists
color_list = ["coral", "gold", "lime", "magenta", "purple", "yellow", "red", "blue", "green", "emerald", "bronze", "azure", "amber", "black", "cobalt", "crimson", "cyan", "jade", "olive", "plum"]
flower_list = ["sweetpea", "stargazer", "hydrangea", "daffodil", "begonia", "camellia", "plumeria", "dahlia", "tulip", "iris", "lotus", "anemone", "orchid", "magnolia", "peony", "protea", "rose"]

env = {
    **os.environ,
    "ESPRESSIF_TOOLCHAIN_PATH":"/home/mike/.espressif/tools/zephyr",
    "ZEPHYR_TOOLCHAIN_VARIANT":"espressif"
    }

venv = '''. /home/mike/compile/zephyrproject/.venv/bin/activate'''
cmd = '''/home/mike/compile/zephyrproject/.venv/bin/west build -b esp32s2_saola /home/mike/golioth-compile/magtag-demo -p'''
proc = subprocess.Popen([venv, cmd], env=env, shell=True, stdout=subprocess.PIPE)
for line in proc.stdout:
    print(line) 

def generate_names(name_count):
    name_dict = dict()
    while(len(name_dict) < name_count):
        new_name = "-".join([random.choice(color_list), random.choice(flower_list)])
        if new_name not in name_dict:
            name_dict[new_name] = {"psk_id":new_name.lower()+"-id", "psk":token_hex(16)}
    return name_dict

def provision_device(magtag_name, psk_id, psk):
    cmd = '''goliothctl provision --name "%s" --credId "%s" --psk "%s"''' % (magtag_name, psk_id, psk)
    proc=subprocess.Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE, executable='/bin/bash', universal_newlines=True)
    for i in proc.communicate():
        print(i)
    if proc.returncode:
        print("ERROR: %d" % proc.returncode)
        sys.exit("Error: failed to provision device on Golioth")
    else:
        print("Device provisioned!")


def build_firmware(magtag_name, psk_id, psk):
    print("\nBuilding firmware\n")
    cmd="; ".join([venv, outoftree, exports, build.format(magtag_name, psk_id+"@"+project_id, psk)])
    print("Running:\n%s\n" % cmd)
    proc=subprocess.Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE, executable='/bin/bash', universal_newlines=True)
    for i in proc.communicate():
        print(i)
    if proc.returncode:
        print("ERROR: %d" % proc.returncode)
        sys.exit("Error: west build failed")
    else:
        print("Build successful!")

    if os.path.isdir(bin_output_dir) == False:
        os.mkdir(bin_output_dir)
    new_filename = os.path.join(bin_output_dir, magtag_name + ".bin")
    print("Moving file to: %s" % new_filename)
    old_filename = os.path.join(build_dir, "zephyr", "zephyr.bin")
    shutil.move(old_filename, new_filename)


def auto_provision(device_count):
    device_names = generate_names(device_count)
    for d_name in device_names:
        print("Adding %s to Golioth" % d_name)
        provision_device(d_name, device_names[d_name]["psk_id"], device_names[d_name]["psk"])
        build_firmware(d_name, device_names[d_name]["psk_id"], device_names[d_name]["psk"])
        print("\nComplete! Flash firmware using:\nwest flash --esp-device /dev/ttyACM0 --build-dir %s --skip-rebuild --bin-file %s\n" % (build_dir, os.path.join(bin_output_dir, "firmware_filename.bin")))

def main(argv):
    if len(sys.argv) != 3 or sys.argv[1] != "run":
        print("\nUsage: python3 provision_devices.py run 5\n")
        print("\tspecify the number of devices to generate as the second argument\n")
    else:
        try:
            device_count = int(sys.argv[2])
            auto_provision(device_count)
        except:
            print("Error: The second argument must be a number")

if __name__ == "__main__":
   main(sys.argv[1:])
