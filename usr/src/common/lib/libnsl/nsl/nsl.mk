#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libnsl:common/lib/libnsl/nsl/nsl.mk	1.14.9.5"
#ident	"$Header: $"

# 
# Network services library
#

include $(LIBRULES)
include ../libnsl.rules

.SUFFIXES:	.c .o 

LOCALDEF=	-DNO_IMPORT $(NSL_LOCALDEF)

#LIBOBJS= t_accept.o t_bind.o t_connect.o t_error.o t_close.o\
#	 t_getinfo.o t_getstate.o t_getpraddr.o t_listen.o t_look.o\
#	 t_rcv.o t_rcvconnect.o t_rcvdis.o t_snd.o t_snddis.o\
#	 t_unbind.o t_optmgmt.o\
#	 t_rcvudata.o t_rcvuderr.o t_sndudata.o t_sndrel.o t_rcvrel.o\
#	 t_alloc.o t_free.o t_open.o t_sync.o t_getname.o\
#	 _errlst.o _data.o _data2.o _conn_util.o _utility.o \
#	 nsl_mt.o t_strerror.o

OBJS= ../t_accept.o ../t_bind.o ../t_connect.o ../t_error.o ../t_close.o\
	 ../t_getinfo.o ../t_getstate.o ../t_listen.o ../t_look.o\
	 ../t_rcv.o ../t_rcvconnect.o ../t_rcvdis.o ../t_snd.o ../t_snddis.o\
	 ../t_unbind.o ../t_optmgmt.o ../t_rcvudata.o ../t_rcvuderr.o\
	 ../t_sndudata.o ../t_sndrel.o ../t_rcvrel.o ../t_alloc.o ../t_free.o\
	 ../t_open.o ../t_sync.o ../t_getname.o ../_errlst.o ../_data.o\
	 ../_data2.o ../_conn_util.o ../_utility.o ../nsl_mt.o\
	 ../xti_bind.o ../xti_getinfo.o ../xti_getpraddr.o ../xti_open.o\
	 ../xti_strerror.o ../xti_alloc.o ../xti_look.o

PLIBOBJS= t_accept.o t_bind.o t_connect.o t_error.o t_close.o\
	 t_getinfo.o t_getstate.o t_listen.o t_look.o\
	 t_rcv.o t_rcvconnect.o t_rcvdis.o t_snd.o t_snddis.o\
	 t_unbind.o t_optmgmt.o t_rcvudata.o t_rcvuderr.o\
	 t_sndudata.o t_sndrel.o t_rcvrel.o t_alloc.o t_free.o\
	 t_open.o t_sync.o t_getname.o _errlst.o _data.o\
	 _data2.o _conn_util.o _utility.o nsl_mt.o\
	 xti_bind.o xti_getinfo.o xti_getpraddr.o xti_open.o\
	 xti_strerror.o xti_alloc.o xti_look.o

SRCS = $(PLIBOBJS:.o=.c)

INCLUDES=  	$(INC)/sys/param.h\
		$(INC)/sys/types.h\
		$(INC)/errno.h\
		$(INC)/sys/stream.h\
		$(INC)/sys/stropts.h\
		$(INC)/sys/tihdr.h\
		$(INC)/sys/signal.h\
		$(INC)/sys/timod.h\
		$(INC)/sys/xti.h\
		$(INC)/mt.h\
		./_import.h\
		./nsl_mt.h

all:
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then \
		$(MAKE) -f nsl.mk allcoff $(MAKEARGS) \
			NSL_LOCALDEF="$(NSL_LOCALDEF)" ; \
	else \
		$(MAKE) -f nsl.mk allelf $(MAKEARGS) \
			NSL_LOCALDEF="$(NSL_LOCALDEF)"; \
	fi
	cp *.o ..

allelf:       $(INCLUDES) $(PLIBOBJS)

allcoff:      $(INCLUDES) $(LIBOBJS)

install:

lintit:
	$(LINT) $(LINTFLAGS) $(INCLIST) $(DEFLIST) $(SRCS)

clean:
	-rm -f *.o 

clobber:	clean
	-rm -f $(OBJS)
