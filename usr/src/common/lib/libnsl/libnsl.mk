#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libnsl:common/lib/libnsl/libnsl.mk	1.5.17.9"
#ident  "$Header: $"

include $(LIBRULES)
include libnsl.rules

# 
# Network services library
#

LIBSODIR=	/usr/lib/
LIBNAME=	libnsl.so.1
NSL_LDFLAGS=	-G -dy -h $(LIBSODIR)$(LIBNAME)

SHLIB = $(ROOT)/$(MACH)/shlib

DIRS=cr1 cs des mt netdir netselect nsl rexec rpc yp nsload

all: 
	@for i in $(DIRS);\
	do\
		if test -d $$i;then\
		case $$i in\
		*.*)\
			;;\
		*)\
		cd  $$i;\
		echo "===== $(MAKE) -f $$i.mk all";\
		$(MAKE) -f $$i.mk $(MAKEARGS) NSL_LOCALDEF="$(NSL_LOCALDEF)" ; \
		cd .. ;;\
		esac;\
		fi;\
	done

	if [ x$(CCSTYPE) = xCOFF ] ; \
	then \
		rm -f libnsl_s;\
		rm -f libnsl_s.a;\
		$(PFX)mkshlib -q -s _spec -h libnsl_s.a -t libnsl_s ;\
	else \
		rm -f libnsl_i.so;\
		$(LD) $(NSL_LDFLAGS) -o libnsl_i.so *.o *.o_i ;\
		if [ -s des_crypt.o_d -a -s des_soft.o_d -a -s des_mt.o_d ] ;\
		then \
			rm -f libnsl_d.so;\
			$(LD) $(NSL_LDFLAGS) -o libnsl_d.so *.o *.o_d ;\
		fi ;\
	fi
	-rm -f *.o *.o_i *.o_d

install:  all
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then \
		$(INS) -f $(USRLIB) -m 0664 libnsl_s.a ; \
		$(INS) -o -f $(SHLIB) -m 0775 libnsl_s ; \
	else \
		$(INS) -f $(USRLIB) -m 0755 libnsl_i.so ; \
		rm -f $(USRLIB)/$(LIBNAME) ; \
		rm -f $(USRLIB)/libnsl.so ; \
		rm -f $(USRLIB)/libxti.so ; \
		ln $(USRLIB)/libnsl_i.so $(USRLIB)/$(LIBNAME) ; \
		ln $(USRLIB)/libnsl_i.so $(USRLIB)/libnsl.so ; \
		cp $(USRLIB)/libnsl_i.so $(USRLIB)/libxti.so ; \
		if [ -s libnsl_d.so ]; \
		then \
			$(INS) -f $(USRLIB) -m 0755 libnsl_d.so ; \
			rm -f $(USRLIB)/$(LIBNAME) ; \
			rm -f $(USRLIB)/libnsl.so ; \
			rm -f $(USRLIB)/libxti.so ; \
			ln $(USRLIB)/libnsl_d.so $(USRLIB)/$(LIBNAME) ; \
			ln $(USRLIB)/libnsl_d.so $(USRLIB)/libnsl.so ; \
			cp $(USRLIB)/libnsl_d.so $(USRLIB)/libxti.so ; \
		fi ; \
	fi

lintit:
	echo Starting lint...
	@for i in $(DIRS);\
	do\
		if test -d $$i;then\
		case $$i in\
		*.*)\
			;;\
		*)\
		cd  $$i;\
		echo "===== $(MAKE) -f $$i.mk lintit";\
		$(MAKE) -f $$i.mk $(MAKEARGS) lintit ; \
		cd .. ;;\
		esac;\
		fi;\
	done

clean:
	-rm -f *.o *.o_i *.o_d
	@for i in $(DIRS);\
	do\
		if test -d $$i;then\
		case $$i in\
		*.*)\
			;;\
		*)\
		cd  $$i;\
		$(MAKE) -f $$i.mk $(MAKEARGS) clean; \
		cd .. ;;\
		esac;\
		fi;\
	done

clobber:	clean
	-rm -f libnsl_s.a libnsl_s *.so
