#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nlms:nlms.mk	1.18"
#ident "$Header: $"

include $(CMDRULES)

INSDIR = $(VAR)
OWN = root
GRP = sys

VSP=		$(VAR)/spool/nwsup
NW3=		$(VSP)/NWserv/nwuc/disk1
NW3SYS=		$(NW3)/system
NW3SYS3=	$(NW3)/system/311
NW3SYS4=	$(NW3)/system/40
NW4=		$(VSP)/NWserv/nwuc/disk2
NW4ETC=		$(NW4)/etc
NW4SYS=		$(NW4)/system
NW4SYS3=	$(NW4)/system/311
NW4SYS4=	$(NW4)/system/40
DIR14=		$(NW3)/system/nls/14
DIR4=		$(NW3)/system/nls/4
DIR6=		$(NW3)/system/nls/6
DIR7=		$(NW3)/system/nls/7
DIR8=		$(NW3)/system/nls/8
DIR9=		$(NW3)/system/nls/9

NVTDOS=		$(VSP)/nvt/dos
NVTWIN=		$(VSP)/nvt/win

DIRS=		$(NW3) $(NW3SYS) $(NW3SYS3) $(NW3SYS4) \
		$(NW4) $(NW4ETC) $(NW4SYS) $(NW4SYS3) $(DIR7) \
		$(NVTDOS) $(NVTWIN) $(DIR14) $(DIR4) $(DIR6) $(DIR8) \
		$(NW4SYS4) $(DIR9)


all:

install: $(DIRS) all
	$(INS) -f $(VSP)/NWserv/nwuc -m 755 -u $(OWN) -g $(GRP) nwserv/nwuc/.dtinfo
	for i in pinstall.nlm space.dat;\
	do\
		$(INS) -f $(NW3) -m 755 -u $(OWN) -g $(GRP) nwserv/nwuc/disk1/$$i;\
	done
	for i in nuc.nlm unixlib.nlm pconfig.nlm uninstal.nlm;\
	do\
		$(INS) -f $(NW3SYS) -m 755 -u $(OWN) -g $(GRP) nwserv/nwuc/disk1/system/$$i;\
	done
	for i in netdb.nlm \
		nfs_3x.nam pkernel.nlm remfilfx.nlm rpcbstub.nlm;\
	do\
		$(INS) -f $(NW3SYS3) -m 755 -u $(OWN) -g $(GRP) nwserv/nwuc/disk1/system/311/$$i;\
	done
	for i in inetdb.nlm nfs.nam nfs_41x.nam pkernel.nlm \
			rpcbstub.nlm netdb.nlm;\
	do\
		$(INS) -f $(NW3SYS4) -m 755 -u $(OWN) -g $(GRP) nwserv/nwuc/disk1/system/40/$$i;\
	done
	for i in hosts group passwd nfsusers nfsgroup nwparams xfonteur;\
	do\
		$(INS) -f $(NW4ETC) -m 755 -u $(OWN) -g $(GRP) nwserv/nwuc/disk2/etc/$$i;\
	done
	for i in smdr.nlm tsaproxy.nlm v_nfs.nlm xconsole.nlm \
		 telnetd.nlm nwccss.nlm;\
	do\
		$(INS) -f $(NW4SYS) -m 755 -u $(OWN) -g $(GRP) nwserv/nwuc/disk2/system/$$i;\
	done
	for i in after311.nlm bcastlib.nlm clib.nlm \
			sbackup.nlm tsa311.nlm tsa312.nlm;\
	do\
		$(INS) -f $(NW4SYS3) -m 755 -u $(OWN) -g $(GRP) nwserv/nwuc/disk2/system/311/$$i;\
	done
	for i in sbackup.nlm smsdi.nlm tsa400.nlm tsa410.nlm;\
	do\
		$(INS) -f $(NW4SYS4) -m 755 -u $(OWN) -g $(GRP) nwserv/nwuc/disk2/system/40/$$i;\
	done

	for i in inetdb.msg netdb.msg nuc.msg pconfig.dat pconfig.hlp \
			pconfig.msg pfiles.dat pinstall.hlp pinstall.msg \
			pkernel.msg telnetd.msg uninstal.hlp uninstal.msg \
			smdr.msg smsdi.msg tsa400.msg tsa410.msg \
			unixlib.msg v_nfs.msg xconsole.msg;\
	do\
		$(INS) -f $(DIR14) -m 755 -u $(OWN) -g $(GRP) nwserv/nwuc/disk1/system/nls/14/$$i;\
	done
	for i in inetdb.msg netdb.msg nuc.msg pconfig.dat pconfig.hlp \
			pconfig.msg pfiles.dat pinstall.hlp pinstall.msg \
			pkernel.msg telnetd.msg uninstal.hlp uninstal.msg \
			smdr.msg smsdi.msg tsa400.msg tsa410.msg \
			unixlib.msg v_nfs.msg xconsole.msg ;\
	do\
		$(INS) -f $(DIR4) -m 755 -u $(OWN) -g $(GRP) nwserv/nwuc/disk1/system/nls/4/$$i;\
	done
	for i in inetdb.msg netdb.msg nuc.msg pconfig.dat pconfig.hlp \
			pconfig.msg pfiles.dat pinstall.hlp pinstall.msg \
			pkernel.msg telnetd.msg uninstal.hlp uninstal.msg \
			smdr.msg smsdi.msg tsa400.msg tsa410.msg \
			unixlib.msg v_nfs.msg xconsole.msg;\
	do\
		$(INS) -f $(DIR6) -m 755 -u $(OWN) -g $(GRP) nwserv/nwuc/disk1/system/nls/6/$$i;\
	done
	for i in inetdb.msg netdb.msg nuc.msg pconfig.dat pconfig.hlp \
			pconfig.msg pfiles.dat pinstall.hlp pinstall.msg \
			pkernel.msg telnetd.msg uninstal.hlp uninstal.msg \
			smdr.msg smsdi.msg tsa400.msg tsa410.msg \
			unixlib.msg v_nfs.msg xconsole.msg;\
	do\
		$(INS) -f $(DIR7) -m 755 -u $(OWN) -g $(GRP) nwserv/nwuc/disk1/system/nls/7/$$i;\
	done
	for i in inetdb.msg netdb.msg nuc.msg pconfig.dat pconfig.hlp \
			pconfig.msg pfiles.dat pinstall.hlp pinstall.msg \
			pkernel.msg telnetd.msg uninstal.hlp uninstal.msg \
			smdr.msg smsdi.msg tsa400.msg tsa410.msg \
			unixlib.msg v_nfs.msg xconsole.msg;\
	do\
		$(INS) -f $(DIR8) -m 755 -u $(OWN) -g $(GRP) nwserv/nwuc/disk1/system/nls/8/$$i;\
	done
	for i in inetdb.msg netdb.msg nuc.msg pconfig.dat pconfig.hlp \
			pconfig.msg pfiles.dat pinstall.hlp pinstall.msg \
			pkernel.msg telnetd.msg uninstal.hlp uninstal.msg \
			smdr.msg smsdi.msg tsa400.msg tsa410.msg \
			unixlib.msg v_nfs.msg xconsole.msg;\
	do\
		$(INS) -f $(DIR9) -m 755 -u $(OWN) -g $(GRP) nwserv/nwuc/disk1/system/nls/9/$$i;\
	done

	$(INS) -f $(NVTDOS) -m 755 -u $(OWN) -g $(GRP) nvt/dos/disk.id
	$(INS) -f $(NVTDOS) -m 755 -u $(OWN) -g $(GRP) nvt/dos/makefile
	$(INS) -f $(NVTDOS) -m 755 -u $(OWN) -g $(GRP) nvt/dos/readme.txt
	$(INS) -f $(NVTDOS) -m 755 -u $(OWN) -g $(GRP) nvt/dos/readme.wri
	$(INS) -f $(NVTDOS) -m 755 -u $(OWN) -g $(GRP) nvt/dos/setup.exe
	$(INS) -f $(NVTDOS) -m 755 -u $(OWN) -g $(GRP) nvt/dos/tnvt220.001
	$(INS) -f $(NVTDOS) -m 755 -u $(OWN) -g $(GRP) nvt/dos/tnvt220.inf
	$(INS) -f $(NVTWIN) -m 755 -u $(OWN) -g $(GRP) nvt/win/makefile
	$(INS) -f $(NVTWIN) -m 755 -u $(OWN) -g $(GRP) nvt/win/presentr.z
	$(INS) -f $(NVTWIN) -m 755 -u $(OWN) -g $(GRP) nvt/win/readme.wri
	$(INS) -f $(NVTWIN) -m 755 -u $(OWN) -g $(GRP) nvt/win/setup.exe
	$(INS) -f $(NVTWIN) -m 755 -u $(OWN) -g $(GRP) nvt/win/setup.ins
	$(INS) -f $(NVTWIN) -m 755 -u $(OWN) -g $(GRP) nvt/win/setup.lgo
	$(INS) -f $(NVTWIN) -m 755 -u $(OWN) -g $(GRP) nvt/win/setup.pkg
	$(INS) -f $(NVTWIN) -m 755 -u $(OWN) -g $(GRP) nvt/win/~ins0762.lib

$(DIRS):
		[ -d $@ ] || mkdir -p $@

clean:
	

clobber: 
	
