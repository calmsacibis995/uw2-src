#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ktool:common/ktool/idtools/idtools.mk	1.37"
#ident	"$Header: $"

# ID/TP commands in this directory may be built either for a cross-environment
# or for a target system.
#
# ID/TP commands which are only built for the target system are in the
# idtarg subdirectory.

include $(CMDRULES)

.MUTEX: nativeX cmds

LDLIBS = -lgen
LDLIBS_SYM = $(LIBELF) $(LDLIBS)

SHELLS = idbuild idtune idtype
CMDS =	$(SGS)idconfupdate \
	$(SGS)idinstall \
	$(SGS)idmkdtune \
	$(SGS)idmkunix \
	$(SGS)idreadauto \
	$(SGS)idval

INSDIR = $(CONF)/bin
MSGFILESDIR = $(USRLIB)/locale/C/MSGFILES

CFILES = \
	devconf.c \
	entry.c \
	getinst.c \
	getmajors.c \
	idconfupdate.c \
	idinstall.c \
	idmkdtune.c \
	idmkunix.c \
	idreadauto.c \
	idval.c \
	interface.c \
	mdep.c \
	setlevel.c \
	symtab.c

TARGET_HDRS = \
	libelf.h \
	sys/elf.h \
	sys/elftypes.h \
	sys/mod.h \
	sys/param_p.h \
	sys/param.h \
	sys/confmgr.h \
	sys/cm_i386at.h \
	sys/resmgr.h


all:	nativeX cmds
	@if [ -z "$(SGS)" ]; \
	then cd idtarg; echo "=== $(MAKE) -f idtarg.mk"; \
		$(MAKE) -f idtarg.mk $(MAKEARGS); \
	fi

clean:	localclean
	@cd idtarg; echo "=== $(MAKE) -f idtarg.mk clean"; \
		$(MAKE) -f idtarg.mk $(MAKEARGS) clean

localclean:
	-rm -f *.o

clobber: localclobber
	@cd idtarg; echo "=== $(MAKE) -f idtarg.mk clobber"; \
		$(MAKE) -f idtarg.mk $(MAKEARGS) clobber

localclobber: localclean
	-rm -f $(CMDS)

install: all $(SHELLS) idtools.str
	-[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	for f in $(CMDS) $(SHELLS); \
	do \
		$(INS) -f $(INSDIR) -m $(BINMODE) -u $(OWN) -g $(GRP) $$f; \
	done
	@if [ -z "$(SGS)" ]; \
	then cd idtarg; echo "=== $(MAKE) -f idtarg.mk install"; \
		$(MAKE) -f idtarg.mk $(MAKEARGS) install; \
	fi
	-[ -d $(MSGFILESDIR) ] || mkdir -p $(MSGFILESDIR)
	$(INS) -f $(MSGFILESDIR) -m 444 -u $(OWN) -g $(GRP) idtools.str

lintit:
	for f in $(CFILES); \
	do \
		$(LINT) $(LINTFLAGS) $$f; \
	done
	@cd idtarg; echo "=== $(MAKE) -f idtarg.mk lintit"; \
		$(MAKE) -f idtarg.mk $(MAKEARGS) lintit

IDCONFUPDATE_OFILES = idconfupdate.o getinst.o getmajors.o entry.o util.o setlevel.o

$(SGS)idconfupdate: $(IDCONFUPDATE_OFILES)
	$(CC) -o $(SGS)idconfupdate -Iinc $(IDCONFUPDATE_OFILES) $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

idconfupdate.o: FRC
	if [ "$(SGS)" ]; \
	then \
		$(CC) -DCROSS -Iinc $(CFLAGS) $(DEFLIST) $(INCLIST) -c idconfupdate.c; \
	else \
		$(CC) -Iinc $(CFLAGS) $(DEFLIST) $(INCLIST) -c idconfupdate.c; \
	fi

$(SGS)idmkdtune: FRC
	$(CC) -o $(SGS)idmkdtune -Iinc $(CFLAGS) $(DEFLIST) $(INCLIST) idmkdtune.c $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

interface.o: FRC

IDMKUNIX_OFILES = idmkunix.o getinst.o getmajors.o symtab.o entry.o devconf.o \
			mdep.o fdep.o interface.o util.o

$(SGS)idmkunix: $(IDMKUNIX_OFILES)
	$(CC) -o $(SGS)idmkunix $(IDMKUNIX_OFILES) \
		$(LDFLAGS) $(LDLIBS_SYM) $(ROOTLIBS)

idmkunix.o: nativeX FRC
	$(CC) -Iinc $(CFLAGS) $(DEFLIST) $(INCLIST) -c $<

IDINSTALL_OFILES = idinstall.o getinst.o getmajors.o entry.o setlevel.o \
			interface.o symtab.o
$(SGS)idinstall: $(IDINSTALL_OFILES)
	$(CC) -o $(SGS)idinstall $(IDINSTALL_OFILES) \
		$(LDFLAGS) $(LDLIBS_SYM) $(ROOTLIBS)

idinstall.o: FRC
	if [ "$(SGS)" ]; \
	then \
		$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -DCROSS -c idinstall.c; \
	else \
		$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c idinstall.c; \
	fi

setlevel.o: FRC
	if [ "$(SGS)" ]; \
	then \
		$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -DCROSS -c setlevel.c; \
	else \
		$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c setlevel.c; \
	fi

getinst.o: FRC

getmajors.o: FRC

devconf.o: FRC

util.o: FRC

symtab.o: FRC
	$(CC) -Iinc $(CFLAGS) $(DEFLIST) $(INCLIST) -c symtab.c

entry.o: FRC

mdep.o: FRC

fdep.o: FRC
	$(CC) -Iinc $(CFLAGS) $(DEFLIST) $(INCLIST) -c $<

$(SGS)idval:  idval.o
	$(CC) -o $(SGS)idval idval.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

idval.o: FRC

IDREADAUTO_OFILES = getinst.o getmajors.o entry.o devconf.o \
			mdep.o util.o
$(SGS)idreadauto: $(IDREADAUTO_OFILES)
	if [ "$(SGS)" ]; \
	then \
		$(CC) -o $(SGS)idreadauto idreadauto.c $(IDREADAUTO_OFILES) $(LDFLAGS) $(LDLIBS) $(ROOTLIBS) $(INCLIST) -DCROSS ; \
	else \
		$(CC) -o idreadauto idreadauto.c $(IDREADAUTO_OFILES) $(LDFLAGS) $(LDLIBS) $(ROOTLIBS) $(INCLIST);  \
	fi


cmds: $(CMDS)

nativeX:
	@echo creating temp include directory
	-@rm -rf inc; mkdir inc
	-@for i in $(TARGET_HDRS) ;\
	do \
		p=inc/`dirname $$i` ;\
		if [ ! -d "$$p" ] ;\
		then \
			mkdir -p $$p ;\
		fi ;\
		rm -f inc/$$i ;\
		/bin/cp $(TARGINC)/$$i $$p ;\
	done

FRC:
