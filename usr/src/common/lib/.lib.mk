#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lib:.lib.mk	1.7"

include $(LIBRULES)

LIBLIST=*

install:
	for i in $(LIBLIST); \
	do \
	   if [ -d $$i ]; \
	   then \
		echo "=== $$i `date`"; \
		case "$$i" \
		{ \
		libC		|\
		libc		|\
		libcomplex	|\
		libdl		|\
		libl		|\
		libm		|\
		libmalloc	|\
		liby		|\
		sc) 	  cd $$i; $(MAKE) -f $$i.mk install LIBRULES=$(LIBRULES); cd ..;; \
	        libcrypt) cd $$i; $(MAKE) -f $$i.mk install CFLAGS=-D_REENTRANT LIBRULES=$(OSRULES); cd ..;; \
		*)	  cd $$i; $(MAKE) -f $$i.mk install LIBRULES=$(OSRULES); cd ..;; \
		} \
	   fi \
	done
	if [ "$(CPU)" = "u3b2" ] ; \
	then \
		cd libc; $(MAKE) -f libc.mk clobber; \
		$(MAKE) -f libc.mk MAC=$(MAC) CFLAGS="-O -K mau" archive; \
	fi

clean:
	for i in $(LIBLIST); \
	do \
	   if [ -d $$i ]; \
	   then \
	   	echo $$i; cd $$i; $(MAKE) -f $$i.mk clean; cd ..; \
	   fi \
	done

clobber:
	for i in $(LIBLIST); \
	do \
	   if [ -d $$i ]; \
	   then \
	   	echo $$i; cd $$i; $(MAKE) -f $$i.mk clobber; cd ..; \
	   fi \
	done

lintit:
	for i in $(LIBLIST); \
	do \
	   if [ -d $$i ]; \
	   then \
	   	echo $$i; cd $$i; $(MAKE) -f $$i.mk lintit; cd ..; \
	   fi \
	done
