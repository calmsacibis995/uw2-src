#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)rpcinfo:rpcinfo.mk	1.10.9.6"
#ident	"$Header: $"

include $(CMDRULES)


OWN = bin
GRP = bin

#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#       PROPRIETARY NOTICE (Combined)
#
# This source code is unpublished proprietary information
# constituting, or derived under license from AT&T's UNIX(r) System V.
# In addition, portions of such source code were derived from Berkeley
# 4.3 BSD under license from the Regents of the University of
# California.
#
#
#
#       Copyright Notice
#
# Notice of copyright on this source code product does not indicate
#  publication.
#
#       (c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
#       (c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
#       (c) 1990,1991,1992  UNIX System Laboratories, Inc
#          All rights reserved.
#

#
# Sun RPC is a product of Sun Microsystems, Inc. and is provided for
# unrestricted use provided that this legend is included on all tape
# media and as a part of the software program in whole or part.  Users
# may copy or modify Sun RPC without charge, but are not authorized
# to license or distribute it to anyone else except as part of a product or
# program developed by the user.
#
# SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
# WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
# PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
#
# Sun RPC is provided with no support and without any obligation on the
# part of Sun Microsystems, Inc. to assist in its use, correction,
# modification or enhancement.
#
# SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
# INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
# OR ANY PART THEREOF.
#
# In no event will Sun Microsystems, Inc. be liable for any lost revenue
# or profits or other special, indirect and consequential damages, even if
# Sun has been advised of the possibility of such damages.
#
# Sun Microsystems, Inc.
# 2550 Garcia Avenue
# Mountain View, California  94043
#
#
# Makefile for rpcinfo
#

LDLIBS= -lnsl -lgen
LOCALDEF= -DPORTMAP
PROG = rpcinfo
MSGDIR = $(USRLIB)/locale/C/MSGFILES

OBJS = rpcinfo.o
SRCS = $(OBJS:.o=.c)

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS) $(SHLIBS) 

rpcinfo.o: rpcinfo.c \
	$(INC)/rpc/rpc.h \
	$(INC)/stdio.h \
	$(INC)/rpc/rpcb_prot.h \
	$(INC)/rpc/nettype.h \
	$(INC)/netdir.h \
	$(INC)/rpc/rpcent.h \
	$(INC)/sys/utsname.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/netinet/in.h \
	$(INC)/sys/socket.h \
	$(INC)/netdb.h \
	$(INC)/arpa/inet.h \
	$(INC)/rpc/pmap_prot.h \
	$(INC)/rpc/pmap_clnt.h

install: $(PROG)
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) $(PROG)
	[ -d $(MSGDIR) ] || mkdir -p $(MSGDIR)
	$(INS) -f $(MSGDIR) -m 0644 -u $(OWN) -g $(GRP) uxrpcinfo.str

lintit:
	$(LINT) $(LINTFLAGS) $(CPPFLAGS) $(SRCS) 

clean:
	$(RM) -f $(OBJS)

clobber: clean
	$(RM) -f $(PROG)
