#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/tbl/tbl.mk	1.1"
#ident	"$Header: $"
#     Portions Copyright(c) 1988, Sun Microsystems, Inc.
#     All Rights Reserved


#	Makefile for tbl.	
#

include $(CMDRULES)

OWN = bin

GRP = bin

CFILES=t0.c t1.c t2.c t3.c t4.c t5.c t6.c t7.c t8.c t9.c tb.c tc.c te.c tf.c \
	tg.c ti.c tm.c ts.c tt.c tu.c tv.c

OFILES=t0.o t1.o t2.o t3.o t4.o t5.o t6.o t7.o t8.o t9.o tb.o tc.o te.o tf.o \
	tg.o ti.o tm.o ts.o tt.o tu.o tv.o

INSDIR = $(ROOT)/$(MACH)/usr/ucb

all:	tbl

tbl:	$(OFILES)
	$(CC) -o tbl $(OFILES) $(LDFLAGS) $(PERFLIBS)

install: all
	$(INS) -f $(INSDIR) -u $(OWN) -g $(GRP) -m 00555 tbl

clean: 
	rm -f *.o

clobber:	clean
	rm -f tbl
