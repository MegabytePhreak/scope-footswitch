#! /usr/bin/sh

openocd -f board/stm32f0discovery.cfg -c "program build/tmcemu.elf verify reset exit"
