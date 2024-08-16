#!./perl

# $Header: /SRCS/esmp/usr/src/nw/cmd/gnu/perl-4.036/t/op/join.t,v 1.1.1.1 1993/10/11 20:27:01 ram Exp $

print "1..3\n";

@x = (1, 2, 3);
if (join(':',@x) eq '1:2:3') {print "ok 1\n";} else {print "not ok 1\n";}

if (join('',1,2,3) eq '123') {print "ok 2\n";} else {print "not ok 2\n";}

if (join(':',split(/ /,"1 2 3")) eq '1:2:3') {print "ok 3\n";} else {print "not ok 3\n";}
