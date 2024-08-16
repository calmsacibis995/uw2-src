#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libiaf:common/lib/libiaf/libiaf.mk	1.1.2.7"
#ident "$Header: $"

include $(LIBRULES)

# 
# Identification and Authentication Facility Library
#

LIBSODIR=       /usr/lib/
LIBNAME=        libiaf.so
IAFLDFLAGS=	-G -dy -ztext -h $(LIBSODIR)$(LIBNAME)
LOCALDEF=	$(PICFLAG)
ARFLAGS=	r
AROBJS=`echo *.o | sed 's/nsgetia_info.o//'`
SHOBJS=`echo *.o | sed 's/getia_info.o//'`

all:
	@for i in ia iaf idmap saf;\
	do\
		if test -d $$i;then\
		case $$i in\
		*.*)\
			;;\
		*)\
		cd  $$i;\
		echo "===== $(MAKE) -f $$i.mk all";\
		$(MAKE) -f $$i.mk $(MAKEARGS); \
		cd .. ;;\
		esac;\
		fi;\
	done

	-rm -f libiaf.so
	$(CC) -o libiaf.so $(SHOBJS) $(IAFLDFLAGS)
	-rm -f libiaf.a
	$(AR) $(ARFLAGS) libiaf.a $(AROBJS)

install:  all
		$(INS) -f $(USRLIB) -m 0664 libiaf.so
		$(INS) -f $(USRLIB) -m 0664 libiaf.a

clean:
	-rm -f *.o
	@for i in ia iaf idmap saf;\
	do\
		if test -d $$i;then\
		case $$i in\
		*.*)\
			;;\
		*)\
		cd  $$i;\
		$(MAKE) -f $$i.mk clean $(MAKEARGS); \
		cd .. ;;\
		esac;\
		fi;\
	done

clobber:	clean
	-rm -f libiaf.so
	-rm -f libiaf.a
	@for i in ia iaf idmap saf;\
	do\
		if test -d $$i;then\
		case $$i in\
		*.*)\
			;;\
		*)\
		cd  $$i;\
		$(MAKE) -f $$i.mk clobber $(MAKEARGS); \
		cd .. ;;\
		esac;\
		fi;\
	done
