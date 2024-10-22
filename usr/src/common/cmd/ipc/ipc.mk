#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)ipc:ipc.mk	1.7.6.12"
#ident "$Header: ipc.mk 1.3 91/05/23 $"

include $(CMDRULES)


INSDIR = $(USRBIN)
LDLIBS = $(LIBELF)
LINTFLAGS = -x
LOCALDEF = -D_KMEMUSER

all: ipcs ipcrm

ipcs: ipcs.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

ipcrm: ipcrm.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

ipcrm.o: ipcrm.c \
	$(INC)/sys/types.h \
	$(INC)/sys/ipc.h \
	$(INC)/sys/msg.h \
	$(INC)/sys/sem.h \
	$(INC)/sys/shm.h \
	$(INC)/sys/errno.h \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/priv.h $(INC)/sys/privilege.h

ipcs.o: ipcs.c \
	$(INC)/sys/types.h \
	$(INC)/priv.h $(INC)/sys/privilege.h \
	$(INC)/malloc.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/acl.h $(INC)/sys/acl.h \
	$(INC)/sys/ipcsec.h \
	$(INC)/sys/ipc.h \
	$(INC)/sys/msg.h \
	$(INC)/sys/sem.h \
	$(INC)/sys/shm.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/param.h \
	$(INC)/sys/var.h \
	$(INC)/sys/fs/xnamnode.h \
	$(INC)/sys/sd.h \
	$(INC)/nlist.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/time.h \
	$(INC)/grp.h \
	$(INC)/pwd.h \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/unistd.h $(INC)/sys/unistd.h \
	$(INC)/sys/time.h \
	$(INC)/mac.h $(INC)/sys/mac.h \
	$(INC)/sys/ksym.h

install: all
	$(INS) -f $(INSDIR) -m 02555 -u bin -g sys ipcs
	$(INS) -f $(INSDIR) -m 0555 -u bin -g bin ipcrm

clean:
	rm -f *.o

clobber: clean
	rm -f ipcs ipcrm

lintit:
	lint $(LINTFLAGS) ipcs.c
	lint $(LINTFLAGS) ipcrm.c
