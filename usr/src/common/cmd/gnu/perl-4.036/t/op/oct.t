#!./perl

# $Header: /SRCS/esmp/usr/src/nw/cmd/gnu/perl-4.036/t/op/oct.t,v 1.1.1.1 1993/10/11 20:27:01 ram Exp $

print "1..3\n";

if (oct('01234') == 01234) {print "ok 1\n";} else {print "not ok 1\n";}
if (oct('0x1234') == 0x1234) {print "ok 2\n";} else {print "not ok 2\n";}
if (hex('01234') == 0x1234) {print "ok 3\n";} else {print "not ok 3\n";}
