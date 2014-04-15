.. role:: menuselection

.. role:: program

PenPlotter eCos demo
======================

This is a repository for the XYPenPlotter demo for the port of `eCos 3.0 RTOS <http://ecos.sourceware.org/>`_ for the `Toradex Colibri VF61 Freescale Vybrid Computer on Module <http://developer.toradex.com/product-selector/colibri-vf61>`_.

The plotter application is a simple example of using Vybrid's Cortex-M4 to control a device in real time and communicate with Linux running on the Cortex-A5 core to drive a GUI which enables the user to select the content being drawn.
The plotter motors are driven by interpreting g-codes generated from SVG image files.

This application is intended to be combined with the XYPenPlotter Linux Qt application running on the Cortex-A5 core `also available from GitHub <https://github.com/toradex/XYPenPlotter>`_.

The eCos application consists of four threads:

* *thread_fn_1* and
* *thread_fn_2* - responsible for generating control signals for the step motors
* *thread_fn_3* - providing inter-core communication
* *main* thread - the main eCos thread which starts other threads, sets GPIOs and executes drawing instructions from a predefined memory region.
  
This document also gives a brief description about preparing images for the plotter and putting them into memory. 

Compilation
-----------

The compilation can be done using the following steps:

.. code-block:: bash

   git clone https://github.com/mgielda/ecos-colibri-vf61-xypenplotter
   cd ecos-colibri-vf61-xypenplotter/src
   vi make.sh
   [use your favourite editor and set toolchain path]
   ./make.sh

This repository already contains a pre-compiled eCos kernel suitable for use with the plotter application.

If a custom eCos kernel is to be used instead, it can be compiled separately and pointed to by the ``KPATH`` variable.

Preparing images for drawing
----------------------------

``G-code`` is the name of a very popular numerical control (NC) programming language.
This format was chosen for controlling the plotter as there are many aplications supporting conversion of vector graphics into g-codes.

Prerequisites
~~~~~~~~~~~~~

To prepare the graphics and g-codes you need:

1. `Inkscape <http://www.inkscape.org/en/>`_ - fully open source, free and professional vector graphics editor.
   In Debian-like systems you may install Inkscape using:

   .. code-block:: bash 

      sudo apt-get install inkscape

2. Gcodetools plugin for Inkscape.
   More information and an associated installation guide is available on the `plugin website <http://www.cnc-club.ru/forum/viewtopic.php?t=35>`_.
   
Creating g-codes from sample graphics
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The current distribution consists of an already prepared svg file with a properly assigned center for the coordinate system.
We recommend to use this file as reference for further work.

To generate such a file from scratch, it is good to refer to the documentation provided with the g-code generator plugin to make sure that the transformation is correct.
Failing to do so may cause the plotting head to drive beyond the physical drawing area which may lead to damage in the plotter's mechanical assembly. 

Launch Inkscape and open the reference image named ``gcode.svg`` from the ``tools`` folder. 
This file has a starting point for the coordinate systems assigned; it is in the top left corner of the drawing area.

This will be called the ``home`` position for plotter.

By default the plotter starts and ends drawing in the ``home`` position.

It is now time to release your creativity - draw something.
Your Inkscape window should look similar to the figure below. 

.. image:: antmicro-symbol.png

Now select all the elements of your image and group them (:menuselection:`Object --> Group`).
Next, convert the object to path with :menuselection:`Path --> Object to path`.
Lastly, choose :menuselection:`Extensions --> Gcodetools --> Path to gcode`.
Switch to the :guilabel:`Preferences` tab and edit the :guilabel:`Directory` to decide where to save your g-codes. 
Next select the :menuselection:`Path to Gcode` tab and click :menuselection:`Apply`.

Before you use the g-code file generated this way to drive the plotter, you have to convert them to a binary file suitable for placing in memory, so that eCos will be able to read it.

The default approach assumes that the whole set of g-codes is placed in memory before the drawing process begins.
Then eCos software starts reading the memory and executing the g-code commands one by one. 

.. warning:: 
   
   Currently only a basic set of g-codes is implemented. The plotter is able to execute the following gcodes:
   
   * G00 - Rapid linear move
   * G01 - Feed linear move
   * G02 - Circular move CW (Arcs only)
   * G03 - Circular move CCW (Arcs only)
   * G04 - Delay

Creating the binary file
~~~~~~~~~~~~~~~~~~~~~~~~

The ``g2b.py`` Python script in the ``tools`` directory converts the g-codes file to the binary file that needs to be placed into memory.
It is executed as follows:

.. code-block:: bash 

   ./python g2b.py <image_filename>
   
The script creates an ``image.bin`` output binary file.
This file includes values from your g-code file.

Every instruction is divided into 7 fields:

.. csv-table::
   :header: Position, Field, Type, Description

   1,GCode, INT, G-code number
   2,X value, FLOAT, Position in X Axis
   3,Y value, FLOAT, Position in Y Axis
   4,Z value, FLOAT, Position in Z Axis
   5,I value, FLOAT, Center of circle in X Axis
   6,J value, FLOAT, Center of circle in Y Axis
   7,Parameter value, INT, Additional parameter (Delay)
   
If a g-code has no value for a parameter its value is set to ``0xffffffff``.

Prepare the system for running Plotter application
--------------------------------------------------

Currently the image data is loaded into the DRAM memory space which must be separated from the Linux memory space.
This prevents image data corruption by applications running in the Cortex-A5 Linux.

To do this you have to reduce the default memory size assigned to Linux, which may be obtained by modifying the Linux boot arguments at U-Boot.

Start your system and enter U-Boot, then type

.. code-block:: bash 

   set memargs mem=240M
   save
   reset

This will shrink the DDR memory available for Linux into 240MB leaving a 16MB buffer for storing image data shared between Linux and eCos. In terms of the Vybrid memory map it introduces the following partitioning:
   
.. csv-table::
   :header: Position, Start Addr, End Addr, Description
   
   1, 0x80000000, 0x8EFFFFFF, Linux RAM memory
   2, 0x8F000000, 0x8F0003FF, NVIC Vector table
   3, 0x8F000400, 0x8F9FFFF3, ~10MB for Cortex-M4 eCos purposes
   4, 0x8F9FFFF4, 0x8F9FFFF7, 4 bytes. Change to ``0xDEADBEEF`` when plotter aplication is started
   5, 0x8F9FFFF8, 0x8F9FFFFB, 4 bytes. Instruction code from Linux
   6, 0x8F9FFFFC
