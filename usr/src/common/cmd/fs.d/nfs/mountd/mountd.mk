#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nfs.cmds:mountd/mountd.mk	1.1.1.2"
#ident	"$Header: $"

include $(CMDRULES)

BINS= mountd
OBJS= mountd.o issubdir.o mountxdr.o innetgr.o sharetab.o 
SRCS= $(OBJS:.o=.c)
INCSYS = $(INC)
INSDIR = $(USRLIB)/nfs
OWN = bin
GRP = bin

LINTFLAGS= -hbax $(DEFLIST)
COFFLIBS= -lrpc -ldes -lnsl_s -lyp -lnet -lsocket -lgen
#ELFLIBS = -dy -lnsl -lyp -lrpc -ldes -lnet -lsocket -lgen
ELFLIBS = -lnsl -lsocket -lgen
LDLIBS=`if [ x$(CCSTYPE) = xCOFF ] ; then echo "$(COFFLIBS)" ; else echo "$(ELFLIBS)" ; fi`

all: $(BINS)

$(BINS): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) $(BINS)

lintit:
	$(LINT) $(LINTFLAGS) $(SRCS)

tags: $(SRCS)
	ctags $(SRCS)

clean:
	-rm -f $(OBJS)

clobber: clean
	-rm -f $(BINS)
