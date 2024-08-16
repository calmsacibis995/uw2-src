#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nlsadmin:nlsadmin.mk	1.5.8.1"
#ident "$Header: nlsadmin.mk 1.2 91/04/16 $"

include $(CMDRULES)


OWN = root
GRP = adm

SOURCE  = nlsadmin.c 
OBJECTS = nlsadmin.o 

all: nlsadmin

nlsadmin: $(OBJECTS)
	$(CC) -o nlsadmin $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

nlsadmin.o: nlsadmin.c \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/string.h \
	$(INC)/sac.h \
	nlsadmin.h

install: all
	 $(INS) -f $(USRSBIN) -m 0755 -u $(OWN) -g $(GRP) nlsadmin

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f nlsadmin

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)
