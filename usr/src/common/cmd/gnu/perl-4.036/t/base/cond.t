#!./perl

# $Header: /SRCS/esmp/usr/src/nw/cmd/gnu/perl-4.036/t/base/cond.t,v 1.1.1.1 1993/10/11 20:26:51 ram Exp $

# make sure conditional operators work

print "1..4\n";

$x = '0';

$x eq $x && (print "ok 1\n");
$x ne $x && (print "not ok 1\n");
$x eq $x || (print "not ok 2\n");
$x ne $x || (print "ok 2\n");

$x == $x && (print "ok 3\n");
$x != $x && (print "not ok 3\n");
$x == $x || (print "not ok 4\n");
$x != $x || (print "ok 4\n");
