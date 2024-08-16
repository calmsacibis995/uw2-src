#!./perl

# $Header: /SRCS/esmp/usr/src/nw/cmd/gnu/perl-4.036/t/op/cond.t,v 1.1.1.1 1993/10/11 20:26:58 ram Exp $

print "1..4\n";

print 1 ? "ok 1\n" : "not ok 1\n";	# compile time
print 0 ? "not ok 2\n" : "ok 2\n";

$x = 1;
print $x ? "ok 3\n" : "not ok 3\n";	# run time
print !$x ? "not ok 4\n" : "ok 4\n";
