# List of all the board related files.
BOARDSRC = $(ROOT_DIR)/board_$(BOARD)/board.c

# Required include directories
BOARDINC = $(ROOT_DIR)/board_$(BOARD)
# Shared variables
ALLCSRC += $(BOARDSRC)
ALLINC  += $(BOARDINC)

LDSCRIPT= $(ROOT_DIR)/board_$(BOARD)/STM32F401xB.ld

