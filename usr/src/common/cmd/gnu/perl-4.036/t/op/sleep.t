#!./perl

# $Header: /SRCS/esmp/usr/src/nw/cmd/gnu/perl-4.036/t/op/sleep.t,v 1.1.1.1 1993/10/11 20:27:04 ram Exp $

print "1..1\n";

$x = sleep 2;
if ($x >= 2 && $x <= 10) {print "ok 1\n";} else {print "not ok 1 $x\n";}
