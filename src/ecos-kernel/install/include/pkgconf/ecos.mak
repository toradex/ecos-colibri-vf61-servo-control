ECOS_GLOBAL_CFLAGS = -Wall -Wpointer-arith -Wstrict-prototypes -Wundef -Woverloaded-virtual -Wno-write-strings -mcpu=cortex-m4 -mthumb -g -O2 -ffunction-sections -fdata-sections -fno-rtti -fno-exceptions
ECOS_GLOBAL_LDFLAGS = -mcpu=cortex-m4 -mthumb -Wl,--gc-sections -Wl,-static -Wl,-n -g -nostdlib
ECOS_COMMAND_PREFIX = arm-eabi-
