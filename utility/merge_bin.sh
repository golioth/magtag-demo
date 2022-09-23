#!/bin/bash

##
# Merge the three bin files needed by ESP32 boards into one:
#    ./merge_bin.sh <zephyr_build_directory>
#
# Output:
#    <zephyr_build_directory>/zephyr/merged.bin
#
# Flash command:
#    esptool.py --chip esp32s2 -p /dev/ttyACM0 write_flash 0x0 ../build/zephyr/merged.bin
##

if [[ -z $1 ]]; then
		echo "Usage: merge.bin <path_of_zephyr_build_directory>"
		exit 1
fi

if [ ! -d $1 ]; then
		echo "Build directory not found"
		exit 2
fi

sanitized_root=${@%/}
zephyrbin_path=$sanitized_root/zephyr/zephyr.bin
bootloader_path=$sanitized_root/esp-idf/build/bootloader/bootloader.bin
partitions_path=$sanitized_root/esp-idf/build/partitions_singleapp.bin

for i in $zephyrbin_path $bootloader_path $partitions_path
do
	if [ ! -f $i ]; then
			echo "File not found: $i"
			exit 2
	fi
done

echo "Merging bin files"

cmd_string="esptool.py --chip esp32s2 merge_bin -o $sanitized_root/zephyr/merged.bin --flash_mode dio --flash_freq keep --flash_size keep 0x1000 $bootloader_path 0x8000 $partitions_path 0x10000 $zephyrbin_path"

eval $cmd_string
