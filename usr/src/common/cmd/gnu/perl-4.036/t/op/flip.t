#!./perl

# $Header: /SRCS/esmp/usr/src/nw/cmd/gnu/perl-4.036/t/op/flip.t,v 1.1.1.1 1993/10/11 20:27:00 ram Exp $

print "1..8\n";

@a = (1,2,3,4,5,6,7,8,9,10,11,12);

while ($_ = shift(a)) {
    if ($x = /4/../8/) { $z = $x; print "ok ", $x + 0, "\n"; }
    $y .= /1/../2/;
}

if ($z eq '5E0') {print "ok 6\n";} else {print "not ok 6\n";}

if ($y eq '12E0123E0') {print "ok 7\n";} else {print "not ok 7\n";}

@a = ('a','b','c','d','e','f','g');

open(of,'../Makefile');
while (<of>) {
    (3 .. 5) && $foo .= $_;
}
$x = ($foo =~ y/\n/\n/);

if ($x eq 3) {print "ok 8\n";} else {print "not ok 8 $x:$foo:\n";}
