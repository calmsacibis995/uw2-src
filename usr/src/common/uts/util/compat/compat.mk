#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:util/compat/compat.mk	1.7"
#ident	"$Header: $"

# Support for compatibility; nothing in this directory should be used
# by any new code.

include $(UTSRULES)

MAKEFILE=	compat.mk
KBASE = ../..
DIR = util/compat

all:

install: all

clean:

clobber:	clean

lintit:

fnames:

include compat_f.mk

# Note: these header files are headinstalled as links to compat_hdr.h
# They should also be packaged to be links on the target system.

sysHeaders = \
	bitmap.h \
	callo.h \
	fsinode.h \
	hrtsys.h \
	info.h \
	module.h \
	page.h \
	pfdat.h \
	sema.h \
	region.h \
	sysenvmt.h \
	vm.h \
	vmmac.h \
	vmsystm.h \
	$(compat_sysHeaders_f)

vmHeaders = \
	bootconf.h \
	cpu.h \
	debugger.h \
	kernel.h \
	mp.h \
	pte.h \
	reboot.h \
	rm.h \
	trace.h \
	vmlog.h \
	$(compat_vmHeaders_f)

headinstall:	compat_hdr.h headinstall_f
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@-rm -f $(INC)/sys/compat_hdr.h
	@$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) compat_hdr.h
	@for f in $(sysHeaders); \
	 do \
	    rm -f $(INC)/sys/$$f; \
	    echo Linking compat_hdr.h to sys/$$f; \
	    ln $(INC)/sys/compat_hdr.h $(INC)/sys/$$f; \
	 done
	@-rm -f $(INC)/sys/compat_hdr.h
	@-[ -d $(INC)/vm ] || mkdir -p $(INC)/vm
	@-rm -f $(INC)/vm/compat_hdr.h
	@$(INS) -f $(INC)/vm -m $(INCMODE) -u $(OWN) -g $(GRP) compat_hdr.h
	@for f in $(vmHeaders); \
	 do \
	    rm -f $(INC)/vm/$$f; \
	    echo Linking compat_hdr.h to vm/$$f; \
	    ln $(INC)/vm/compat_hdr.h $(INC)/vm/$$f; \
	 done
	@-rm -f $(INC)/vm/compat_hdr.h

include $(UTSDEPEND)

include $(MAKEFILE).dep
