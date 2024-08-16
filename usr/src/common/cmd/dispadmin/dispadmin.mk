#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mp.cmds:common/cmd/dispadmin/dispadmin.mk	1.3"
#ident "$Header: dispadmin.mk 1.3 91/04/08 $"

include $(CMDRULES)
INSDIR = $(USRSBIN)
OWN = bin
GRP = bin
CLASSDIR = $(USRLIB)/class
LDLIBS = -lgen
CFLAGS = -O -I$(INC)

all: dispadmin classes

dispadmin: dispadmin.o subr.o
	$(CC) dispadmin.o subr.o -o $@ $(LDFLAGS) $(LDLIBS) $(SHLIBS)

dispadmin.o: dispadmin.c \
	$(INC)/stdio.h\
	$(INC)/string.h\
	$(INC)/unistd.h\
	$(INC)/sys/types.h\
	$(INC)/sys/priocntl.h

classes: FPdispadmin TSdispadmin FCdispadmin

fpdispadmin.o: fpdispadmin.c\
	$(INC)/stdio.h\
	$(INC)/string.h\
	$(INC)/sys/types.h\
	$(INC)/sys/priocntl.h\
	$(INC)/sys/fppriocntl.h\
	$(INC)/sys/param.h\
	$(INC)/sys/hrtcntl.h\
	$(INC)/sys/fpri.h

FPdispadmin: fpdispadmin.o subr.o
	$(CC) fpdispadmin.o subr.o -o $@ $(LDFLAGS) $(LDLIBS) $(SHLIBS)

fcdispadmin.o: fcdispadmin.c\
	$(INC)/stdio.h\
	$(INC)/string.h\
	$(INC)/sys/types.h\
	$(INC)/sys/priocntl.h\
	$(INC)/sys/fcpriocntl.h\
	$(INC)/sys/param.h\
	$(INC)/sys/hrtcntl.h\
	$(INC)/sys/fc.h

FCdispadmin: fcdispadmin.o subr.o
	$(CC) fcdispadmin.o subr.o -o $@ $(LDFLAGS) $(LDLIBS) $(SHLIBS)

tsdispadmin: tsdispadmin.c\
	$(INC)/stdio.h\
	$(INC)/string.h\
	$(INC)/sys/types.h\
	$(INC)/sys/priocntl.h\
	$(INC)/sys/tspriocntl.h\
	$(INC)/sys/param.h\
	$(INC)/sys/hrtcntl.h\
	$(INC)/errno.h

TSdispadmin: tsdispadmin.o subr.o
	$(CC) tsdispadmin.o subr.o -o $@ $(LDFLAGS) $(LDLIBS) $(SHLIBS)

subr.o: subr.c\
	$(INC)/stdio.h\
	$(INC)/priv.h\
	$(INC)/sys/types.h\
	$(INC)/sys/hrtcntl.h

install: all
	$(INS) -f $(INSDIR) -u $(OWN) -g $(GRP) -m 555 dispadmin
	@[ -d $(CLASSDIR) ] || mkdir $(CLASSDIR)
	@[ -d $(CLASSDIR)/FP ] || mkdir $(CLASSDIR)/FP
	@[ -d $(CLASSDIR)/FC ] || mkdir $(CLASSDIR)/FC
	@[ -d $(CLASSDIR)/TS ] || mkdir $(CLASSDIR)/TS
	$(INS) -f $(CLASSDIR)/FP -u $(OWN) -g $(GRP) -m 555 FPdispadmin
	$(INS) -f $(CLASSDIR)/FC -u $(OWN) -g $(GRP) -m 555 FCdispadmin
	$(INS) -f $(CLASSDIR)/TS -u $(OWN) -g $(GRP) -m 555 TSdispadmin

clean:
	rm -f *.o

clobber: clean
	rm -f dispadmin FPdispadmin TSdispadmin FCdispadmin
