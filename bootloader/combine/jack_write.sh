#!/bin/bash
#st-flash write combined.bin 0x8000000
arm-none-eabi-gdb --nx -ex 'set remotetimeout unlimited' -ex 'set confirm off' -ex 'target remote 127.0.0.1:3333' -ex 'monitor reset halt' -ex 'monitor flash write_image erase /home/jack/work/eclipse_mcu/bithd/spartacus_forked/tmp/bithd-mcu/bootloader/combine/combined.bin 0x08000000' -ex 'monitor reset halt' /home/jack/work/eclipse_mcu/bithd/spartacus_forked/tmp/bithd-mcu/bootloader/bootloader.elf
