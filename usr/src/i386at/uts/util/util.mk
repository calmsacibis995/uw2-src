#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:util/util.mk	1.83"
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
	bitmasks.o \
	calldemon.o \
	cmn_err.o \
	debug.o \
	engine.o \
	event.o \
	fspins.o \
	ksynch.o \
	ksynch_dbg.o \
	ladd.o \
	ldivide.o \
	list.o \
	lkstat.o \
	lmul.o \
	locks.o \
	locks_dbg.o \
	locktest.o \
	lshiftl.o \
	lsign.o \
	lsub.o \
	malloc.o \
	metrics.o \
	minmax.o \
	misc.o \
	rwlocks.o \
	rwlocks_dbg.o \
	rwsleep.o \
	setrun.o \
	sleep.o \
	string.o \
	subr.o \
	subr_f.o \
	sv.o

CFILES = \
	bitmasks.c \
	calldemon.c \
	cmn_err.c \
	debug.c \
	engine.c \
	event.c \
	ksynch.c \
	ldivide.c \
	list.c \
	lkstat.c \
	lmul.c \
	locks_dbg.c \
	locktest.c \
	malloc.c \
	metrics.c \
	rwlocks_dbg.c \
	rwsleep.c \
	setrun.c \
	sleep.c \
	subr.c \
	subr_f.c \
	sv.c

SFILES = \
	ladd.s \
	locks.s \
	lshiftl.s \
	lsign.s \
	lsub.s \
	minmax.s \
	misc.s \
	rwlocks.s \
	string.s

SRCFILES = $(CFILES) $(SFILES)

LFILES = \
	bitmasks.ln \
	calldemon.ln \
	cmn_err.ln \
	debug.ln \
	engine.ln \
	event.ln \
	ksynch.ln \
	ldivide.ln \
	list.ln \
	lkstat.ln \
	lmul.ln \
	locks_dbg.ln \
	malloc.ln  \
	metrics.ln \
	rwlocks_dbg.ln \
	rwsleep.ln \
	setrun.ln \
	sleep.ln \
	subr.ln \
	subr_f.ln \
	sv.ln
## 	lintasm.ln	# not there for AT yet

SUBDIRS = compat kdb mod v3compat merge nuc_tools ccnv

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


# to get NVLT lock tracing:
#	specify -DDEBUG *and* -DDEBUG_TRACE when compiling symbols_dbg.c
#
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

minmax.o: assym.h assym_dbg.h

ladd.o: assym.h assym_dbg.h

lsign.o: assym.h assym_dbg.h

lsub.o: assym.h assym_dbg.h

lshiftl.o: assym.h assym_dbg.h

string.o: assym.h assym_dbg.h

misc.o: assym.h assym_dbg.h

locks.o: assym.h assym_dbg.h locks.s
	$(M4) -DKBASE=$(KBASE) $(DEFLIST) -UDEBUG locks.s | \
			$(AS) -o locks.o -

ksynch.o: ksynch.c
	$(CC) -UDEBUG -USPINDEBUG $(DEFLIST) $(INCLIST) $(CFLAGS) -c $*.c

ksynch_dbg.o: ksynch.c
	rm -f ksynch_dbg.c
	cp ksynch.c ksynch_dbg.c
	$(CC) -DDEBUG $(DEFLIST) $(INCLIST) $(CFLAGS) -c ksynch_dbg.c
	rm -f ksynch_dbg.c

# to get NVLT lock tracing:
#	specify -DDEBUG *and* -DDEBUG_TRACE when compiling locks_dbg.c
#
locks_dbg.o: locks_dbg.c
	$(CC) -DDEBUG $(DEFLIST) $(INCLIST) $(CFLAGS) -c locks_dbg.c

locks_dbg.ln: locks_dbg.c
	echo "\n$(DIR)/$*.c:" > $*.L
	-$(LINT) -DDEBUG $(LINTFLAGS) $(INCLIST) $(DEFLIST) \
		-c -u $*.c >> $*.L

# to get NVLT lock tracing:
#	specify -DDEBUG *and* -DDEBUG_TRACE when m4'ing rwlocks.s
#
rwlocks_dbg.o: rwlocks_dbg.c
	$(CC) -DDEBUG $(DEFLIST) $(INCLIST) $(CFLAGS) -c rwlocks_dbg.c

rwlocks_dbg.ln: rwlocks_dbg.c
	echo "\n$(DIR)/$*.c:" > $*.L
	-$(LINT) -DDEBUG $(LINTFLAGS) $(INCLIST) $(DEFLIST) \
		-c -u $*.c >> $*.L

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
	-rm -f *.o $(LFILES) *.L $(UTIL)

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
