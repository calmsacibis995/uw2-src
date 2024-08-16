#!./perl

# $Header: /SRCS/esmp/usr/src/nw/cmd/gnu/perl-4.036/t/op/sprintf.t,v 1.1.1.1 1993/10/11 20:27:04 ram Exp $

print "1..1\n";

$x = sprintf("%3s %-4s%%foo %5d%c%3.1f","hi",123,456,65,3.0999);
if ($x eq ' hi 123 %foo   456A3.1') {print "ok 1\n";} else {print "not ok 1 '$x'\n";}
