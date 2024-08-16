#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ttymon:common/cmd/ttymon/ttymon.notp/ttymon.notp.mk	1.7"

include $(CMDRULES)

#
# ttymon.mk: makefile for non TP version of ttymon 
#

OWN = root
GRP = sys

LOCALDEF =

# If debug is needed then add -DDEBUG to following line
# If trace is needed then add -DTRACE to following line
LDFLAGS = $(LLDFLAGS)
LDLIBS = -ladm -liaf  -lcmd -lgen -lnsl
CFLAGS =

# change the next two lines to compile with -g
# CFLAGS = -g
LLDFLAGS = -s

TTYMONSRC= \
		ttymon.c \
		tmglobal.c \
		tmhandler.c \
		tmpmtab.c \
		tmttydefs.c \
		tmparse.c \
		tmsig.c \
		tmsac.c \
		tmchild.c \
		tmautobaud.c \
		tmterm.c \
		tmutmp.c \
		tmpeek.c \
		tmlog.c \
		tmlock.c \
		tmutil.c \
		tmexpress.c \
		sttytable.c \
		sttyparse.c \
		ulockf.c

TTYMONOBJ= $(TTYMONSRC:.c=.o)


PRODUCTS = ttymon.notp

all: $(PRODUCTS)
	@echo "           $(MAKE) all done!"


ttymon.notp: $(TTYMONOBJ)
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then \
	 	$(CC) -o $@ $(TTYMONOBJ) $(LDFLAGS) $(LDLIBS) $(SHLIBS) ; \
	else \
	 	$(CC) -o $@ $(TTYMONOBJ) $(LDFLAGS) $(LDLIBS) $(SHLIBS) ; \
	fi


lintit:
	$(LINT) $(LINTFLAGS) $(TTYMONSRC)

ttymon.o:	ttymon.c \
		ttymon.h \
		tmstruct.h \
		tmextern.h \
		$(INC)/sac.h \
		$(INC)/pwd.h \
		$(INC)/grp.h \
		$(INC)/poll.h \
		$(INC)/unistd.h \
		$(INC)/stdlib.h \
		$(INC)/locale.h \
		$(INC)/stdio.h \
		$(INC)/time.h \
		$(INC)/errno.h $(INC)/sys/errno.h  \
		$(INC)/fcntl.h \
		$(INC)/signal.h $(INC)/sys/signal.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/types.h \
		$(INC)/sys/stropts.h

tmglobal.o:	tmglobal.c \
		ttymon.h \
		tmstruct.h \
		$(INC)/sac.h \
		$(INC)/poll.h \
		$(INC)/stdio.h

tmhandler.o:	tmhandler.c \
		ttymon.h \
		tmstruct.h \
		tmextern.h \
		$(INC)/sac.h \
		$(INC)/poll.h \
		$(INC)/unistd.h \
		$(INC)/stdlib.h \
		$(INC)/stdio.h \
		$(INC)/errno.h $(INC)/sys/errno.h  \
		$(INC)/fcntl.h \
		$(INC)/time.h \
		$(INC)/signal.h $(INC)/sys/errno.h \
		$(INC)/termio.h $(INC)/sys/termio.h \
		$(INC)/sys/types.h \
		$(INC)/sys/stropts.h 

tmpmtab.o:	tmpmtab.c \
		ttymon.h \
		tmstruct.h \
		tmextern.h \
		$(INC)/pwd.h \
		$(INC)/grp.h \
		$(INC)/unistd.h \
		$(INC)/stdlib.h \
		$(INC)/stdio.h \
		$(INC)/ctype.h \
		$(INC)/signal.h $(INC)/sys/signal.h \
		$(INC)/string.h

tmttydefs.o:	tmttydefs.c \
		ttymon.h \
		tmstruct.h \
		$(INC)/unistd.h \
		$(INC)/stdlib.h \
		$(INC)/stdio.h \
		$(INC)/ctype.h \
		$(INC)/string.h \
		$(INC)/termio.h $(INC)/sys/termio.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/types.h 

tmparse.o:	tmparse.c \
		$(INC)/stdio.h \
		$(INC)/ctype.h

tmsig.o:	tmsig.c \
		tmextern.h \
		$(INC)/stdio.h \
		$(INC)/signal.h $(INC)/sys/signal.h 

tmsac.o:	tmsac.c \
		ttymon.h \
		$(INC)/sac.h \
		$(INC)/stdlib.h \
		$(INC)/stdio.h \
		$(INC)/errno.h $(INC)/sys/errno.h  \
		$(INC)/fcntl.h \
		$(INC)/signal.h $(INC)/sys/signal.h \
		$(INC)/string.h \
		$(INC)/unistd.h \
		$(INC)/sys/types.h

tmchild.o:	tmchild.c \
		ttymon.h \
		tmstruct.h \
		tmextern.h \
		$(INC)/sac.h \
		$(INC)/stdlib.h \
		$(INC)/stdio.h \
		$(INC)/fcntl.h \
		$(INC)/termio.h $(INC)/sys/termio.h \
		$(INC)/string.h \
		$(INC)/signal.h $(INC)/sys/signal.h \
		$(INC)/poll.h \
		$(INC)/unistd.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/types.h \
		$(INC)/sys/stropts.h

tmautobaud.o:	tmautobaud.c \
		$(INC)/stdio.h \
		$(INC)/errno.h $(INC)/sys/errno.h  \
		$(INC)/fcntl.h \
		$(INC)/termio.h $(INC)/sys/termio.h \
		$(INC)/signal.h $(INC)/sys/signal.h \
		$(INC)/sys/types.h \
		$(INC)/sys/stropts.h

tmterm.o:	tmterm.c \
		ttymon.h \
		$(INC)/unistd.h \
		$(INC)/stdlib.h \
		$(INC)/stdio.h \
		$(INC)/errno.h $(INC)/sys/errno.h  \
		$(INC)/ctype.h \
		$(INC)/termio.h $(INC)/sys/termio.h \
		$(INC)/string.h \
		$(INC)/sys/types.h \
		$(INC)/sys/stropts.h

tmutmp.o:	tmutmp.c \
		$(INC)/sac.h \
		$(INC)/utmp.h \
		$(INC)/unistd.h \
		$(INC)/stdlib.h \
		$(INC)/stdio.h \
		$(INC)/string.h \
		$(INC)/memory.h \
		$(INC)/sys/types.h

tmpeek.o:	tmpeek.c \
		ttymon.h \
		tmstruct.h \
		tmextern.h \
		$(INC)/poll.h \
		$(INC)/stdio.h \
		$(INC)/unistd.h \
		$(INC)/stdlib.h \
		$(INC)/errno.h $(INC)/sys/errno.h  \
		$(INC)/ctype.h \
		$(INC)/signal.h $(INC)/sys/signal.h \
		$(INC)/sys/types.h \
		$(INC)/sys/stropts.h

tmlog.o:	tmlog.c \
		ttymon.h \
		tmstruct.h \
		tmextern.h \
		$(INC)/unistd.h \
		$(INC)/stdlib.h \
		$(INC)/stdio.h \
		$(INC)/fcntl.h \
		$(INC)/string.h \
		$(INC)/signal.h $(INC)/sys/signal.h \
		$(INC)/sys/types.h 

tmlock.o:	tmlock.c \
		$(INC)/stdio.h \
		$(INC)/errno.h $(INC)/sys/errno.h  \
		$(INC)/fcntl.h \
		$(INC)/string.h \
		$(INC)/unistd.h 

tmutil.o:	tmutil.c \
		ttymon.h \
		tmstruct.h \
		$(INC)/unistd.h \
		$(INC)/stdlib.h \
		$(INC)/stdio.h \
		$(INC)/ctype.h \
		$(INC)/string.h \
		$(INC)/poll.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/types.h

tmexpress.o:	tmexpress.c \
		ttymon.h \
		tmextern.h \
		tmstruct.h \
		$(INC)/stdio.h \
		$(INC)/stdlib.h \
		$(INC)/unistd.h \
		$(INC)/fcntl.h \
		$(INC)/errno.h $(INC)/sys/errno.h  \
		$(INC)/ctype.h \
		$(INC)/string.h 

ulockf.o:	ulockf.c \
		uucp.h \
		parms.h

ttyadm.o:	ttyadm.c \
		ttymon.h \
		tmstruct.h \
		$(INC)/unistd.h \
		$(INC)/stdlib.h \
		$(INC)/stdio.h \
		$(INC)/ctype.h \
		$(INC)/string.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/types.h

sttydefs.o:	sttydefs.c \
		ttymon.h \
		tmstruct.h \
		$(INC)/stdio.h \
		$(INC)/unistd.h \
		$(INC)/stdlib.h \
		$(INC)/errno.h $(INC)/sys/errno.h  \
		$(INC)/ctype.h \
		$(INC)/termio.h $(INC)/sys/termio.h \
		$(INC)/signal.h $(INC)/sys/signal.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/types.h



install: all
	 $(INS) -o -f $(USRLIB)/saf -m 0544 -u $(OWN) -g $(GRP) ttymon.notp
	@echo "========== $(INS) done!"

clean:
	-rm -f *.o
	
clobber: clean
	-rm -f $(PRODUCTS)
