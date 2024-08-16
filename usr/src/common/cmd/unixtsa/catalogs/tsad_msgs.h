/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/catalogs/tsad_msgs.h	1.6"
/***
 *
 *  name	tsad_msgs.h - i18n messages for tsad
 *
 ***/

#define MSG_SET		1
#define MCAT		"tsad.cat"

#define C_USAGE		1
#define	M_USAGE		"usage: %s [-f <config_file>] [ -l <log_file>] [ -c <connect_timeout> ]\n\t[-t <register_timeout>] [-p <tsaunix path>]"

#define	C_NOPROTO	2
#define M_NOPROTO	"No suitable network protocol found."

#define C_NOLISTEN	3
#define	M_NOLISTEN	"tsad: unable to open any network endpoints; exiting.\n"

#define C_EBIND		4
#define M_EBIND		"tsad: cannot t_bind %s: error %d\n"

#define C_EALLOC	5
#define M_EALLOC	"tsad: cannot t_alloc a %s: error %d\n"

#define C_TCONNECT	6
#define M_TCONNECT	"tsad: cannot connect to server %s via %s: error %d\n"

#define C_NOADDRESS	7
#define M_NOADDRESS	"No address or %s port for server %s via %s\n"

#define C_NOPROTOCOL	8
#define	M_NOPROTOCOL	"Protocol %s to server %s is not available\n"

#define C_TSND		9
#define M_TSND		"tsad: error %d from t_snd (t_look = %d)\n"

#define C_TRCV		10
#define M_TRCV		"tsad: error %d from t_rcv (t_look = %d)\n"

#define C_BADREPLY	11
#define M_BADREPLY	"tsad: unrecognized reply type %d\n"

#define C_NOSERVERS	12
#define M_NOSERVERS	"Could not contact the SMS server(s)\n"

#define C_EPOLL		13
#define M_EPOLL		"tsad: poll: error %d\n"

#define C_PEVENT	14
#define	M_PEVENT	"tsad: poll: unexpected %s event %d\n"

#define C_TOPEN		15
#define M_TOPEN		"tsad: cannot t_open %s: error %d (errno=%d)\n"

#define C_EFORK		16
#define M_EFORK		"tsad: cannot fork %d: error %d\n"

#define C_EPGRP		17
#define M_EPGRP		"tsad: cannot setpgrp: error %d\n"

#define C_ELISTEN	18
#define M_ELISTEN	"tsad: cannot t_listen to incoming request: error %d\n"

#define C_EACCEPT	19
#define M_EACCEPT	"tsad: cannot t_accept: error %d\n"

#define C_EXEC		20
#define M_EXEC		"exec failed"

#define	C_STAT		21
#define M_STAT		"tsad: cannot stat %s: error %d\n"

#define C_MALLOC	22
#define M_MALLOC	"tsad: malloc failure: error %d\n"

#define C_TIMEOUT	23
#define M_TIMEOUT	"tsad: registration connection to server %s timed out\n"

#define C_SDIED		24
#define M_SDIED		"tsad: timeout during registration with server %s\n"

#define C_STARTUP	25
#define M_STARTUP	"tsad: initializing at %s"

#define C_NONLM		26
#define M_NONLM		"\tTSAPROXY.NLM may not be running on %s\n"

#define C_NOTSA		27
#define M_NOTSA		"TSA executable %s not accessible; exiting\n"

#define C_NOCONFIG	28
#define M_NOCONFIG	"tsad: no unixtsa configuration data found.\n"

#define C_NOLOG		29
#define M_NOLOG		"tsad: cannot open log file %s, using %s\n"

#define C_UNKNOWN	30
#define M_UNKNOWN	"tsad: connection from unknown server %s refused\n"

#define C_CONNECT	31
#define M_CONNECT	"tsad: accepted connection from server %s via %s.\n"
