#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/nprinter/nprinter.mk	1.8"
#ident	"$Id: nprinter.mk,v 1.16 1994/09/27 16:59:04 novell Exp $"

include $(CMDRULES)

TOP = ../..

include $(TOP)/local.def

LOCALDEF =  -DNWCM -DSVR4 -DLO_HI_MACH_TYPE -DN_PLAT_UNIX

LOCALINC = -I../include 

LINTFLAGS = $(GLOBALINC) $(LOCALINC)

NPRINTER_SRCS = commands.c inform.c ipxapi.c ipxtdr.c main.c misc.c entry.c \
		prtapi.c prtconf.c prtjob.c psapi.c pstdr.c rpapi.c rptdr.c \
		status.c upconfig.c upcntrl.c config.c

STOP_SRCS = stopnp.c

RESTART_SRCS = restartnp.c

NPRINTER_OBJS = $(NPRINTER_SRCS:.c=.o)

STOP_OBJS = stopnp.o

RESTART_OBJS = restartnp.o

SRCS = $(NPRINTER_SRCS) $(STOP_SRCS) $(RESTART_SRCS)

OBJS = $(NPRINTER_OBJS) $(STOP_OBJS) $(RESTART_OBJS)

LDLIBS = -L../lib $(LIBUTIL) -lprint

all: $(LIBNWCM) $(LIBNPS) nprinter stopnp restartnp

NPRINTDIR = $(ROOT)/$(MACH)/etc/netware/nprinter

install: all
	if [ ! -d ${NPRINTDIR} ]; \
	then \
		mkdir -p ${NPRINTDIR}; \
                eval ${CH}chmod 755 ${NPRINTDIR}\
        fi
	$(INS) -f $(USRSBIN) -m $(XMODE) -u $(OWN) -g $(GRP) nprinter
	$(INS) -f $(USRSBIN) -m $(XMODE) -u $(OWN) -g $(GRP) stopnp
	$(INS) -f $(USRSBIN) -m $(XMODE) -u $(OWN) -g $(GRP) restartnp
	$(INS) -f $(NPRINTDIR) -m $(XMODE) -u $(OWN) -g $(GRP) PRTConfig
	$(INS) -f $(NPRINTDIR) -m $(XMODE) -u $(OWN) -g $(GRP) RPControl
	$(INS) -f $(NPRINTDIR) -m $(XMODE) -u $(OWN) -g $(GRP) RPConfig

nprinter:	$(NPRINTER_OBJS)
		$(CC) -o $@ $(NPRINTER_OBJS) $(LDFLAGS) $(LDOPTIONS) $(LDLIBS)

stopnp:		$(STOP_OBJS)
		$(CC) -o $@ $(STOP_OBJS) $(LDFLAGS) $(LDOPTIONS) $(LDLIBS)

restartnp:	$(RESTART_OBJS)
		$(CC) -o $@ $(RESTART_OBJS) $(LDFLAGS) $(LDOPTIONS) $(LDLIBS)

restartnp.o: restartnp.c

stopnp.o: stopnp.c

$(NPRINTER_OBJS):

clean:
	rm -f *.o

clobber: clean
	rm -f nprinter stopnp restartnp lint.out

lintit: $(SRCS)
	-@rm -f lint.out ; \
	for file in $(SRCS) ;\
	do \
	echo '## lint output for '$$file' ##' >>lint.out ;\
	$(LINT) $(LINTFLAGS) $$file $(LINTLIBS) >>lint.out ;\
	done
