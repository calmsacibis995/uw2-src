#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)stand:i386sym/standalone/stand.mk	1.1"

#
# Makefile for Symmetry standalone utilities
#
include $(UTSRULES)

#must undef GLOBALDEF so we don't have _KERNEL defined
GLOBALDEF =
KBASE = .
INSDIR = $(ROOT)/etc
LINTDIR = $(KBASE)/lintdir
SUBDIRS = cmd sys i386
LOCALLFLAGS = -x

LDFLAGS	= -dn -e start -Mmapfile
CMDS	= copy prtvtoc dump CCSformat
LIBI386	= i386/16start.o i386/bcopy.o i386/bzero.o i386/gsp.o i386/crt0.o
LIBSYS	= sys/atoi.o sys/bfs.o sys/calloc.o sys/clearbss.o sys/conf.o 	      \
	  sys/conf_devsw.o sys/conf_scsi.o sys/exit.o sys/getchar.o           \
	  sys/gets.o sys/index.o sys/prf.o sys/prompt.o sys/putchar.o         \
	  sys/qsort.o sys/saio.o sys/scsidisk.o    			      \
	  sys/slic_error.o sys/slic_init.o sys/slic_mIntr.o sys/slic_rdData.o \
	  sys/slic_rdslave.o sys/slic_setGM.o sys/slic_wrAddr.o 	      \
	  sys/slic_wrData.o sys/slic_wrslave.o sys/ssm.o sys/stop.o 	      \
	  sys/strcmp.o sys/strcpy.o sys/strlen.o sys/strncmp.o 		      \
	  sys/tm.o sys/vtoc.o

all: 	cmdobjs libsys libi386 
	$(MAKE) -f stand.mk boot $(CMDS)

cmdobjs:	
	cd cmd; $(MAKE) -f cmd.mk all $(MAKEARGS)

libsys:
	cd sys; $(MAKE) -f sys.mk all $(MAKEARGS)

libi386:
	cd i386; $(MAKE) -f i386.mk all $(MAKEARGS)

$(CMDS): $(LIBI386) $(LIBSYS) mapfile
	  $(LD) $(LDFLAGS) cmd/$@.o $(LIBI386) $(LIBSYS) -o $@ \
	  && chmod 644 $@

boot: 	$(LIBI386) $(LIBSYS) i386/crt0.o mapfile
	  $(LD) $(LDFLAGS) i386/crt0.o cmd/boot.o $(LIBI386) $(LIBSYS) -o $@ \
	  && chmod 644 boot

clean:
	-/bin/rm -rf $(LINTDIR)/*.ln $(LINTDIR)/*.L $(LINTDIR)/lint.out
	@for d in $(SUBDIRS); do 				\
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clean"; 	\
		$(MAKE) -f $$d.mk clean $(MAKEARGS));		\
	done
clobber: 
	-/bin/rm -f $(CMDS) boot
	-/bin/rm -rf $(LINTDIR)
	@for d in $(SUBDIRS); do 				\
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clobber"; 	\
		$(MAKE) -f $$d.mk clobber $(MAKEARGS));		\
	done

lintit: $(LINTDIR)
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk lintit"; \
		$(MAKE) -f $$d.mk lintit $(MAKEARGS)); \
	done
	cd $(LINTDIR); \
	cat *.L > lint.out; \
	echo "============ combined output ========" >> lint.out; \
	$(LINT) $(LINTFLAGS) $(LOCALLFLAGS) *.ln >> lint.out; \
	flint ../flint.data lint.out >> LINT.ERRS

$(LINTDIR):
	-mkdir -p $@

