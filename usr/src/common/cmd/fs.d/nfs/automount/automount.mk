#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nfs.cmds:automount/automount.mk	1.1.1.8"
#ident	"$Header: $"

#
# Make file for automount
#

include $(CMDRULES)

BINS= automount

OBJS= nfs_prot.o nfs_server.o nfs_trace.o nfs_cast.o \
	auto_main.o auto_look.o auto_proc.o auto_node.o \
	auto_mount.o auto_all.o mountxdr.o bindresvport.o ns.o
#	innetgr.o

SRCS= $(OBJS:.o=.c)
HDRS= automount.h nfs_prot.h
COFFLIBS= -lnsl_s -lsocket -lgen -lthread
ELFLIBS= -lnsl -lsocket -lgen -lthread
LDLIBS=`if [ x$(CCSTYPE) = xCOFF ] ; then echo "$(COFFLIBS)" ; else echo "$(ELFLIBS)" ; fi`

INCSYS= $(INC)
LOCALDEF= -D_REENTRANT

LINTFLAGS= -hbax $(DEFLIST) $(INCLIST)

# For debugging
#DEVINC1= -I.
#LDFLAGS= -g
#CFLAGS= -g
#CFLAGS= -P

INSDIR = $(USRLIB)/nfs
OWN = bin
GRP = bin

all: $(BINS)

$(BINS): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

#nfs_prot.c: nfs_prot.h nfs_prot.x
#	rpcgen -c nfs_prot.x -o $@

#nfs_prot.h: nfs_prot.x
#	rpcgen -h nfs_prot.x -o $@

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) $(BINS)

tags: $(SRCS)
	ctags $(SRCS)

lintit: $(SRCS)
	$(LINT) $(LOCALDEF) $(LINTFLAGS) $(SRCS)

clean:
	-rm -f $(OBJS)

clobber: clean
	-rm -f $(BINS)
