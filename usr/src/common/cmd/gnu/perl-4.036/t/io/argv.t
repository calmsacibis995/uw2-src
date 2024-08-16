#!./perl

# $Header: /SRCS/esmp/usr/src/nw/cmd/gnu/perl-4.036/t/io/argv.t,v 1.1.1.1 1993/10/11 20:26:56 ram Exp $

print "1..5\n";

open(try, '>Io.argv.tmp') || (die "Can't open temp file.");
print try "a line\n";
close try;

$x = `./perl -e 'while (<>) {print \$.,\$_;}' Io.argv.tmp Io.argv.tmp`;

if ($x eq "1a line\n2a line\n") {print "ok 1\n";} else {print "not ok 1\n";}

$x = `echo foo|./perl -e 'while (<>) {print $_;}' Io.argv.tmp -`;

if ($x eq "a line\nfoo\n") {print "ok 2\n";} else {print "not ok 2\n";}

$x = `echo foo|./perl -e 'while (<>) {print $_;}'`;

if ($x eq "foo\n") {print "ok 3\n";} else {print "not ok 3 :$x:\n";}

@ARGV = ('Io.argv.tmp', 'Io.argv.tmp', '/dev/null', 'Io.argv.tmp');
while (<>) {
    $y .= $. . $_;
    if (eof()) {
	if ($. == 3) {print "ok 4\n";} else {print "not ok 4\n";}
    }
}

if ($y eq "1a line\n2a line\n3a line\n")
    {print "ok 5\n";}
else
    {print "not ok 5\n";}

`/bin/rm -f Io.argv.tmp`;
