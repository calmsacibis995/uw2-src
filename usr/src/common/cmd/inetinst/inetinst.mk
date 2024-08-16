#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)inetinst:inetinst.mk	1.18"

#
#  Makefile for inetinst - network installation of packages.
#
include $(CMDRULES)
include $(ROOT)/$(MACH)/var/sadm/dist/rel_fullname


ADMDIR = $(VAR)/sadm/dist
XBINDIR = $(USR)/X/bin
XADMDIR = $(USR)/X/adm
XICONDIR = $(USR)/X/lib/pixmaps
XICONBWDIR = $(USR)/X/lib/bitmaps
XCLDBDIR = $(USR)/X/lib/classdb
XINCDIR = $(USR)/X/include
DEVDIR = $(ROOT)/$(MACH)/dev
SUBDIRS = catalogs

REV=1
OWN=root
GRP=bin
LOCALDEF=       -DSYSV -DSTRNET -DBSD_COMP
LDLIBS =	-lsocket -lnsl -ldl -lgen

INSLIB = 	./inetinstlib.a
INSLIBSRC = 	log.c options.c checks.c netio.c
INSLIBOBJS =	$(INSLIBSRC:.c=.o)

INETINST =	in.inetinst
INETINSTOBJ =	inetinst.o do_svc.o

INSTALLSRV =	installsrv
INSTALLSRVOBJ =	installsrv.sh

MKNETFLOP =	mknetflop
MKNETFLOPOBJ =	mknetflop.sh

PKGLIST =	pkglist
PKGLISTOBJ =	pkglist.o

PKGINSTALL =	pkginstall
PKGINSTALLOBJ =	pkginstall.o

PKGCAT =	pkgcat
PKGCATOBJ =	pkgcat.o

PKGCOPY =	pkgcopy
PKGCOPYOBJ =	pkgcopy.o

ADMINTOOLS =	IS_lib.so Install_Server IS_support IS_messages IS_pkgsetup

OBJS = $(PKGCAT) $(PKGCOPY) $(PKGINSTALL) $(PKGLIST) $(INETINST) $(INSTALLSRV) $(MKNETFLOP) $(ADMINTOOLS)
ADMOBJS = .pkgdefaults
MANOBJS = man/pkgcopy.1 man/pkginstall.1 man/pkglist.1 man/inetinst.1

all: $(OBJS)

$(OBJS): inetinst.h inetinst_msgs.h

inetinst_msgs.h:
	cd catalogs; make -f *mk all

$(INSLIB): $(INSLIBOBJS)
	$(AR) $(ARFLAGS) $(INSLIB) $?

$(INSTALLSRV): $(INSTALLSRVOBJ)
	- ( sed -e "s/XYZXYZ/${REL_FULLNAME}/" < $(INSTALLSRVOBJ) > $(INSTALLSRV) ; exit 0;)

$(MKNETFLOP): $(MKNETFLOPOBJ)
	cp $(MKNETFLOPOBJ) $(MKNETFLOP)

$(PKGCAT): $(INSLIB) $(PKGCATOBJ)
	$(CC) -o $(PKGCAT) $(PKGCATOBJ) $(INSLIB) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

$(PKGCOPY): $(INSLIB) $(PKGCOPYOBJ)
	$(CC) -o $(PKGCOPY) $(LDFLAGS) $(PKGCOPYOBJ) $(INSLIB) $(LDLIBS) $(SHLIBS)

$(PKGINSTALL): $(INSLIB) $(PKGINSTALLOBJ)
	$(CC) -o $(PKGINSTALL) $(LDFLAGS) $(PKGINSTALLOBJ) $(INSLIB) $(LDLIBS) $(SHLIBS)

$(PKGLIST): $(INSLIB) $(PKGLISTOBJ)
	$(CC) -o $(PKGLIST) $(LDFLAGS) $(PKGLISTOBJ) $(INSLIB) $(LDLIBS) $(SHLIBS)

$(INETINST): $(INSLIB) $(INETINSTOBJ)
	$(CC) -o $(INETINST) $(LDFLAGS) $(INETINSTOBJ) $(INSLIB) $(LDLIBS) $(SHLIBS)

IS_lib.so : IS_lib.o
	$(LD) -o $@ -G -h $@ IS_lib.o

IS_lib.o : IS_lib.c
	$(CC) -c -I$(XINCDIR) IS_lib.c

Install_Server : Install_Server.sh
	-rm -f Install_Server
	- ( sed -e "s/XYZXYZ/${REL_FULLNAME}/" < Install_Server.sh > Install_Server ; exit 0;)
	chmod 755 Install_Server

IS_pkgsetup : IS_pkgsetup.sh
	-rm -f IS_pkgsetup
	cp IS_pkgsetup.sh IS_pkgsetup
	chmod 755 IS_pkgsetup

IS_support : IS_support.sh
	-rm -f IS_support
	cp IS_support.sh IS_support
	chmod 755 IS_support

IS_messages : IS_messages.sh
	-rm -f IS_messages
	cp IS_messages.sh IS_messages
	chmod 755 IS_messages

install: all
	-[ -d $(ADMDIR) ] || mkdir -p $(ADMDIR) 
	-[ -d $(XBINDIR) ] || mkdir -p $(XBINDIR) 
	-[ -d $(USRSBIN) ] || mkdir -p $(USRSBIN) 
	-[ -d $(USRBIN) ] || mkdir -p $(USRBIN) 
	-[ -d $(XADMDIR) ] || mkdir -p $(XADMDIR) 
	-[ -d $(XICONDIR) ] || mkdir -p $(XICONDIR) 
	-[ -d $(XICONBWDIR) ] || mkdir -p $(XICONBWDIR) 
	-[ -d $(XCLDBDIR) ] || mkdir -p $(XCLDBDIR) 
	-[ -d $(DEVDIR) ] || mkdir -p $(DEVDIR) 
	-[ -r $(DEVDIR)/null ] || touch $(DEVDIR)/null
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) $(INETINST)
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) $(PKGINSTALL)
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) $(PKGCOPY)
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) $(PKGCAT)
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) $(PKGLIST)
	$(INS) -f $(ADMDIR) -m 0555 -u $(OWN) -g $(GRP) $(ADMOBJS)
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) $(INSTALLSRV)
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) $(MKNETFLOP)
	$(INS) -f $(XBINDIR) -m 0555 -u $(OWN) -g $(GRP) Install_Server
	$(INS) -f $(XADMDIR) -m 0555 -u $(OWN) -g $(GRP) IS_lib.so
	$(INS) -f $(XADMDIR) -m 0555 -u $(OWN) -g $(GRP) IS_support
	$(INS) -f $(XADMDIR) -m 0555 -u $(OWN) -g $(GRP) IS_messages
	$(INS) -f $(XADMDIR) -m 0555 -u $(OWN) -g $(GRP) IS_pkgsetup
	$(INS) -f $(XICONDIR) -m 0444 -u $(OWN) -g $(GRP) instsvr.icon
	rm -f $(XICONBWDIR)/instsvr.icon
	cp instbw.icon $(XICONBWDIR)/instsvr.icon
	$(CH)chgrp $(GRP) $(XICONBWDIR)/instsvr.icon
	$(CH)chmod 0444 $(XICONBWDIR)/instsvr.icon
	$(CH)chown $(OWN) $(XICONBWDIR)/instsvr.icon
	$(INS) -f $(XCLDBDIR) -m 0755 -u $(OWN) -g $(GRP) instsvr.cdb
	cd catalogs; make -f *mk install
	
clean:
	rm -f *.a *.o core

clobber: clean
	rm -f $(INSIB) $(OBJS)
