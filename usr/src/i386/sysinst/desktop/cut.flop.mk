#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:desktop/cut.flop.mk	1.11"
COMPRESS=bzip
LCL_MACH=.$(MACH)
IDCONFUPDATE=$(ROOT)/$(LCL_MACH)/etc/conf/bin/$(PFX)idconfupdate 
HBA_MODS = adsc cpqsc dpt ictha dcd athd mcesdi mcst

all: mip sip unix dcmp resmgr logo.img

mip: $(ROOT)/$(LCL_MACH)/etc/initprog/mip
	$(COMPRESS) -s28k $(ROOT)/$(LCL_MACH)/etc/initprog/mip > mip

sip: $(ROOT)/$(LCL_MACH)/etc/initprog/sip
	$(COMPRESS) -s28k $(ROOT)/$(LCL_MACH)/etc/initprog/sip > sip

unix: $(ROOT)/$(LCL_MACH)/stand/unix
	$(COMPRESS) -s28k $(ROOT)/$(LCL_MACH)/stand/unix > unix

logo.img: $(ROOT)/$(LCL_MACH)/etc/initprog/logo.img
	$(COMPRESS) -s28k $(ROOT)/$(LCL_MACH)/etc/initprog/logo.img > logo.img

dcmp: $(ROOT)/$(LCL_MACH)/etc/initprog/dcmp
	cp $(ROOT)/$(LCL_MACH)/etc/initprog/dcmp dcmp

resmgr: unix
	@cd $(ROOT)/$(LCL_MACH)/etc/conf/sdevice.d; \
	sed -e 's/[ 	]N[ 	]/	Y	/' .save/lp >lp; \
	cp ../mdevice.d/.save/lp ../mdevice.d/lp; \
	for i in $(HBA_MODS); \
	do \
		mv $$i .$$i ; \
	done
	$(IDCONFUPDATE) -s -r $(ROOT)/$(LCL_MACH)/etc/conf -o $(ROOT)/$(LCL_MACH)/stand/resmgr
	@cd $(ROOT)/$(LCL_MACH)/etc/conf/sdevice.d; \
	rm -f lp ../mdevice.d/lp; \
	for i in $(HBA_MODS); \
	do \
		mv .$$i $$i ; \
	done
	$(COMPRESS) -s28k $(ROOT)/$(LCL_MACH)/stand/resmgr > resmgr
