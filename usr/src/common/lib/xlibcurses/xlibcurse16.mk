#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)curses:common/lib/xlibcurses/xlibcurse16.mk	1.1"
#ident "$Header: $"
#
#	Curses Library High Level Makefile.
#
#	To INSTALL libcurses16.a, the tic compiler and the tools type:
#
#		"make install"
#
#
#	To COMPILE libcurses16.a, the tic compiler and the tools, type:
#
#		"make all"
#
#
#	To compile a particular file with normal compilation type:
#
#		"make FILES='<particular .o files>"
#
#
#	If debugging is desired then type:
#
#		"make O=debug FILES='<particular .o files>"
#
#
#	If tracing is desired then type:
#
#		"make O=trace FILES='<particular .o files>"
#
#
#	If profiling is desired then type:
#
#		"make O=profile FILES='<particular .o files>"
#
#
#	To compile only the tic compiler type:
#
#		"make tic"
#
#
#	To create cpio files for all directories type:
#		"make cpio"
#
#

include $(LIBRULES)

SRCD=./screen

all:
	@cd $(SRCD) ; $(MAKE) -f makefile.16 rmhdrs $(MAKEARGS)
	@cd $(SRCD) ; $(MAKE) -f makefile.16 cktmp $(MAKEARGS)
	@cd $(SRCD) ; $(MAKE) -f makefile.16 $(MAKEARGS) VSSCANF="$(4.2COMPAT)"

libcurses16.a:
	#@cd $(SRCD) ; $(MAKE) -f makefile.16 rmhdrs $(MAKEARGS)
	#@cd $(SRCD) ; $(MAKE) -f makefile.16 cktmp $(MAKEARGS)
	@cd $(SRCD) ; $(MAKE) -f makefile.16 libcurses16.a $(MAKEARGS) VSSCANF="$(4.2COMPAT)"
	@echo
	@echo "libcurses16.a has been made."
	@echo

tools:
	@cd $(SRCD) ; $(MAKE) -f makefile.16 tools $(MAKEARGS) VSSCANF="$(4.2COMPAT)"
	@cd $(SRCD) ; $(MAKE) -f makefile.16 llib-lcurses.ln $(MAKEARGS) VSSCANF="$(4.2COMPAT)"
	@echo
	@echo "Libcurses/Terminfo tools have been made."
	@echo

tic:
	@cd $(SRCD) ; $(MAKE) -f makefile.16 tic $(MAKEARGS) VSSCANF="$(4.2COMPAT)"
	@echo
	@echo "The tic compiler has been made."
	@echo
	
install:
	# make and install libcurses16.a and tic
	@cd $(SRCD) ; $(MAKE) -f makefile.16 cktmp $(MAKEARGS)
	@cd $(SRCD) ; $(MAKE) -f makefile.16 install $(MAKEARGS) VSSCANF="$(4.2COMPAT)"
	@echo
	@echo libcurses16.a, the tic compiler, and associated tools have
	@echo been installed.
	@echo
	@if [ "$(CCSBIN)" = "$(TOOLS)/usr/ccs/bin" ]; \
	then \
		cd $(SRCD) ; $(MAKE) -f makefile.16 CC='$(HCC)' INC='/usr/include' ticclob tic ; \
                $(RM) -f $(CCSBIN)/captoinfo $(CCSBIN)/infocmp $(CCSBIN)/tput ;\
	else \
		cd $(SRCD) ; $(MAKE) -f makefile.16 ticclob tic ; \
	fi
	$(INS) -f $(CCSBIN)  -m 755 screen/tic

clean:
	@cd $(SRCD) ; $(MAKE) -f makefile.16 clean $(MAKEARGS)

clobber:
	@cd $(SRCD) ; $(MAKE) -f makefile.16 clobber $(MAKEARGS)

cpio:
	@echo
	@/bin/echo "\n\tBuilding cpio files in ${HOME}\n\n\t\c"
	@find . -print|cpio -ocud|split -10000 - ${HOME}/crs.`date +%m%d`.
	@/bin/echo "\n\tcpio files have been built\n"

bsd:
	@echo
	cd $(SRCD); mv makefile makefile.sysv; cp makefile.bsd makefile
	cd $(SRCD); make rmident
	@echo "Curses has been converted for BSD"
# this has only been tested on 4.2BSD, but we assume libc has getopt.

