#!./perl

# $Header: /SRCS/esmp/usr/src/nw/cmd/gnu/perl-4.036/t/op/ord.t,v 1.1.1.1 1993/10/11 20:27:02 ram Exp $

print "1..2\n";

# compile time evaluation

if (ord('A') == 65) {print "ok 1\n";} else {print "not ok 1\n";}

# run time evaluation

$x = 'ABC';
if (ord($x) == 65) {print "ok 2\n";} else {print "not ok 2\n";}
