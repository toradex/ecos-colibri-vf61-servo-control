#!/bin/bash

# Set compiler path
export PATH="{path/to/toolchain}/bin:$PATH"   # modify this

# Set compiler options
OPT="-Wall -Wpointer-arith -Wstrict-prototypes -Wundef -Wno-write-strings -mthumb -g -O2 -ffunction-sections -fdata-sections -fno-exceptions -nostdlib -mcpu=cortex-m4"

# Set path to eCos kernel
KPATH="ecos-kernel"

# Do compilation and link your application with kernel.
arm-eabi-gcc -g -I./ -g -I${KPATH}/install/include plotter.c \
-L${KPATH}/install/lib -Ttarget.ld ${OPT}

# Copy to binary file
arm-eabi-objcopy -O binary a.out plotter.bin
