#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:fs/s5fs/s5fs.mk	1.14"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	s5fs.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = fs/s5fs

S5FS = s5.cf/Driver.o
LFILE = $(LINTDIR)/s5fs.ln

MODULES = \
	$(S5FS)

FILES = \
	s5alloc.o \
	s5bmap.o \
	s5dir.o \
	s5inode.o \
	s5rdwri.o \
	s5data.o \
	s5vfsops.o \
	s5vnops.o 

LFILES = s5alloc.ln \
	s5bmap.ln \
	s5dir.ln \
	s5inode.ln \
	s5rdwri.ln \
	s5data.ln \
	s5vfsops.ln \
	s5vnops.ln 

CFILES = s5alloc.c \
	s5bmap.c \
	s5dir.c \
	s5inode.c \
	s5rdwri.c \
	s5data.c \
	s5vfsops.c \
	s5vnops.c 

SRCFILES = $(CFILES)

all: $(MODULES)

install: all
	(cd s5.cf; $(IDINSTALL) -R$(CONF) -M s5)

$(S5FS): $(FILES)
	$(LD) -r -o $(S5FS) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(S5FS)

clobber:        clean
	-$(IDINSTALL) -R$(CONF) -d -e s5

$(LINTDIR):


lintit: $(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES);  \
		do \
			echo $$i; \
		done

sysfsHeaders = \
	s5dir.h \
	s5fblk.h \
	s5filsys.h \
	s5ino.h \
	s5inode.h \
	s5inode_f.h \
	s5macros.h \
	s5param.h

headinstall: $(sysfsHeaders)
	@-[ -d $(INC)/sys/fs ] || mkdir -p $(INC)/sys/fs
	@for f in $(sysfsHeaders); \
	 do \
	    $(INS) -f $(INC)/sys/fs -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
