#!./perl

# $Header: /SRCS/esmp/usr/src/nw/cmd/gnu/perl-4.036/t/base/pat.t,v 1.1.1.1 1993/10/11 20:26:51 ram Exp $

print "1..2\n";

# first test to see if we can run the tests.

$_ = 'test';
if (/^test/) { print "ok 1\n"; } else { print "not ok 1\n";}
if (/^foo/) { print "not ok 2\n"; } else { print "ok 2\n";}
