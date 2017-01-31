This is a template SDCC project for the STM8S103F3 development boards.

As is, running 'make' should produce a `STM8S103.hex` file 
in './STM8S103' and `make flash` should upload it to your STM8 board.

The main.c is a simple blinky. There's tons of excessive comments in 
the files, blame ST for that. They're copied from the SPL template 
project. Clean as you see fit.

NB. You may need to adjust paths to SPL folders in the Makefile.
