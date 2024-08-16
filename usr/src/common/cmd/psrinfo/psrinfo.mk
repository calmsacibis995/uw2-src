#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mp.cmds:common/cmd/psrinfo/psrinfo.mk	1.2"

include $(CMDRULES)

OC=$(CFLAGAS)
CFLAGS= ${OC}

OWN = root
GRP = sys

CMD= psrinfo
SOURCES=psrinfo.c

all: $(CMD)

psrinfo: psrinfo.o 
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

psrinfo.o: psrinfo.c 

install: all
	-rm -f $(ETC)/$(CMD)
	-rm -f $(USRSBIN)/$(CMD)
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) $(CMD)
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) $(CMD)
	-$(SYMLINK) /sbin/$(CMD) $(ETC)/$(CMD)

clean:
	rm -f *.o

clobber: clean
	rm -f $(CMD)

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)
