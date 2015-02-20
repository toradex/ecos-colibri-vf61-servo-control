# Servo Motor Control #

This is a eCos application driving a servo motor by generating a PWM signal
using a GPIO. The Demo was shown at the Toradex booth at Embedded World 2015.

The eCos firmware is loaded by the Qt UI (see QtAutomotiveClusterDemo branch
embedded-world-2015) and uses Multi-Core Communication (MCC) to start the
servo motors synchronized.

The firmware can also be loaded manually using

    $ mqxboot /home/root/servo.bin 0x8f000400 0x0f000411

In order to make loading of the eCos firmware possible the Linux memory
resources need to be restricted using U-Boot arguments:

    Colibri VFxx # set memargs mem=240M

Also make sure the device tree is still loaded in a reachable memory area:

    Colibri VFxx # set fdt_high 0x88000000
