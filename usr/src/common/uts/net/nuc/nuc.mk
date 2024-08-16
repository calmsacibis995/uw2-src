#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-nuc:net/nuc/nuc.mk	1.1.1.19"
#ident 	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/nuc.mk,v 2.54.2.1 1994/12/29 20:16:30 hashem Exp $"

include $(UTSRULES)

# LOCALDEF = -Kframe -W0,-2C -DNUC_DEBUG -DFS_ONLY
# LOCALDEF = -DDEBUG_TRACE -Kframe -DNUC_DEBUG -DFS_ONLY -DINLINE_SEMA
LOCALDEF = -DFS_ONLY -DINLINE_SEMA

# Use when FS is being built in, otherwise don't define LOCALDEF
# LOCALDEF = -DFS_ONLY

DEVINC1 = -I.
KBASE    = ../..
LINTDIR = $(KBASE)/lintdir
DIR = net/nuc
MAKEFILE = nuc.mk

NUC = nuc.cf/Driver.o

MODULES = $(NUC)

FILES = 						\
		NWstrOps.o				\
		NWstrSVr4.o				\
		dpchannel.o				\
		dppool.o				\
		ncpdisp.o				\
		ncphdr.o				\
		nucInit.o				\
		nwcred.o				\
		nwlist.o				\
		nwmpioctl.o				\
		nwsema.o				\
		siauth.o				\
		silauth.o				\
		silserver.o				\
		siltask.o				\
		slservice.o				\
		sltask.o				\
		spauth.o				\
		spistart.o				\
		ipxcallout.o			\
		ipxengine.o				\
		ipxengmgr.o				\
		ipxengoob.o				\
		ipxestruct.o			\
		ipxipc.o				\
		ipxwd.o

FSFILES = 						\
		ipxipc_fs.o				\
		nwlist_fs.o				\
		nwstring.o				\
		semcvt.o				\
		siauth_fs.o				\
		sifhlist.o				\
		silvolume.o				\
		slhandle.o				\
		sltask_fs.o				\
		spauth_fs.o				\
		spdir3ns.o				\
		spfile2x.o				\
		spgeneral.o				\
		spidir.o				\
		spifile.o				\
		spifsys.o				\
		spivolume.o				\
		spvolume2x.o			\
		spvolume3x.o			\
		spdir3ns2.o				\
		spfilepb.o

LFILE = $(LINTDIR)/nuc.ln
LFILES =						\
		NWstrOps.ln				\
		NWstrSVr4.ln			\
		dpchannel.ln			\
		dppool.ln				\
		ncpdisp.ln				\
		ncphdr.ln				\
		nucInit.ln				\
		nwcred.ln				\
		nwlist.ln				\
		nwmpioctl.ln			\
		nwsema.ln				\
		siauth.ln				\
		silauth.ln				\
		silserver.ln			\
		siltask.ln				\
		slservice.ln			\
		sltask.ln				\
		spauth.ln				\
		spistart.ln				\
		ipxcallout.ln			\
		ipxengine.ln			\
		ipxengmgr.ln			\
		ipxengoob.ln			\
		ipxestruct.ln			\
		ipxipc.ln				\
		ipxwd.ln

FSLFILES =						\
		ipxipc_fs.ln			\
		nwlist_fs.ln			\
		nwstring.ln				\
		semcvt.ln				\
		siauth_fs.ln			\
		sifhlist.ln				\
		silvolume.ln			\
		slhandle.ln				\
		sltask_fs.ln			\
		spauth_fs.ln			\
		spdir3ns.ln				\
		spfile2x.ln				\
		spgeneral.ln			\
		spidir.ln				\
		spifile.ln				\
		spifsys.ln				\
		spivolume.ln			\
		spvolume2x.ln			\
		spvolume3x.ln			\
		spdir3ns2.ln			\
		spfilepb.ln

CFILES =						\
		NWstrOps.c				\
		NWstrSVr4.c				\
		dpchannel.c				\
		dppool.c				\
		ncpdisp.c				\
		ncphdr.c				\
		nucInit.c				\
		nwcred.c				\
		nwlist.c				\
		nwmpioctl.c				\
		nwsema.c				\
		siauth.c				\
		silauth.c				\
		silserver.c				\
		siltask.c				\
		slservice.c				\
		sltask.c				\
		spauth.c				\
		spistart.c				\
		ipxcallout.c			\
		ipxengine.c				\
		ipxengmgr.c				\
		ipxengoob.c				\
		ipxestruct.c			\
		ipxipc.c				\
		ipxwd.c

FSSRCFILES =					\
		ipxipc_fs.c				\
		nwlist_fs.c				\
		nwstring.c				\
		semcvt.c				\
		siauth_fs.c				\
		sifhlist.c				\
		silvolume.c				\
		slhandle.c				\
		sltask_fs.c				\
		spauth_fs.c				\
		spdir3ns.c				\
		spfile2x.c				\
		spgeneral.c				\
		spidir.c				\
		spifile.c				\
		spifsys.c				\
		spivolume.c				\
		spvolume2x.c			\
		spvolume3x.c			\
		spdir3ns2.c				\
		spfilepb.c

all:	$(MODULES)

install: all
	(cd nuc.cf; $(IDINSTALL) -R$(CONF) -M nuc)

# Use when FS support is required
$(NUC):	$(FILES) $(FSFILES)
		$(LD) -r -o $(NUC) $(FILES) $(FSFILES)
#
# No FS support use lines below
#$(NUC):	$(FILES) 
#		$(LD) -r -o $(NUC) $(FILES) 


#
# Configuration Section
#
ID:
	cd nuc.cf;       $(IDINSTALL) -R$(CONF) -M nuc

# Use when FS support is required
clean:
	-rm -f $(FILES)
	-rm -f $(FSFILES)
	-rm -f $(LFILES)
	-rm -f $(FSLFILES)
	-rm -f *.L *.klint $(NUC)
#
# No FS support use lines below
#clean:
#	-rm -f $(FILES)
#	-rm -f $(LFILES) *.L *.klint
#	-rm -f *.L *.klint


clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e nuc

$(LINTDIR):
	-mkdir -p $@


# Use when FS support is required
nuc.klint: $(CFILES) $(FSSRCFILES)
#
# No FS support use lines below
#nuc.klint: $(CFILES)


# Use when FS support is required
klintit: nuc.klint
	klint $(CFILES) $(FSSRCFILES) >nuc.klint 2>&1 || true
#
# No FS support use lines below
#klintit: nuc.klint
#	klint $(CFILES) >nuc.klint 2>&1 || true


lintit:	$(LFILE) 


# Use when FS support is required
$(LFILE): $(LINTDIR) $(LFILES) $(FSLFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES) $(FSLFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done
#
# No FS support use lines below
#$(LFILE): $(LINTDIR) $(LFILES)
#	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
#	for i in $(LFILES); do \
#		cat $$i >> $(LFILE); \
#		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
#	done



# Use when FS support is required
fnames:
	@for i in $(CFILES) $(FSSRCFILES);	\
	do \
		echo $$i; \
	done
#
# No FS support use lines below
#fnames:
#	@for i in $(CFILES);	\
#	do \
#		echo $$i; \
#	done



nucHeaders =						\
				gipcchannel.h		\
				gipccommon.h		\
				gipcconf.h			\
				gipcmacros.h		\
				gtscommon.h			\
				gtsconf.h			\
				gtsendpoint.h		\
				gtsmacros.h			\
				headstrconf.h		\
				ipxconf.h			\
				ipxengcommon.h			\
				ipxengine.h			\
				ipxengparam.h			\
				ipxengtune.h			\
				ncpconst.h			\
				ncpiopack.h			\
				nuc_hier.h			\
				nucerror.h			\
				nuclocks.h			\
				nucmachine.h		\
				nuctool.h		\
				nwctypes.h			\
				nwlist.h			\
				nwmp.h				\
				nwmpccode.h			\
				nwmpdev.h			\
				nwmptune.h			\
				nwncpconf.h			\
				nwncptune.h			\
				nwncpspi.h			\
				nwspi.h				\
				nwspiswitch.h		\
				nwstr_tune.h		\
				nwtypes.h		\
				requester.h			\
				sistructs.h			\
				slstruct.h			\
				spilcommon.h		\
				spimacro.h			\
				strchannel.h		\
				streamsconf.h

fsnucHeaders =						\
				ncpdvops.h			\
				nwerrors.h			\
				spfilepb.h


FRC:

include $(UTSDEPEND)


# Use when FS support is required
headinstall: $(nucHeaders) $(fsnucHeaders)
	@for f in $(nucHeaders) $(fsnucHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done
#
# No FS support use lines below
#headinstall: $(nucHeaders)
#	@for f in $(nucHeaders); \
#	 do \
#	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
#	 done

include $(MAKEFILE).dep
