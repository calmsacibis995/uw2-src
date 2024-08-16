#!./perl

# $Header: /SRCS/esmp/usr/src/nw/cmd/gnu/perl-4.036/t/io/dup.t,v 1.1.1.1 1993/10/11 20:26:56 ram Exp $

print "1..6\n";

print "ok 1\n";

open(dupout,">&STDOUT");
open(duperr,">&STDERR");

open(STDOUT,">Io.dup") || die "Can't open stdout";
open(STDERR,">&STDOUT") || die "Can't open stderr";

select(STDERR); $| = 1;
select(STDOUT); $| = 1;

print STDOUT "ok 2\n";
print STDERR "ok 3\n";
system 'echo ok 4';
system 'echo ok 5 1>&2';

close(STDOUT);
close(STDERR);

open(STDOUT,">&dupout");
open(STDERR,">&duperr");

system 'cat Io.dup';
unlink 'Io.dup';

print STDOUT "ok 6\n";
