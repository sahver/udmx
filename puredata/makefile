# -*- mode: makefile; -*-

# Makefile - build control for the pdwiringPi external for the Raspberry Pi.
# Copyright (c) 2014, Garth Zeglin.  All rights reserved.  Provided under the terms of the BSD 3-clause license.

# ---- user-customizable settings ----------------------------------

# Configure the include path for Pure Data. Under Debian or Ubuntu, the pd
# include files can be found on the standard system include path.

PD_INCLUDE_PATH=/usr/include

# ---- The following settings do not usually need to be changed ----

# Specify the extension to use for the loadable module.
EXTERNALS_EXT = pd_linux

# Specify a folder to hold the compiled binary Pd loadable modules.
EXTERNALS_DIR = uDMX

# Specify the default targets to build.
default: $(EXTERNALS_DIR) $(EXTERNALS_DIR)/uDMX.$(EXTERNALS_EXT)

# Create the target folder if it doesn't exist.
$(EXTERNALS_DIR):
	mkdir $(EXTERNALS_DIR)

# Define the compile and link flags for producing a loadable module.
MODULE_CFLAGS = -fPIC -shared -I$(PD_INCLUDE_PATH)

# Build the loadable module
$(EXTERNALS_DIR)/uDMX.$(EXTERNALS_EXT): uDMX.c
	$(CC) `libusb-config --cflags` $(MODULE_CFLAGS) -c $^ -o $(EXTERNALS_DIR)/uDMX.o 
	$(CC) -undefined $(MODULE_CFLAGS) -o $@ $(EXTERNALS_DIR)/uDMX.o `libusb-config --libs`
	rm $(EXTERNALS_DIR)/uDMX.o

# Target to clean up the build folder.
clean: $(EXTERNALS_DIR)
	-rm -r $(EXTERNALS_DIR)/*
