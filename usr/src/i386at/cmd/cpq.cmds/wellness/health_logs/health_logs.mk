#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cpq.cmds:wellness/health_logs/health_logs.mk	1.3"

include $(CMDRULES)

MAKEFILE = health_logs.mk

BINS = health_logs

PROBEFILE = health_logs.c

all:
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
            
$(BINS):	health_logs.o
		$(CC) -O -s -o $@ health_logs.o $(LDLIBS)

health_logs.o:	health_logs.c \
		$(INC)/sys/cpqw.h \
		$(INC)/sys/cpqw_lib.h \
		$(INC)/sys/crom.h

install: all
	-[ -d $(USRBIN)/compaq/inspect/I30health_logs ] || mkdir -p $(USRBIN)/compaq/inspect/I30health_logs

	$(INS) -f $(USRBIN)/compaq/inspect/I30health_logs -m 0700 -u root -g other health_logs

clean:
	rm -f *.o health_logs

clobber: clean
