#!./perl

# $Header: /SRCS/esmp/usr/src/nw/cmd/gnu/perl-4.036/t/comp/script.t,v 1.1.1.1 1993/10/11 20:26:54 ram Exp $

print "1..3\n";

$x = `./perl -e 'print "ok\n";'`;

if ($x eq "ok\n") {print "ok 1\n";} else {print "not ok 1\n";}

open(try,">Comp.script") || (die "Can't open temp file.");
print try 'print "ok\n";'; print try "\n";
close try;

$x = `./perl Comp.script`;

if ($x eq "ok\n") {print "ok 2\n";} else {print "not ok 2\n";}

$x = `./perl <Comp.script`;

if ($x eq "ok\n") {print "ok 3\n";} else {print "not ok 3\n";}

`/bin/rm -f Comp.script`;
