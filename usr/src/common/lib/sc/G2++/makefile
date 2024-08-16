#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident  "@(#)sc:G2++/makefile	3.19"

include $(LIBRULES)

PRODUCTS = libg2++.a g2++comp

all : $(PRODUCTS)

install : all

.MUTEX: $(PRODUCTS)

libg2++.a :
	cd g2++lib; $(MAKE) \
		DEFLIST="$(DEFLIST)" \
		INCLIST="$(INCLIST:-I=-I../)" \
		LINKDIRS="$(LINKDIRS:-L=-L../)" \
		CFLAGS="$(CFLAGS)" \
		C++FLAGS="$(C++FLAGS)" \
		C++CMD="$(C++C) $(C++FLAGS) $(DEFLIST) $(INCLIST:-I=-I../) $(LINKDIRS:-L=-L../)" \
		ARCHIVES="$(ARCHIVES)";

g2++comp :
	cd compsrc; $(MAKE) \
		DEFLIST="$(DEFLIST)" \
		INCLIST="$(INCLIST:-I=-I../)" \
		LINKDIRS="$(LINKDIRS:-L=-L../)" \
		CFLAGS="$(CFLAGS)" \
		C++FLAGS="$(C++FLAGS)" \
		C++CMD="$(C++C) $(C++FLAGS) $(DEFLIST) $(INCLIST:-I=-I../) $(LINKDIRS:-L=-L../)" \
		ARCHIVES="$(ARCHIVES)";

demos:
	cd demos; $(MAKE)

clean:
	$(RM) -f *.ii *.o *.a core a.out
	cd compsrc; $(MAKE) clean
	cd g2++lib; $(MAKE) clean
	cd demos; $(MAKE) clean SCLIB=../$(SCLIB)

clobber: clean
	$(RM) -f $(PRODUCTS)
	cd compsrc; $(MAKE) clobber
	cd g2++lib; $(MAKE) clobber
	cd demos; $(MAKE) clobber SCLIB=../$(SCLIB)
