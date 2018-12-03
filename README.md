
SCOPE-FOOTSWITCH
=================

STM32F401 based oscilloscope-footswitch interface using USBTMC.

Hardware is licensed CC-BY-SA-4.0
Software is GPLv3 for easy license compatibility with ChibiOS

Tools:

Other versions may work, this is just what is currently tested

HW:
* KiCAD 5.0.1

SW:
* arm-none-eabi-gcc (GNU Tools for ARM Embedded Processors 7-2018-q2-update) 7.3.1 20180622 (release) [ARM/embedded-7-branch revision 261907]
* GNU gdb (GNU Tools for ARM Embedded Processors 7-2018-q2-update) 8.1.0.20180315-git
* ChibiOS-RT with community USB Host framework
* libopencm3 and DFU code from BlackMagicProbe

This repo uses several submodules, make sure they are up to date.
After checking out the kicad-library repo, make sure to cd to it and run make to generate the actual KiCAD libraries.

SW Build Process:
1. Make sure the arm-none-eabi toolchain is on your path: `which arm-none-eabi-gcc`
2. Pre-build the libopencm3 library: `cd sw/libopencm3 && make -j && cd ../..`
3. Build the bootloader: `cd sw/bootloader && make -j && cd ../..`
4. Build the main application: `cd sw/scope-footswitch && make -j && cd ../..`

SW Programming process:
1. Connect SWD debugger (e.g. blackmagicprobe) to hardware
2. Connect micro-usb from pc to hardware
3. `cd sw/bootloader`
4. `./flash.sh`
5. Power cycle hardware, holding down the front panel button to force entry to the bootloader
6. `cd ../scope-footswitch`
7. `./dfu.sh`


