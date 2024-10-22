#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libnsl:common/lib/libnsl/yp/yp.mk	1.10.9.2"
#ident	"$Header: $"

#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#	PROPRIETARY NOTICE (Combined)
#
# This source code is unpublished proprietary information
# constituting, or derived under license from AT&T's UNIX(r) System V.
# In addition, portions of such source code were derived from Berkeley
# 4.3 BSD under license from the Regents of the University of
# California.
#
#
#
#	Copyright Notice 
#
# Notice of copyright on this source code product does not indicate 
#  publication.
#
#	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#          All rights reserved.
# 
#

include $(LIBRULES)
include ../libnsl.rules

.SUFFIXES:	.c .o 

LOCALDEF=-D_NSL_RPC_ABI $(NSL_LOCALDEF)
BINDOBJS= yp_b_clnt.o yp_b_xdr.o
OBJS= dbm.o yp_all.o yp_bind.o yp_enum.o yp_master.o yp_match.o yp_order.o \
	yp_update.o ypupd.o yperr_string.o ypprot_err.o ypxdr.o ypmaint_xdr.o\
	$(BINDOBJS) yp_mt.o

LIBOBJS= ../dbm.o ../yp_all.o ../yp_bind.o ../yp_enum.o\
	../yp_master.o ../yp_match.o ../yp_mt.o ../yp_order.o \
	../yp_update.o ../ypupd.o ../yperr_string.o ../ypprot_err.o \
	../ypxdr.o ../ypmaint_xdr.o\
	../yp_b_clnt.o ../yp_b_xdr.o

YPSRCS= yp_b_clnt.c yp_b_xdr.c yp_mt.c \
	dbm.c yp_all.c yp_bind.c yp_enum.c yp_master.c yp_match.c yp_order.c \
	yp_update.c ypupd.c yperr_string.c ypprot_err.c ypxdr.c ypmaint_xdr.c

HDRS = dbm.h yp_prot.h ypclnt.h yp_mt.h

INCLUDES=$(INC)/dbm.h $(INC)/yp_prot.h $(INC)/ypclnt.h  \
	 yp_b.h

all: $(OBJS)
	cp $(OBJS) ../

dbm.o: dbm.c\
	$(INC)/rpcsvc/dbm.h\
	$(INC)/sys/types.h\
	$(INC)/sys/stat.h
 
yp_all.o: yp_all.c\
	$(INC)/rpc/rpc.h\
	$(INC)/sys/syslog.h\
	yp_b.h\
	$(INC)/rpcsvc/yp_prot.h\
	$(INC)/rpcsvc/ypclnt.h
 
yp_b_clnt.o: yp_b_clnt.c\
	$(INC)/rpc/rpc.h\
	$(INC)/sys/time.h\
	yp_b.h
 
yp_b_xdr.o: yp_b_xdr.c\
	$(INC)/rpc/rpc.h\
	yp_b.h
 
yp_bind.o: yp_bind.c\
	$(INC)/stdio.h\
	$(INC)/errno.h\
	$(INC)/unistd.h\
	$(INC)/rpc/rpc.h\
	$(INC)/sys/syslog.h\
	yp_b.h\
	$(INC)/rpcsvc/yp_prot.h\
	$(INC)/rpcsvc/ypclnt.h
 
yp_enum.o: yp_enum.c\
	$(INC)/rpc/rpc.h\
	yp_b.h\
	$(INC)/rpcsvc/yp_prot.h\
	$(INC)/rpcsvc/ypclnt.h
 
yp_master.o: yp_master.c\
	$(INC)/rpc/rpc.h\
	yp_b.h\
	$(INC)/rpcsvc/yp_prot.h\
	$(INC)/rpcsvc/ypclnt.h
 
yp_match.o: yp_match.c\
	$(INC)/rpc/rpc.h\
	yp_b.h\
	$(INC)/rpcsvc/yp_prot.h\
	$(INC)/rpcsvc/ypclnt.h
 
yp_order.o: yp_order.c\
	$(INC)/rpc/rpc.h\
	yp_b.h\
	$(INC)/rpcsvc/yp_prot.h\
	$(INC)/rpcsvc/ypclnt.h
 
yp_update.o: yp_update.c\
	$(INC)/stdio.h\
	$(INC)/rpc/rpc.h\
	$(INC)/rpcsvc/ypupd.h\
	yp_b.h

yperr_string.o: yperr_string.c\
	$(INC)/rpcsvc/ypclnt.h
 
ypmaint_xdr.o: ypmaint_xdr.c\
	$(INC)/rpc/rpc.h\
	yp_b.h\
	$(INC)/rpcsvc/yp_prot.h\
	$(INC)/rpcsvc/ypclnt.h
 
ypprot_err.o: ypprot_err.c\
	$(INC)/rpc/rpc.h\
	$(INC)/rpcsvc/yp_prot.h\
	$(INC)/rpcsvc/ypclnt.h
 
ypupd.o: ypupd.c\
	$(INC)/rpc/rpc.h\
	$(INC)/rpcsvc/ypupd.h
 
ypxdr.o: ypxdr.c\
	$(INC)/rpc/rpc.h\
	$(INC)/rpcsvc/yp_prot.h\
	$(INC)/rpcsvc/ypclnt.h
 
lintit:
	$(LINT) $(LINTFLAGS) $(INCLIST) $(DEFLIST) $(YPSRCS)

clean:
	rm -f $(OBJS) 

clobber: clean
	rm -f $(LIBOBJS)

strip:	all
	$(STRIP) $(LIBOBJS)

size:	all
	$(SIZE) $(LIBOBJS)
