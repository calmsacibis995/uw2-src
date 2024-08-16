#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)stand:i386sym/standalone/sys/sys.mk	1.1"

include $(UTSRULES)

#must undef GLOBALDEF so we don't have _KERNEL defined
GLOBALDEF =
KBASE = ..
LOCALINC = -I$(INC)
LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/sys.ln
LFILES = atoi.ln bfs.ln calloc.ln clearbss.ln conf.ln conf_devsw.ln 	      \
	conf_scsi.ln exit.ln getchar.ln gets.ln index.ln prf.ln prompt.ln     \
	putchar.ln qsort.ln saio.ln scsidisk.ln slic_error.ln slic_init.ln    \
	slic_mIntr.ln slic_rdData.ln slic_rdslave.ln slic_setGM.ln 	      \
	slic_wrAddr.ln slic_wrData.ln slic_wrslave.ln ssm.ln stop.ln 	      \
	strcmp.ln strcpy.ln strlen.ln strncmp.ln tm.ln vtoc.ln

SRCS =  atoi.c bfs.c calloc.c clearbss.c conf.c conf_devsw.c 		      \
	conf_scsi.c exit.c getchar.c gets.c index.c prf.c prompt.c    	      \
	putchar.c qsort.c saio.c scsidisk.c slic_error.c slic_init.c          \
	slic_mIntr.c slic_rdData.c slic_rdslave.c slic_setGM.c slic_wrAddr.c  \
	slic_wrData.c slic_wrslave.c ssm.c stop.c strcmp.c strcpy.c strlen.c  \
	strncmp.c tm.c vtoc.c

HDRS =  bfs.h ccs.h saio.h scsidisk.h ssm.h ssm_cons.h ssm_misc.h      \
	ssm_scsi.h

OBJS =  atoi.o bfs.o calloc.o clearbss.o conf.o conf_devsw.o 		      \
	conf_scsi.o exit.o getchar.o gets.o index.o prf.o prompt.o    \
	putchar.o qsort.o saio.o scsidisk.o slic_error.o slic_init.o 	      \
	slic_mIntr.o slic_rdData.o slic_rdslave.o slic_setGM.o slic_wrAddr.o  \
	slic_wrData.o slic_wrslave.o ssm.o stop.o strcmp.o strcpy.o strlen.o  \
	strncmp.o tm.o vtoc.o

all:	$(OBJS)

install: all

clean:
	-/bin/rm -f *.o *.ln *.L

clobber: clean

lintit: $(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

$(LINTDIR):
	-mkdir -p $@
