#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libc-port:gen/makefile	1.17.11.58"
#
# makefile for libc/port/gen
#
#

include $(LIBRULES)

.SUFFIXES: .p .P
PROF=
NONPROF=
PIC=
ROOT=
INCPORT=../inc
INCCPU=../../$(CPU)/inc
SDEFLIST=
INCLIST=-I$(INCCPU) -I$(INCPORT)
SGSINC=../../../../cmd/sgs/inc
INCLIST2=-I$(INC) -I$(SGSINC)/common
AWK=awk

OBJECTS=\
_collelem.o	  _collmbs.o	    _collmult.o	      _collwcs.o	\
_ctype.o	  _divrem.o	    _era_info.o	      _fullocale.o	\
_ipfmt.o  	  _g1txt.o	  _lc_time.o	      _lc_collate.o	\
_loc_data.o	  _locale.o						\
_nlday.o	  _norm_tm.o	    _threxit.o	      _time2tm.o	\
_time2year.o	  _tz_file.o	    _tz_info.o	      _tz_time.o	\
_xftw.o		  abort.o	    abs.o	      aclsort.o	\
addsev.o	  assert.o	    atexit.o	      bsearch.o	\
calloc.o	  cat_init.o	    catclose.o	      catgets.o	\
catopen.o	  cfgetispeed.o	    cfgetospeed.o     cfree.o	\
cfsetispeed.o	  cfsetospeed.o	    cftime.c	      cftime.o	\
clock.o		  closedir.o	    \
ctime.o		  ctime_r.o	    ctype.o	      ctypefcns.o	\
div.o		  drand48.o	    dup2.o	      errlst.o	\
execvp.o	  fattach.o	    fdetach.o	      ffs.o	\
fmtmsg.o	  frexp.o	    ftok.o	      ftw.o	\
getcwd.o	  getdate.o	    getdate_data.o    getdate_gd.o	\
getenv.o	  getgrnam.o	      gethz.o	\
getlogin.o	  getopt.o	      \
getpwnam.o	  getsubopt.o	  gettxt.o	      \
getwidth.o	      gtxt.o	\
hsearch.o	  insque.o	      isastream.o	\
isatty.o	  iswctype.o	    ldexp.o	\
lfind.o		  lfmt.o	    _lc_ctype.o	\
localeconv.o	  lsearch.o	    lvlin.o	      lvlintersect.o	\
lvlout.o	  lvlunion.o	    lvlvalid.o	      malloc.o	\
mkdev.o	    	  mkfifo.o	    mktemp.o	\
mktime.o	  mlock.o	    mlockall.o	      modf.o	\
modff.o		  mon.o		    msync.o	      munlock.o	\
munlockall.o	  new_list.o	    nftw.o	      nl_langinfo.o	\
old_tolower.o	  old_toupper.o	    opendir.o	      opt_data.o	\
optfgfns.o	  perror.o	    pfmt.o	      pfmt_data.o	\
priocntl.o	    procprivl.o	      psiginfo.o	\
psignal.o	  pt.o		    putenv.o	      putpwent.o	\
qsort.o		  raise.o	    rand.o	      rand_r.o	\
readdir.o	  realpath.o	    rename.o	      rewinddir.o	\
seekdir.o	  select.o	    setcat.o	      setlabel.o	\
setlocale.o	  sigflag.o	    siglist.o	      sigsend.o	\
sigsetops.o	  sleep.o	    stdlock.o	\
str2id.o	  str2sig.o	    strftime.o	      strptime.o	\
swab.o		  swapctl.o	    sysconf.o	      syslog.o	\
tcdrain.o	  tcflow.o	    tcflush.o	      tcgetattr.o	\
tcgetpgrp.o	  tcgetsid.o	    tcsendbreak.o     tcsetattr.o	\
tcsetpgrp.o	  tell.o	    telldir.o	      tfind.o	\
time_data.o	  tolower.o	    toupper.o	      truncate.o	\
trwctype.o	  tsearch.o	    ttyname.o	      ttyslot.o	\
tzset.o		  valloc.o	    values-Xa.o	      values-Xc.o	\
values-Xt.o	  vlfmt.o	    vpfmt.o	      waitpid.o	\
wcsftime.o	  wctypefcns.o	    time_gdata.o

MCNT_OBJS=newent.o

POBJECTS=$(OBJECTS:.o=.p) 

PIC_OBJECTS=$(OBJECTS:.o=.P) $(MCNT_OBJS:.o=.P)

nonprof: $(OBJECTS) $(MCNT_OBJS)

prof: $(POBJECTS) $(MCNT_OBJS)

pic: $(PIC_OBJECTS)  

.c.o .c.p .c.P:
	$(NONPROF)@echo $*.c:
	$(NONPROF)$(CC) $(INCLIST) $(DEFLIST) $(SDEFLIST) $(CFLAGS) $(LOCALFLAGS) -c $*.c
	$(PROF)@echo $*.c:
	$(PROF)$(CC) $(INCLIST) $(DEFLIST) $(CFLAGS) $(LOCALFLAGS) -c -p $*.c   && mv $(*F).o $*.p
	$(PIC)@echo $*.c:
	$(PIC)$(CC) $(INCLIST) $(DEFLIST) $(SDEFLIST) $(CFLAGS) $(LOCALFLAGS) -DDSHLIB -DPIC -Kpic -c $*.c && mv $(*F).o $*.P

newent.o:
	$(CC) $(INCLIST) $(DEFLIST) $(SDEFLIST) $(INCLIST2) $(CFLAGS) $(LOCALFLAGS) -c newent.c

newent.p:
	$(PROF)$(CC) $(INCLIST) $(DEFLIST) $(INCLIST2) $(CFLAGS) $(LOCALFLAGS) -c -p newent.c && mv newent.o newent.p

newent.P:
	$(PIC)$(CC) $(INCLIST) $(DEFLIST) $(SDEFLIST) $(INCLIST2) $(CFLAGS) $(LOCALFLAGS) -DDSHLIB -DPIC -Kpic -c newent.c && mv newent.o newent.P


new_list.c errlst.c: errlist errlist.awk
	$(AWK) -f errlist.awk <errlist
