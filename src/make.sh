#!/bin/bash

# Set compiler path
#export PATH="{path/to/toolchain}/bin:$PATH"   # modify this
export PATH="/home/ags/projects/toradex/vybrid/ecos/gnutools/arm-eabi/bin:$PATH"


# Set compiler options
OPT="-Wall -Wpointer-arith -Wstrict-prototypes -Wundef -Wno-write-strings -mthumb -g -O2 -ffunction-sections -fdata-sections -fno-exceptions -nostdlib -mcpu=cortex-m4"

# Set path to eCos build tree
BTPATH="build-tree"

# Do compilation and link your application with kernel.
arm-eabi-gcc -g -I./ -g -I${BTPATH}/install/include servo.c \
-L${BTPATH}/install/lib -Ttarget.ld ${OPT}

# Copy to binary file
arm-eabi-objcopy -O binary a.out servo.bin
