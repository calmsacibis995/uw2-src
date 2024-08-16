#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-nuc:fs/nucfs/nucfs.mk	1.15"
#ident 	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucfs/nucfs.mk,v 2.56.2.3 1995/01/05 01:42:51 stevbam Exp $"

include $(UTSRULES)

LOCALDEF = -DIAPX386 -Di386
#
# LOCALDEF = -DIAPX386 -Di386 -Kframe -DDEBUG_TRACE

DEVINC1 = -I../../net/nuc

KBASE    = ../..
LINTDIR = $(KBASE)/lintdir
DIR	 = fs/nucfs

NUCFS    = nucfs.cf/Driver.o
LFILE	 = $(LINTDIR)/nucfs.ln

MODULES = \
        $(NUCFS)


FILES = chandleops.o \
	comops.o \
	nodeops.o \
	nucfs_comops.o \
	nucfs_debug.o \
	nucfs_init.o \
	nucfs_lk.o \
	nucfs_vfsops.o \
	nucfs_vnops.o \
	nwfsname.o \
	nwfsflushd.o \
	volops.o \
	flock_cache.o

LFILES = chandleops.ln \
        comops.ln \
        nodeops.ln \
        nucfs_comops.ln \
	nucfs_debug.ln \
	nucfs_init.ln \
	nucfs_lk.ln \
        nucfs_vfsops.ln \
        nucfs_vnops.ln \
        nwfsname.ln \
	nwfsflushd.ln \
        volops.ln \
	flock_cache.ln

SRCFILES = chandleops.c \
        comops.c \
        nodeops.c \
        nucfs_comops.c \
	nucfs_debug.c \
	nucfs_init.c \
	nucfs_lk.c \
        nucfs_vfsops.c \
        nucfs_vnops.c \
	nwfsname.c \
	nwfsflushd.c \
        volops.c \
	flock_cache.c

all:	$(MODULES)

install: all
	(cd nucfs.cf; $(IDINSTALL) -R$(CONF) -M nucfs)

$(NUCFS):	$(FILES)
	$(LD) -r -o $(NUCFS) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(NUCFS)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e nucfs

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done


fnames:
	@for i in $(SRCFILES);  \
        do \
                echo $$i; \
        done



#
# Configuration Section
#

nucfsHeaders = \
	nucfs_tune.h \
	nucfscommon.h \
	nucfsglob.h \
	nucfsspace.h \
	nwficommon.h \
	nwfimacro.h \
	nwfschandle.h \
	nwfsnode.h \
	nwfsops.h \
	nwfsvolume.h \
	nwfidata.h \
	nwfslock.h \
	nucfslk.h \
	flock_cache.h

headinstall: $(nucfsHeaders)
	@-[ -d $(INC)/nucfs ] || mkdir -p $(INC)/nucfs
	@for f in $(nucfsHeaders); \
	 do \
		$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done
chandleops.o: chandleops.c \
	$(KBASE)/fs/nucfs/nucfs_tune.h \
	$(KBASE)/fs/nucfs/nucfscommon.h \
	$(KBASE)/fs/nucfs/nucfsglob.h \
	$(KBASE)/fs/nucfs/nucfsspace.h \
	$(KBASE)/fs/nucfs/nwficommon.h \
	$(KBASE)/fs/nucfs/nwfimacro.h \
	$(KBASE)/fs/nucfs/nwfschandle.h \
	$(KBASE)/fs/nucfs/nwfsnode.h \
	$(KBASE)/fs/nucfs/nwfsops.h \
	$(KBASE)/fs/nucfs/nwfsvolume.h \
	$(FRC)

comops.o: comops.c \
	$(KBASE)/fs/nucfs/nucfs_tune.h \
	$(KBASE)/fs/nucfs/nucfscommon.h \
	$(KBASE)/fs/nucfs/nucfsglob.h \
	$(KBASE)/fs/nucfs/nucfsspace.h \
	$(KBASE)/fs/nucfs/nwficommon.h \
	$(KBASE)/fs/nucfs/nwfimacro.h \
	$(KBASE)/fs/nucfs/nwfschandle.h \
	$(KBASE)/fs/nucfs/nwfsnode.h \
	$(KBASE)/fs/nucfs/nwfsops.h \
	$(KBASE)/fs/nucfs/nwfsvolume.h \
	$(FRC)

nodeops.o: nodeops.c \
	$(KBASE)/fs/nucfs/nucfs_tune.h \
	$(KBASE)/fs/nucfs/nucfscommon.h \
	$(KBASE)/fs/nucfs/nucfsglob.h \
	$(KBASE)/fs/nucfs/nucfsspace.h \
	$(KBASE)/fs/nucfs/nwficommon.h \
	$(KBASE)/fs/nucfs/nwfimacro.h \
	$(KBASE)/fs/nucfs/nwfschandle.h \
	$(KBASE)/fs/nucfs/nwfsnode.h \
	$(KBASE)/fs/nucfs/nwfsops.h \
	$(KBASE)/fs/nucfs/nwfsvolume.h \
	$(FRC)

nucfs_comops.o: nucfs_comops.c \
	$(KBASE)/fs/nucfs/nucfs_tune.h \
	$(KBASE)/fs/nucfs/nucfscommon.h \
	$(KBASE)/fs/nucfs/nucfsglob.h \
	$(KBASE)/fs/nucfs/nucfsspace.h \
	$(KBASE)/fs/nucfs/nwficommon.h \
	$(KBASE)/fs/nucfs/nwfimacro.h \
	$(KBASE)/fs/nucfs/nwfschandle.h \
	$(KBASE)/fs/nucfs/nwfsnode.h \
	$(KBASE)/fs/nucfs/nwfsops.h \
	$(KBASE)/fs/nucfs/nwfsvolume.h \
	$(FRC)

nucfs_vfsops.o: nucfs_vfsops.c \
	$(KBASE)/fs/nucfs/nucfs_tune.h \
	$(KBASE)/fs/nucfs/nucfscommon.h \
	$(KBASE)/fs/nucfs/nucfsglob.h \
	$(KBASE)/fs/nucfs/nucfsspace.h \
	$(KBASE)/fs/nucfs/nwficommon.h \
	$(KBASE)/fs/nucfs/nwfimacro.h \
	$(KBASE)/fs/nucfs/nwfschandle.h \
	$(KBASE)/fs/nucfs/nwfsnode.h \
	$(KBASE)/fs/nucfs/nwfsops.h \
	$(KBASE)/fs/nucfs/nwfsvolume.h \
	$(FRC)

nucfs_vnops.o: nucfs_vnops.c \
	$(KBASE)/fs/nucfs/nucfs_tune.h \
	$(KBASE)/fs/nucfs/nucfscommon.h \
	$(KBASE)/fs/nucfs/nucfsglob.h \
	$(KBASE)/fs/nucfs/nucfsspace.h \
	$(KBASE)/fs/nucfs/nwficommon.h \
	$(KBASE)/fs/nucfs/nwfimacro.h \
	$(KBASE)/fs/nucfs/nwfschandle.h \
	$(KBASE)/fs/nucfs/nwfsnode.h \
	$(KBASE)/fs/nucfs/nwfsops.h \
	$(KBASE)/fs/nucfs/nwfsvolume.h \
	$(FRC)

nwfsflushd.o: nwfsflushd.c \
	$(KBASE)/fs/nucfs/nucfs_tune.h \
	$(KBASE)/fs/nucfs/nucfscommon.h \
	$(KBASE)/fs/nucfs/nucfsglob.h \
	$(KBASE)/fs/nucfs/nucfsspace.h \
	$(KBASE)/fs/nucfs/nwficommon.h \
	$(KBASE)/fs/nucfs/nwfimacro.h \
	$(KBASE)/fs/nucfs/nwfschandle.h \
	$(KBASE)/fs/nucfs/nwfslock.h \
	$(KBASE)/fs/nucfs/nwfsnode.h \
	$(KBASE)/fs/nucfs/nwfsops.h \
	$(KBASE)/fs/nucfs/nwfsvolume.h \
	$(FRC)

volops.o: volops.c \
	$(KBASE)/fs/nucfs/nucfs_tune.h \
	$(KBASE)/fs/nucfs/nucfscommon.h \
	$(KBASE)/fs/nucfs/nucfsglob.h \
	$(KBASE)/fs/nucfs/nucfsspace.h \
	$(KBASE)/fs/nucfs/nwficommon.h \
	$(KBASE)/fs/nucfs/nwfimacro.h \
	$(KBASE)/fs/nucfs/nwfschandle.h \
	$(KBASE)/fs/nucfs/nwfsnode.h \
	$(KBASE)/fs/nucfs/nwfsops.h \
	$(KBASE)/fs/nucfs/nwfsvolume.h \
	$(FRC)

flock_cache.o: flock_cache.c \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/buf_f.h \
	$(KBASE)/fs/file.h \
	$(KBASE)/fs/nucfs/flock_cache.h \
	$(KBASE)/fs/nucfs/nucfs_tune.h \
	$(KBASE)/fs/nucfs/nucfscommon.h \
	$(KBASE)/fs/nucfs/nucfsglob.h \
	$(KBASE)/fs/nucfs/nucfslk.h \
	$(KBASE)/fs/nucfs/nucfsspace.h \
	$(KBASE)/fs/nucfs/nwficommon.h \
	$(KBASE)/fs/nucfs/nwfidata.h \
	$(KBASE)/fs/nucfs/nwfimacro.h \
	$(KBASE)/fs/nucfs/nwfiname.h \
	$(KBASE)/fs/nucfs/nwfschandle.h \
	$(KBASE)/fs/nucfs/nwfsname.h \
	$(KBASE)/fs/nucfs/nwfsnode.h \
	$(KBASE)/fs/nucfs/nwfsops.h \
	$(KBASE)/fs/nucfs/nwfsvolume.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/fs/vfs.h \
	$(KBASE)/fs/vnode.h \
	$(KBASE)/io/conssw.h \
	$(KBASE)/io/stream.h \
	$(KBASE)/io/strmdep.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/mem/faultcatch.h \
	$(KBASE)/mem/faultcode.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/kmem.h \
	$(KBASE)/mem/seg.h \
	$(KBASE)/mem/seg_map_u.h \
	$(KBASE)/mem/ublock.h \
	$(KBASE)/net/nuc/ncpconst.h \
	$(KBASE)/net/nuc/ncpiopack.h \
	$(KBASE)/net/nuc/nucerror.h \
	$(KBASE)/net/nuc/nucmachine.h \
	$(KBASE)/net/nuc/nuctool.h \
	$(KBASE)/net/nuc/nwctypes.h \
	$(KBASE)/net/nuc/requester.h \
	$(KBASE)/net/nuc/slstruct.h \
	$(KBASE)/net/nuc/spfilepb.h \
	$(KBASE)/net/nuc/spilcommon.h \
	$(KBASE)/net/nw/nwportable.h \
	$(KBASE)/net/nw/nwtdr.h \
	$(KBASE)/net/xti.h \
	$(KBASE)/proc/cred.h \
	$(KBASE)/proc/disp_p.h \
	$(KBASE)/proc/lwp.h \
	$(KBASE)/proc/lwp_f.h \
	$(KBASE)/proc/proc.h \
	$(KBASE)/proc/regset.h \
	$(KBASE)/proc/seg.h \
	$(KBASE)/proc/siginfo.h \
	$(KBASE)/proc/signal.h \
	$(KBASE)/proc/tss.h \
	$(KBASE)/proc/user.h \
	$(KBASE)/svc/clock.h \
	$(KBASE)/svc/clock_p.h \
	$(KBASE)/svc/fault.h \
	$(KBASE)/svc/fp.h \
	$(KBASE)/svc/intr.h \
	$(KBASE)/svc/reg.h \
	$(KBASE)/svc/systm.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/svc/trap.h \
	$(KBASE)/util/bitmasks.h \
	$(KBASE)/util/cmn_err.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/dl.h \
	$(KBASE)/util/engine.h \
	$(KBASE)/util/ipl.h \
	$(KBASE)/util/kcontext.h \
	$(KBASE)/util/kdb/kdebugger.h \
	$(KBASE)/util/ksinline.h \
	$(KBASE)/util/ksynch.h \
	$(KBASE)/util/ksynch_p.h \
	$(KBASE)/util/list.h \
	$(KBASE)/util/listasm.h \
	$(KBASE)/util/nuc_tools/trace/nwctrace.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/param_p.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/sysmacros_f.h \
	$(KBASE)/util/types.h \
	$(FRC)
