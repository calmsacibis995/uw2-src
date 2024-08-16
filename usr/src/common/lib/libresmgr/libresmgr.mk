#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libresmgr:libresmgr.mk	1.1"

include $(LIBRULES)

# 
# Resource Manager Interface Library
#

OWN=root
GRP=sys
OBJECTS = libresmgr.o
LIBA=libresmgr.a
LIBSO=libresmgr.so

all:	$(LIBA)

$(LIBA):	$(OBJECTS)
	$(CC) -G -h $(LIBSO) -o $(LIBSO) $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBA) $(OBJECTS)

libresmgr.o: libresmgr.c \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/fcntl.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/errno.h \
 	$(INC)/sys/resmgr.h \
 	$(INC)/sys/confmgr.h \
 	$(INC)/sys/cm_i386at.h

install: all
	$(INS) -f $(USRLIB) -m 0444 -u $(OWN) -g $(GRP) $(LIBA)
	$(INS) -f $(USRLIB) -m 0444 -u $(OWN) -g $(GRP) $(LIBSO)

clean:
	-rm -f *.o

clobber:	clean
	-rm -f $(LIBA)
	-rm -f $(LIBSO)

