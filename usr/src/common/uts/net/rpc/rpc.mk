#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/rpc/rpc.mk	1.16"
#ident 	"$Header: $"

#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#

include $(UTSRULES)

# RPCESV is used to compile with AUTH_ESV. However, it is not
# supported in SVR4 ES/MP
#LOCALDEF = -DRPCESV

MAKEFILE=	rpc.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = net/rpc

KRPC = krpc.cf/Driver.o
LFILE = $(LINTDIR)/krpc.ln

MODULES = \
	$(KRPC)

FILES = clnt_clts.o \
	clnt_gen.o \
	svc_gen.o \
	svc_clts.o \
	xdr_mblk.o \
	xdr_mem.o \
	svc.o \
	auth_kern.o \
	rpc_prot.o \
	rpc_calmsg.o \
	xdr.o \
	rpcinit.o \
	rpclk_clnt.o \
	rpclk_svc.o \
	svc_auth.o \
	authu_prot.o \
	svcauthdes.o \
	svc_authu.o \
	xdr_array.o \
	key_call.o \
	key_prot.o \
	clnt_perr.o \
	svcauthesv.o \
	auth_esv.o \
	authesvprt.o \
	auth_des.o \
	authdesprt.o \
	authdesubr.o \
	rpc_subr.o \
	token.o

LFILES = clnt_clts.ln \
	clnt_gen.ln \
	svc_gen.ln \
	svc_clts.ln \
	xdr_mblk.ln \
	xdr_mem.ln \
	svc.ln \
	auth_kern.ln \
	rpc_prot.ln \
	rpc_calmsg.ln \
	xdr.ln \
	rpcinit.ln \
	rpclk_clnt.ln \
	rpclk_svc.ln \
	svc_auth.ln \
	authu_prot.ln \
	svcauthdes.ln \
	svc_authu.ln \
	xdr_array.ln \
	key_call.ln \
	key_prot.ln \
	clnt_perr.ln \
	svcauthesv.ln \
	auth_esv.ln \
	authesvprt.ln \
	auth_des.ln \
	authdesprt.ln \
	authdesubr.ln \
	rpc_subr.ln \
	token.ln

CFILES = clnt_clts.c \
	clnt_gen.c \
	svc_gen.c \
	svc_clts.c \
	xdr_mblk.c \
	xdr_mem.c \
	svc.c \
	auth_kern.c \
	rpc_prot.c \
	rpc_calmsg.c \
	xdr.c \
	rpcinit.c \
	rpclk_clnt.c \
	rpclk_svc.c \
	svc_auth.c \
	authu_prot.c \
	svcauthdes.c \
	svc_authu.c \
	xdr_array.c \
	key_call.c \
	key_prot.c \
	clnt_perr.c \
	svcauthesv.c \
	auth_esv.c \
	authesvprt.c \
	auth_des.c \
	authdesprt.c \
	authdesubr.c \
	rpc_subr.c \
	token.c

SRCFILES = $(CFILES)


all:	$(MODULES)

install: all
	(cd krpc.cf; $(IDINSTALL) -R$(CONF) -M krpc)

$(KRPC): $(FILES)
	$(LD) -r -o $(KRPC) $(FILES)

clean:
	rm -f *.o $(LFILES) *.L $(KRPC)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e krpc

$(LINTDIR):
	-mkdir -p $@

lintit: $(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES);	\
	do \
		echo $$i; \
	done

rpcHeaders = \
	auth.h \
	auth_des.h \
	auth_esv.h \
	auth_sys.h \
	auth_unix.h \
	clnt.h \
	clnt_soc.h \
	des_crypt.h \
	key_prot.h \
	nettype.h \
	pmap_clnt.h \
	pmap_prot.h \
	pmap_rmt.h \
	raw.h \
	rpc.h \
	rpc_com.h \
	rpc_msg.h \
	rpcb_clnt.h \
	rpcb_prot.h \
	rpcent.h \
	svc.h \
	svc_auth.h \
	svc_soc.h \
	types.h \
	token.h \
	xdr.h

headinstall: $(rpcHeaders)
	@-[ -d $(INC)/rpc ] || mkdir -p $(INC)/rpc
	@for f in $(rpcHeaders); \
	 do \
		$(INS) -f $(INC)/rpc -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done


include $(UTSDEPEND)

include $(MAKEFILE).dep
