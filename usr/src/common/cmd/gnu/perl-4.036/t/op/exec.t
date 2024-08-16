#!./perl

# $Header: /SRCS/esmp/usr/src/nw/cmd/gnu/perl-4.036/t/op/exec.t,v 1.1.1.1 1993/10/11 20:26:59 ram Exp $

$| = 1;				# flush stdout
print "1..8\n";

print "not ok 1\n" if system "echo ok \\1";	# shell interpreted
print "not ok 2\n" if system "echo ok 2";	# split and directly called
print "not ok 3\n" if system "echo", "ok", "3"; # directly called

if (system "true") {print "not ok 4\n";} else {print "ok 4\n";}

if ((system "/bin/sh -c 'exit 1'") != 256) { print "not "; }
print "ok 5\n";

if ((system "lskdfj") == 255 << 8) {print "ok 6\n";} else {print "not ok 6\n";}

unless (exec "lskdjfalksdjfdjfkls") {print "ok 7\n";} else {print "not ok 7\n";}

exec "echo","ok","8";
