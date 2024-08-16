#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/nvt/nvtd/nvtd.mk	1.8"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nps/nvt/nvtd/nvtd.mk,v 1.14 1994/05/10 20:45:15 mark Exp $"

include $(CMDRULES)

TOP = ../../..

include $(TOP)/local.def

LINTFLAGS = $(LOCALDEF) $(GLOBALINC) $(LOCALINC) -c

SRCS = nvt_util.c \
	nvtd.c

OBJS = nvt_util.o \
	nvtd.o

LDLIBS = $(LIBUTIL) -liaf -lgen

all: nvtd

install: all
	$(INS) -f $(USRSBIN) -m $(XMODE) -u $(OWN) -g $(GRP) nvtd

nvtd:	$(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDOPTIONS) $(LDLIBS)

nvtd.o: nvtd.c

clean:
	rm -f *.o
	rm -f *.ln

clobber: clean
	rm -f nvtd lint.out

lintit: $(SRCS)
	-@rm -f lint.out ; \
	for file in $(SRCS) ;\
	do \
	echo '## lint output for '$$file' ##' >>lint.out ;\
	$(LINT) $(LINTFLAGS) $$file $(LINTLIBS) >>lint.out ;\
	done
