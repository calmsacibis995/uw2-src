#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)i286emu:i286emul.mk	1.1.2.1"

include $(CMDRULES)
CFLAGS = -O -I$(INC) -D_STYPES
LDFLAGS = -s
# Warning, the kernel expects i286emul to be in /bin!
INSDIR = $(USRBIN)
INS = install

OFILES1 = main.o utils.o run286.o setdscr.o
OFILES2 = exec.o gethead.o text.o
OFILES4 = sendsig.o Signal.o
OFILES3 = Sbreak.o ipc.o miscsys.o Sioctl.o
OFILES5 = sysent.o syscall.o syscalla.o

OFILES = $(OFILES1) $(OFILES2) $(OFILES3) $(OFILES4) $(OFILES5)

i286emul: $(OFILES)
	$(CC) $(CFLAGS) $(OFILES) -o i286emul $(LDFLAGS) $(SHLIBS)

$(OFILES):      vars.h

install:        i286emul
	$(INS) -f $(INSDIR) -m 755 -u bin -g bin i286emul

clobber:        clean
	rm -f i286emul

clean:
	rm -f $(OFILES)
