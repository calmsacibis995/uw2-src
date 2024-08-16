#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)crash:i386sym/cmd/crash/crash.mk	1.1.1.7"

#
# Tool Section
#

include $(UTSRULES)

CFLAGS= -ILOCAL 

#
# Define Section
#
INSPERM  = -m 0555 -u $(OWN) -g $(GRP)
LOCALDEF = -D_KMEMUSER -U_KERNEL -U_KERNEL_HEADERS
LDLIBS   = -lelf -lia -lcmd -lc
XLDLIBS  = -lelf -lia -lcmd -lc
FRC 	 =
MSGS 	 = memsize.str

.c.o:
	$(CC) $(CFLAGS) $(DEFLIST) -c $<

OFILES= disp.o \
	kma.o \
	page.o \
	prnode.o \
	pty.o \
	rt.o \
	stream.o \
	ts.o \
	tty.o

KMAOBJ= kmacct.o \
	kmasym.o

XFILES= abuf.o \
	base.o \
	buf.o \
	callout.o \
	cdfs_inode.o \
	class.o \
	dis.o \
	dummy.o \
	engine.o \
	fpriv.o \
	i386.o \
	init.o \
	inode.o \
	lck.o \
	lidcache.o \
	main.o \
	map.o \
	misc.o \
	proc.o \
	search.o \
	sfs_inode.o \
	size.o \
	sizenet.o \
	snode.o \
	stacktrace.o \
	stat.o \
	symtab.o \
	u.o \
	util.o \
	var.o \
	vfs.o \
	vfssw.o \
	vtop.o \
	vxfs_inode.o

crash:	$(XFILES)	
	$(CC) -o $@ $(XFILES) $(LDFLAGS) $(LDLIBS) $(NOSHLIBS)

all: 		crash ldsysdump memsize

xall: 		crash crash ldsysdump memsize

ldsysdump: 	ldsysdump.sh
		cp ldsysdump.sh ldsysdump

memsize: 	memsize.o
		$(CC) $(LDFLAGS) -o $@ memsize.o $(ROOTLIBS)

memsize.dy: 	memsize.o
		$(CC) $(LDFLAGS) -o $@ memsize.o 

kmacct:		$(KMAOBJ)
		$(CC) $(DEFLIST) $(LDFLAGS) -o $@ $(KMAOBJ) $(LDLIBS) $(SHLIBS)



install: 	ins_crash ins_ldsysdump ins_memsize $(MSGS)
		-[ -d $(USRLIB)/locale/C/MSGFILES ] || \
			mkdir -p $(USRLIB)/locale/C/MSGFILES
		$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 memsize.str

ins_crash: 	crash
		-rm -f $(ETC)/crash
		$(INS) -f $(USRSBIN) -m 0555 -u bin -g bin crash
		-$(SYMLINK) /usr/sbin/crash $(ETC)/etc/crash

ins_ldsysdump: 	ldsysdump
		-rm -f $(ETC)/ldsysdump
		$(INS) -f $(USRSBIN) -m 0555 -u bin -g bin ldsysdump
		-$(SYMLINK) /usr/sbin/ldsysdump $(ETC)/ldsysdump

ins_memsize:	memsize memsize.dy
		$(INS) -f $(SBIN) -m 0555 -u bin -g bin memsize
		$(INS) -f $(SBIN) -m 0555 -u bin -g bin memsize.dy

clean:
		-rm -f *.o

clobber: 	clean
		-rm -f crash
		-rm -f ldsysdump
		-rm -f memsize
		-rm -f memsize.dy

lint: 		$(CFILES) $(HFILES) 
		lint $(CPPFLAGS) -uh $(CFILES) 

#FRC:

async.o:	async.c
