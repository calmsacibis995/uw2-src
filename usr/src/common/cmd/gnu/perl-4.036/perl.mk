#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)gnu.cmd:perl-4.036/perl.mk	1.13"
# : Makefile.SH,v 17005Revision: 1.1.1.1 17005Date: 1993/10/11 20:25:49 $
#
# $Log: perl.mk,v $
# Revision 1.18  1994/06/14  18:54:56  eric
# Changed perl.mk so that the install is done in $(ROOT)/$(MACH) not /
#
# Revision 1.17  1994/06/13  18:35:15  eric
# Added localperl to clobber.
#
# Revision 1.16  1994/06/13  18:28:52  eric
# Changed localperl to link from /usr/lib instead of /usr/ccs/lib.
#
# Revision 1.15  1994/06/13  17:40:41  sharriso
# Added localperl linked with /usr/lib for installation.
#
# Revision 1.11  1994/04/08  21:31:09  sharriso
# Added artistic and copying.
#
# Revision 1.10  1994/04/04  22:17:00  sharriso
# Fixed dependencies.
#
# Revision 1.9  1994/04/04  22:08:57  sharriso
# Fixed dependencies
#
# Revision 1.8  1994/04/04  21:37:15  sharriso
# Fixed ucb paths
#
# Revision 1.7  1994/04/01  14:12:45  eric
# Updated makefile per Scott Harrison
#
# Revision 1.6  1994/03/17  23:49:17  sharriso
# Added -lcrypt to the libs.
#
# Revision 1.5  1994/03/17  23:03:19  sharriso
# Stopped clean from deleting cflags.
#
# Revision 1.4  1994/03/17  22:47:07  sharriso
# Stopped clean removing config.h.
#
# Revision 1.3  1994/03/17  02:50:42  sharriso
# Fixed pel.mk to work with usl tree.
#
# Revision 1.1.1.1  1993/10/11  20:25:49  ram
# NUC code from 1.1d release
#
# Revision 4.0.1.4  92/06/08  11:40:43  lwall
# patch20: cray didn't give enough memory to /bin/sh
# patch20: various and sundry fixes
# 
# Revision 4.0.1.3  91/11/05  15:48:11  lwall
# patch11: saberized perl
# patch11: added support for dbz
# 
# Revision 4.0.1.2  91/06/07  10:14:43  lwall
# patch4: cflags now emits entire cc command except for the filename
# patch4: alternate make programs are now semi-supported
# patch4: uperl.o no longer tries to link in libraries prematurely
# patch4: installperl now installs x2p stuff too
# 
# Revision 4.0.1.1  91/04/11  17:30:39  lwall
# patch1: C flags are now settable on a per-file basis
# 
# Revision 4.0  91/03/20  00:58:54  lwall
# 4.0 baseline.
# 
# 

include $(CMDRULES)
bin = /usr/gnu/bin
scriptdir = /tmp/perl
privlib = $(ROOT)/$(MACH)/usr/gnu/lib/perl
mansrc = 
manext = 
LDFLAGS = -L$(TOOLS)/usr/ccs/lib -L$(ROOT)/$(WORK)/usr/ucblib
CLDFLAGS = -L$(TOOLS)/usr/ccs/lib -L$(ROOT)/$(WORK)/usr/ucblib
LCLDFLAGS = -L/usr/lib -L/usr/ucblib
SMALL = 
LARGE =  
mallocsrc = 
mallocobj = 
SLN = ln -s
RMS = rm -f
LOCALINC=-I$(ROOT)/$(MACH)/usr/ucbinclude

libs = -lsocket -lnsl -lmalloc -lm -lx -lcrypt

public = perl

shellflags = 

CCCMD = `sh $(shellflags) cflags $@`

private = localperl

scripts = h2ph

manpages = perl.man h2ph.man

util =

sh = Makefile.SH makedepend.SH h2ph.SH

h1 = EXTERN.h INTERN.h arg.h array.h cmd.h config.h form.h handy.h
h2 = hash.h perl.h regcomp.h regexp.h spat.h stab.h str.h util.h

h = $(h1) $(h2)

c1 = array.c cmd.c cons.c consarg.c doarg.c doio.c dolist.c dump.c
c2 = eval.c form.c hash.c $(mallocsrc) perl.c regcomp.c regexec.c
c3 = stab.c str.c toke.c util.c usersub.c

c = $(c1) $(c2) $(c3)

s1 = array.c cmd.c cons.c consarg.c doarg.c doio.c dolist.c dump.c
s2 = eval.c form.c hash.c perl.c regcomp.c regexec.c
s3 = stab.c str.c toke.c util.c usersub.c perly.c

saber = $(s1) $(s2) $(s3)

obj1 = array.o cmd.o cons.o consarg.o doarg.o doio.o dolist.o dump.o
obj2 = eval.o form.o $(mallocobj) perl.o regcomp.o regexec.o
obj3 = stab.o str.o toke.o util.o

obj = $(obj1) $(obj2) $(obj3)

tobj1 = tarray.o tcmd.o tcons.o tconsarg.o tdoarg.o tdoio.o tdolist.o tdump.o
tobj2 = teval.o tform.o thash.o $(mallocobj) tregcomp.o tregexec.o
tobj3 = tstab.o tstr.o ttoke.o tutil.o

tobj = $(tobj1) $(tobj2) $(tobj3)

lintflags = -hbvxac

addedbyconf = Makefile.old bsd eunice filexp loc pdp11 usg v7

# grrr
SHELL = /bin/sh

all: $(public) $(private) $(util) uperl.o $(scripts)
	cd x2p; $(MAKE) -f *.mk all

# This is the standard version that contains no "taint" checks and is
# used for all scripts that aren't set-id or running under something set-id.
# The $& notation is tells Sequent machines that it can do a parallel make,
# and is harmless otherwise.

perl: $& perly.o $(obj) hash.o usersub.o
	$(CC) $(LARGE) $(CLDFLAGS) $(obj) hash.o perly.o usersub.o $(libs) -o $@

localperl: $& perly.o $(obj) hash.o usersub.o
	$(CC) $(LARGE) $(LCLDFLAGS) $(obj) hash.o perly.o usersub.o $(libs) -o $@

# This command assumes that /usr/include/dbz.h and /usr/lib/dbz.o exist.

dbzperl: $& perly.o $(obj) zhash.o usersub.o
	$(CC) $(LARGE) $(CLDFLAGS) $(obj) zhash.o /usr/lib/dbz.o perly.o usersub.o $(libs) -o dbzperl

zhash.o: hash.c $(h)
	$(RMS) zhash.c
	$(SLN) hash.c zhash.c
	$(CCCMD) -DWANT_DBZ zhash.c
	$(RMS) zhash.c

uperl.o: $& perly.o $(obj) hash.o
	-ld $(LARGE) $(LDFLAGS) -r $(obj) hash.o perly.o -o uperl.o

saber: $(saber)
	# load $(saber)
	# load /lib/libm.a

# This version, if specified in Configure, does ONLY those scripts which need
# set-id emulation.  Suidperl must be setuid root.  It contains the "taint"
# checks as well as the special code to validate that the script in question
# has been invoked correctly.

suidperl: $& tperly.o sperl.o $(tobj) usersub.o
	$(CC) $(LARGE) $(CLDFLAGS) sperl.o $(tobj) tperly.o usersub.o $(libs) \
	    -o suidperl

# This version interprets scripts that are already set-id either via a wrapper
# or through the kernel allowing set-id scripts (bad idea).  Taintperl must
# NOT be setuid to root or anything else.  The only difference between it
# and normal perl is the presence of the "taint" checks.

taintperl: $& tperly.o tperl.o $(tobj) usersub.o
	$(CC) $(LARGE) $(CLDFLAGS) tperl.o $(tobj) tperly.o usersub.o $(libs) \
	    -o taintperl

# Replicating all this junk is yucky, but I don't see a portable way to fix it.

tperly.o: perly.c perly.h $(h)
	$(RMS) tperly.c
	$(SLN) perly.c tperly.c
	$(CCCMD) -DTAINT tperly.c
	$(RMS) tperly.c

tperl.o: perl.c perly.h patchlevel.h perl.h $(h)
	$(RMS) tperl.c
	$(SLN) perl.c tperl.c
	$(CCCMD) -DTAINT tperl.c
	$(RMS) tperl.c

sperl.o: perl.c perly.h patchlevel.h $(h)
	$(RMS) sperl.c
	$(SLN) perl.c sperl.c
	$(CCCMD) -DTAINT -DIAMSUID sperl.c
	$(RMS) sperl.c

tarray.o: array.c $(h)
	$(RMS) tarray.c
	$(SLN) array.c tarray.c
	$(CCCMD) -DTAINT tarray.c
	$(RMS) tarray.c

tcmd.o: cmd.c $(h)
	$(RMS) tcmd.c
	$(SLN) cmd.c tcmd.c
	$(CCCMD) -DTAINT tcmd.c
	$(RMS) tcmd.c

tcons.o: cons.c $(h) perly.h
	$(RMS) tcons.c
	$(SLN) cons.c tcons.c
	$(CCCMD) -DTAINT tcons.c
	$(RMS) tcons.c

tconsarg.o: consarg.c $(h)
	$(RMS) tconsarg.c
	$(SLN) consarg.c tconsarg.c
	$(CCCMD) -DTAINT tconsarg.c
	$(RMS) tconsarg.c

tdoarg.o: doarg.c $(h)
	$(RMS) tdoarg.c
	$(SLN) doarg.c tdoarg.c
	$(CCCMD) -DTAINT tdoarg.c
	$(RMS) tdoarg.c

tdoio.o: doio.c $(h)
	$(RMS) tdoio.c
	$(SLN) doio.c tdoio.c
	$(CCCMD) -DTAINT tdoio.c
	$(RMS) tdoio.c

tdolist.o: dolist.c $(h)
	$(RMS) tdolist.c
	$(SLN) dolist.c tdolist.c
	$(CCCMD) -DTAINT tdolist.c
	$(RMS) tdolist.c

tdump.o: dump.c $(h)
	$(RMS) tdump.c
	$(SLN) dump.c tdump.c
	$(CCCMD) -DTAINT tdump.c
	$(RMS) tdump.c

teval.o: eval.c $(h)
	$(RMS) teval.c
	$(SLN) eval.c teval.c
	$(CCCMD) -DTAINT teval.c
	$(RMS) teval.c

tform.o: form.c $(h)
	$(RMS) tform.c
	$(SLN) form.c tform.c
	$(CCCMD) -DTAINT tform.c
	$(RMS) tform.c

thash.o: hash.c $(h)
	$(RMS) thash.c
	$(SLN) hash.c thash.c
	$(CCCMD) -DTAINT thash.c
	$(RMS) thash.c

tregcomp.o: regcomp.c $(h)
	$(RMS) tregcomp.c
	$(SLN) regcomp.c tregcomp.c
	$(CCCMD) -DTAINT tregcomp.c
	$(RMS) tregcomp.c

tregexec.o: regexec.c $(h)
	$(RMS) tregexec.c
	$(SLN) regexec.c tregexec.c
	$(CCCMD) -DTAINT tregexec.c
	$(RMS) tregexec.c

tstab.o: stab.c $(h)
	$(RMS) tstab.c
	$(SLN) stab.c tstab.c
	$(CCCMD) -DTAINT tstab.c
	$(RMS) tstab.c

tstr.o: str.c $(h) perly.h
	$(RMS) tstr.c
	$(SLN) str.c tstr.c
	$(CCCMD) -DTAINT tstr.c
	$(RMS) tstr.c

ttoke.o: toke.c $(h) perly.h
	$(RMS) ttoke.c
	$(SLN) toke.c ttoke.c
	$(CCCMD) -DTAINT ttoke.c
	$(RMS) ttoke.c

tutil.o: util.c $(h)
	$(RMS) tutil.c
	$(SLN) util.c tutil.c
	$(CCCMD) -DTAINT tutil.c
	$(RMS) tutil.c

perly.h: perly.c
	@ echo Dummy dependency for dumb parallel make
	touch perly.h

perly.c: perly.y perly.fixer
	@ \
case "$(YACC)" in \
    *bison*) echo 'Expect' 25 shift/reduce and 59 reduce/reduce conflicts;; \
    *) echo 'Expect' 27 shift/reduce and 57 reduce/reduce conflicts;; \
esac
	$(YACC) -d perly.y
	sh $(shellflags) ./perly.fixer y.tab.c perly.c
	mv y.tab.h perly.h
	echo 'extern YYSTYPE yylval;' >>perly.h

perly.o: perly.c perly.h $(h)
	$(CCCMD) perly.c

install: all localperl
	./localperl installperl
	$(INS) -f $(privlib) -m 444 -u bin -g bin Artistic
	$(INS) -f $(privlib) -m 444 -u bin -g bin Copying

clobber:
	rm -f perl taintperl suidperl localperl
	cd x2p; $(MAKE) -f *.mk clobber

clean:
	rm -f *.o perly.c
	cd x2p; $(MAKE) -f *.mk clean

realclean: clean
	cd x2p; $(MAKE) -f *.mk realclean
	rm -f *.orig */*.orig *~ */*~ core $(addedbyconf) h2ph h2ph.man
	rm -f perly.c perly.h t/perl Makefile makedepend makedir
	rm -f makefile x2p/Makefile x2p/makefile x2p/cflags
	rm -f c2ph pstruct

# The following lint has practically everything turned on.  Unfortunately,
# you have to wade through a lot of mumbo jumbo that can't be suppressed.
# If the source file has a /*NOSTRICT*/ somewhere, ignore the lint message
# for that spot.

lint: perly.c $(c)
	lint $(lintflags) $(defs) perly.c $(c) > perl.fuzz

depend: makedepend
	- test -f perly.h || cp /dev/null perly.h
	./makedepend
	- test -s perly.h || /bin/rm -f perly.h
	cd x2p; $(MAKE) -f *.mk depend

test: perl
	- cd t && chmod +x TEST */*.t
	- cd t && (rm -f perl; $(SLN) ../perl perl) && ./perl TEST </dev/tty

clist:
	echo $(c) | tr ' ' '\012' >.clist

hlist:
	echo $(h) | tr ' ' '\012' >.hlist

shlist:
	echo $(sh) | tr ' ' '\012' >.shlist

# AUTOMATICALLY GENERATED MAKE DEPENDENCIES--PUT NOTHING BELOW THIS LINE
# If this runs make out of memory, delete /usr/include lines.
array.o: EXTERN.h
array.o: arg.h
array.o: array.c
array.o: array.h
array.o: cmd.h
array.o: config.h
array.o: form.h
array.o: handy.h
array.o: hash.h
array.o: perl.h
array.o: regexp.h
array.o: spat.h
array.o: stab.h
array.o: str.h
array.o: util.h
cmd.o: EXTERN.h
cmd.o: arg.h
cmd.o: array.h
cmd.o: cmd.c
cmd.o: cmd.h
cmd.o: config.h
cmd.o: form.h
cmd.o: handy.h
cmd.o: hash.h
cmd.o: perl.h
cmd.o: regexp.h
cmd.o: spat.h
cmd.o: stab.h
cmd.o: str.h
cmd.o: util.h
cons.o: EXTERN.h
cons.o: arg.h
cons.o: array.h
cons.o: cmd.h
cons.o: config.h
cons.o: cons.c
cons.o: form.h
cons.o: handy.h
cons.o: hash.h
cons.o: perl.h
cons.o: regexp.h
cons.o: spat.h
cons.o: stab.h
cons.o: str.h
cons.o: util.h
consarg.o: EXTERN.h
consarg.o: arg.h
consarg.o: array.h
consarg.o: cmd.h
consarg.o: config.h
consarg.o: consarg.c
consarg.o: form.h
consarg.o: handy.h
consarg.o: hash.h
consarg.o: perl.h
consarg.o: regexp.h
consarg.o: spat.h
consarg.o: stab.h
consarg.o: str.h
consarg.o: util.h
doarg.o: EXTERN.h
doarg.o: arg.h
doarg.o: array.h
doarg.o: cmd.h
doarg.o: config.h
doarg.o: doarg.c
doarg.o: form.h
doarg.o: handy.h
doarg.o: hash.h
doarg.o: perl.h
doarg.o: regexp.h
doarg.o: spat.h
doarg.o: stab.h
doarg.o: str.h
doarg.o: util.h
doio.o: EXTERN.h
doio.o: arg.h
doio.o: array.h
doio.o: cmd.h
doio.o: config.h
doio.o: doio.c
doio.o: form.h
doio.o: handy.h
doio.o: hash.h
doio.o: perl.h
doio.o: regexp.h
doio.o: spat.h
doio.o: stab.h
doio.o: str.h
doio.o: util.h
dolist.o: EXTERN.h
dolist.o: arg.h
dolist.o: array.h
dolist.o: cmd.h
dolist.o: config.h
dolist.o: dolist.c
dolist.o: form.h
dolist.o: handy.h
dolist.o: hash.h
dolist.o: perl.h
dolist.o: regexp.h
dolist.o: spat.h
dolist.o: stab.h
dolist.o: str.h
dolist.o: util.h
dump.o: EXTERN.h
dump.o: arg.h
dump.o: array.h
dump.o: cmd.h
dump.o: config.h
dump.o: dump.c
dump.o: form.h
dump.o: handy.h
dump.o: hash.h
dump.o: perl.h
dump.o: regexp.h
dump.o: spat.h
dump.o: stab.h
dump.o: str.h
dump.o: util.h
eval.o: EXTERN.h
eval.o: arg.h
eval.o: array.h
eval.o: cmd.h
eval.o: config.h
eval.o: eval.c
eval.o: form.h
eval.o: handy.h
eval.o: hash.h
eval.o: perl.h
eval.o: regexp.h
eval.o: spat.h
eval.o: stab.h
eval.o: str.h
eval.o: util.h
form.o: EXTERN.h
form.o: arg.h
form.o: array.h
form.o: cmd.h
form.o: config.h
form.o: form.c
form.o: form.h
form.o: handy.h
form.o: hash.h
form.o: perl.h
form.o: regexp.h
form.o: spat.h
form.o: stab.h
form.o: str.h
form.o: util.h
hash.o: EXTERN.h
hash.o: arg.h
hash.o: array.h
hash.o: cmd.h
hash.o: config.h
hash.o: form.h
hash.o: handy.h
hash.o: hash.c
hash.o: hash.h
hash.o: perl.h
hash.o: regexp.h
hash.o: spat.h
hash.o: stab.h
hash.o: str.h
hash.o: util.h
perl.o: EXTERN.h
perl.o: arg.h
perl.o: array.h
perl.o: cmd.h
perl.o: config.h
perl.o: form.h
perl.o: handy.h
perl.o: hash.h
perl.o: perl.c
perl.o: perl.h
perl.o: regexp.h
perl.o: spat.h
perl.o: stab.h
perl.o: str.h
perl.o: util.h
regcomp.o: EXTERN.h
regcomp.o: INTERN.h
regcomp.o: arg.h
regcomp.o: array.h
regcomp.o: cmd.h
regcomp.o: config.h
regcomp.o: form.h
regcomp.o: handy.h
regcomp.o: hash.h
regcomp.o: perl.h
regcomp.o: regcomp.c
regcomp.o: regcomp.h
regcomp.o: regexp.h
regcomp.o: spat.h
regcomp.o: stab.h
regcomp.o: str.h
regcomp.o: util.h
regexec.o: EXTERN.h
regexec.o: arg.h
regexec.o: array.h
regexec.o: cmd.h
regexec.o: config.h
regexec.o: form.h
regexec.o: handy.h
regexec.o: hash.h
regexec.o: perl.h
regexec.o: regcomp.h
regexec.o: regexec.c
regexec.o: regexp.h
regexec.o: spat.h
regexec.o: stab.h
regexec.o: str.h
regexec.o: util.h
stab.o: EXTERN.h
stab.o: arg.h
stab.o: array.h
stab.o: cmd.h
stab.o: config.h
stab.o: form.h
stab.o: handy.h
stab.o: hash.h
stab.o: perl.h
stab.o: regexp.h
stab.o: spat.h
stab.o: stab.c
stab.o: stab.h
stab.o: str.h
stab.o: util.h
str.o: EXTERN.h
str.o: arg.h
str.o: array.h
str.o: cmd.h
str.o: config.h
str.o: form.h
str.o: handy.h
str.o: hash.h
str.o: perl.h
str.o: regexp.h
str.o: spat.h
str.o: stab.h
str.o: str.c
str.o: str.h
str.o: util.h
toke.o: EXTERN.h
toke.o: arg.h
toke.o: array.h
toke.o: cmd.h
toke.o: config.h
toke.o: form.h
toke.o: handy.h
toke.o: hash.h
toke.o: perl.h
toke.o: regexp.h
toke.o: spat.h
toke.o: stab.h
toke.o: str.h
toke.o: toke.c
toke.o: util.h
util.o: EXTERN.h
util.o: arg.h
util.o: array.h
util.o: cmd.h
util.o: config.h
util.o: form.h
util.o: handy.h
util.o: hash.h
util.o: perl.h
util.o: regexp.h
util.o: spat.h
util.o: stab.h
util.o: str.h
util.o: util.c
util.o: util.h
usersub.o: EXTERN.h
usersub.o: arg.h
usersub.o: array.h
usersub.o: cmd.h
usersub.o: config.h
usersub.o: form.h
usersub.o: handy.h
usersub.o: hash.h
usersub.o: perl.h
usersub.o: regexp.h
usersub.o: spat.h
usersub.o: stab.h
usersub.o: str.h
usersub.o: usersub.c
usersub.o: util.h
h2ph: h2ph.SH config.sh ; /bin/sh h2ph.SH
# WARNING: Put nothing here or make depend will gobble it up!
