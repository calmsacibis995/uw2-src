#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)fur:i386/cmd/fur/fur.mk	1.6"
#ident	"$Header: $"

include	$(CMDRULES)

OBJS = rel.o fur.o

CINC = ../sgs/inc/$(CPU)

all: clean $(SGS)fur

install: all
	[ -d $(CCSBIN) ] || mkdir -p $(CCSBIN)
	$(INS) -f $(CCSBIN) -m 755 -u $(OWN) -g $(GRP) $(SGS)fur

$(SGS)fur: $(OBJS)
	$(CC) -I$(CINC) $(LDFLAGS) $(ROOTLIBS) -o $@ $(OBJS) $(LIBELF)

fill.o: fill.c 

rel.o: rel.c \
       $(INC)/libelf.h \
       $(INC)/sys/elf_386.h \
       ./fur.h 
	$(CC) $(CFLAGS) -I$(CINC) $(DEFLIST) -c rel.c

fur.o: fur.c \
	$(INC)/fcntl.h \
	$(INC)/errno.h \
	$(INC)/stdlib.h \
	$(INC)/stdarg.h \
	$(INC)/string.h \
	$(INC)/stdio.h \
	$(INC)/libelf.h \
	./fur.h 
	$(CC) $(CFLAGS) -I$(CINC) $(DEFLIST) -c fur.c


clean:
	-rm -f *.o

clobber: clean
	-rm -f $(SGS)fur
