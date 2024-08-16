#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/metamail/metamail.mk	1.2"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

# @(#)makefile	1.7

CFLAGS = -O -I.
LDLIBS=
INS= install
USRBIN= /usr/bin
USRLIB= /usr/lib
ETC= /etc

include $(CMDRULES)

.SUFFIXES: .ln
.c.ln:
	$(LINT) $(LINTFLAGS) $(DEFLIST) -c $*.c > $*.lerr

#
# This is where config.h lives
CONFIGDIR = .

# these are the programs created here
BINPROGS1= extcompose metasend
BINPROGS2= getfilename showaudio showexternal shownonascii showpartial showpicture
BINPROGS= $(BINPROGS1) $(BINPROGS2)
METAPROGS= mailto metamail mimencode splitmail
RICHPROGS= richtext



#### build targets ####

all: all-bin all-metamail all-richmail metamail.str

all-bin:
	cd bin && make $(BINPROGS)

all-metamail:
	cd metamail && $(MAKE) CONFIGDIR=../${CONFIGDIR}

all-richmail:
	cd richmail && $(MAKE) CONFIGDIR=../${CONFIGDIR} richtext

metamail.str: pfmt.msgs
	@if [ -r metamail.str ]; then rm -f metamail.str; fi
	sed	-e 's/^:[0-9][0-9]*://' \
		-e 's/^:U_[0-9][0-9]*://' \
		-e '/^#ident.*attmail/d' < pfmt.msgs > metamail.str



# save all administerable data files. ('make save' before 'make install')
save:
	if [ -f $(ETC)/mail/mailcap ];then mv $(ETC)/mail/mailcap $(ETC)/mail/mailcap.old;fi


#### install targets ####

install: install-usr-lib install-mailcap install-bin install-metamail install-richmail install-messages

install-usr-lib: $(USRLIB)/mail/metamail
$(USRLIB)/mail/metamail:
	if [ ! -d $(USRLIB)/mail/metamail ]; \
	then mkdir $(USRLIB)/mail/metamail ; \
	     $(CH)chown bin $(USRLIB)/mail/metamail ; \
	     $(CH)chgrp mail $(USRLIB)/mail/metamail ; \
	     $(CH)chmod 775 $(USRLIB)/mail/metamail ; \
	fi

install-mailcap: mailcap
	$(INS) -f $(ETC)/mail -m 0644 -u bin -g bin mailcap

install-bin: all-bin
	cd bin && for i in $(BINPROGS1); do $(INS) -f $(USRBIN) -m 0555 -u bin -g bin $$i; done
	cd bin && for i in $(BINPROGS2); do $(INS) -f $(USRLIB)/mail/metamail -m 0555 -u bin -g bin $$i; done

install-metamail: all-metamail
	cd metamail && for i in $(METAPROGS); do $(INS) -f $(USRBIN) -m 0555 -u bin -g bin $$i; done

install-richmail: all-richmail
	cd richmail && for i in $(RICHPROGS); do $(INS) -f $(USRLIB)/mail/metamail -m 0555 -u bin -g bin $$i; done


install-messages: metamail.str
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 -u bin -g mail metamail.str

#### clean targets ####

clean: clean-bin clean-metamail clean-richmail clean-here

clean-bin:
	cd bin && rm -f *BAK

clean-metamail:
	cd metamail && rm -f *BAK *.o

clean-richmail:
	cd richmail && rm -f *BAK *.o

clean-here:
	rm -f *BAK


#### clobber targets ####

clobber: clean clobber-bin clobber-metamail clobber-richmail clobber-here

clobber-bin:

clobber-metamail:
	cd metamail && rm -f $(METAPROGS)

clobber-richmail:
	cd richmail && rm -f $(RICHPROGS)

clobber-here:
	rm -f metamail.str


#### lintit targets

lintit: lint-metamail lint-richmail

lint-metamail:
	# cd metamail && $(LINT) ??

lint-richmail:
	# cd richmail && $(LINT) ??



#### strip targets ####

strip: strip-bin strip-metamail strip-richmail

strip-bin:

strip-metamail:
	cd metamail && strip $(METAPROGS)

strip-richmail:
	cd richmail && strip $(RICHPROGS)

###### install on local machine #####

localinstall: localinstall-usr-lib localinstall-mailcap localinstall-bin localinstall-metamail localinstall-richmail localinstall-messages

localinstall-usr-lib: /usr/lib/mail/metamail
/usr/lib/mail/metamail:
	if [ ! -d /usr/lib/mail/metamail ]; \
	then mkdir /usr/lib/mail/metamail ; \
	     $(CH)chown bin /usr/lib/mail/metamail ; \
	     $(CH)chgrp mail /usr/lib/mail/metamail ; \
	     $(CH)chmod 775 /usr/lib/mail/metamail ; \
	fi

localinstall-mailcap: mailcap
	$(INS) -f /etc/mail -m 0644 -u bin -g bin mailcap

localinstall-bin: all-bin
	cd bin && for i in $(BINPROGS1); do $(INS) -f /usr/bin -m 0555 -u bin -g bin $$i; done
	cd bin && for i in $(BINPROGS2); do $(INS) -f /usr/lib/mail/metamail -m 0555 -u bin -g bin $$i; done

localinstall-metamail: all-metamail
	cd metamail && for i in $(METAPROGS); do $(INS) -f /usr/bin -m 0555 -u bin -g bin $$i; done

localinstall-richmail: all-richmail
	cd richmail && for i in $(RICHPROGS); do $(INS) -f /usr/lib/mail/metamail -m 0555 -u bin -g bin $$i; done


localinstall-messages: metamail.str
	$(INS) -f /usr/lib/locale/C/MSGFILES -m 644 -u bin -g mail metamail.str

