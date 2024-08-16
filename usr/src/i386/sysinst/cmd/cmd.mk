#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident "@(#)proto:cmd/cmd.mk	1.1.1.24"

include $(CMDRULES)

BINCMDS = big_file bootstrap tapeop getcylsize odm sap_nearest
SCRIPTS = conframdfs cut.flop cut.netflop mini_kernel pick.set putdev rmwhite
CMDS    = $(BINCMDS) $(SCRIPTS)
NATIVE  = bzip iscompress wrt hsflop chall checkwhite
INSDIR  = $(PROTO)/bin

all: $(CMDS)

big_file: $$@.o
	$(CC) -o $@ $? $(LDFLAGS) $(LDLIBS)

bootstrap: $$@.o
	$(CC) -o $@ $? $(LDFLAGS) $(LDLIBS) $(NOSHLIBS)

bootstrap.o: $(@:.o=.c)
# Move the # sign to the second line below to disable the VT0 debug shell.
#	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -D_KMEMUSER -DLAST_LOAD -c $<
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -D_KMEMUSER -c $<

odm: $$@.o
	$(CC) -o $@ $? $(LDFLAGS) -lelf

sap_nearest: $$@.o
	$(CC) -o $@ $? $(LDFLAGS) -L $(ROOT)/$(MACH)/usr/lib -lnwutil -lnsl -lthread

tapeop: $$@.o
	$(CC) -o $@ $? $(LDFLAGS) $(LDLIBS)

getcylsize: $$@.o
	$(CC) -o $@ $? $(LDFLAGS) $(LDLIBS)

native: $(NATIVE)

bzip: zip/gzip
	cp $? $@

zip/gzip:
	@(cd zip; \
	chmod +x configure; \
	touch configure; \
	./configure --srcdir=. > /dev/null 2>&1; \
	$(MAKE))

hsflop.c:
	ln -s $(PROTO)/desktop/instcmd/sflop.c $@

install: all
	@if [ ! -d $(INSDIR) ] ;\
	then \
		mkdir -p $(INSDIR) ;\
	fi
	@for i in $(CMDS) ;\
	do \
		$(INS) -f $(INSDIR) $$i ;\
	done

clean:
	rm -f *.o

clobber: clean
	rm -f $(CMDS) $(NATIVE)
	(cd zip; $(MAKE) distclean)
