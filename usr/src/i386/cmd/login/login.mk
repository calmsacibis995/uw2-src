#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)login:i386/cmd/login/login.mk	1.6.12.12"
#ident  "$Header: login.mk 1.4 91/06/27 $"

include $(CMDRULES)


OWN = root
GRP = bin

#	Where MAINS are to be installed.
INSBASE = $(USRLIB)/iaf
INSDIR = $(INSBASE)/login

LOCALDEF =

CONS 	=  $(LOCALDEF)
LDLIBS = -lcmd -lgen -lcrypt_i -lia -liaf -Bstatic -lsl -Bdynamic

OBJECTS = login.o

all: scheme

scheme: login.o 
	$(CC) -o scheme login.o $(LDFLAGS) $(LDLIBS) $(CONS)

login.o: login.c \
	$(INC)/sys/types.h \
	$(INC)/utmpx.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/pwd.h \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/unistd.h \
	$(INC)/string.h \
	$(INC)/sys/stat.h \
	$(INC)/dirent.h \
	$(INC)/sys/utsname.h \
	$(INC)/utime.h \
	$(INC)/termio.h $(INC)/sys/termio.h \
	$(INC)/sys/stropts.h \
	$(INC)/shadow.h \
	$(INC)/time.h \
	$(INC)/sys/param.h \
	$(INC)/sys/fcntl.h \
	$(INC)/deflt.h \
	$(INC)/grp.h \
	$(INC)/mac.h \
	$(INC)/ia.h \
	$(INC)/sys/vnode.h \
	$(INC)/audit.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/lastlog.h \
	$(INC)/iaf.h \
	$(INC)/priv.h \
	$(INC)/sys/secsys.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tp.h \
	copyright.h
	$(CC) $(CONS) $(CFLAGS) $(DEFLIST) -c login.c

install: scheme $(DIRS)
	[ -d $(INSBASE) ] || mkdir -p $(INSBASE)
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR) ;\
		$(CH) chmod 755 $(INSDIR) ; \
		$(CH) chown bin $(INSDIR)
	[ -d $(ETC)/default ] || mkdir -p $(ETC)/default
	-rm -f $(INSDIR)/scheme
	$(INS) -f $(INSDIR) -m 04550 -u $(OWN) -g $(GRP) scheme
	cp login.dfl $(ETC)/default/login
	
clean:
	rm -f $(OBJECTS)
	
clobber: clean
	rm -f scheme

lintit:
	$(LINT) $(LINTFLAGS) login.c

# 	These targets are useful but optional

partslist:
	@echo login.mk $(LOCALINCS) login.c | tr ' ' '\012' | sort

product:
	@echo scheme | tr ' ' '\012' | \
	sed -e 's;^;$(INSDIR)/;' -e 's;//*;/;g'

productdir:
	@echo $(INSDIR)

srcaudit: # will not report missing nor present object or product files.
	@fileaudit login.mk $(LOCALINCS) login.c -o $(OBJECTS) scheme
