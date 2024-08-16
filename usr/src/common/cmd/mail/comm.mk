#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/comm.mk	1.9.6.12"
#
#	mail make file
#

# If SVR4DT, use the following...

# first, in case $CMDRULES is empty, we define
# the things that $CMDRULES is supposed to supply.
LINT= $(PFX)lint -s
LINTFLAGS= $(LOCALDEF) $(LOCALINC)
INS = install
CFLAGS= -O $(LOCALDEF) $(LOCALINC)
INC= /usr/include
USRLIB= /usr/lib
MBINDIR=/usr/bin
USRBIN= /usr/bin
ETC= /etc
VAR= /var
USRSHARE= /usr/share
#LDFLAGS= -g

include $(CMDRULES)

OWN = 
GRP = 
#CFLAGS= -g $(LOCALDEF) $(LOCALINC)
#LDFLAGS= -g

LDLIBS=-lgen
MBINDIR = $(USRBIN)
CBINDIR = $(USRBIN)
MBOXDIR = $(VAR)/mail
FILEDIR = $(ETC)/mail
LFILEDIR = /etc/mail
SHRLIB  = $(USRSHARE)/lib
MPDIR = $(USRLIB)/mail
USRINC = $(ROOT)/$(MACH)/usr/include

REAL_MBOXDIR = /var/mail
REAL_SHRLIB = /usr/share/lib
REAL_PATH = /usr/lib/mail/surrcmd:/usr/bin
REAL_SHELL = /sbin/sh
REAL_VARSPOOLSMTPQ = /var/spool/smtpq

SYMLINK = ln -s
VERS = SVR4
VERS2 = -DSVR4_1
#LIBMAIL = libmail.a
LIBMAIL = -lmail
LIBRE = libre.a
LOCALDEF= -D$(VERS) $(VERS2)
LOCALINC= -I. -I$(INC)
#LD_LIBS= $(LIBMAIL) $(LIBRE)
LD_LIBS= $(LIBRE)
MAILLIBS = $(LD_LIBS) -lw
TMPDIR = /usr/tmp
SH_OPTCMD=USEGETOPTS
SH_PRTCMD=USEPFMT

REFLAGS = -qp -v
#REFLAGS = -g -v
LINT_C = lint-c
VAC_MSG_LOC = $(USRSHARE)/lib/mail
INST_MSGFILES = yes
SMSRCMAKE=src.mk
RANLIB=:

PREFIX=ma
HDR = mail.h libmail.h s_string.h config.h stdc.h r822.h
SURRDIR = $(USRLIB)/mail/surrcmd

DIRS =	$(MBOXDIR) \
	$(MBOXDIR)/:saved \
	$(MBOXDIR)/:forward \
	$(MPDIR) \
	$(FILEDIR) \
	$(SURRDIR) \
	$(SHRLIB)/mail

PRODUCT = mail
LPDEST =
TMPDIR = /usr/tmp

MAKE= make
SLIST=$(PREFIX).sl
ID=$(PREFIX)id

.SUFFIXES: .ln
.c.ln:
	$(LINT) $(LINTFLAGS) $(CPPDEFS) -c $*.c > $*.lerr || { cat $*.lerr; exit 1; }

SRC = \
	Dout.c Tout.c add_recip.c altenviron.c arefwding.c \
	cat.c ckdlivopts.c cksaved.c cksurg_rc.c clr_hinfo.c \
	cmdexpand.c copyback.c copylet.c copymt.c createmf.c del_Hdrs.c \
	del_Msg.c del_Recip.c delete.c doeopt.c doFopt.c done.c doopen.c \
	dowait.c dumpaff.c dumprcv.c encodebody.c errmsg.c fini_Let.c fini_Msg.c \
	fini_Rcpl.c fini_Tmp.c gendeliv.c getarg.c getcomment.c \
	gethead.c getlasthdr.c getline.c getnumbr.c getsurr.c goback.c init.c \
	init_Hdri.c init_Let.c init_Msg.c init_Rcpl.c init_Tmp.c \
	initsurr.c isheader.c isit.c legal.c lock.c \
	mailcomp.c main.c matchsurr.c maxbatch.c \
	mcopylet.c mkdate.c mkdead.c mktmp.c mmail.c msetup_ex.c mta_ercode.c my_open.c \
	new_Hdrs.c new_Msg.c new_Recip.c nwsendlist.c ofrwd.c parse.c pckaffspot.c \
	pckrcvspot.c pickFrom.c pipletr.c poplist.c printhdr.c printmail.c \
	r822_addr.c recip_par.c retmail.c rewrite.c savdead.c savehdrs.c sel_disp.c \
	send2acc.c send2bmvr.c send2clean.c send2d_p.c send2deny.c send2exec.c \
	send2fleft.c send2frt.c send2loc.c send2move.c send2mvr.c \
	send2post.c send2quit.c send2rwrt.c send2tran.c sendlist.c sendmail.c setletr.c \
	setmail.c setsig.c setsurg_bt.c setsurg_rc.c sizehdrs.c \
	stamp.c systm.c tokdef.c validmsg.c
OBJS = $(SRC:.c=.o) $(ID).o

LIBSRC=
#LIBSRC = abspath.c basename.c bcollapse.c cascmp.c casncmp.c config.c check4mld.c \
#	closefiles.c compat.c copystream.c copynstr.c delempty.c encodefile.c errexit.c \
#	expargvec.c getdomain.c getmsgid.c loopfork.c islocal.c istext.c isyesno.c \
#	long2str.c maillock.c mailsystem.c mgetenv.c mgetcharset.c newer.c notifyu.c \
#	parse_ex.c popenvp.c poschown.c rename.c \
#	rmfopen.c rmopendir.c s_string.c setup_exec.c sortafile.c strmove.c \
#	skipspace.c skip2space.c Strerror.c substr.c systemvp.c trimnl.c xgetenv.c \
#	xtmpfile.c
LIBOBJS = $(LIBSRC:.c=.o)

RESRC = re/bm.c re/cw.c re/eg.c re/egbr.c re/egcanon.c re/egcomp.c \
	re/egcw.c re/egerror.c re/eglit.c re/egmatch.c re/egparen.c \
	re/egpos.c re/egstate.c re/re.c re/refile.c
REHDRS= re/io.h re/libc.h re/lre.h re/re.h

DSRC =	my_open.c
DOBJS =

MPSRC = _mail_pipe.c
MPOBJS = $(MPSRC:.c=.o) $(ID).o

CBSRC = ckbinarsys.c
CBOBJS = $(CBSRC:.c=.o) $(ID).o

NSRC =	notify2.c
NOBJS = $(NSRC:.c=.o) $(ID).o

LSRC = maillog.c
LOBJS = $(LSRC:.c=.o) $(ID).o

VSRC = vacation.sh vacation2.sh STD_VAC_MSG

MCSRC = mailcheck.c
MCOBJS = $(MCSRC:.c=.o) $(ID).o

LTOOLS = pmkid

LINTOBJS = $(SRC:.c=.ln) $(ID).ln
LINTERR = $(SRC:.c=.lerr) $(ID).lerr

MISRC = mailinfo.c
MIOBJS = mailinfo.o $(ID).o

PCHSRC = pchown.c
PCHOBJS= pchown.o $(ID).o

UUCSRC = uucollapse.c
UUCOBJS= uucollapse.o $(ID).o

LMSRC = localmail.c
LMOBJS= localmail.o $(ID).o

RTSRC = retest.c
RTOBJS= retest.o $(ID).o

RWSRC = r822_fld.c r822_getc.c r822_sopt.c r822_test.c 
RWOBJS= r822_fld.o r822_getc.o r822_sopt.o r822_test.o $(ID).o

M7BSRC = mimeto7bit.c
M7BOBJS= mimeto7bit.o mimecomm.o $(ID).o

MMTASRC = mimetomta.c
MMTAOBJS= mimetomta.o mimecomm.o $(ID).o

MTAMSRC = mtatomime.c
MTAMOBJS= mtatomime.o mimecomm.o $(ID).o

ALLSRC = $(SRC) $(DSRC) $(MPSRC) $(CBSRC) $(NSRC) $(VSRC) $(LSRC) $(HDR) $(MCSRC) \
	$(MAKEFILE) $(LIBSRC) $(LTOOLS) binarsys \
	datemask rewrite rewrite.sample \
	$(MISRC) $(PCHSRC) $(UUCSRC) $(LMSRC) $(RTSRC) $(RWSRC) \
	$(M7BSRC) $(MTAMSRC) $(MMTASRC)

.MUTEX:	all

all: alllib allmail

#alllib: $(LIBMAIL) $(LIBRE)
alllib:  $(LIBRE)

EXES=	ckbinarsys localmail mail mail_pipe mailcheck \
	mailinfo maillog notify2 pchown uucollapse \
	mimeto7bit mimetomta mtatomime

#allmail: $(EXES) notify llib-lmail.ln std_vac_msg vacation vacation2 email.str
allmail: $(EXES) notify std_vac_msg vacation vacation2 email.str

mail:		$(OBJS) $(DOBJS) $(LD_LIBS)
#	$(CC) -p -L /usr/ccs/lib $(LDFLAGS) -o $(PRODUCT) $(OBJS) $(DOBJS) $(MAILLIBS) $(LDLIBS) -lmail
	$(CC) $(LDFLAGS) -o $(PRODUCT) $(OBJS) $(DOBJS) $(MAILLIBS) $(LDLIBS) -lmail

#mail_pipe:	$(MPOBJS) $(LIBMAIL)
mail_pipe:	$(MPOBJS)
	$(CC) $(LDFLAGS) -o mail_pipe $(MPOBJS) $(LIBMAIL) $(LDLIBS)

#ckbinarsys:	$(CBOBJS) $(LIBMAIL)
ckbinarsys:	$(CBOBJS)
	$(CC) $(LDFLAGS) -o ckbinarsys $(CBOBJS) $(LIBMAIL) $(LDLIBS)

#notify2:	$(NOBJS) $(LIBMAIL)
notify2:	$(NOBJS)
	$(CC) $(LDFLAGS) -o notify2 $(NOBJS) $(LIBMAIL) $(LDLIBS)

#mailcheck:	$(MCOBJS) $(LIBMAIL)
mailcheck:	$(MCOBJS)
	$(CC) $(LDFLAGS) -o mailcheck $(MCOBJS) $(LIBMAIL) $(LDLIBS)

#maillog:	$(LOBJS) $(LIBMAIL)
maillog:	$(LOBJS)
	$(CC) $(LDFLAGS) -o maillog $(LOBJS) $(LIBMAIL) $(LDLIBS)

#mailinfo:	$(MIOBJS) $(LIBMAIL)
mailinfo:	$(MIOBJS)
	$(CC) $(LDFLAGS) -o mailinfo $(MIOBJS) $(LIBMAIL) $(LDLIBS)

#pchown:		$(PCHOBJS) $(LIBMAIL)
pchown:		$(PCHOBJS)
	$(CC) $(LDFLAGS) -o pchown $(PCHOBJS) $(LIBMAIL) $(LDLIBS)

#uucollapse:	$(UUCOBJS) $(LIBMAIL)
uucollapse:	$(UUCOBJS)
	$(CC) $(LDFLAGS) -o uucollapse $(UUCOBJS) $(LIBMAIL) $(LDLIBS)

#localmail:	$(LMOBJS) $(LIBMAIL)
localmail:	$(LMOBJS)
	$(CC) $(LDFLAGS) -o localmail $(LMOBJS) $(LIBMAIL) $(LDLIBS)

retest:		$(RTOBJS) $(LIBRE)
	$(CC) $(LDFLAGS) -o retest $(RTOBJS) $(LIBRE) $(LDLIBS)

r822_test:	r822_addr.o $(RWOBJS) $(LIBRE)
	$(CC) $(LDFLAGS) -o r822_test r822_addr.o $(RWOBJS) $(LIBRE) $(LDLIBS)

encodefile:	encodefile.c
	cp encodefile.c Mencodefile.c
	$(CC) -DMAIN $(CFLAGS) $(LDFLAGS) -o encodefile Mencodefile.c
	rm -f Mencodefile.c

#mimeto7bit:	$(M7BOBJS) $(LIBMAIL)
mimeto7bit:	$(M7BOBJS)
	$(CC) $(LDFLAGS) -o mimeto7bit $(M7BOBJS) $(LIBMAIL)

#mimetomta:	$(MMTAOBJS) $(LIBMAIL)
mimetomta:	$(MMTAOBJS)
	$(CC) $(LDFLAGS) -o mimetomta $(MMTAOBJS) $(LIBMAIL)

#mtatomime:	$(MTAMOBJS) $(LIBMAIL)
mtatomime:	$(MTAMOBJS)
	$(CC) $(LDFLAGS) -o mtatomime $(MTAMOBJS) $(LIBMAIL)

email.str: pfmt.msgs
	sed	-e 's/^:[0-9][0-9]*://' \
		-e 's/^:U_[0-9][0-9]*://' \
		-e '/^#ident.*attmail/d' < pfmt.msgs > email.str

mcs-d: $(EXES)
	mcs -d $(EXES)

debug:
	$(MAKE) "CFLAGS=-g -DDEBUG" DOBJS=my_open.o LDFLAGS= PRODUCT=Dmail mail

#$(LIBMAIL): $(LIBOBJS)
#	$(AR) $(ARFLAGS) $(LIBMAIL) $?
#	$(RANLIB) $(LIBMAIL)

$(LIBRE): $(RESRC) $(REHDRS)
	@echo '\t( cd re;'; cd re && $(MAKE) $(LIBRE) VERS=$(VERS) VERS2=$(VERS2) REFLAGS="$(REFLAGS)" CMDRULES=$(CMDRULES); echo '\t)'
	$(RANLIB) $(LIBRE)

rewrite.c: rewrite.y
	@echo "Expect: conflicts: 1 shift/reduce"
	$(YACC) rewrite.y
	mv y.tab.c rewrite.c

r822_addr.c: r822_addr.y
	$(YACC) r822_addr.y
	mv y.tab.c r822_addr.c

#llib-lmail.ln: llib-lmail maillock.h libmail.h
#	case $(LINT_C) in \
#		lint-c ) $(LINT) $(LINTFLAGS) $(LOCALDEF) $(LOCALINC) -o mail llib-lmail ;; \
#		*    )	 $(CC) -E -C -Dlint -D$(VERS) -I. -I$(INC) -I$(CRX)/usr/include llib-lmail | \
#			 /usr/lib/lint1 -vx -H$(TMPDIR)/hlint.$$$$ > $(TMPDIR)/nlint$$$$ && \
#			 mv $(TMPDIR)/nlint$$$$ llib-lmail.ln; \
#			 rm -f $(TMPDIR)/hlint$$$$ ;; \
#	esac

std_vac_msg: STD_VAC_MSG
	grep -v '^#.*ident' < STD_VAC_MSG > std_vac_msg

EDITPATH=	\
	sed < $? > $@ \
	    -e 's!REAL_PATH!$(REAL_PATH)!g' \
	    -e 's!USR_SHARE_LIB!$(REAL_SHRLIB)!g' \
	    -e 's!FORWARDDIR!$(REAL_MBOXDIR)/:forward!g' \
	    -e 's!MBOXDIR!$(REAL_MBOXDIR)!g' \
	    -e 's!VARSPOOLSMTPQ!$(REAL_VARSPOOLSMTPQ)!g' \
	    -e 's!SHELL!$(REAL_SHELL)!g' \
	    -e 's!^$(SH_OPTCMD) !!' \
	    -e 's!^$(SH_PRTCMD) !!' \
	    -e '/^USE/d' \
	    -e 's!VAR_MAIL!$(REAL_MBOXDIR)!g'

notify: notify.sh
	$(EDITPATH)

vacation: vacation.sh
	$(EDITPATH)

vacation2: vacation2.sh
	$(EDITPATH)

#maillock.h: maillock.H
#	$(EDITPATH)

ckdirs:
	@for i in $(DIRS); \
	do \
		echo "\tmkdir $${i}"; \
		[ ! -d $$i ] && mkdir -p $$i; \
		echo "\t$(CH)chmod 775 $${i}"; \
		$(CH)chmod 775 $${i}; \
		echo "\t$(CH)chgrp mail $${i}"; \
		$(CH)chgrp mail $${i}; \
		echo; \
	done

calls:
	cflow $(CPPDEFS) $(SRC) > cflow.out
	cflow -r $(CPPDEFS) $(SRC) > cflow-r.out
	calls $(CPPDEFS) $(SRC) > calls.out
	cscope -b $(SRC) > calls.out
	ccalls | sort -u > ccalls.edges
	ccalls -p | sort -u > ccalls.params

# save all administerable data files. ('make save' before 'make install')
save:
	for i in /etc/mail/datemask /etc/mail/binarsys /etc/mail/rewrite ;\
	do if [ -f $$i ]; then mv $$i $$i.old; fi; \
	done

install: ckdirs installmail

installmail: allmail nocheckinstallmail

nocheckinstallmail:
	case $(INST_MSGFILES) in \
	  yes ) \
	    [ ! -d $(USRLIB)/locale/C/MSGFILES ] && mkdir -p $(USRLIB)/locale/C/MSGFILES ; \
	    $(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 -u bin -g mail email.str ;; \
	esac

	rm -f $(MBINDIR)/$(PRODUCT) $(MBINDIR)/r$(PRODUCT)
	$(INS) -f $(MBINDIR) -m 2555 -u bin -g mail $(PRODUCT)
	ln $(MBINDIR)/$(PRODUCT) $(MBINDIR)/r$(PRODUCT)
	$(INS) -f $(VAC_MSG_LOC) -m 444 -u bin -g bin std_vac_msg

	rm -f $(MPDIR)/mail_pipe
	$(INS) -f $(MPDIR)   -m 4511 -u root -g bin mail_pipe
	$(INS) -f $(MPDIR)   -m 4511 -u root -g bin pchown

	$(INS) -f $(SURRDIR) -m 755 -u bin -g bin ckbinarsys
	$(INS) -f $(SURRDIR) -m 2755 -u bin -g mail maillog
	$(INS) -f $(SURRDIR) -m 755 -u bin -g bin mailinfo
	$(INS) -f $(SURRDIR) -m 755 -u bin -g bin uucollapse
	$(INS) -f $(SURRDIR) -m 755 -u bin -g bin localmail
	$(INS) -f $(SURRDIR) -m 755 -u bin -g bin mimeto7bit
	$(INS) -f $(SURRDIR) -m 755 -u bin -g bin mimetomta
	$(INS) -f $(SURRDIR) -m 755 -u bin -g bin mtatomime

	$(INS) -f $(CBINDIR) -m 555 -u bin -g bin notify
	$(INS) -f $(MPDIR)   -m 555 -u bin -g bin notify2

	$(INS) -f $(CBINDIR) -m 555 -u bin -g bin vacation
	$(INS) -f $(MPDIR)   -m 555 -u bin -g bin vacation2
	$(INS) -f $(FILEDIR) -m 644 -u bin -g bin datemask
	$(INS) -f $(FILEDIR) -m 644 -u bin -g bin rewrite
	$(INS) -f $(FILEDIR) -m 644 -u bin -g bin rewrite.samp

	$(INS) -f $(FILEDIR) -m 644 -u bin -g bin binarsys
	rm -f $(USRLIB)/binarsys
	$(CH)$(SYMLINK) $(LFILEDIR)/binarsys $(USRLIB)/binarsys

	#$(INS) -f $(USRLIB) -m 644 -u bin -g bin $(LIBMAIL)
	# also copy the library into the cross compilation environment
	#if [ -n "$${TOOLS}" -a -d "$${TOOLS}/usr/lib" ];then $(INS) -f $${TOOLS}/usr/lib -m 644 -u bin -g bin $(LIBMAIL); fi
	#$(INS) -f $(USRLIB) -m 644 -u bin -g bin llib-lmail
	#$(INS) -f $(USRLIB) -m 644 -u bin -g bin llib-lmail.ln
	#$(INS) -f $(USRINC) -m 644 -u bin -g bin maillock.h
	# also copy the header into the cross compilation environment
	#if [ -n "$(TARGETINC)" ];then $(INS) -f $(TARGETINC) -m 644 -u bin -g bin maillock.h; fi

	$(INS) -f $(CBINDIR) -m 555 -u bin -g mail mailcheck

#$(CBOBJS) $(DOBJS)   $(LIBOBJS): $(HDR)
$(CBOBJS) $(DOBJS): $(HDR)
$(LMOBJS) $(LOBJS)  $(MCOBJS)  $(MIOBJS): $(HDR)
$(MPOBJS) $(NOBJS)  $(PCHOBJS) $(OBJS): $(HDR)
$(RTOBJS) $(RWOBJS) $(UUCOBJS) $(LINTOBJS): $(HDR)
$(M7BOBJS) $(MMTAOBJS) $(MTAMOBJS): $(HDR)

print:
	pr -n $(ALLSRC) | lp $(LPDEST)

#lintit: $(LINTOBJS) llib-lmail.ln /tmp
lintit: $(LINTOBJS) /tmp
#	@echo ==== libmail ====
#	$(LINT) $(LINTFLAGS) $(CPPDEFS) $(LIBSRC)
	@echo ==== mail ====
	for i in $(LINTERR);do if [ -s $$i ]; then echo `basename $$i .lerr`.c ; cat $$i; fi; done
#	$(LINT) $(LINTFLAGS) $(LINTOBJS) llib-lmail.ln
	@echo ==== ckbinarsys ====
	$(LINT) $(LINTFLAGS) $(CPPDEFS) $(CBSRC) casncmp.c $(ID).c
	@echo ==== mail_pipe ====
	$(LINT) $(LINTFLAGS) $(CPPDEFS) $(MPSRC) xgetenv.c setup_exec.c skipspace.c $(ID).c

clean: cleanmail

cleanmail: cleanlint cleanre
	-rm -f *.o rewrite.c r822_addr.c y.tab.c y.output Mencodefile.c
cleanlint:
	-rm -f $(LINTOBJS)
	-rm -f $(LINTERR)
cleanre:
	@echo '\t( cd re;'; cd re; $(MAKE) clean CMDRULES=$(CMDRULES); echo '\t)'

clobber: clobbermail

clobbermail: cleanmail
	rm -f mail rmail Dmail a.out mail_pipe ckbinarsys notify notify2 \
		std_vac_msg core *makeout* mon.out nohup.out pchown \
		llib-lmail.ln vacation vacation2 libre.a maillog \
		mailinfo mailcheck email.str $(ID).* \
		uucollapse localmail retest r822_test encodefile \
		mimeto7bit mimetomta mtatomime
	@echo '\t( cd re;'; cd re; $(MAKE) clobber CMDRULES=$(CMDRULES); echo '\t)'


$(ID).c: $(SLIST)
	sh ./pmkid $(SLIST)

chgrp:
	-chgrp mail mail
	chmod g+s mail

localinstall: allmail nochecklocalinstall

nochecklocalinstall:
	case $(INST_MSGFILES) in \
	  yes ) \
	    [ ! -d /usr/lib/locale/C/MSGFILES ] && mkdir -p /usr/lib/locale/C/MSGFILES ; \
	    $(INS) -f /usr/lib/locale/C/MSGFILES -m 644 -u bin -g mail email.str ;; \
	esac

	rm -f /usr/bin/$(PRODUCT) /usr/bin/r$(PRODUCT)
	$(INS) -f /usr/bin -m 2555 -u bin -g mail $(PRODUCT)
	ln /usr/bin/$(PRODUCT) /usr/bin/r$(PRODUCT)
	$(INS) -f /usr/share/lib/mail -m 444 -u bin -g bin std_vac_msg

	rm -f /usr/lib/mail/mail_pipe
	$(INS) -f /usr/lib/mail   -m 4511 -u root -g bin mail_pipe
	$(INS) -f /usr/lib/mail   -m 4511 -u root -g bin pchown

	$(INS) -f /usr/lib/mail/surrcmd -m 755 -u bin -g bin ckbinarsys
	$(INS) -f /usr/lib/mail/surrcmd -m 2755 -u bin -g mail maillog
	$(INS) -f /usr/lib/mail/surrcmd -m 755 -u bin -g bin mailinfo
	$(INS) -f /usr/lib/mail/surrcmd -m 755 -u bin -g bin uucollapse
	$(INS) -f /usr/lib/mail/surrcmd -m 755 -u bin -g bin localmail
	$(INS) -f /usr/lib/mail/surrcmd -m 755 -u bin -g bin mimeto7bit
	$(INS) -f /usr/lib/mail/surrcmd -m 755 -u bin -g bin mimetomta
	$(INS) -f /usr/lib/mail/surrcmd -m 755 -u bin -g bin mtatomime

	$(INS) -f /usr/bin -m 555 -u bin -g bin notify
	$(INS) -f /usr/lib/mail   -m 555 -u bin -g bin notify2

	$(INS) -f /usr/bin -m 555 -u bin -g bin vacation
	$(INS) -f /usr/lib/mail   -m 555 -u bin -g bin vacation2
	$(INS) -f /etc/mail -m 644 -u bin -g bin datemask
	$(INS) -f /etc/mail -m 644 -u bin -g bin rewrite
	$(INS) -f /etc/mail -m 644 -u bin -g bin rewrite.samp

	$(INS) -f /etc/mail -m 644 -u bin -g bin binarsys
	rm -f /usr/lib/binarsys
	$(CH)$(SYMLINK) $/etc/mail/binarsys /usr/lib/binarsys

	#$(INS) -f /usr/lib -m 644 -u bin -g bin $(LIBMAIL)
	$(INS) -f /usr/lib -m 644 -u bin -g bin llib-lmail
	#$(INS) -f /usr/lib -m 644 -u bin -g bin llib-lmail.ln
	#$(INS) -f /usr/include -m 644 -u bin -g bin maillock.h
	$(INS) -f /usr/bin -m 555 -u bin -g mail mailcheck

