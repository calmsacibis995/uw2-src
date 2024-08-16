#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cpq.cmds:wellness/cpqasrd/cpqasrd.mk	1.6"

include $(CMDRULES)

MAKEFILE = cpqasrd.mk

LDLIBS = -lgen -lcmd

BINS = cpqasrd

PROBEFILE = cpqasrd.c

all:
	> cpqasrd.log
	@if [ -f $(PROBEFILE) ]; then \
		find $(BINS) \( ! -type f -o -links +1 \) \
			-exec echo rm -f {} \; -exec rm -f {} \; 2> /dev/null ;\
		$(MAKE) -f $(MAKEFILE) $(BINS) $(MAKEARGS) ;\
	else \
		if [ ! -r $(BINS) ]; then \
			echo "ERROR: $(BINS) is missing" 1>&2 ;\
			false ;\
			break ;\
		fi \
	fi

$(BINS):	cpqasrd.o
		$(CC) -O -s -o $@ $@.o $(LDLIBS)

cpqasrd.o:	cpqasrd.c \
		$(INC)/sys/cpqw.h

install:	all
	-[ -d $(USRBIN)/compaq/asr ] || mkdir -p $(USRBIN)/compaq/asr
	-[ -d $(ROOT)/$(MACH)/var/spool/compaq/asr ] || mkdir -p $(ROOT)/$(MACH)/var/spool/compaq/asr
	$(INS) -f $(USRBIN)/compaq/asr -m 0700 -u root -g other cpqasrd
	$(INS) -f $(USRBIN)/compaq/asr -m 0700 -u root -g other cpqasrd.etc
	$(INS) -f $(ROOT)/$(MACH)/var/spool/compaq/asr -m 0600 -u root -g other cpqasrd.log

clean:
	rm -f *.o cpqasrd

clobber:	clean
