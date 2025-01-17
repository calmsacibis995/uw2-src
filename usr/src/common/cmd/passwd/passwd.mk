#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)passwd:passwd.mk	1.5.7.8"

include $(CMDRULES)

#	Makefile for passwd

OWN = root
GRP = sys

LDLIBS = -lcmd -lcrypt_i -lia -lgen -liaf -lnsl -lsocket

all: passwd passwd.stdin
# passwd.stdin reads the clear-text password from stdin instead of from
# /dev/tty.  passwd.stdin is used only during installation, and it is removed
# from the system during installation.

passwd: passwd.o yppasswd.o
	$(CC) -o passwd passwd.o yppasswd.o $(LDFLAGS) $(LDLIBS) 

passwd.stdin: passwd.o yppasswd.o getpass.o
	$(CC) -o passwd.stdin passwd.o yppasswd.o getpass.o $(LDFLAGS) $(LDLIBS) 

passwd.o: passwd.c \
	$(INC)/stdio.h \
	$(INC)/signal.h \
	$(INC)/pwd.h \
	$(INC)/shadow.h \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/vnode.h \
	$(INC)/time.h \
	$(INC)/string.h \
	$(INC)/ctype.h \
	$(INC)/errno.h \
	$(INC)/crypt.h \
	$(INC)/deflt.h \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/sys/mman.h \
	$(INC)/fcntl.h \
	$(INC)/ia.h \
	$(INC)/audit.h \
	$(INC)/priv.h \
	$(INC)/sys/secsys.h \
	$(INC)/sys/mac.h \
	$(INC)/sys/systeminfo.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h

getpass.c: $(ROOT)/usr/src/$(WORK)/lib/libc/port/stdio/getpass.c
	sed -e 's/open(_str_devtty, O_RDONLY)/0/' -e '/close(fd);/d' \
		$(ROOT)/usr/src/$(WORK)/lib/libc/port/stdio/getpass.c \
		> getpass.c
# The sed line above forces our local version of getpass_r() to read from
# stdin instead of from /dev/tty.

getpass.o: getpass.c
	$(CC) -Xa -D_EFTSAFE -DCALL_TZSET \
		-I$(ROOT)/usr/src/$(WORK)/lib/libc/$(PFX)/inc \
		-I$(ROOT)/usr/src/$(WORK)/lib/libc/port/inc \
		$(CFLAGS) -Kno_host -c getpass.c

install: all
	$(INS) -f $(USRBIN) -m 06555 -u $(OWN) -g $(GRP) passwd
	$(INS) -f $(USRBIN) -m 06555 -u $(OWN) -g $(GRP) passwd.stdin
	-mkdir ./tmp
	-$(CP) passwd.dfl ./tmp/passwd
	$(INS) -f $(ETC)/default -m 0444 -u $(OWN) -g $(GRP) ./tmp/passwd
	-rm -rf ./tmp

clean:
	rm -f passwd.o yppasswd.o getpass.o getpass.c

clobber: clean
	rm -f passwd passwd.stdin

lintit:
	$(LINT) $(LINTFLAGS) passwd.c

#	These targets are useful but optional

partslist:
	@echo passwd.mk passwd.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo passwd | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit passwd.mk $(LOCALINCS) passwd.c -o passwd.o passwd
