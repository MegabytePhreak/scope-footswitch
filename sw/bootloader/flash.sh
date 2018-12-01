#! /usr/bin/sh

arm-none-eabi-gdb -batch build/bootloader.elf -ex "source ../scope-footswitch/gdbinit" -ex "load"
