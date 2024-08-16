#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mailx:comm.mk	1.2.4.8"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

# "@(#)comm.mk	1.11 'attmail mail(1) command'"
#
# mailx -- a modified version of a University of California at Berkeley
#	mail program
#
# for standard Unix
#

# This is the common makefile for all versions.
# This file is "include"d by the other makefiles.

VERSION=4.2mp
HDR=	hdr
HELP=	help
DESTDIR= $(USRBIN)

SRCS=	aux.c cmd1.c cmd2.c cmd3.c cmd4.c cmdtab.c codes.c  collect.c edit.c \
	init.c is.c fio.c getname.c head.c hostname.c lex.c list.c lock.c \
	lpaths.c main.c myfopen.c names.c optim.c popen.c quit.c receipt.c \
	send.c shared.c sigretro.c stralloc.c temp.c translate.c tty.c \
	usg.local.c vars.c version.c

OBJS=	$(SRCS:.c=.o)

HDRS=	$(HDR)/def.h \
	$(HDR)/glob.h \
	$(HDR)/local.h \
	$(HDR)/rcv.h \
	$(HDR)/sigretro.h \
	$(HDR)/uparm.h \
	$(HDR)/usg.local.h

S=	$(SRCS) version.c $(HDRS)

all:	mailx mailx.help mailx.help.~ mailx.rc

mailx:	$S $(OBJS)
	-rm -f mailx
	$(CC) $(LD_FLAGS) -o mailx $(OBJS) $(LD_LIBS)

mailx.help: $(HELP)/mailx.help
	grep -v '^#.*@(' $(HELP)/mailx.help > mailx.help

mailx.help.~: help/mailx.help.~
	grep -v '^#.*@(' $(HELP)/mailx.help.~ > mailx.help.~

mailx.rc:
	echo "# global mailx setup file" > mailx.rc

install: ckdirs all symlinks
	$(INS) -f $(DESTDIR) -m 2511 -g mail -u bin mailx
	$(INS) -f $(DESTLIB) -m 644 -u bin -g bin mailx.help
	$(INS) -f $(DESTLIB) -m 644 -u bin -g bin mailx.help.~
	$(INS) -f $(RCDIR) -m 644 -u bin -g bin mailx.rc

version.o:	mailx.mk version.c
	$(CC) -c version.c

version.c:
	echo "char *version=\"$(VERSION)\";" > version.c

clean:
	-rm -f *.o
	-rm -f version.c a.out core makeout* nohup.out

clobber:	clean
	-rm -f mailx mailx.help mailx.help.~ mailx.rc

lintit:	version.c
	$(LINT) $(LINTFLAGS) $(CPPDEFS) $(SRCS)

mailx.cpio:	$(SRCS) $(HDRS) mailx.mk
	@ls $(SRCS) $(HDRS) mailx.mk | cpio -oc > mailx.cpio

listing:
	pr mailx.mk hdr/*.h [a-l]*.c | lp
	pr [m-z]*.c | lp

ckdirs:
	case $(VERS) in \
		SVR4* ) DIRS="$(USRSHARE)/lib/mailx $(DESTLIB) $(USRLIB)/mailx $(RCDIR)" ;; \
		*     ) DIRS="$(DESTLIB)"  ;; \
	esac ; \
	for i in $$DIRS; \
	do \
		if [ ! -d $$i ] ; then mkdir -p $$i ; fi ; \
		$(CH)chmod 775 $$i ; \
		$(CH)chown bin $$i ; \
		$(CH)chgrp mail $$i ; \
	done

symlinks:
	case $(VERS) in \
		SVR4* )	rm -f	$(USRLIB)/mailx/mailx.help \
				$(USRLIB)/mailx/mailx.help.~ \
				$(USRLIB)/mailx/mailx.rc ; \
			$(CH)$(SYMLINK) $(LDESTLIB)/mailx.help \
				$(CH)$(USRLIB)/mailx/mailx.help ; \
			$(CH)$(SYMLINK) $(LDESTLIB)/mailx.help.~ \
				$(CH)$(USRLIB)/mailx/mailx.help.~ ; \
			$(CH)$(SYMLINK) $(LRCDIR)/mailx.rc \
				$(CH)$(USRLIB)/mailx/mailx.rc ; \
			;; \
	esac

chgrp:
	chgrp mail mailx
	chmod g+s mailx
