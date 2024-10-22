#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)gencat:gencat.mk	1.3.8.2"
#ident  "$Header: gencat.mk 1.5 91/06/27 $"

include $(CMDRULES)

OWN = bin
GRP = bin 

OBJECTS = gencat.o msg_conv.o cat_misc.o cat_build.o cat_mmp_dump.o \
	cat_sco_rd.o cat_sco_wr.o cat_isc_rd.o cat_isc_wr.o
SOURCES = $(OBJECTS:.o=.c)

all: gencat

gencat: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

cat_build.o: cat_build.c \
	$(INC)/dirent.h $(INC)/sys/dirent.h \
	$(INC)/locale.h \
	$(INC)/stdio.h \
	$(INC)/nl_types.h \
	$(INC)/malloc.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/string.h \
	gencat.h

cat_misc.o: cat_misc.c \
	$(INC)/stdio.h \
	$(INC)/nl_types.h \
	$(INC)/pfmt.h \
	$(INC)/string.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	gencat.h

cat_mmp_dump.o: cat_mmp_dump.c \
	$(INC)/dirent.h $(INC)/sys/dirent.h \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/nl_types.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/string.h \
	gencat.h

gencat.o: gencat.c \
	$(INC)/dirent.h $(INC)/sys/dirent.h \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/nl_types.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/string.h \
	gencat.h

msg_conv.o: msg_conv.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/pfmt.h \
	gencat.h

cat_sco_rd.o: cat_sco_rd.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/fcntl.h \
	$(INC)/nl_types.h \
	$(INC)/unistd.h \
	gencat.h

cat_sco_wr.o: cat_sco_wr.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/fcntl.h \
	$(INC)/nl_types.h \
	$(INC)/unistd.h \
	gencat.h

cat_isc_rd.o: cat_isc_rd.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/fcntl.h \
	$(INC)/nl_types.h \
	$(INC)/unistd.h \
	gencat.h

cat_isc_wr.o: cat_isc_wr.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/fcntl.h \
	$(INC)/nl_types.h \
	$(INC)/unistd.h \
	gencat.h

install: all
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) gencat

clean:
	rm -f $(OBJECTS)
	
clobber: clean
	rm -f *.o gencat gencat

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)
