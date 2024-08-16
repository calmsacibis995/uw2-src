#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident  "@(#)sc:fs/makefile	3.11"

include $(LIBRULES)

all:	libfs.a fsipp

libfs.a:
	cd libsrc; $(MAKE) \
                DEFLIST="$(DEFLIST)" \
                INCLIST="$(INCLIST:-I=-I../)" \
                LINKDIRS="$(LINKDIRS:-L=-L../)" \
                CFLAGS="$(CFLAGS)" \
                C++FLAGS="$(C++FLAGS)" \
                C++CMD="$(C++C) $(C++FLAGS) $(DEFLIST) $(INCLIST:-I=-I../) $(LINKDIRS:-L=-L../)" \
                ARCHIVES="$(ARCHIVES)";

fsipp:
	cd fsippsrc; $(MAKE)  \
                DEFLIST="$(DEFLIST)" \
                INCLIST="$(INCLIST:-I=-I../)" \
                LINKDIRS="$(LINKDIRS:-L=-L../)" \
                CFLAGS="$(CFLAGS)" \
                C++FLAGS="$(C++FLAGS)" \
                C++CMD="$(C++C) $(C++FLAGS) $(DEFLIST) $(INCLIST:-I=-I../) $(LINKDIRS:-L=-L../)" \
                ARCHIVES="$(ARCHIVES)";

demos:
	cd demos; $(MAKE) SCLIB=../$(SCLIB)

tests:	

clean:
	cd libsrc; $(MAKE) clean
	cd fsippsrc; $(MAKE) clean
	$(RM) -f *.a fsippsrc/fsipp
	cd demos; $(MAKE) clean SCLIB=../$(SCLIB)

clobber: clean
	cd libsrc; $(MAKE) clobber
	cd fsippsrc; $(MAKE) clobber
	$(RM) -f *.a

