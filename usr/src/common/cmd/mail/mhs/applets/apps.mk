#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/mhs/applets/apps.mk	1.1"
#ident	"@(#)nwwhoami:nwwhoami.mk	1.4"
include $(CMDRULES)

#	Where MAINS are to be installed.
INSDIR = $(USRBIN)

SURRDIR =	$(USRLIB)/mail/surrcmd

GLOBALINC =
LOCALDEF = -DN_PLAT_UNIX -DN_USE_CRT

OWN = bin
GRP = bin

all: nwwritefile nwreadfile nwdeletefile nwrenamefile nwreaddirectory

nwwritefile: nwwritefile.o
	echo $(CC) ; \
	$(CC) -o nwwritefile nwwritefile.o $(LDFLAGS) \
         -lNwCal -lNwNcp -lNwClnt -lNwLoc -lgen -lnsl \
		 -lnwutil -lnct

nwreadfile: nwreadfile.o
	echo $(CC) ; \
	$(CC) -o nwreadfile nwreadfile.o $(LDFLAGS) \
         -lNwCal -lNwNcp -lNwClnt -lNwLoc -lgen -lnsl \
		 -lnwutil -lnct

nwdeletefile: nwdeletefile.o
	echo $(CC) ; \
	$(CC) -o nwdeletefile nwdeletefile.o $(LDFLAGS) \
         -lNwCal -lNwNcp -lNwClnt -lNwLoc -lgen -lnsl \
		 -lnwutil -lnct

nwrenamefile: nwrenamefile.o
	echo $(CC) ; \
	$(CC) -o nwrenamefile nwrenamefile.o $(LDFLAGS) \
         -lNwCal -lNwNcp -lNwClnt -lNwLoc -lgen -lnsl \
		 -lnwutil -lnct

nwreaddirectory: nwreaddirectory.o
	echo $(CC) ; \
	$(CC) -o nwreaddirectory nwreaddirectory.o $(LDFLAGS) \
         -lNwCal -lNwNcp -lNwClnt -lNwLoc -lgen -lnsl \
		 -lnwutil -lnct

clean:
	rm -f *.o
	
clobber: clean
	rm -f nwwritefile nwreadfile nwdeletefile nwrenamefile nwreaddirectory

lintit:
	$(LINT) $(LINTFLAGS) *.c

install: all
	$(INS) -f $(SURRDIR) -m 0555 -u $(OWN) -g $(GRP) nwreadfile
	$(INS) -f $(SURRDIR) -m 0555 -u $(OWN) -g $(GRP) nwwritefile
	$(INS) -f $(SURRDIR) -m 0555 -u $(OWN) -g $(GRP) nwdeletefile
	$(INS) -f $(SURRDIR) -m 0555 -u $(OWN) -g $(GRP) nwrenamefile
	$(INS) -f $(SURRDIR) -m 0555 -u $(OWN) -g $(GRP) nwreaddirectory

localinstall: nwwritefile nwreadfile nwdeletefile nwrenamefile nwreaddirectory
	$(INS) -f /usr/lib/mail/surrcmd -m 0555 -u $(OWN) -g $(GRP) nwwritefile
	$(INS) -f /usr/lib/mail/surrcmd -m 0555 -u $(OWN) -g $(GRP) nwreadfile
	$(INS) -f /usr/lib/mail/surrcmd -m 0555 -u $(OWN) -g $(GRP) nwdeletefile
	$(INS) -f /usr/lib/mail/surrcmd -m 0555 -u $(OWN) -g $(GRP) nwrenamefile
	$(INS) -f /usr/lib/mail/surrcmd -m 0555 -u $(OWN) -g $(GRP) nwreaddirectory

