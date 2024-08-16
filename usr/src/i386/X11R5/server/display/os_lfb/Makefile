#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)os_lfb:os_lfb/Makefile	1.8"

#
# to install this kernel driver manually,
#
#       make install_local
#


CFLAGS= -Xa -D_KERNEL -DPUBLIC= -DPRIVATE=
   CC = $(PFX)cc
   MV = /bin/mv
   RM = /bin/rm -f
   CP = /bin/cp
MKDIR = /bin/mkdir -p

OS_LFB_DIR = $(ROOT)/$(MACH)/usr/X/lib/display/os_lfb

M4FLAGS = 

IDBUILD_ROOT = /etc/conf
IDINSTALL = $(IDBUILD_ROOT)/bin/idinstall
IDBUILD = $(IDBUILD_ROOT)/bin/idbuild

DRIVER_NAME = lfb

M4FLAGS= -DDRIVER_NAME=$(DRIVER_NAME)

# Target
all:	Driver.o

# Generate the driver object from the C source and ensure the interface file
# is also freshly generated
Driver.o: lfb_map.c lfb_map.h
		$(CC) -c $(CFLAGS) lfb_map.c
		$(MV) lfb_map.o Driver.o



# Installation tasks.

Node : Node.sed
	sed -e 's/DRIVER_NAME/$(DRIVER_NAME)/g' < $(@).sed > $(@)

System : System.sed
	sed -e 's/DRIVER_NAME/$(DRIVER_NAME)/g' < $(@).sed > $(@)

Master : Master.sed
	sed -e 's/DRIVER_NAME/$(DRIVER_NAME)/g' < $(@).sed > $(@)

#
# install is used during full builds
#
install:	Master System Node Driver.o
	@if [ -d $(OS_LFB_DIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(OS_LFB_DIR)); fi
	(cd $(OS_LFB_DIR);  $(RM) Makefile Driver.o Master Node System lfb_map.h lfb_map.c) ;
	$(CP) Makefile Driver.o Master Node System lfb_map.h lfb_map.c $(OS_LFB_DIR)/ ;

#
# this is used by individual developers
# or to manually install os_lfb driver
#
install_local:
	if [ -d /etc/conf/pack.d/$(DRIVER_NAME) ]; then\
		$(IDINSTALL) -ku $(DRIVER_NAME);\
	else\
		$(IDINSTALL) -ka $(DRIVER_NAME);\
	fi
	$(IDBUILD) -B	# Force a build

# Cleanliness is next to godliness for some ...
lint:	lfb_map.c lfb_map.h
	lint $(CFLAGS) lfb_map.c > lfb_map.d

clean:
	$(RM) Driver.o lfb_map.o

