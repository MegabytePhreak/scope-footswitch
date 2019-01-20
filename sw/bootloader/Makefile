PROJECT = bootloader
BUILD_DIR = build

CFILES = dfu_f4.c dfucore.c  hidcore.c main.c
INCLUDES += -I../common

DEVICE=stm32f401cbu6

# You shouldn't have to edit anything below here.
OPENCM3_DIR=../ext/libopencm3

include $(OPENCM3_DIR)/mk/genlink-config.mk
include rules.mk
include $(OPENCM3_DIR)/mk/genlink-rules.mk
