#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#	Portions Copyright (c) 1988, Sun Microsystems, Inc.
# 	All Rights Reserved

#ident	"@(#)script:script.mk	1.3.6.2"
#ident  "$Header: script.mk 1.3 91/07/01 $"

include $(CMDRULES)

#     Makefile for script

OWN = bin
GRP = bin

all:     script

script: script.c \
	$(INC)/stdio.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/fcntl.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/file.h \
	$(INC)/sys/wait.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	$(INS) -f $(USRBIN) -m 555 -u $(OWN) -g $(GRP) script 

clean:
	rm -f script.o

clobber: clean
	rm -f script

lintit:
	$(LINT) $(LINTFLAGS) script.c

#     These targets are useful but optional

partslist:
	@echo script.mk script.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo script | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit script.mk $(LOCALINCS) script.c -o script.o script

