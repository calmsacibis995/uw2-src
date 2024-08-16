#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libthread:common/lib/libthread/libthread.mk	1.3.7.14"

#
# Top-level makefile for libthread.
#
# We include the local rules file for a couple of definitions, e.g. $(MASK),
# but are careful to stay away from the compilation targets.  This is easy,
# because we define no sources or objects in this directory.
#

LIBTHREADRULES = libthread.rules

include $(LIBRULES)
include $(LIBTHREADRULES)

MAKEFILE = libthread.mk

LDFLAGS=-G -dy
LIBSODIR=/usr/lib
INSTALLDIR = $(USRLIB)

PRODUCTS = libthread.so libthread.so.1 libthreadT.so libthreadT.so.1 libthread.a

# Don't holler about undefined symbols or unreferenced externs.
LINTFLAGS = ux

# Default is to build the shared object.
TGT = dotso

MAKEALL = asyncio.d thread.d sync.d sys.d archdep.d tsd.d
DIRS = $(MAKEALL:.d= )

SYMBOLIC = _thr_debug_notify, _thr_resume, _thr_swtch, _thr_resendsig, \
	   _thr_disp, _thr_setrq, _thr_get_thread

# Default target is non-trace and trace shared object

all:: libso libt

common:	$(MAKEALL)

$(MAKEALL):
	@cd $(@F:.d=);\
	echo "===== $(MAKE) -f $(@F:.d=.mk) $(TGT) ALLFLAGS=\"$(ALLFLAGS)\"";\
	$(MAKE) -f $(@F:.d=).mk $(TGT) ALLFLAGS="$(ALLFLAGS)"
.MUTEX: libso libt liba

libso:	common
	-@rm -f *.o
	@echo Making libthread.so.1 ...;\
	for i in */*.P;\
	do\
		ln $$i `basename $$i .P`.o;\
	done
	$(MASK) \
	$(LD) $(LDFLAGS) -Bsymbolic="$(SYMBOLIC)" \
		-Bbind_now \
		-h $(LIBSODIR)/libthread.so.1 -o libthread.so.1 \
		`$(LORDER) *.o | $(TSORT)`
	-@rm -f libthread.so
	@ln libthread.so.1 libthread.so
	-@rm -f *.o

libt:
	$(MAKE) -f $(MAKEFILE) $(ARGS) TRACE='-DTRACE' TGT='dott' \
		PICDEF='pic.def' common
	-@rm -f *.o
	@echo Making libthreadT.so.1 ...;\
	for i in */*.T;\
	do\
		ln $$i `basename $$i .T`.o;\
	done
	$(MASK)\
	$(LD) $(LDFLAGS) -Bsymbolic="$(SYMBOLIC)" \
		-Bbind_now \
		-h $(LIBSODIR)/libthreadT.so.1 -o libthreadT.so.1 \
		`$(LORDER) *.o | $(TSORT)`
	-@rm -f libthreadT.so
	@ln libthreadT.so.1 libthreadT.so
	-@rm -f *.o

liba: 
	$(MAKE) -f $(MAKEFILE) TGT='dota' common
	-@rm -f *.o
	@echo Making libthread.a ...;\
	for i in */*.O;\
	do\
		ln $$i `basename $$i .O`.o;\
	done
	-@rm -f tinit.o
	$(MASK)\
	$(AR) rcu libthread.a `$(LORDER) *.o | $(TSORT)`
	-@rm -f *.o

lintall:
	$(MAKE) -f $(MAKEFILE) $(ARGS) TGT='dotln' common ;\
	$(LINT) -$(LINTFLAGS) */*.ln

install::  all
	mv libthread.so.1 $(INSTALLDIR)/libthread.so.1 ;\
	mv libthreadT.so.1 $(INSTALLDIR)/libthreadT.so.1
	-rm -f $(INSTALLDIR)/libthread.so
	ln $(INSTALLDIR)/libthread.so.1 $(INSTALLDIR)/libthread.so
	-rm -f $(INSTALLDIR)/libthreadT.so
	ln $(INSTALLDIR)/libthreadT.so.1 $(INSTALLDIR)/libthreadT.so

clean::
	@echo "Cleaning libthread"
	-rm -f *.o *.O *.P *.T *.ln
	@for i in $(DIRS);\
	do\
		echo "Cleaning $$i";\
		if test -d $$i;then\
			cd  $$i;\
			$(MAKE) -f $$i.mk $(ARGS) clean;\
			cd ..;\
		else\
			echo "$$i not found";\
		fi;\
	done

clobber::	clean
	@echo "Clobbering libthread"
	-rm -f $(PRODUCTS)
