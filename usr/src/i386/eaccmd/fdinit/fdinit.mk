#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)eac:i386/eaccmd/fdinit/fdinit.mk	1.3"

include	$(CMDRULES)

FILE	= fdinit
SRC	= fdinit.c
OBJ	= $(SRC:.c=.o)
LDFLAGS	= -s

all		: $(FILE) 

install: all
	$(INS) -f $(SBIN) -m 755 -u bin -g bin $(FILE)

clobber: clean
	rm -f $(FILE)

clean:
	rm -f $(OBJ)

$(FILE): $(OBJ) $(LIBS)
	$(CC) $(OBJ) -o $(FILE) $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

$(OBJ): $(HDRS)
