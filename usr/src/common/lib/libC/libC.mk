#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libC:libC.mk	1.16"
#
#  makefile for libC.a
#

include $(LIBRULES)

#  This builds both a normal (non-profiled) and a profiled version of 
#  the library.  'n' and 'p' respectively serve as directory prefixes 
#  and file name suffixes in the building process.  

.SUFFIXES: .n .p

LIBP = $(CCSLIB)/libp

CRTOBJS = Crti.o pCrti.o Crtn.o pCrtn.o
PRODUCTS = nobjects/libC.a pobjects/libC.a $(CRTOBJS)

.MUTEX: nobjects/libC.a pobjects/libC.a
.MUTEX: Crti.o pCrti.o
.MUTEX: Crtn.o pCrtn.o

PROBEFILE = Crti.c

SGSBASE = ../../cmd/sgs

DEFLIST = -DSHOBJ_SUPPORT -DVSPRINTF=vsprintf
INCLIST = -I.

CC_CMD = $(C++C) $(CFLAGS) $(DEFLIST) $(INCLIST)
cc_CMD = $(CC) $(CFLAGS) $(DEFLIST) $(INCLIST) -KPIC

OBJECTS = generic.o \
	delete.o edg_exit.o eh_util.o exit.o \
	main.o munch_ctors.o munch_dtors.o \
	new.o placenew.o pure_virt.o \
	set_new.o static_init.o throw.o \
	vars.o vec_cctor.o vec_newdel.o \
	in.o out.o stream.o fstream.o manip.o cstreams.o \
	flt.o strstream.o rawin.o intin.o stdiobuf.o \
	streambuf.o filebuf.o  oldformat.o sbuf.dbp.o \
	swstdio.o ios_compat.o

NOBJECTS = $(OBJECTS:.o=.n)

POBJECTS = $(OBJECTS:.o=.p)

all:
	if [ -f $(PROBEFILE) ]; \
	then \
		find $(PRODUCTS) \( ! -type f -o -links +1 \) \
		    -exec rm -f {} \; 2>/dev/null; \
		$(MAKE) -f libC.mk $(PRODUCTS) $(MAKEARGS); \
	else \
		for file in $(PRODUCTS); \
		do \
			if [ ! -r $$file ]; \
			then \
				echo "ERROR: $$file is missing" 1>&2; \
				false; \
				break; \
			fi \
		done \
	fi 

binaries: $(PRODUCTS)

install : all
	if [ ! -d $(LIBP) ];\
	then\
		mkdir $(LIBP);\
	fi
	sh $(SGSBASE)/sgs.install 644 $(OWN) $(GRP) $(CCSLIB)/libC.a nobjects/libC.a
	sh $(SGSBASE)/sgs.install 644 $(OWN) $(GRP) $(CCSLIB)/$(SGS)Crti.o Crti.o
	sh $(SGSBASE)/sgs.install 644 $(OWN) $(GRP) $(CCSLIB)/$(SGS)pCrti.o pCrti.o
	sh $(SGSBASE)/sgs.install 644 $(OWN) $(GRP) $(CCSLIB)/$(SGS)Crtn.o Crtn.o
	sh $(SGSBASE)/sgs.install 644 $(OWN) $(GRP) $(CCSLIB)/$(SGS)pCrtn.o pCrtn.o
	sh $(SGSBASE)/sgs.install 644 $(OWN) $(GRP) $(LIBP)/libC.a pobjects/libC.a

#  the .n and .p files (renamed .o's) persist across makes, while the
#  nobjects and pobjects subdirectories are recreated with each make

nobjects/libC.a : $(NOBJECTS)
	$(RM) -rf nobjects
	mkdir nobjects
	for i in $(NOBJECTS);\
	do\
		$(CP) $$i nobjects/`basename $$i .n`.o;\
	done
	cd nobjects; $(AR) q libC.a `$(LORDER) $(OBJECTS) | $(TSORT)`

pobjects/libC.a : $(POBJECTS)
	$(RM) -rf pobjects
	mkdir pobjects
	for i in $(POBJECTS);\
	do\
		$(CP) $$i pobjects/`basename $$i .p`.o;\
	done
	cd pobjects; $(AR) q libC.a `$(LORDER) $(OBJECTS) | $(TSORT)`

pCrti.o Crti.o : Crti.c
	$(cc_CMD) -DPROFILING -c Crti.c && mv Crti.o pCrti.o
	$(cc_CMD) -c Crti.c

pCrtn.o Crtn.o : Crtn.c
	$(cc_CMD) -DPROFILING -c Crtn.c && mv Crtn.o pCrtn.o
	$(cc_CMD) -c Crtn.c

.c.n:
	$(CC_CMD) -c $(?)
	$(MV) $*.o $@

.c.p:
	$(CC_CMD) -c -p $(?)
	$(MV) $*.o $@

clean:
	$(RM) -rf nobjects pobjects $(NOBJECTS) $(POBJECTS) $(CRTOBJS)

clobber: clean
	-rm -f $(CCSLIB)/libC.a $(LIBP)/libC.a
	-rm -f $(CCSLIB)/$(SGS)Crti.o $(CCSLIB)/$(SGS)pCrti.o
	-if [ -f $(PROBEFILE) ]; \
	then \
		rm -f $(PRODUCTS); \
	fi
