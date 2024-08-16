#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)rpcgen:rpcgen.mk	1.10.10.4"
#ident  "$Header: rpcgen.mk 1.4 91/07/01 $"

include $(CMDRULES)


OWN = bin
GRP = bin

#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#	PROPRIETARY NOTICE (Combined)
#
# This source code is unpublished proprietary information
# constituting, or derived under license from AT&T's UNIX(r) System V.
# In addition, portions of such source code were derived from Berkeley
# 4.3 BSD under license from the Regents of the University of
# California.
#
#
#
#	Copyright Notice 
#
# Notice of copyright on this source code product does not indicate 
#  publication.
#
#       (c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc                     
#       (c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.                      
#       (c) 1990,1991,1992  UNIX System Laboratories, Inc
#          All rights reserved.
# 

PROG = rpcgen

OBJS = rpc_clntout.o rpc_cout.o rpc_hout.o rpc_main.o rpc_parse.o \
	  rpc_scan.o rpc_svcout.o rpc_tblout.o rpc_util.o rpc_sample.o

SRCS = $(OBJS:.o=.c)

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

rpc_clntout.o: rpc_clntout.c \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/rpc/types.h \
	rpc_parse.h \
	rpc_util.h 

rpc_cout.o: rpc_cout.c \
	$(INC)/stdio.h \
	$(INC)/string.h \
	rpc_parse.h \
	rpc_util.h

rpc_hout.o: rpc_hout.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	rpc_parse.h \
	rpc_util.h 

rpc_main.o: rpc_main.c \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/unistd.h \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/file.h \
	$(INC)/sys/stat.h \
	rpc_parse.h \
	rpc_util.h \
	rpc_scan.h

rpc_parse.o: rpc_parse.c \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/rpc/types.h \
	rpc_scan.h \
	rpc_parse.h \
	rpc_util.h

rpc_sample.o: rpc_sample.c \
	$(INC)/stdio.h \
	$(INC)/string.h \
	rpc_parse.h \
	rpc_util.h

rpc_scan.o: rpc_scan.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	rpc_scan.h \
	rpc_parse.h \
	rpc_util.h

rpc_svcout.o: rpc_svcout.c \
	$(INC)/stdio.h \
	$(INC)/string.h \
	rpc_parse.h \
	rpc_util.h

rpc_tblout.o: rpc_tblout.c \
	$(INC)/stdio.h \
	$(INC)/string.h \
	rpc_parse.h \
	rpc_util.h

rpc_util.o: rpc_util.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	rpc_scan.h \
	rpc_parse.h \
	rpc_util.h

clean:
	$(RM) -f $(OBJS)

clobber: clean
	$(RM) -f $(PROG)

lintit:
	$(LINT) $(LINTFLAGS) $(SRCS)

install: all
	[ -d $(USRBIN) ] || mkdir $(USRBIN)
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) $(PROG)
