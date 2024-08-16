#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libnsl:common/lib/libnsl/rpc/rpc.mk	1.9.18.5"
#ident	"$Header: $"

# Sun RPC is a product of Sun Microsystems, Inc. and is provided for
# unrestricted use provided that this legend is included on all tape
# media and as a part of the software program in whole or part.  Users
# may copy or modify Sun RPC without charge, but are not authorized
# to license or distribute it to anyone else except as part of a product or
# program developed by the user.
#
# SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
# WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
# PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
#
# Sun RPC is provided with no support and without any obligation on the
# part of Sun Microsystems, Inc. to assist in its use, correction,
# modification or enhancement.
#
# SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
# INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
# OR ANY PART THEREOF.
#
# In no event will Sun Microsystems, Inc. be liable for any lost revenue
# or profits or other special, indirect and consequential damages, even if
# Sun has been advised of the possibility of such damages.
#
# Sun Microsystems, Inc.
# 2550 Garcia Avenue
# Mountain View, California  94043
#
# Copyright (c) 1986-1991 by Sun Microsystems Inc.

include $(LIBRULES)
include ../libnsl.rules

#
# Kernel level sources are missing here. Only USER
#	level rpc sources here
#
# Sources deleted because of duplicate or obsolete functionality
#	clnt_udp.c\ clnt_tcp.c
#	get_myaddress.c\
#	pmap_getport.c\ pmap_getmaps.c
#	pmap_prot2.c\ pmap_rmt.c
#	rpc_dtbsize.c\
#	svc_tcp.c\ svc_udp.c
#	xdr_mbuf.c \: Kernel stuff
#
# Sources renamed due to file name length and copyright infringement problems:
#	authunix_prot.c\ ==> authsys_prot.c
#	svc_auth_unix.c\ ==>svc_auth_sys.c
#	xdr_reference.c\ ==> xdr_refer.c
#	auth_unix.h ==> auth_sys.h
#
# New sources added:
#	clnt_bcast.c\: clnt_broadcast here.
#	clnt_dg.c\: All datagram support for the clients
#	clnt_vc.c\: All connectionful (virtual circuit) support for the clients
#	clnt_generic.c\ : All the new tli stuff for client side
#	rpc_soc.c\ : All the socket compatibility stuff
#	rpc_generic.c\ : generic routines used by both svc and client
#	rpcb_prot.c\: All the new rpcbind protocol xdr routines here
#	rpcb_clnt.h: All the rpcbind client side interfaces.
#	svc_dg.c\: All datagram support for the servers
#	svc_vc.c\: All connectionful (virtual circuit) support for the servers
#	svc_generic.c\ : All the new interface routines for svc side
#
# Sources in the compatibility package
#	clnt_soc.h
#	dbx_rpc.c\
#	pmap_clnt.c\
#	pmap_prot.c\
#	port.c\
#	rpc_soc.c\
#	svc_soc.h
#
# Source in the secure package
#	auth_des.c\
#	authdes_prot.c\
#	rpcdname.c\
#	rtime_tli.c\
#	svcdesname.c\
#	svc_authdes.c\
#
# Headers included in component head.usrs:
# HDRS = auth.h auth_des.h auth_unix.h auth_sys.h clnt.h clnt_soc.h \
#	keyprot.h \
#	nettype.h pmap_clnt.h pmap_prot.h pmap_rmt.h raw.h \
#	rpc.h rpc_com.h rpcent.h rpc_msg.h rpcb_clnt.h rpcb_prot.h \
#	svc.h svc_soc.h svc_auth.h types.h xdr.h \
#	rpc_mt.h

LOCALDEF =	-D_NSL_RPC_ABI -DPORTMAP -DYP $(MPOPT) $(NSL_LOCALDEF)
#LOCALDEF = -D_NSL_RPC_ABI -DCALLBACK -Dhavdndbm -Dlint -Duse_file

# These should be defined in libnsl.mk
#DEVDEF = -DND_DEBUG -DRPC_CACHE_DEBUG -DRPC_DEBUG

HDRS =	auth.h auth_des.h auth_sys.h auth_unix.h clnt.h clnt_soc.h \
	key_prot.h nettype.h \
	pmap_clnt.h pmap_prot.h pmap_rmt.h raw.h \
	rpc.h rpc_com.h rpcent.h rpc_msg.h rpcb_clnt.h rpcb_prot.h \
	svc.h svc_soc.h svc_auth.h types.h xdr.h \
	rpc_mt.h

SECOBJS=auth_des.o authdes_prot.o getdname.o key_prot.o key_call.o \
	netname.o netnamer.o openchild.o rpcdname.o rtime_tli.o\
	svcauth_des.o svcdesname.o

INETOBJS=gethostent.o inet_ntoa.o

OBJS =	$(SECOBJS) $(INETOBJS)  auth_none.o auth_sys.o authsys_prot.o \
	clnt_bcast.o clnt_dg.o clnt_generic.o \
	clnt_perror.o clnt_raw.o clnt_simple.o clnt_vc.o \
	gethostname.o getrpcent.o \
	pmap_clnt.o pmap_prot.o port.o publickey.o \
	rpc_callmsg.o rpc_comdata.o rpc_generic.o \
	rpc_mt.o rpc_prot.o rpc_sel2poll.o rpc_soc.o \
	rpc_trace.o rpcb_clnt.o rpcb_prot.o rpcb_st_xdr.o secretkey.o \
	ti_opts.o svc.o svc_auth.o svc_auth_sys.o svc_dg.o \
	svc_vc.o svc_generic.o svc_raw.o svc_run.o svc_simple.o \
	svid_funcs.o syslog.o xdr.o xdr_array.o xdr_float.o xcrypt.o\
	xdr_mem.o xdr_rec.o xdr_refer.o xdr_sizeof.o xdr_stdio.o 

LIBOBJS= ../auth_des.o ../authdes_prot.o ../getdname.o ../key_prot.o \
	../key_call.o ../netname.o ../netnamer.o ../openchild.o \
	../rpcdname.o ../rtime_tli.o ../svcauth_des.o ../svcdesname.o \
	../gethostent.o ../inet_ntoa.o \
	../auth_none.o ../auth_sys.o ../authsys_prot.o \
	../clnt_bcast.o ../clnt_dg.o ../clnt_generic.o \
	../clnt_perror.o ../clnt_raw.o ../clnt_simple.o ../clnt_vc.o \
	../gethostname.o ../getrpcent.o \
	../pmap_clnt.o ../pmap_prot.o ../port.o ../publickey.o \
	../rpc_callmsg.o ../rpc_comdata.o ../rpcdname.o \
	../rpc_generic.o ../rpc_mt.o ../rpc_prot.o ../rpc_sel2poll.o \
	../rpc_soc.o ../rpc_trace.o ../rpcb_clnt.o ../rpcb_prot.o \
	../rpcb_st_xdr.o ../secretkey.o \
	../svc.o ../svc_auth.o ../svc_auth_sys.o ../svc_dg.o \
	../svc_generic.o ../svc_raw.o ../svc_run.o ../svc_simple.o \
	../svc_vc.o ../svid_funcs.o ../syslog.o ../ti_opts.o\
	../xdr.o ../xdr_array.o ../xdr_float.o ../xdr_mem.o ../xdr_rec.o \
	../xdr_refer.o ../xdr_sizeof.o ../xdr_stdio.o 


SRCS = $(OBJS:.o=.c)

all: $(OBJS)
	cp $(OBJS) ../

#auth_des.c: $(INC)/stdlib.h \
#	$(INC)/rpc/rpc.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/rpc/des_crypt.h \
#	$(INC)/sys/time.h \
#	$(INC)/sys/syslog.h
#
#auth_none.c: $(INC)/stdlib.h \
#	$(INC)/rpc/types.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/rpc/xdr.h \
#	$(INC)/rpc/auth.h \
#	rpc_mt.h
#
#auth_sys.c: $(INC)/stdio.h \
#	$(INC)/sys/syslog.h \
#	$(INC)/rpc/types.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/rpc/xdr.h \
#	$(INC)/rpc/auth.h \
#	$(INC)/rpc/auth_sys.h 
#
#authdes_prot.c: $(INC)/sys/types.h \
#	$(INC)/rpc/types.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/rpc/xdr.h \
#	$(INC)/rpc/auth.h \
#	$(INC)/rpc/auth_des.h
#
#authsys_prot.c: $(INC)/rpc/types.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/rpc/xdr.h \
#	$(INC)/rpc/auth.h \
#	$(INC)/rpc/auth_sys.h
#
#clnt_bcast.c: $(INC)/string.h \
#	$(INC)/rpc/rpc.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/rpc/nettype.h \
#	$(INC)/sys/poll.h \
#	$(INC)/netdir.h \
#	$(INC)/rpc/pmap_prot.h \
#	$(INC)/rpc/pmap_clnt.h \
#	$(INC)/rpc/pmap_rmt.h \
#	$(INC)/stdio.h \
#	$(INC)/errno.h \
#	$(INC)/sys/syslog.h
#
#clnt_bsoc.c: $(INC)/rpc/rpc.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/netconfig.h
#
#clnt_dg.c: $(INC)/stdlib.h \
#	$(INC)/rpc/rpc.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/errno.h \
#	$(INC)/sys/poll.h \
#	$(INC)/sys/syslog.h \
#	$(INC)/sys/types.h \
#	$(INC)/sys/kstat.h \
#	$(INC)/sys/time.h \
#	rpc_mt.h
#
#clnt_generic.c: $(INC)/stdio.h \
#	$(INC)/rpc/rpc.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/errno.h \
#	$(INC)/rpc/nettype.h \
#	rpc_mt.h
#
#clnt_perror.c: $(INC)/stdio.h \
#	$(INC)/stdlib.h \
#	$(INC)/string.h \
#	$(INC)/netdir.h \
#	$(INC)/rpc/types.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/rpc/auth.h \
#	$(INC)/sys/xti.h \
#	$(INC)/rpc/clnt.h \
#	rpc_mt.h
#
#clnt_raw.c: $(INC)/rpc/rpc.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/rpc/raw.h \
#	$(INC)/sys/syslog.h
#
#clnt_simple.c: $(INC)/stdio.h \
#	$(INC)/stdlib.h \
#	$(INC)/errno.h \
#	$(INC)/rpc/rpc.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/string.h \
#	$(INC)/sys/param.h \
#	rpc_mt.h
#
#clnt_vc.c: $(INC)/stdlib.h \
#	$(INC)/rpc/rpc.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/errno.h \
#	$(INC)/sys/byteorder.h \
#	$(INC)/sys/mkdev.h \
#	$(INC)/sys/poll.h \
#	$(INC)/sys/syslog.h \
#	rpc_mt.h
#
#getdname.c: $(INC)/stdio.h \
#	$(INC)/stdlib.h \
#	$(INC)/sys/types.h \
#	$(INC)/rpc/types.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/sys/utsname.h \
#	$(INC)/sys/systeminfo.h \
#	rpc_mt.h
#
#gethostent.c: $(INC)/stdio.h \
#	$(INC)/rpc/types.h \
#	$(INC)/sys/types.h \
#	$(INC)/sys/socket.h \
#	$(INC)/netdb.h \
#	$(INC)/ctype.h \
#	$(INC)/ndbm.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/sys/syslog.h \
#	rpc_mt.h
#
#gethostname.c: $(INC)/sys/utsname.h \
#	$(INC)/sys/types.h \
#	$(INC)/rpc/trace.h
#
#getrpcent.c: $(INC)/stdio.h \
#	$(INC)/stdlib.h \
#	$(INC)/string.h \
#	$(INC)/sys/types.h \
#	$(INC)/rpc/rpcent.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/sys/syslog.h \
#	rpc_mt.h
#
#inet_ntoa.c: $(INC)/sys/types.h \
#	$(INC)/ctype.h \
#	$(INC)/netinet/in.h \
#	$(INC)/rpc/trace.h \
#	rpc_mt.h
#
#key_call.c: $(INC)/rpc/rpc.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/rpc/key_prot.h \
#	$(INC)/stdio.h \
#	$(INC)/stdlib.h \
#	$(INC)/string.h \
#	$(INC)/netconfig.h \
#	$(INC)/sys/utsname.h \
#	rpc_mt.h
#
#key_prot.c: $(INC)/rpc/rpc.h \
#	$(INC)/rpc/key_prot.h \
#	$(INC)/rpc/trace.h
#
#netname.c: $(INC)/string.h \
#	$(INC)/sys/param.h \
#	$(INC)/ctype.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/rpc/rpc.h
#
#netnamer.c: $(INC)/sys/param.h \
#	$(INC)/rpc/rpc.h \
#	$(INC)/ctype.h \
#	$(INC)/stdio.h \
#	$(INC)/grp.h \
#	$(INC)/pwd.h \
#	$(INC)/string.h \
#	$(INC)/syslog.h \
#	$(INC)/rpc/trace.h
#
#openchild.c: $(INC)/stdio.h \
#	$(INC)/sys/types.h \
#	$(INC)/rpc/trace.h
#
#pmap_clnt.c: $(INC)/string.h \
#	$(INC)/rpc/rpc.h \
#	$(INC)/rpc/nettype.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/netdir.h \
#	$(INC)/rpc/pmap_prot.h \
#	$(INC)/rpc/pmap_clnt.h \
#	$(INC)/rpc/pmap_rmt.h \
#	$(INC)/sys/syslog.h \
#	$(INC)/netinet/in.h \
#	$(INC)/sys/socket.h \
#	rpc_mt.h
#
#pmap_prot.c: $(INC)/rpc/types.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/rpc/xdr.h \
#	$(INC)/rpc/pmap_prot.h \
#	$(INC)/rpc/pmap_rmt.h
#
#port.c: $(INC)/rpc/rpc.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/stdio.h
#
#publickey.c: $(INC)/stdio.h \
#	$(INC)/pwd.h \
#	$(INC)/rpc/rpc.h \
#	$(INC)/rpc/key_prot.h \
#	$(INC)/string.h \
#	$(INC)/syslog.h \
#	$(INC)/rpc/trace.h
#
#rpc_callmsg.c: $(INC)/sys/param.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/rpc/rpc.h \
#	$(INC)/sys/byteorder.h
#
#rpc_comdata.c: $(INC)/rpc/rpc.h \
#	$(INC)/rpc/trace.h
#
#rpc_generic.c: $(INC)/stdio.h \
#	$(INC)/sys/types.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/rpc/rpc.h \
#	$(INC)/rpc/nettype.h \
#	$(INC)/sys/param.h \
#	$(INC)/sys/mkdev.h \
#	$(INC)/sys/stat.h \
#	$(INC)/ctype.h \
#	$(INC)/sys/resource.h \
#	$(INC)/netconfig.h \
#	$(INC)/malloc.h \
#	rpc_mt.h
#
#rpc_mt.c: $(INC)/stdlib.h \
#	$(INC)/rpc/rpc.h \
#	$(INC)/thread.h \
#	$(INC)/synch.h \
#	$(INC)/rpc/trace.h
#
#rpc_prot.c: $(INC)/sys/param.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/rpc/rpc.h
#
#rpc_sel2poll.c: $(INC)/sys/select.h \
#	$(INC)/sys/types.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/sys/time.h \
#	$(INC)/sys/poll.h
#
#rpc_soc.c: $(INC)/stdio.h \
#	$(INC)/sys/types.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/rpc/rpc.h \
#	$(INC)/netinet/in.h \
#	$(INC)/sys/socket.h \
#	$(INC)/netdb.h \
#	$(INC)/netdir.h \
#	$(INC)/errno.h \
#	$(INC)/sys/syslog.h \
#	$(INC)/rpc/pmap_clnt.h \
#	$(INC)/rpc/pmap_prot.h \
#	$(INC)/rpc/nettype.h \
#	rpc_mt.h
#
#rpc_trace.c: $(INC)/stdio.h \
#	$(INC)/sys/types.h \
#	$(INC)/sys/time.h \
#	$(INC)/sys/fcntl.h \
#	$(INC)/rpc/types.h \
#	$(INC)/rpc/trace.h \
#	rpc_mt.h \
#	$(INC)/thread.h
#
#rpcb_clnt.c: $(INC)/rpc/rpc.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/rpc/rpcb_prot.h \
#	$(INC)/netconfig.h \
#	$(INC)/netdir.h \
#	$(INC)/rpc/nettype.h \
#	$(INC)/netinet/in.h \
#	$(INC)/stdio.h \
#	$(INC)/sys/utsname.h \
#	$(INC)/stdlib.h \
#	$(INC)/string.h \
#	rpc_mt.h
#
#rpcb_prot.c: $(INC)/rpc/rpc.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/rpc/types.h \
#	$(INC)/rpc/xdr.h \
#	$(INC)/rpc/rpcb_prot.h
#
#rpcb_st_xdr.c: $(INC)/rpc/rpc.h \
#	$(INC)/rpc/trace.h
#
#rpcdname.c: $(INC)/string.h \
#	$(INC)/sys/types.h \
#	$(INC)/rpc/trace.h \
#	rpc_mt.h
#
#rtime_tli.c: $(INC)/rpc/rpc.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/errno.h \
#	$(INC)/sys/poll.h \
#	$(INC)/rpc/nettype.h \
#	$(INC)/netdir.h \
#	$(INC)/stdio.h
#
#secretkey.c: $(INC)/stdio.h \
#	$(INC)/pwd.h \
#	$(INC)/rpc/rpc.h \
#	$(INC)/rpc/key_prot.h \
#	$(INC)/string.h \
#	$(INC)/rpc/trace.h
#
#svc.c: $(INC)/errno.h \
#	$(INC)/sys/types.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/rpc/rpc.h \
#	$(INC)/rpc/pmap_clnt.h \
#	$(INC)/sys/poll.h \
#	$(INC)/netconfig.h \
#	$(INC)/sys/syslog.h \
#	$(INC)/stdlib.h \
#	$(INC)/thread.h \
#	$(INC)/stropts.h \
#	rpc_mt.h
#
#svc_auth.c: $(INC)/rpc/rpc.h \
#	$(INC)/sys/types.h \
#	$(INC)/rpc/trace.h \
#	rpc_mt.h
#
#svc_auth_sys.c: $(INC)/stdio.h \
#	$(INC)/rpc/rpc.h \
#	$(INC)/sys/syslog.h \
#	$(INC)/sys/types.h \
#	$(INC)/rpc/trace.h
#
#svc_dg.c: $(INC)/stdio.h \
#	$(INC)/sys/types.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/rpc/rpc.h \
#	$(INC)/errno.h \
#	$(INC)/sys/syslog.h \
#	$(INC)/netconfig.h \
#	$(INC)/netdir.h
#
#svc_generic.c: $(INC)/stdio.h \
#	$(INC)/string.h \
#	$(INC)/rpc/rpc.h \
#	$(INC)/sys/types.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/errno.h \
#	$(INC)/sys/syslog.h \
#	$(INC)/rpc/nettype.h \
#	$(INC)/malloc.h \
#	rpc_mt.h
#
#svc_raw.c: $(INC)/rpc/rpc.h \
#	$(INC)/sys/types.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/rpc/raw.h
#
#svc_run.c: $(INC)/rpc/rpc.h \
#	$(INC)/errno.h \
#	$(INC)/sys/poll.h \
#	$(INC)/sys/types.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/sys/syslog.h \
#	rpc_mt.h
#
#svc_simple.c: $(INC)/stdio.h \
#	$(INC)/stdlib.h \
#	$(INC)/string.h \
#	$(INC)/rpc/rpc.h \
#	$(INC)/sys/types.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/sys/syslog.h \
#	$(INC)/rpc/nettype.h \
#	rpc_mt.h
#
#svc_vc.c: $(INC)/stdio.h \
#	$(INC)/rpc/rpc.h \
#	$(INC)/sys/types.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/errno.h \
#	$(INC)/sys/stat.h \
#	$(INC)/sys/mkdev.h \
#	$(INC)/sys/poll.h \
#	$(INC)/sys/syslog.h \
#	$(INC)/rpc/nettype.h \
#	$(INC)/xti.h
#
#svcauth_des.c: $(INC)/string.h \
#	$(INC)/rpc/des_crypt.h \
#	$(INC)/rpc/rpc.h \
#	$(INC)/sys/types.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/sys/syslog.h \
#	rpc_mt.h
#
#svcdesname.c: $(INC)/string.h \
#	$(INC)/rpc/des_crypt.h \
#	$(INC)/rpc/rpc.h \
#	$(INC)/sys/types.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/sys/syslog.h \
#	rpc_mt.h
#
#svid_funcs.c: $(INC)/rpc/rpc.h \
#	$(INC)/sys/types.h \
#	$(INC)/rpc/trace.h
#
#ti_opts.c: $(INC)/stdio.h \
#	$(INC)/errno.h \
#	$(INC)/sys/types.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/xti.h \
#	$(INC)/rpc/rpc.h \
#	$(INC)/sys/ticlts.h \
#	$(INC)/sys/ticots.h \
#	$(INC)/sys/ticotsord.h \
#	$(INC)/sys/syslog.h
#
#xcrypt.c: $(INC)/stdio.h \
#	$(INC)/sys/types.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/rpc/des_crypt.h \
#	$(INC)/memory.h
#
#xdr.c: $(INC)/sys/types.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/sys/syslog.h \
#	$(INC)/stdio.h \
#	$(INC)/limits.h \
#	$(INC)/rpc/types.h \
#	$(INC)/rpc/xdr.h
#
#xdr_array.c: $(INC)/sys/types.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/sys/syslog.h \
#	$(INC)/stdio.h \
#	$(INC)/rpc/types.h \
#	$(INC)/rpc/xdr.h \
#	$(INC)/memory.h
#
#xdr_float.c: $(INC)/sys/types.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/stdio.h \
#	$(INC)/rpc/types.h \
#	$(INC)/rpc/xdr.h
#
#xdr_mem.c: $(INC)/sys/types.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/rpc/types.h \
#	$(INC)/rpc/xdr.h \
#	$(INC)/memory.h
#
#xdr_rec.c: $(INC)/stdio.h \
#	$(INC)/rpc/types.h \
#	$(INC)/rpc/xdr.h \
#	$(INC)/sys/types.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/sys/syslog.h \
#	$(INC)/memory.h
#
#xdr_refer.c: $(INC)/sys/types.h \
#	$(INC)/rpc/trace.h \
#	$(INC)/sys/syslog.h \
#	$(INC)/stdio.h \
#	$(INC)/rpc/types.h \
#	$(INC)/rpc/xdr.h \
#	$(INC)/memory.h
#
#xdr_sizeof.c: $(INC)/rpc/types.h \
#	$(INC)/rpc/xdr.h \
#	$(INC)/sys/types.h \
#	$(INC)/rpc/trace.h
#
#xdr_stdio.c: $(INC)/rpc/types.h \
#	$(INC)/stdio.h \
#	$(INC)/rpc/xdr.h \
#	$(INC)/sys/types.h \
#	$(INC)/rpc/trace.h

lintit:
	$(LINT) $(LINTFLAGS) $(INCLIST) $(DEFLIST) $(SRCS)

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(LIBOBJS)

strip:	all
	$(STRIP) $(LIBOBJS)

size:	all
	$(SIZE) $(LIBOBJS)
