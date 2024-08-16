#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#

#ident	"@(#)ypcmd:ypcmd.mk	1.6.19.7"
#ident  "$Header: $"

include $(CMDRULES)

#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# PROPRIETARY NOTICE (Combined)
#
# This source code is unpublished proprietary information
# constituting, or derived under license from AT&T's UNIX(r) System V.
# In addition, portions of such source code were derived from Berkeley
# 4.3 BSD under license from the Regents of the University of
# California.
#
#
#
# Copyright Notice 
#
# Notice of copyright on this source code product does not indicate 
# publication.
#
#     (c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
#     (c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
#     (c) 1990,1991  UNIX System Laboratories, Inc.
#     All rights reserved.
# 

LDLIBS  = -lnsl -lgen 

BIN = ypmatch ypwhich ypcat 
SBIN = makedbm yppoll yppush ypset ypxfr ypalias mkalias stdhosts \
	stdethers ypshad2pwd
SCRIPTS = ypinit ypxfr_1day ypxfr_2day ypxfr_1hour
YPSVC = ypbind ypserv ypupdated udpublickey
YPALIAS= yp_getalias.o getlist.o

YPSERVOBJ = ypserv.o ypserv_ancil.o ypserv_map.o ypserv_proc.o \
	$(YPALIAS)
YPBINDOBJ = yp_b_svc.o yp_b_subr.o pong.o $(YPALIAS)
YPUPDOBJ = ypupdated.o openchild.o

SUBDIRS= yppasswd revnetgroup mknetid msgs

NETSVC = $(USRLIB)/netsvc
NETYP  = $(NETSVC)/yp
YP = $(VAR)/yp
BINDINGS= $(YP)/binding

VARFILES= ./net_files/aliases ./net_files/updaters \
	./net_files/Makefile ./net_files/YPMAPS ypxfr_1day ypxfr_2day ypxfr_1hour
INITD=$(ETC)/init.d
RC2D=$(ETC)/rc2.d
RC1D=$(ETC)/rc1.d
RC0D=$(ETC)/rc0.d
NISRC=./init.d/nis
STARTNIS=$(RC2D)/S80nis
STOPNIS1=$(RC1D)/K80nis
STOPNIS0=$(RC0D)/K80nis


all: $(BIN) $(SBIN) $(SCRIPTS) $(YPSVC) $(ETCFILES) $(VARFILES) \
		subdirs init.d/nis

ypserv: $(YPSERVOBJ)
	$(CC) -o $@ $(YPSERVOBJ) $(LDFLAGS) $(LDLIBS) $(SHLIBS)
	
ypbind: $(YPBINDOBJ)
	$(CC) -o $@ $(YPBINDOBJ) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

ypupdated: $(YPUPDOBJ)
	$(CC) -o $@ $(YPUPDOBJ) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

udpublickey: udpublickey.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)
makedbm yppush: $$@.o $(YPALIAS)
	$(CC) -o $@ $@.o $(YPALIAS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

ypxfr: $$@.o ypalias.o $(YPALIAS)
	$(CC) -o $@ $@.o ypalias.o $(YPALIAS) \
		$(LDFLAGS) $(LDLIBS) $(SHLIBS)

ypalias: ypaliasm.o $(YPALIAS)
	$(CC) -o $@ ypaliasm.o $(YPALIAS) \
		$(LDFLAGS) $(LDLIBS) $(SHLIBS)

$(BIN) yppoll ypset: $$@.o $(YPALIAS)
	$(CC) -o $@ $@.o $(YPALIAS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

stdhosts: $$@.o 
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

stdethers: $$@.o 
	$(CC) -o $@ $@.o $(LDFLAGS) -lsocket $(LDLIBS)  $(SHLIBS) 

mkalias: $$@.o 
	$(CC) -o $@ $@.o $(LDFLAGS) -lsocket $(LDLIBS) -L $(ROOT)/$(MACH)/usr/ucblib -lucb $(SHLIBS) 

ypshad2pwd: $$@.o 
	$(CC) -o $@ $@.o -lgen $(NOSHLIBS)

subdirs:
	for i in $(SUBDIRS) ; \
	do \
		cd $$i; $(MAKE) -f $$i.mk all ; cd ..; \
	done

getlist.o: getlist.c \
	$(INC)/stdio.h 

makedbm.o: makedbm.c \
	$(INC)/rpcsvc/dbm.h \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/file.h \
	$(INC)/sys/param.h \
	$(INC)/sys/stat.h \
	$(INC)/ctype.h \
	ypdefs.h 

openchild.o: openchild.c \
	$(INC)/stdio.h 

pong.o: pong.c \
	$(INC)/rpc/rpc.h \
	yp_b.h \
	$(INC)/rpcsvc/yp_prot.h 

yp_b_subr.o: yp_b_subr.c \
	$(INC)/signal.h \
	$(INC)/stdio.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpcsvc/yp_prot.h \
	$(INC)/dirent.h \
	$(INC)/sys/wait.h \
	yp_b.h 

yp_b_svc.o: yp_b_svc.c \
	$(INC)/stdio.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpc/nettype.h \
	yp_b.h \
	$(INC)/netconfig.h 

ypcat.o: ypcat.c \
	$(INC)/stdio.h \
	$(INC)/rpc/rpc.h \
	yp_b.h \
	$(INC)/rpcsvc/ypclnt.h \
	$(INC)/rpcsvc/yp_prot.h 

ypmatch.o: ypmatch.c \
	$(INC)/stdio.h \
	$(INC)/rpc/rpc.h \
	$(INC)/sys/socket.h \
	$(INC)/rpcsvc/yp_prot.h \
	$(INC)/rpcsvc/ypclnt.h 

yppoll.o: yppoll.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/rpc/rpc.h \
	$(INC)/sys/socket.h \
	$(INC)/rpcsvc/ypclnt.h \
	$(INC)/rpcsvc/yp_prot.h \
	yp_b.h 

yppush.o: yppush.c \
	$(INC)/stdio.h \
	$(INC)/errno.h \
	$(INC)/signal.h \
	$(INC)/sys/types.h \
	$(INC)/sys/wait.h \
	$(INC)/sys/stat.h \
	$(INC)/ctype.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpc/rpcb_prot.h \
	$(INC)/rpc/rpcb_clnt.h \
	$(INC)/rpcsvc/ypclnt.h \
	$(INC)/rpcsvc/yp_prot.h \
	yp_b.h \
	ypdefs.h

ypserv.o: ypserv.c \
	ypsym.h \
	$(INC)/sys/ioctl.h \
	$(INC)/sys/file.h \
	ypdefs.h 

ypserv_ancil.o: ypserv_ancil.c \
	ypsym.h \
	ypdefs.h \
	$(INC)/dirent.h 

ypserv_map.o: ypserv_map.c \
	ypsym.h \
	ypdefs.h \
	$(INC)/dirent.h \
	$(INC)/ctype.h 

ypserv_proc.o: ypserv_proc.c \
	ypsym.h \
	ypdefs.h \
	$(INC)/ctype.h 

ypset.o: ypset.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpcsvc/ypclnt.h \
	yp_b.h \
	$(INC)/rpcsvc/yp_prot.h 

ypupdated.o: ypupdated.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/file.h \
	$(INC)/sys/wait.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpc/auth_des.h \
	$(INC)/rpc/nettype.h \
	$(INC)/rpcsvc/ypupd.h \
	$(INC)/rpcsvc/ypclnt.h 

ypwhich.o: ypwhich.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/rpc/rpc.h \
	yp_b.h \
	$(INC)/rpcsvc/yp_prot.h \
	ypv2_bind.h \
	$(INC)/rpcsvc/ypclnt.h 

ypxfr.o: ypxfr.c \
	$(INC)/rpcsvc/dbm.h \
	$(INC)/stdio.h \
	$(INC)/errno.h \
	$(INC)/ctype.h \
	$(INC)/dirent.h \
	$(INC)/rpc/rpc.h \
	$(INC)/sys/socket.h \
	$(INC)/sys/file.h \
	$(INC)/sys/stat.h \
	$(INC)/rpcsvc/ypclnt.h \
	ypdefs.h \
	$(INC)/rpcsvc/yp_prot.h \
	yp_b.h

udpublickey.o: udpublickey.c

ypalias.o: ypalias.c \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/limits.h \
	$(INC)/sys/types.h \
	$(INC)/sys/statvfs.h \
	ypsym.h

ypaliasm.o: ypaliasm.c \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/limits.h \
	$(INC)/sys/types.h \
	$(INC)/sys/statvfs.h \
	ypsym.h
	$(CC) $(CFLAGS) $(DEFLIST) -DMAIN -c ypaliasm.c

ypaliasm.c: ypalias.c
	[ -f $@ ] || ln -s $? $@

mkalias.o: mkalias.c \
	$(INC)/sys/file.h \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/rpcsvc/dbm.h \
	ypdefs.h
	$(CC) $(CFLAGS) $(DEFLIST) -I $(INC) -I $(ROOT)/$(MACH)/usr/ucbinclude -c mkalias.c

stdethers.o: stdethers.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/socket.h \
	$(INC)/net/if.h \
	$(INC)/netinet/in.h \
	$(INC)/netinet/if_ether.h

stdhosts.o: stdhosts.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/netinet/in.h

ypshad2pwd: ypshad2pwd.c \
	$(INC)/unistd.h \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/signal.h \
	$(INC)/ctype.h \
	$(INC)/pwd.h \
	$(INC)/shadow.h \
	$(INC)/sys/types.h \
	$(INC)/string.h \
	$(INC)/sys/socket.h \
	$(INC)/sys/file.h \
	$(INC)/errno.h \
	$(INC)/sys/stat.h

install: all
	[ -d $(NETSVC) ] || mkdir -p $(NETSVC)
	[ -d $(NETYP) ] || mkdir -p $(NETYP)
	$(INS) -f $(USRBIN) ypmatch
	$(INS) -f $(USRBIN) ypcat
	$(INS) -f $(USRBIN) ypwhich
	$(INS) -f $(NETYP) ypserv
	$(INS) -f $(USRSBIN) ypalias
	$(INS) -f $(USRSBIN) makedbm 
	$(INS) -f $(USRSBIN) yppoll
	$(INS) -f $(USRSBIN) yppush
	$(INS) -f $(USRSBIN) ypset
	$(INS) -f $(USRSBIN) ypxfr
	$(INS) -f $(USRSBIN) ypinit
	$(INS) -f $(NETYP) ypbind 
	$(INS) -f $(NETYP) ypupdated
	$(INS) -f $(USRSBIN) udpublickey
	[ -d $(YP) ] || mkdir -p $(YP)
	$(INS) -f $(YP) stdhosts
	$(INS) -f $(YP) stdethers
	$(INS) -f $(YP) mkalias
	$(INS) -f $(YP) ypshad2pwd
	[ -d $(BINDINGS) ] || mkdir -p $(BINDINGS)
	for i in $(VARFILES) ; \
	do \
		$(INS) -f $(YP) -m 0444 -u root -g sys $$i ; \
	done 
	for i in $(SUBDIRS) ; \
	do \
		cd $$i; $(MAKE) -f $$i.mk install ; cd ..; \
	done
	[ -d $(RC0D) ] || mkdir -p $(RC0D)
	[ -d $(RC1D) ] || mkdir -p $(RC1D)
	[ -d $(RC0D) ] || mkdir -p $(RC0D)
	[ -d $(INITD) ] || mkdir -p $(INITD)
	$(INS) -f $(INITD) -m 0444 -u root -g sys init.d/nis
	rm -f $(STARTNIS) $(STOPNIS1) $(STOPNIS0)
	-ln $(INITD)/nis $(STARTNIS)
	-ln $(INITD)/nis $(STOPNIS1)
	-ln $(INITD)/nis $(STOPNIS0)

clean: 
	-rm -f getlist.o makedbm.o openchild.o \
		$(YPSERVOBJ) $(YPBINDOBJ) ypcat.o ypmatch.o yppoll.o \
		ypupdated.o ypwhich.o ypxfr.o yppush.o ypset.o ypaliasm.c \
		udpublickey.o ypshad2pwd.o stdhosts.o stdethers.o mkalias.o \
		ypalias.o ypaliasm.o
	for i in $(SUBDIRS) ; \
	do \
		cd $$i; $(MAKE) -f $$i.mk $@ ; cd ..; \
	done

clobber: clean
	-rm -f $(BIN) $(SBIN) $(SCRIPTS) $(YPSVC)
	for i in $(SUBDIRS) ; \
	do \
		cd $$i; $(MAKE) -f $$i.mk $@ ; cd ..; \
	done

lintit:
	$(LINT) $(LINTFLAGS) $(YPSERVOBJ:.o=.c) 
	$(LINT) $(LINTFLAGS) $(YPBINDOBJ:.o=.c)
	$(LINT) $(LINTFLAGS) $(YPUPDOBJ:.o=.c)
	$(LINT) $(LINTFLAGS) ypmatch.c
	$(LINT) $(LINTFLAGS) ypwhich.c
	$(LINT) $(LINTFLAGS) ypcat.c
	$(LINT) $(LINTFLAGS) yppoll.c
	$(LINT) $(LINTFLAGS) yppush.c
	$(LINT) $(LINTFLAGS) ypset.c
	$(LINT) $(LINTFLAGS) ypxfr.c yp_getalias.c getlist.c
	$(LINT) $(LINTFLAGS) makedbm.c yp_getalias.c getlist.c

