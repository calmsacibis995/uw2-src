#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libc-i386:makefile	1.44.1.62"
#
#  makefile for libc/i386
#
#
# The variable PROF is null by default, causing profiled object to be
# maintained.  If profiled object is not desired, the reassignment
# PROF=@# should appear in the make command line.
#

include $(LIBRULES)

.SUFFIXES: .p .P .xo .xp

PROF=
NONPROF=
PIC=
DEFLIST=
INCCPU=./inc
INCPORT=../port/inc
INCLIST=-I$(INCCPU) -I$(INCPORT)
VERSDEF=ansi.def
M4DEFS=m4.def sys.def $(VERSDEF)
AWK=awk

OBJECTS_A=\
crt/cerror.o      crt/fpreal.o      crt/mcount.o      	\
csu/crt0.o	  csu/crt1.o        csu/crti.o        	\
csu/crtn.o        csu/mcrt0.o	    csu/mcrt1.o       	\
csu/pcrt1.o       csu/pcrti.o       csu/pcrtn.o		csu/thread.o	\
fp/finite.o       fp/finitel.o      fp/fp_data.o	\
fp/fpgetmask.o    fp/fpgetround.o   fp/fpgetsticky.o  	\
fp/fpsetmask.o	  fp/fpsetround.o   fp/fpsetsticky.o	\
fp/fpstart.o      fp/fpstart0.o	    fp/fpstart1.o	\
fp/logb.o         fp/logbl.o        fp/scalb.o		\
fp/scalbl.o	  gen/abs.o         gen/tcsendbreak.o	\
gen/biglitpow.o   gen/dtop.o	    gen/ltostr.o	\
gen/cuexit.o      gen/cvtl.o	    gen/ecvt.o	  	gen/fpgetrnd.o 	\
gen/fpsetrnd.o    gen/frexp.o	    gen/frexpl.o      	\
gen/gen_def.o     gen/getctxt.o     gen/gettimeofday.o	gen/i386_data.o	\
gen/isnand.o      gen/isnanf.o      gen/isnanl.o      	\
gen/l3.o	  gen/ladd.o        gen/lconstants.o  	\
gen/ldexp.o       gen/ldexpl.o	    gen/ldivide.o     	\
gen/lexp10.o      gen/limits.o      gen/llog10.o	\
gen/lmul.o        gen/lshiftl.o     gen/lsign.o       	\
gen/lsub.o	  gen/makectxt.o    gen/mall_data.o   	gen/mpsys.o     \
gen/modfl.o       gen/p_info.o      gen/setjmp.o        gen/sh_data.o 	\
gen/siginfolst.o  gen/siglongjmp.o  gen/sigsetjmp.o 	gen/swapctxt.o	\
fmt/_cvt.o      					\
str/_mf_pow10.o   str/_mf_tod.o	    str/_mf_tof.o	\
str/_mf_told.o    str/memccpy.o     str/memchr.o	\
str/memcmp.o      str/memcpy.o      str/memmove.o	str/memset.o   	\
str/strcat.o      str/strchr.o      str/strcmp.o      	\
str/strcpy.o      str/strlen.o      str/strncat.o     	\
str/strncmp.o     str/strncpy.o     str/strrchr.o

OBJECTS_B=\
sys/_lwpprivate.o   sys/_lwpself.o      sys/_nfssys.o       sys/_rename.o	\
sys/_sigaction.o    sys/_signal.o       sys/access.o        sys/acct.o		\
sys/acl.o           sys/aclipc.o        sys/adjtime.o       sys/alarm.o		\
sys/auditbuf.o      sys/auditctl.o      sys/auditdmp.o      sys/auditevt.o	\
sys/auditlog.o      sys/block.o         sys/cancelblock.o   sys/chdir.o		\
sys/chmod.o         sys/chown.o         sys/chroot.o        sys/close.o		\
sys/creat.o         sys/devstat.o       sys/dup.o           sys/execl.o		\
sys/execle.o        sys/exect.o         sys/execv.o         sys/execve.o	\
sys/exit.o          sys/fchdir.o        sys/fchmod.o        sys/fchown.o	\
sys/fcntl.o         sys/fdevstat.o      sys/filepriv.o      sys/flvlfile.o	\
sys/fork.o          sys/fork1.o         sys/forkall.o       sys/fpathconf.o	\
sys/fstat.o         sys/fstatf.o        sys/fstatfs.o       sys/fstatvfs.o	\
sys/fsync.o         sys/ftruncate.o     sys/fxstat.o        sys/getdents.o	\
sys/getegid.o       sys/geteuid.o       sys/getgid.o        sys/getgroups.o	\
sys/getksym.o       sys/getmsg.o        sys/getpid.o        sys/getpmsg.o	\
sys/getppid.o       sys/getrlimit.o     sys/getsetitimer.o  sys/sys_gettimeofday.o	\
sys/getuid.o        sys/gtty.o          sys/ioctl.o         sys/kill.o		\
sys/lchown.o        sys/link.o          sys/lseek.o         sys/lsemainit.o	\
sys/lsemapost.o     sys/lsematry.o      sys/lsemawait.o     sys/lstat.o		\
sys/lvldom.o        sys/lvlequal.o      sys/lvlfile.o       sys/lvlipc.o	\
sys/lvlproc.o       sys/lvlvfs.o        sys/lwpcond.o       sys/lwpcontinue.o	\
sys/lwpcreate.o     sys/lwpexit.o       sys/lwpinfo.o       sys/lwpkill.o	\
sys/lwpmakectxt.o   sys/lwpmutex.o      sys/lwpprivate.o    sys/lwpself.o	\
sys/lwpsuspend.o    sys/lwpwait.o       sys/lxstat.o        sys/makectxt.o	\
sys/memcntl.o       sys/mincore.o       sys/mkdir.o         sys/mkmld.o		\
sys/mknod.o         sys/mldmode.o       sys/mmap.o          sys/modadm.o	\
sys/modload.o       sys/modpath.o       sys/modstat.o       sys/moduload.o	\
sys/mount.o         sys/mprotect.o      sys/munmap.o        sys/nice.o		\
sys/nuname.o        sys/online.o        sys/open.o          sys/p_online.o	\
sys/pathconf.o      sys/pause.o         sys/pipe.o          sys/plock.o		\
sys/poll.o          sys/pread.o         sys/prepblock.o     sys/priocntlset.o	\
sys/priocntlst.o    sys/procbind.o      sys/procexbind.o    sys/procpriv.o	\
sys/profil.o        sys/putmsg.o        sys/putpmsg.o	\
sys/pwrite.o        sys/rdblock.o       sys/read.o          sys/readlink.o	\
sys/readv.o         sys/rfsys.o         sys/rmdir.o         sys/sbrk.o		\
sys/secadvise.o     sys/secsys.o        sys/seteguid.o      sys/setgid.o	\
sys/setgroups.o     sys/setpgrp.o       sys/setprmptoff.o   sys/setrlimit.o	\
sys/setsid.o        sys/settimeofday.o  sys/setuid.o   	\
sys/sigaction.o     sys/sigaltstk.o     sys/sigpending.o    sys/sigprocmsk.o	\
sys/sigsendset.o    sys/sigsuspend.o    sys/sigwait.o       sys/sleep.o		\
sys/stat.o          sys/statf.o         sys/statfs.o        sys/statvfs.o	\
sys/stime.o         sys/stty.o          sys/symlink.o       sys/sync.o		\
sys/syncmbox.o      sys/syscall.o       sys/sysconfig.o     sys/sysfs.o		\
sys/sysi86.o        sys/sysinfo.o       sys/time.o          sys/times.o		\
sys/truncate.o      sys/uadmin.o        sys/ucontext.o      sys/ulimit.o	\
sys/umask.o         sys/umount.o        sys/uname.o         sys/unblock.o	\
sys/unlink.o        sys/ustat.o         sys/utime.o         sys/utssys.o	\
sys/vfork.o         sys/wait.o          sys/waitid.o        sys/write.o		\
sys/writev.o        sys/xmknod.o        sys/xstat.o	    sys/keyctl.o	\
sys/_sigwait.o

POBJECTS_A=$(OBJECTS_A:.o=.p)

POBJECTS_B=$(OBJECTS_B:.o=.p)

POBJECTS_X=$(OBJECTS_X:.xo=.xp) 

PIC_OBJECTS_A=$(OBJECTS_A:.o=.P)

PIC_OBJECTS_B=$(OBJECTS_B:.o=.P)

all:
	if [ "$(ISANSI)" = "TRUE" ] ; \
	then VERSDEF="ansi.def"; \
	else VERSDEF="nonansi.def"; \
	fi ; \
	$(MAKE) -e all_vers VERSDEF=$$VERSDEF

all_vers:
	$(PROF)$(MAKE) -e $(IGN) prof NONPROF=@# PIC=@# 
	$(PIC)$(MAKE) -e $(IGN) pic NONPROF=@# PROF=@# 
	$(NONPROF)$(MAKE) -e $(IGN) nonprof PROF=@# PIC=@# 

nonprof: $(OBJECTS_X) objects_a objects_b

objects_a: $(OBJECTS_A)

objects_b: $(OBJECTS_B)

prof: $(POBJECTS_X) pobjects_a pobjects_b

pobjects_a: $(POBJECTS_A)

pobjects_b: $(POBJECTS_B)

pic: pic_objects_a pic_objects_b

pic_objects_a: $(PIC_OBJECTS_A)

pic_objects_b: $(PIC_OBJECTS_B)

.c.o .c.p .c.P:
	$(NONPROF)@echo $*.c:
	$(NONPROF)$(CC) $(DEFLIST) $(INCLIST) $(CFLAGS) $(LOCALFLAGS) -c $*.c
	$(NONPROF)mv $(*F).o $*.o
	$(PROF)@echo $*.c:
	$(PROF)$(CC) $(DEFLIST) $(INCLIST) $(CFLAGS) $(LOCALFLAGS) -c -p $*.c  && mv $(*F).o $*.p
	$(PIC)@echo $*.c:
	$(PIC)$(CC) $(DEFLIST) $(INCLIST) $(CFLAGS) $(LOCALFLAGS) -c -DDSHLIB -Kpic $*.c  && mv $(*F).o $*.P

.s.o .s.p .s.P:
	$(NONPROF)@echo $*.s:
	$(NONPROF)$(AS) -o $*.o -m -- $(M4DEFS) -DMCOUNT=\# nonpic.def $*.s
	$(PROF)@echo $*.s:
	$(PROF)$(AS) -o $*.p -m -- $(M4DEFS) mcount.def nonpic.def $*.s
	$(PIC)@echo $*.s:
	$(PIC)$(AS) -o $*.P -m -- -DDSHLIB $(M4DEFS) -DMCOUNT=\# pic.def $*.s

.c.xo .c.xp:
	$(NONPROF)@echo $*.c:
	$(NONPROF)$(CC) $(DEFLIST) $(INCLIST) $(CFLAGS) $(LOCALFLAGS) -DUNSHARED -c $*.c
	$(NONPROF)mv $(*F).o $*.xo
	$(PROF)@echo $*.c:
	$(PROF)$(CC) $(DEFLIST) $(INCLIST) $(CFLAGS) $(LOCALFLAGS) -DUNSHARED -c -p $*.c  && mv $(*F).o $*.xp

archive:
	#
	# Note that "archive" is invoked with libc/object as current directory.
	#
	# figure out the correct ordering for all the archive modules
	mv cuexit.o cuexit.x
	mv fpstart.o fpstart.x
	mv fpstart0.o fpstart0.x
	mv _signal.o _signal.x
	mv kill.o kill.x
	mv getpid.o getpid.x
	mv cerror.o cerror.x
	mv fpsetmask.o fpsetmask.x
	mv atexit.o atexit.x
	-rm -f old_toupper.o old_tolower.o
	ls | grep '\.o$$' | $(LORDER) - | $(TSORT) >objlist
	mv cuexit.x cuexit.o
	mv fpstart.x fpstart.o
	mv fpstart0.x fpstart0.o
	mv _signal.x _signal.o
	mv kill.x kill.o
	mv getpid.x getpid.o
	mv cerror.x cerror.o
	mv fpsetmask.x fpsetmask.o
	mv atexit.x atexit.o
	#
	# build the archive with the modules in correct order.
	$(AR) q ../lib.libc atexit.o
	xargs $(AR) q ../lib.libc <objlist
	$(AR) q ../lib.libc _signal.o fpstart.o fpstart0.o \
	fpsetmask.o kill.o getpid.o \
	cerror.o cuexit.o
	if [ "$(MAC)" = BMAUMAC ] ; then \
		$(AR) d ../lib.libc fpstart0.o ; \
	fi
	$(PROF)#
	$(PROF)# build the profiled library archive, first renaming the
	$(PROF)#	.p (profiled object) modules to .o
	$(PROF)find . -name '*.p' -print | while read i; do mv $$i `basename $$i .p`.o ; done
	if [ "$(PROF)" != "@#" ]; \
	then \
	$(PROF)$(AR) q ../libp.libc atexit.o; \
	$(PROF)xargs $(AR) q ../libp.libc <objlist; \
	$(PROF)$(AR) q ../libp.libc _signal.o fpstart.o fpstart0.o \
	fpsetmask.o kill.o getpid.o cerror.o cuexit.o ; \
	if [ "$(MAC)" = BMAUMAC ] ; then \
		$(AR) d ../libp.libc fpstart0.o ; \
	fi \
	fi
shared:
	#
	# Note that "shared" is invoked with libc/object as current directory
	#
	-rm -rf shared
	mkdir shared
	cat ../$(CPU)/rtld_objects ../$(CPU)/shared_objects | grep -v "\#"  | \
	xargs sh -sc 'ln "$$@" shared'
	cat ../$(CPU)/rtld_objects ../$(CPU)/shared_objects | grep "^[^\.\#]" | sed 's/\.P/.o/' | \
	xargs sh -sc 'rm -f "$$@"'
	#
	# create the shared portion of the library
	cd shared; \
	grep -v "#" ../../$(CPU)/rtld_objects ../../$(CPU)/shared_objects | cut -d: -f2 | xargs echo $(LD) -r -o libc_TMP.'$$i' > libcbuild; \
	i=0; \
	while read cmd; \
	do \
	eval "$$cmd"; \
	i=`expr "$$i" + 1`; \
	done < libcbuild; \
	$(LD) -o ../../libc.so.1 -dy -G -e _rt_boot -h /usr/lib/libc.so.1 -ztext -zdefs -Bsymbolic:../../i386/Nbsymbs -Bexport:../../i386/Expsymbs libc_TMP.*; \
	rm -f libcbuild libc_TMP.*
	#
	# figure out the correct ordering for all the archive modules
	mv cerror.o cerror.x
	mv fpstart0.o fpstart0.x
	mv fpsetmask.o fpsetmask.x
	-rm -f old_toupper.o old_tolower.o
	ls | grep '\.o$$' | $(LORDER) - | $(TSORT) >objlist
	mv cerror.x cerror.o
	mv fpstart0.x fpstart0.o
	mv fpsetmask.x fpsetmask.o
	#
	# build the archive with the modules in correct order.
	xargs $(AR) q ../libc.so <objlist
	$(AR) q ../libc.so cerror.o ../libc.so.1 \
		fpstart0.o fpsetmask.o
	if [ "$(MAC)" = BMAUMAC ] ; then \
		$(AR) d ../libc.so fpstart0.o ; \
	fi

compat_lib:
	#build the shared object libc.so.1.1 which contains libc functions 
	#only. It is used for compatibilty with systems supporting the
	#two library approach for dynamic linking.
	-rm -rf shared
	mkdir shared
	cat ../$(CPU)/shared_objects | grep -v "\#"  | \
	xargs sh -sc 'ln "$$@" shared'
	cat ../$(CPU)/shared_objects | grep "^[^\.\#]" | sed 's/\.P/.o/' | \
	xargs sh -sc 'rm -f "$$@"'
	#
	# create the shared library
	cd shared; \
	grep -v "#" ../../$(CPU)/shared_objects | xargs echo $(LD) -r -o compat_TMP.'$$i' > compatbuild; \
	i=0; \
	while read cmd; \
	do \
	eval "$$cmd"; \
	i=`expr "$$i" + 1`; \
	done < compatbuild; \
	$(LD) -o ../../libc.so.1.1 -dy -G -h /usr/lib/libc.so.1.1 compat_TMP.*; \
	rm -f compatbuild compat_TMP.*
	#
clean:
	-rm -f *.o
	-rm -f *.p
	-rm -f *.P
	-rm -f *.s
	-rm -f ./mk_cvt

clobber: clean
	-find . \( -name '*.o' -o -name '*.p' -o -name '*.P' \) \
		-print | xargs rm -f

fmt/_cvt.c:   ./mk_cvt fmt/cvt-config  ../port/fmt/cvt-template \
		../port/fmt/cvt-script
	./mk_cvt -o fmt/_cvt.c -c fmt/cvt-config -s ../port/fmt/cvt-script \
		-t ../port/fmt/cvt-template

./mk_cvt:	../port/fmt/mk_cvt.c
	cc -o ./mk_cvt ../port/fmt/mk_cvt.c



