#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)pkgtools:pkgtrans.mk	1.5"

include $(CMDRULES)

PROC = pkgtrans
OBJECTS = $(PROC)

## options used to build this command
LOCALINC = -I ../hdrs
LDLIBS = -L ../libinst -linst -L $(BASE)/libpkg -lpkg -L $(BASE)/libadm -ladm -L $(BASE)/libcmd -lcmd -L $(BASE)/include -l stubs -lw -lgen

## objects which make up this process
OFILES=\
	main.o

all:	$(PROC)

$(PROC): $(OFILES)
	$(CC) -o $(PROC) $(OFILES) $(LDFLAGS) $(LDLIBS) $(PERFLIBS)
	chmod 775 $(PROC)

clean:
	rm -f $(OFILES)

clobber: clean
	rm -f $(PROC)

