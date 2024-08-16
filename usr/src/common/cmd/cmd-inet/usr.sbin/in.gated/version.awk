#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.gated/version.awk	1.2"
#ident	"$Header: $"

#
#	STREAMware TCP
#	Copyright 1987, 1993 Lachman Technology, Inc.
#	All Rights Reserved.
#

#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#

#
#	System V STREAMS TCP - Release 4.0
#
#   Copyright 1990 Interactive Systems Corporation,(ISC)
#   All Rights Reserved.
#
#	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
#	All Rights Reserved.
#
#	The copyright above and this notice must be preserved in all
#	copies of this source code.  The copyright above does not
#	evidence any actual or intended publication of this source
#	code.
#
#	This is unpublished proprietary trade secret source code of
#	Lachman Associates.  This source code may not be copied,
#	disclosed, distributed, demonstrated or licensed except as
#	expressly authorized by Lachman Associates.
#
#	System V STREAMS TCP was jointly developed by Lachman
#	Associates and Convergent Technologies.
#
#      SCCS IDENTIFICATION
#
#	$Header: /users/jch/src/gated/src/RCS/version.awk,v 2.0 90/04/16 16:54:34 jch Exp $
#
BEGIN {
	maxfields = 4;
	max = 0; strmax = ""; test =""; local="";
	for (i = 1; i <= maxfields; i++) {
		power[i] = exp(log(10)*(maxfields-i));
	}
}
/\$Header/ && !/flex/ {
	if (NF >= 3) {
		version = "";
		if ( substr($2,1,6) == "*rcsid" ) {
			version = $6;
                        locked = $10;
			newlock = $11;
		} 
		if ( $1 == "*" ) {
			version = $4;
			locked = $8;
			newlock = $9;
		}
		if ( version == "" ) {
			continue;
		}
                if ( locked == "Locked" || newlock == "Locker:") {
			test = ".development";
		}
		sum = 0;
		num = split(version, string, ".")
		if (num > maxfields) {
			local = ".local";
			num = maxfields;
		}
		for (i = 1; i <= num; i++) {
			sum += string[i]*power[i];
		}
		if ( sum > max ) {
			max = sum;
			strmax = version;
		}
	}
}
END {
	print "char *version = \"@(#)" strmax local test "\";"
}

