#!./perl

# $Header: /SRCS/esmp/usr/src/nw/cmd/gnu/perl-4.036/t/op/glob.t,v 1.1.1.1 1993/10/11 20:27:00 ram Exp $

print "1..4\n";

@ops = <op/*>;
$list = join(' ',@ops);

chop($otherway = `echo op/*`);

print $list eq $otherway ? "ok 1\n" : "not ok 1\n$list\n$otherway\n";

print $/ eq "\n" ? "ok 2\n" : "not ok 2\n";

while (<jskdfjskdfj* op/* jskdjfjkosvk*>) {
    $not = "not " unless $_ eq shift @ops;
    $not = "not at all " if $/ eq "\0";
}
print "${not}ok 3\n";

print $/ eq "\n" ? "ok 4\n" : "not ok 4\n";
