#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libc:libc.mk	1.27.12.5"
#
# makefile for libc
#
#
# The variable PROF is null by default, causing both the standard C library
# and a profiled library to be maintained.  If profiled object is not 
# desired, the reassignment PROF=@# should appear in the make command line.
#
# The variable IGN may be set to -i by the assignment IGN=-i in order to
# allow a make to complete even if there are compile errors in individual
# modules.
#
# See also the comments in the lower-level machine-dependent makefiles.
#

include $(LIBRULES)

PCFLAGS=
LIBP=$(CCSLIB)/libp
DONE=
PROF=
NONPROF=
PIC=
LOCALFLAGS=-Kno_host -D_REENTRANT
LOCALDEF = -D$(CPU)
SGSBASE=../../cmd/sgs
CATROOT=./catalogs
ISANSI=TRUE
RTLD_DIR=./rtld

all:	all_objects
	$(MAKE) -f libc.mk ISANSI=$$ISANSI LOCALFLAGS="$(LOCALFLAGS)" \
				archive_lib shared_lib compat_lib

archive:	archive_objects
	$(MAKE) -f libc.mk ISANSI=$$ISANSI LOCALFLAGS="$(LOCALFLAGS)" archive_lib

prof:	prof_objects
	$(MAKE) -f libc.mk ISANSI=$$ISANSI LOCALFLAGS="$(LOCALFLAGS)" archive_lib

shared:		shared_objects
	$(MAKE) -f libc.mk ISANSI=$$ISANSI LOCALFLAGS="$(LOCALFLAGS)" shared_lib

compat: compat_objects
	$(MAKE) -f libc.mk ISANSI=$$ISANSI LOCALFLAGS="$(LOCALFLAGS)" compat_lib

.MUTEX:	prof_objects shared_objects compat_objects
all_objects:	prof_objects shared_objects compat_objects

shared_objects:	rtld_obj
	- $(CC) -E tstansi.c; if [ $$? = 0 ] ; \
	then ISANSI="TRUE"; \
	else ISANSI="FALSE"; PIC="@#"; \
	fi ; \
	cd port; $(MAKE) -f makefile pic ISANSI=$$ISANSI PIC=$$PIC LOCALFLAGS="$(LOCALFLAGS)"; \
	cd ../$(CPU); $(MAKE) -f makefile pic ISANSI=$$ISANSI PIC=$$PIC LOCALFLAGS="$(LOCALFLAGS)" NONPROF="@#" PROF="@#"
	$(MAKE) -f libc.mk archive_objects

compat_objects:	
	- $(CC) -E tstansi.c; if [ $$? = 0 ] ; \
	then ISANSI="TRUE"; \
	else ISANSI="FALSE"; PIC="@#"; \
	fi ; \
	cd port; $(MAKE) -f makefile pic ISANSI=$$ISANSI PIC=$$PIC LOCALFLAGS="$(LOCALFLAGS)"; \
	cd ../$(CPU); $(MAKE) -f makefile pic ISANSI=$$ISANSI PIC=$$PIC LOCALFLAGS="$(LOCALFLAGS)" NONPROF="@#" PROF="@#"
	$(MAKE) -f libc.mk archive_objects

archive_objects:
	- $(CC) -E tstansi.c; if [ $$? = 0 ] ; \
	then ISANSI="TRUE"; PIC="@#"; \
	else ISANSI="FALSE"; PIC="@#"; \
	fi ; \
	cd port; $(MAKE) -f makefile nonprof  ISANSI=$$ISANSI PIC=$$PIC LOCALFLAGS="-Kno_host"; \
	cd ../$(CPU); $(MAKE) -f makefile nonprof  ISANSI=$$ISANSI PIC=$$PIC LOCALFLAGS="-Kno_host" PROF="@#"

prof_objects:
	- $(CC) -E tstansi.c; if [ $$? = 0 ] ; \
	then ISANSI="TRUE"; PIC="@#"; \
	else ISANSI="FALSE"; PIC="@#"; \
	fi ; \
	cd port; $(MAKE) -f makefile prof ISANSI=$$ISANSI PIC=$$PIC LOCALFLAGS="-Kno_host"; \
	cd ../$(CPU); $(MAKE) -f makefile prof ISANSI=$$ISANSI PIC=$$PIC LOCALFLAGS="-Kno_host" NONPROF="@#"

rtld_obj:
	# make the rtld objects
	cd $(RTLD_DIR); $(MAKE) -f rtld.mk

archive_lib:
	#
	# place portable modules in "object" directory, then overlay
	# 	the machine-dependent modules.
	-rm -rf object
	mkdir object
	# ignore message about stdio being a directory... sorry about that
	find port $(CPU) -name '*o' -print | \
	xargs sh -sc 'ln "$$@" object'
	$(PROF)find port $(CPU) -name '*p' -print | xargs sh -sc 'ln "$$@" object'
	#
	# delete temporary libraries
	-rm -f lib.libc
	$(PROF)-rm -f libp.libc
	#
	# set aside run-time modules, which don't go in library archive!
	-rm -f *crt?.o values-X?.o
	cd object; for i in *crt?.o values-Xt.o values-Xa.o values-Xc.o; do mv $$i ..; done
	#
	# build archive out of the remaining modules.
	cd object; $(MAKE) -f ../$(CPU)/makefile archive \
		PROF=$(PROF) MAC=$(MAC) ISANSI=$(ISANSI)
	-rm -rf object
	#
	$(DONE)

shared_lib:
	#
	# place portable modules in "object" directory, then overlay
	# 	the machine-dependent modules.
	-rm -rf object
	mkdir object
	find port $(CPU) -name '*.o' -print | \
	xargs sh -sc 'ln "$$@" object'
	find port $(CPU) -name '*.P' -print | \
	xargs sh -sc 'ln "$$@" object'
	ln $(RTLD_DIR)/$(CPU)/*.o object
	#
	# delete temporary libraries
	-rm -f libc.so
	#
	# set aside run-time modules, which don't go in library archive!
	-rm -rf *crt?.o values-X?.o
	cd object; for i in *crt?.o values-Xa.o values-Xc.o; do mv $$i ..; done; \
	ln values-Xt.o ..
	#
	# build archive out of the remaining modules.
	cd object; $(MAKE) -f ../$(CPU)/makefile shared \
		PROF=$(PROF) MAC=$(MAC) ISANSI=$(ISANSI)
#	-rm -rf object
	#
	$(DONE)

compat_lib:
	#
	# place portable modules in "object" directory, then overlay
	# 	the machine-dependent modules.
	-rm -rf object
	mkdir object
	find port $(CPU) -name '*.o' -print | \
	xargs sh -sc 'ln "$$@" object'
	find port $(CPU) -name '*.P' -print | \
	xargs sh -sc 'ln "$$@" object'
	if [ -f $(RTLD_DIR)/$(CPU)/align.o ];  \
		then ln $(RTLD_DIR)/$(CPU)/align.o object;      \
	fi; \
	#
	# delete temporary libraries
	-rm -f libc.so.1.1
	#
	# set aside run-time modules, which don't go in library archive!
	-rm -f *crt?.o values-X?.o
	cd object; for i in *crt?.o values-Xa.o values-Xc.o; do mv $$i ..; done; \
	ln values-Xt.o ..
	# build archive out of the remaining modules.
	cd object; $(MAKE) -f ../$(CPU)/makefile compat_lib \
		PROF=$(PROF) MAC=$(MAC) ISANSI=$(ISANSI)
#	-rm -rf object
	#
	$(DONE)

move:	move_archive
	#
	# move the shared libraries into the correct directories
	sh $(SGSBASE)/sgs.install 644 $(OWN) $(GRP) $(CCSLIB)/lib$(VARIANT)c.so libc.so ; \
	rm -f libc.so
	sh $(SGSBASE)/sgs.install 755 $(OWN) $(GRP) $(USRLIB)/lib$(VARIANT)c.so.1 libc.so.1 ; \
	rm -f libc.so.1
	if [ -f libc.so.1.1 ];  \
 	then	\
		sh $(SGSBASE)/sgs.install 755 $(OWN) $(GRP) $(USRLIB)/lib$(VARIANT)c.so.1.1 libc.so.1.1 ; \
		rm -f libc.so.1.1;	\
	fi
	rm -f $(LIBP)/lib$(VARIANT)c.so
	ln $(CCSLIB)/lib$(VARIANT)c.so $(LIBP)/lib$(VARIANT)c.so
	ln $(USRLIB)/lib$(VARIANT)c.so.1 $(USRLIB)/ld.so.1
	ln $(USRLIB)/lib$(VARIANT)c.so.1 $(USRLIB)/libdl.so.1

move_archive:
	#
	# move the library or libraries into the correct directory
	for i in *crt?.o values-Xt.o values-Xa.o values-Xc.o;  \
	do sh $(SGSBASE)/sgs.install 644 $(OWN) $(GRP) $(CCSLIB)/$(SGS)$$i $$i; \
	rm -f $$i ; done
	sh $(SGSBASE)/sgs.install 644 $(OWN) $(GRP) $(CCSLIB)/lib$(VARIANT)c.a lib.libc ; \
	rm -f lib.libc
	$(PROF) if [ ! -d $(LIBP) ]; then \
	$(PROF) mkdir $(LIBP); \
	$(PROF) fi
	$(PROF)sh $(SGSBASE)/sgs.install 644 $(OWN) $(GRP) $(LIBP)/lib$(VARIANT)c.a libp.libc ; \
	rm -f libp.libc

.MUTEX:	all move msg_catalogs
install:	all move msg_catalogs

.MUTEX:	archive move_archive
install_archive:	archive move_archive

msg_catalogs:
	if [ ! -d $(USRLIB)/locale ]; then \
		mkdir $(USRLIB)/locale; \
	fi
	if [ ! -d $(USRLIB)/locale/C ]; then \
		mkdir $(USRLIB)/locale/C; \
	fi
	if [ ! -d $(USRLIB)/locale/C/MSGFILES ]; then \
		mkdir -p $(USRLIB)/locale/C/MSGFILES; \
	fi
	sh $(SGSBASE)/sgs.install 444 $(OWN) $(GRP) $(USRLIB)/locale/C/MSGFILES/syserr.str port/gen/syserr.str;
	sh $(SGSBASE)/sgs.install 444 $(OWN) $(GRP) $(USRLIB)/locale/C/MSGFILES/ar.str $(CATROOT)/ar.str
	sh $(SGSBASE)/sgs.install 444 $(OWN) $(GRP) $(USRLIB)/locale/C/MSGFILES/cplu.str $(CATROOT)/cplu.str
	sh $(SGSBASE)/sgs.install 444 $(OWN) $(GRP) $(USRLIB)/locale/C/MSGFILES/libc.str $(CATROOT)/libc.str
	sh $(SGSBASE)/sgs.install 444 $(OWN) $(GRP) $(USRLIB)/locale/C/MSGFILES/epu.str $(CATROOT)/epu.str
	sh $(SGSBASE)/sgs.install 444 $(OWN) $(GRP) $(USRLIB)/locale/C/MSGFILES/cds.str $(CATROOT)/cds.str
	sh $(SGSBASE)/sgs.install 444 $(OWN) $(GRP) $(USRLIB)/locale/C/MSGFILES/cplusplus.str $(CATROOT)/cplusplus.str

clean:
	#
	# remove intermediate files except object modules and temp library
	-rm -rf obj*
	cd port ;  $(MAKE) clean
	cd $(CPU) ;  $(MAKE) clean

clobber:
	#
	# remove intermediate files
	-rm -rf *.o lib*.libc obj*
	-rm -rf *.o libc.so libc.so.1 libc.so.1.1
	cd port ;  $(MAKE) clobber
	if [ -d $(RTLD_DIR) ] ; then \
		cd $(RTLD_DIR); $(MAKE) -f rtld.mk clobber; fi
	cd $(CPU) ;  $(MAKE) clobber
