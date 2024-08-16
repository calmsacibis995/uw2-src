;# $Header: /SRCS/esmp/usr/src/nw/cmd/gnu/perl-4.036/lib/importenv.pl,v 1.1.1.1 1993/10/11 20:26:38 ram Exp $

;# This file, when interpreted, pulls the environment into normal variables.
;# Usage:
;#	require 'importenv.pl';
;# or
;#	#include <importenv.pl>

local($tmp,$key) = '';

foreach $key (keys(ENV)) {
    $tmp .= "\$$key = \$ENV{'$key'};" if $key =~ /^[A-Za-z]\w*$/;
}
eval $tmp;

1;
