HEAP_SIZE      = 8388208
STACK_SIZE     = 61800

PRODUCT = build/War.pdx

# Locate the SDK
SDK = ${PLAYDATE_SDK_PATH}
ifeq ($(SDK),)
SDK = $(shell egrep '^\s*SDKRoot' ~/.Playdate/config | head -n 1 | cut -c9-)
endif

ifeq ($(SDK),)
$(error SDK path not found; set ENV value PLAYDATE_SDK_PATH)
endif

# List C source files here
SRC = Source/main.c \
      Source/board.c \
      Source/piece.c \
      Source/move.c \
      Source/game.c \
      Source/render.c \
      Source/input.c

# List all user directories here
UINCDIR =

# List user asm files
UASRC =

# List all user C define here, like -D_DEBUG=1
UDEFS =

# Define ASM defines here
UADEFS =

# List the user directory to look for the libraries here
ULIBDIR =

# List all user libraries here
ULIBS =

include $(SDK)/C_API/buildsupport/common.mk


run:
	open -a "Playdate Simulator" $(PRODUCT)

test:
	$(MAKE) -C tests run

test-clean:
	$(MAKE) -C tests clean
