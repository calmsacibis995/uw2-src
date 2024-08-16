#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/popper/popper.mk	1.1"

include $(CMDRULES)

.SUFFIXES: .ln

.c.ln:
	$(LINT) $(LINTFLAGS) $(DEFLIST) -c $*.c > $*.lerr

OBJS = \
	pop_dele.o \
	pop_dropcopy.o \
	pop_dropinfo.o \
	pop_get_command.o \
	pop_get_subcommand.o \
	pop_init.o \
	pop_last.o \
	pop_list.o \
	pop_log.o \
	pop_lower.o \
	pop_msg.o \
	pop_parse.o \
	pop_pass.o \
	pop_quit.o \
	pop_rset.o \
	pop_send.o \
	pop_stat.o \
	pop_updt.o \
	pop_user.o \
	pop_xtnd.o \
	pop_xmit.o \
	popper.o

LINTFILES = $(OBJS:.o=.ln)

all: popper

DOCS		=	popper.8

INCLUDES	=	popper.h version.h

MAKEFILE	=	popper.mk

#               Options are:
#               BIND43          -	If you are using BSD 4.3 domain 
#					name service.
#		DEBUG		-	Include the debugging code.  Note:  You
#					still have to use the -d or -t flag to
#					enable debugging.
#               HAVE_VSPRINTF   -	If the vsprintf functions are available 
#					on your system.
#               SYSLOG42        -	For BSD 4.2 syslog (default is BSD 4.3 
#					syslog).
#               STRNCASECMP     -	If you do not have strncasecmp()
#              
# 		NOVELL		-	NOVELL UNIX

CFLAGS		=	-O -DBIND43 -DSTRNCASECMP -DNOVELL -dn 
#CFLAGS		=	-DHAVE_VSPRINTF -D_BSD -DDEBUG -g

#LDFLAGS		=	-lbsd
#LDFLAGS		=	-L $(TOOLS)/usr/ccs/lib -L/usr/ucblib
LDFLAGS		=	-L $(TOOLS)/usr/ccs/lib
#LDLIBS		=	-lucb -lnsl -lsocket -lresolv
LDLIBS		=	-lgen -lcrypt -lnsl -lsocket -lresolv

INSTALLDIR	=	/usr/local/bin

MANPAGE		=	popper.8

CATPAGE		=	popper.0

popper: $(OBJS)
	$(CC) -g -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

install: all
	$(INS) -f $(ROOT)/$(MACH)/usr/lib/mail/surrcmd -m 02555 -u bin -g mail popper

clean:
	rm -f *.o *.lerr *.ln core
	
clobber: clean
	rm -f popper popper.lint

lintit: popper.lint

popper.lint: $(LINTFILES)
	$(LINT) $(LINTFLAGS) $(LINTFILES) > $@

$(OBJS) $(LINTFILES):    popper.h version.h

localinstall: popper
	$(INS) -f /usr/lib/mail/surrcmd -m 02555 -u bin -g mail popper
	
