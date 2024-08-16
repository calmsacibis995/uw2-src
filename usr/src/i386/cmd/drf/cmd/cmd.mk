#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)drf:cmd/cmd.mk	1.5"
include $(CMDRULES)

CMDS    = bootstrap wrt tapeop big_file bzip
SCRIPTS1 = prep_flop 
SCRIPTS2 = emergency_rec emergency_disk
OTRFILES = drfram.proto drf_inst.gen 

LOCALDEF  = -DDRF_FLOP
INSDIR  = $(USRLIB)/drf
INSDIRL = $(USRLIB)/drf/locale/C
MSGDIR  = $(USRLIB)/locale/C/MSGFILES

SOURCES = bootstrap.c wrt.c tapeop.c big_file.c

all: $(CMDS) $(SCRIPTS1) $(SCRIPTS2) $(TXTSTR)

$(SOURCES):
	@ln -s $(PROTO)/cmd/$@ $@

bootstrap: $$@.o
	$(CC) -o $@ $? $(LDFLAGS) $(LDLIBS)

wrt:	$$@.o
	$(CC) -o $@ $? $(LDFLAGS) $(LDLIBS)

tapeop:	$$@.o
	$(CC) -o $@ $? $(LDFLAGS) $(LDLIBS)

big_file:$$@.o
	$(CC) -o $@ $? $(LDFLAGS) $(LDLIBS)

bzip:	zip/gzip
	cp $? $@

zip/gzip:	
	(cd zip; \
	 $(MAKE) -f zip.mk gzip)

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	@for i in $(CMDS) $(SCRIPTS1) $(OTRFILES);\
	do \
		$(INS) -f $(INSDIR) $$i ;\
	done

	[ -d $(INSDIRL) ] || mkdir -p $(INSDIRL)
	$(INS) -f $(INSDIRL) txtstr.C

	[ -d $(MSGDIR) ] || mkdir -p $(MSGDIR)
	$(INS) -f $(MSGDIR) drf.str

	[ -d $(SBIN) ] || mkdir -p $(SBIN)
	@for i in $(SCRIPTS2) ;\
	do \
		$(INS) -f $(SBIN) $$i ;\
	done

clean:
	rm -f *.o
	(cd zip; $(MAKE) -f zip.mk clean)

clobber: clean
	rm -f $(CMDS) $(SOURCES)  $(SCRIPTS1) $(SCRIPTS2) 
	(cd zip; $(MAKE) -f zip.mk clobber)
