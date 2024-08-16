#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386sym:util/util.mk	1.65"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	util.mk
KBASE = ..
LINTDIR = $(KBASE)/lintdir
DIR = util

UTIL = util.cf/Driver.o
LFILE = $(LINTDIR)/util.ln

MODULES = \
	$(UTIL)

FILES = \
	calldemon.o \
	cmn_err.o \
	debug.o \
	event.o \
	misc.o \
	minmax.o \
	fspins.o \
	ksynch.o \
	ksynch_dbg.o \
	locks.o \
	locks_dbg.o \
	ladd.o \
	ldivide.o \
	lkstat.o \
	locktest.o \
	lsign.o \
	lsub.o \
	lshiftl.o \
	engine.o \
	list.o \
	lmul.o \
	metrics.o \
	rwlocks.o \
	rwlocks_dbg.o \
	rwsleep.o \
	sleep.o \
	subr.o \
	subr_f.o \
	string.o \
	sv.o \
	setrun.o \
	bitmasks.o \
	malloc.o

CFILES = \
	cmn_err.c \
	debug.c \
	calldemon.c \
	event.c \
	ksynch.c \
	ldivide.c \
	lkstat.c \
	locktest.c \
	engine.c \
	list.c \
	lmul.c \
	metrics.c \
	rwsleep.c \
	sleep.c \
	subr.c \
	subr_f.c \
	sv.c \
	setrun.c \
	bitmasks.c \
	malloc.c

SFILES = \
	misc.s \
	minmax.s \
	fspins.s \
	locks.s \
	ladd.s \
	lsign.s \
	lsub.s \
	lshiftl.s \
	rwlocks.s \
	string.s

SRCFILES = $(CFILES) $(SFILES)

LFILES = \
	calldemon.ln \
	cmn_err.ln \
	debug.ln \
	engine.ln \
	event.ln \
	ksynch.ln \
	ldivide.ln \
	list.ln \
	lmul.ln \
	lkstat.ln \
	metrics.ln \
	rwsleep.ln \
	sleep.ln \
	subr.ln \
	subr_f.ln \
	sv.ln \
	setrun.ln \
	bitmasks.ln \
	malloc.ln

SUBDIRS = compat kdb mod v3compat merge

all:	local FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk all"; \
		 $(MAKE) -f $$d.mk all $(MAKEARGS)); \
	 done

local:	$(MODULES)

$(UTIL): $(FILES)
	$(LD) -r -o $(UTIL) $(FILES)

# assym.h contains defines used from the assembler
# assym_c.h contains defines used from "asm"'s and inlined into
#		      C programs.

assym.h: symbols.c symbols_p.c
	$(CC) -UDEBUG $(CFLAGS) $(DEFLIST) $(INCLIST) -S symbols.c
	$(CC) -UDEBUG $(CFLAGS) $(DEFLIST) $(INCLIST) -S symbols_p.c
	awk -f symbols.awk symbols.s symbols_p.s | \
	sed -e '1,$$s;__SYMBOL__;;' > assym_c.h
	sed -e \
's/^#define	\(.*\)	\[\(.*\)\]/	define(`\1'\'',`ifelse($$#,0,`\2'\'',`\2'\''($$@))'\'')/' \
		assym_c.h > assym.h
	rm -f symbols.s symbols_p.s


assym_dbg.h: symbols.c symbols_p.c
	rm -f symbols.dbg.c
	cp symbols.c symbols.dbg.c
	$(CC) -DDEBUG $(CFLAGS) $(DEFLIST) $(INCLIST) -S symbols.dbg.c
	rm -f sym_p.dbg.c
	cp symbols_p.c sym_p.dbg.c
	$(CC) -DDEBUG $(CFLAGS) $(DEFLIST) $(INCLIST) -S sym_p.dbg.c
	awk -f symbols.awk symbols.dbg.s sym_p.dbg.s | \
	sed -e '1,$$s;__SYMBOL__;;' | sed -e \
's/^#define	\(.*\)	\[\(.*\)\]/	define(`\1'\'',`ifelse($$#,0,`\2'\'',`\2'\''($$@))'\'')/' > assym_dbg.h
	rm -f symbols.dbg.[cs] sym_p.dbg.[cs]

fspins.o: fspins.s assym.h assym_dbg.h

misc.o: assym.h assym_dbg.h

minmax.o: assym.h assym_dbg.h

ladd.o: assym.h assym_dbg.h

lsign.o: assym.h assym_dbg.h

lsub.o: assym.h assym_dbg.h

lshiftl.o: assym.h assym_dbg.h

string.o: assym.h assym_dbg.h

locks_dbg.o: assym.h assym_dbg.h locks.s
	$(M4) -DKBASE=$(KBASE) $(DEFLIST) -DDEBUG locks.s | \
			$(AS) -o locks_dbg.o -

locks.o: assym.h assym_dbg.h locks.s
	$(M4) -DKBASE=$(KBASE) $(DEFLIST) -UDEBUG locks.s | $(AS) -o locks.o -

ksynch.o: ksynch.c
	$(CC) -UDEBUG -USPINDEBUG $(DEFLIST) $(INCLIST) $(CFLAGS) -c $*.c

ksynch_dbg.o: ksynch.c
	rm -f ksynch_dbg.c
	cp ksynch.c ksynch_dbg.c
	$(CC) -DDEBUG $(DEFLIST) $(INCLIST) $(CFLAGS) -c ksynch_dbg.c
	rm -f ksynch_dbg.c

rwlocks_dbg.o: assym.h assym_dbg.h rwlocks.s
	$(M4) -DKBASE=$(KBASE) $(DEFLIST) -DDEBUG rwlocks.s | \
			$(AS) -o rwlocks_dbg.o -

rwlocks.o: assym.h assym_dbg.h rwlocks.s
	$(M4) -DKBASE=$(KBASE) $(DEFLIST) -UDEBUG rwlocks.s | \
			$(AS) -o rwlocks.o -

install: localinstall FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk install"; \
		 $(MAKE) -f $$d.mk install $(MAKEARGS)); \
	 done

localinstall: local FRC
	cd util.cf; $(IDINSTALL) -R$(CONF) -M util

clean:	localclean
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clean"; \
		 $(MAKE) -f $$d.mk clean $(MAKEARGS)); \
	 done

localclean:
	-rm -f *.o $(LFILES) *.L

localclobber:	localclean
	-rm -f assym.h assym_dbg.h
	-$(IDINSTALL) -R$(CONF) -d -e util

clobber:	localclobber
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clobber"; \
		 $(MAKE) -f $$d.mk clobber $(MAKEARGS)); \
	 done

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE) FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk lintit"; \
		 $(MAKE) -f $$d.mk lintit $(MAKEARGS)); \
	 done

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES); do \
		echo $$i; \
	done
	@for i in $(SUBDIRS); do \
	    (cd $$i; \
		$(MAKE) -f $$i.mk fnames $(MAKEARGS) | \
		$(SED) -e "s;^;$$i/;"); \
	done


headinstall: localhead FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk headinstall"; \
		 $(MAKE) -f $$d.mk headinstall $(MAKEARGS)); \
	 done

sysHeaders = \
	bitmasks.h \
	boot.h \
	cmn_err.h \
	debug.h \
	dl.h \
	emask.h \
	engine.h \
	inline.h \
	ipl.h \
	kcontext.h \
	ksinline.h \
	ksynch.h \
	ksynch_p.h \
	list.h \
	listasm.h \
	locktest.h \
	macro.h \
	map.h \
	metrics.h \
	param.h \
	param_p.h \
	plocal.h \
	processor.h \
	sysmacros.h \
	sysmacros_f.h \
	types.h \
	var.h

localhead: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

FRC:

include $(UTSDEPEND)

depend::
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk depend";\
		 touch $$d.mk.dep;\
		 $(MAKE) -f $$d.mk depend $(MAKEARGS));\
	done

include $(MAKEFILE).dep
