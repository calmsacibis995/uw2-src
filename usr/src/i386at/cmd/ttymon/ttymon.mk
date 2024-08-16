#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ttymon:i386at/cmd/ttymon/ttymon.mk	1.17.14.8"

include $(CMDRULES)

#
# ttymon.mk: makefile for ttymon and stty
#

OWN = root
GRP = sys

LOCALDEF =

# If debug is needed then add -DDEBUG to following line
# If trace is needed then add -DTRACE to following line
LDFLAGS = $(LLDFLAGS)
LDLIBS = -ladm -liaf -lcmd -lgen
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
	tmutil.c \
	tmvt.c \
	tmexpress.c \
	sttytable.c \
	sttyparse.c

TTYADMSRC= \
	ttyadm.c \
	tmutil.c \
	admutil.c 

STTYDEFSSRC= \
	sttydefs.c \
	admutil.c \
	tmttydefs.c \
	tmparse.c \
	sttytable.c \
	sttyparse.c

TTYMONEXECSRC= \
	ttymonexec.c  \
	tmlog.c  \
	tmglobal.c  \
	tmutil.c 

HDR = \
	ttymon.h \
	tmstruct.h \
	tmextern.h \
	stty.h 

TTYMONOBJ= $(TTYMONSRC:.c=.o)

TTYADMOBJ= $(TTYADMSRC:.c=.o)

STTYDEFSOBJ= $(STTYDEFSSRC:.c=.o)

TTYMONEXECOBJ= $(TTYMONEXECSRC:.c=.o)

PRODUCTS = stty ttymon ttymon.tp ttyadm sttydefs defsak

SUBDIRS = ttymon.notp



all:	local FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "===== $(MAKE) -f $$d.mk all"; \
		 $(MAKE) -f $$d.mk all $(MAKEARGS)); \
	 done

local:	$(PRODUCTS)

stty: 
	$(MAKE) -f stty.mk $(MAKEARGS)

ttymon.tp: $(TTYMONOBJ)
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then \
	 	$(CC) -o $@ $(TTYMONOBJ) $(LDFLAGS) $(LDLIBS) $(SHLIBS) ; \
	else \
	 	$(CC) -o $@ $(TTYMONOBJ) $(LDFLAGS) $(LDLIBS) $(SHLIBS) ; \
	fi

ttymon: $(TTYMONEXECOBJ)
	$(CC) -o $@ $(TTYMONEXECOBJ) $(LDFLAGS) $(LDLIBS) $(SHLIBS)  

ttyadm: $(TTYADMOBJ)
	$(CC) -o $@ $(TTYADMOBJ) $(LDFLAGS) $(LDLIBS) $(SHLIBS)  

sttydefs: $(STTYDEFSOBJ)
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then \
	 	$(CC) -o $@ $(STTYDEFSOBJ) $(LDFLAGS) $(LDLIBS) $(SHLIBS) ; \
	else \
	 	$(CC) -o $@ $(STTYDEFSOBJ) $(LDFLAGS) $(LDLIBS) $(SHLIBS) ; \
	fi

# defsak is conditionally made if its makefile exists.  Its source and makefile
# are only available with the B2 enhanced security package
defsak:
	@if [ -s defsak.mk ] ; \
	then \
	 	echo "\n\tmake -f defsak.mk" ; \
		$(MAKE) -f defsak.mk $(MAKEARGS) ; \
	fi

lintit:
	$(LINT) $(LINTFLAGS) $(TTYMONSRC)
	$(LINT) $(LINTFLAGS) $(TTYADMSRC)
	$(LINT) $(LINTFLAGS) $(STTYDEFSSRC)

ttymon.o: ttymon.c \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/fcntl.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/poll.h \
	$(INC)/string.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/resource.h \
	$(INC)/limits.h \
	$(INC)/pwd.h \
	$(INC)/grp.h \
	$(INC)/mac.h \
	$(INC)/priv.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/sac.h \
	ttymon.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tp.h \
	tmstruct.h \
	tmextern.h

tmglobal.o: tmglobal.c \
	$(INC)/stdio.h \
	$(INC)/poll.h \
	$(INC)/signal.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/resource.h \
	$(INC)/sac.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tp.h \
	tmstruct.h \
	ttymon.h

tmhandler.o: tmhandler.c \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/poll.h \
	$(INC)/termio.h $(INC)/sys/termio.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/wait.h \
	$(INC)/priv.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/termios.h \
	$(INC)/pfmt.h \
	ttymon.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tp.h \
	tmstruct.h \
	tmextern.h \
	$(INC)/sac.h

tmpmtab.o: tmpmtab.c \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/sys/types.h \
	$(INC)/ctype.h \
	$(INC)/string.h \
	$(INC)/pwd.h \
	$(INC)/grp.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/pfmt.h \
	$(INC)/priv.h \
	ttymon.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tp.h \
	tmstruct.h \
	tmextern.h

tmttydefs.o: tmttydefs.c \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/string.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/termio.h $(INC)/sys/termio.h \
	$(INC)/sys/stermio.h \
	$(INC)/sys/termiox.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/stream.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/pfmt.h \
	ttymon.h \
	$(INC)/sys/tp.h \
	tmstruct.h \
	stty.h

tmparse.o: tmparse.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h

tmsig.o: tmsig.c \
	$(INC)/stdio.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	tmextern.h

tmsac.o: tmsac.c \
	$(INC)/stdlib.h \
	$(INC)/stdio.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/fcntl.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/string.h \
	$(INC)/unistd.h \
	$(INC)/pfmt.h \
	ttymon.h \
	$(INC)/sac.h

tmchild.o: tmchild.c \
	$(INC)/stdlib.h \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/sys/types.h \
	$(INC)/termio.h $(INC)/sys/termio.h \
	$(INC)/string.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/poll.h \
	$(INC)/unistd.h \
	$(INC)/iaf.h \
	$(INC)/priv.h \
	$(INC)/pfmt.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/resource.h \
	$(INC)/sac.h \
	ttymon.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tp.h \
	tmstruct.h \
	tmextern.h \
	$(INC)/sys/utsname.h

tmautobaud.o: tmautobaud.c \
	$(INC)/stdio.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/fcntl.h \
	$(INC)/termio.h $(INC)/sys/termio.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stropts.h \
	$(INC)/priv.h \
	$(INC)/pfmt.h

tmterm.o: tmterm.c \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/stdio.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/termio.h $(INC)/sys/termio.h \
	$(INC)/sys/stermio.h \
	$(INC)/sys/termiox.h \
	$(INC)/string.h \
	$(INC)/ctype.h \
	$(INC)/priv.h \
	$(INC)/pfmt.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/signal.h \
	ttymon.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tp.h \
	tmstruct.h

tmvt.o: tmvt.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/string.h \
	$(INC)/signal.h \
	$(INC)/fcntl.h \
	$(INC)/sys/kd.h \
	$(INC)/sys/vt.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/stat.h \
	$(INC)/errno.h \
	$(INC)/pfmt.h

tmutmp.o: tmutmp.c \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/sys/types.h \
	$(INC)/string.h \
	$(INC)/memory.h \
	$(INC)/utmp.h \
	$(INC)/priv.h \
	$(INC)/pfmt.h \
	$(INC)/sac.h

tmpeek.o: tmpeek.c \
	$(INC)/stdio.h \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/ctype.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/termio.h \
	$(INC)/poll.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/pfmt.h \
	$(INC)/priv.h \
	ttymon.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tp.h \
	tmstruct.h \
	tmextern.h

tmlog.o: tmlog.c \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/string.h \
	$(INC)/sys/types.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/priv.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tp.h \
	$(INC)/sys/stat.h \
	ttymon.h \
	tmstruct.h \
	tmextern.h

tmutil.o: tmutil.c \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/stdio.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/sys/types.h \
	$(INC)/ctype.h \
	$(INC)/string.h \
	$(INC)/sys/stat.h \
	$(INC)/fcntl.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/sad.h \
	$(INC)/mac.h \
	$(INC)/priv.h \
	$(INC)/pfmt.h \
	$(INC)/poll.h \
	ttymon.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tp.h \
	tmstruct.h

tmexpress.o: tmexpress.c \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/unistd.h \
	$(INC)/fcntl.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/ctype.h \
	$(INC)/string.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/sys/stat.h \
	$(INC)/utmp.h \
	$(INC)/stropts.h \
	$(INC)/poll.h \
	$(INC)/pfmt.h \
	ttymon.h \
	tmextern.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tp.h \
	tmstruct.h \
	$(INC)/mac.h \
	$(INC)/priv.h

ttyadm.o: ttyadm.c \
	$(INC)/stdio.h \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/sys/types.h \
	$(INC)/ctype.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tp.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/priv.h \
	tmstruct.h \
	ttymon.h

admutil.o: admutil.c \
	$(INC)/stdio.h \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/sys/types.h \
	$(INC)/ctype.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tp.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	tmstruct.h \
	ttymon.h

sttydefs.o: sttydefs.c \
	$(INC)/stdio.h \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/sys/types.h \
	$(INC)/ctype.h \
	$(INC)/string.h \
	$(INC)/termio.h $(INC)/sys/termio.h \
	$(INC)/sys/stat.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tp.h \
	$(INC)/sys/mac.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	tmstruct.h \
	ttymon.h

ttymonexec.o: ttymonexec.c \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/locale.h \
	$(INC)/deflt.h \
	$(INC)/sys/types.h \
	$(INC)/pfmt.h


install: localinstall FRC
	@for d in $(SUBDIRS); do \
		cd $$d; \
		echo "===== $(MAKE) -f $$d.mk install"; \
		$(MAKE) -f $$d.mk install $(MAKEARGS); \
	done
	@echo ===== ttymon installed at `date`

localinstall: local FRC
	 $(INS) -o -f $(USRLIB)/saf -m 0544 -u $(OWN) -g $(GRP) ttymon
	 $(INS) -o -f $(USRLIB)/saf -m 0544 -u $(OWN) -g $(GRP) ttymon.tp
	[ ! -f $(ETC)/getty ] || mv $(ETC)/getty $(USRSBIN)/OLDgetty 
	-$(SYMLINK) /usr/lib/saf/ttymon $(ETC)/getty
	 $(INS) -f $(USRSBIN) -m 0755 -u $(OWN) -g $(GRP) sttydefs
	 $(INS) -f $(USRSBIN) -m 0755 -u $(OWN) -g $(GRP) ttyadm
	@if [ -s defsak ] ; \
	then \
	 	echo "\n\tmake -f defsak.mk install" ; \
	 	$(MAKE) -f defsak.mk $(MAKEARGS) install ; \
	fi
	$(MAKE) -f stty.mk $(MAKEARGS) install
	[ -d $(ETC)/default ] || mkdir -p $(ETC)/default
	cp tpath.dfl $(ETC)/default/tpath
	@echo "========== $(INS) done!"

clean:
	-rm -f *.o
	
clobber: clean
	-rm -f $(PRODUCTS)



FRC:
