#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/mailproc/mailproc.mk	1.1"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/mailproc/makefile	1.1.2.1"
#ident "@(#)makefile	1.4 'attmail mail(1) command'"
# from ferret SCCSid makefile	3.1

INS= install
USRBIN= /usr/bin
USRLIB= /usr/lib
ETC= /etc

VERS=-DSVR4
VERS2= -DSVR4_1
LOCALINC= -I..
LOCALDEF= $(VERS) $(VERS2)
CPPFLAGS= $(LOCALINC) $(LOCALDEF)
CFLAGS= $(CPPFLAGS)
LDFLAGS=

include $(CMDRULES)

.SUFFIXES: .ln
.c.ln:
	$(LINT) $(LINTFLAGS) $(DEFLIST) -c $*.c > $*.lerr
	
all: mailproc

OBJS = \
    command.o \
    cosh.o \
    cparse.o \
    lparse.o \
    mailproc.o \
    match.o \
    msg.o \
    parse.o \
    process.o

LINTFILES = $(OBJS:.o=.ln)

mailproc: $(OBJS)
	$(CC) $(CFLAGS) -o mailproc \
		command.o cosh.o cparse.o lparse.o mailproc.o match.o msg.o parse.o process.o \
		$(LDFLAGS) -ll -L.. -lmail ../libre.a

command.ln command.o:	command.c mailproc.h
cosh.ln cosh.o:		cosh.c mailproc.h
mailproc.ln mailproc.o:	mailproc.c mailproc.h
match.ln match.o:	match.c mailproc.h
msg.ln msg.o:		msg.c mailproc.h
parse.ln parse.o:	parse.c mailproc.h
process.ln process.o:	process.c mailproc.h

lparse.h cparse.c:	cmd.y
	$(YACC) -d cmd.y
	mv y.tab.c cparse.c
	mv y.tab.h lparse.h

lparse.c:	cmd.l
	$(LEX) cmd.l
	mv lex.yy.c lparse.c

process.ln cparse.o:	cparse.c mailproc.h
process.ln lparse.o:	lparse.c mailproc.h lparse.h

clobber: clean
	rm -f mailproc mailproc.lint

clean:
	rm -f *.o y.tab.c lex.yy.c cparse.c lparse.c lparse.h *.ln *.lerr

lintit: mailproc.lint

mailproc.lint: $(LINTFILES)
	$(LINT) $(LINTFLAGS) $(LINTFILES) > $@

install: mailproc
	$(INS) -f $(USRBIN) -m 2511 -u bin -g mail mailproc

strip:
	strip mailproc

localinstall: mailproc
	$(INS) -f /usr/bin -m 2511 -u bin -g mail mailproc

