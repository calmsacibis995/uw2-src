/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)inetinst:catalogs/inetinst_msgs.h	1.1"
/*
 *  inetinst_msgs.h
 *  i18n messages for inetinst utilities.
 */

#define MSG_SET		1
#define	MCAT		"inetinst.cat"

#define	C_USAGE_PKGCAT	1
#define	M_USAGE_PKGCAT	"usage: %s [-v] [-n tcp|spx] -s source pkg \n Where source is specified as:\n host:[location] or\n [host]:location\n"


#define	C_USAGE_PKGCOPY	2
#define	M_USAGE_PKGCOPY	"usage: %s [-v] [-n tcp|spx] -s source -t target pkg \n Where target and source are specified as:\n host:[location] or\n [host]:location\n"

#define	C_USAGE_PKGLIST	3
#define	M_USAGE_PKGLIST	"usage: %s [-v] [-n tcp|spx] -s source pkg \n Where target and source are specified as:\n host:[location] or\n [host]:location\n"

#define	C_USAGE_PKGINSTALL	4
#define	M_USAGE_PKGINSTALL	"usage: %s [-v] [-N] [-n tcp|spx] -s source -t target pkg \n Where target and source are specified as:\n host:[location] or\n [host]:location\n"

#define	C_ERR_SOURCE	5
#define	M_ERR_SOURCE	"Error in source specifcation - giving up.\n"

#define	C_ERR_TARGET	6
#define	M_ERR_TARGET	"Error in target specifcation - giving up.\n"

#define	C_XFER_FORK	7
#define	M_XFER_FORK	"About to fork data transfer process.\n"

#define	C_ERR_FORK	8
#define	M_ERR_FORK	"Could not fork() - giving up.\n"

#define	C_ERR_XFER	9
#define	M_ERR_XFER	"Data transfer failed.\n"

#define	C_TERMINATED	10
#define	M_TERMINATED	"Service terminated. Check %s on hosts %s and %s.\n"

#define	C_TRANS_START	11
#define	M_TRANS_START	"Transmission from %s follows:\n"

#define	C_TRANS_END	12
#define	M_TRANS_END	"End of transmission from %s.\n"

#define	C_EXIT_OK	13
#define	M_EXIT_OK	"Terminating normally.\n"

#define	C_EXIT_SIG	14
#define	M_EXIT_SIG	"Terminating due to signal %d.\n"

#define	C_COND_USAGE	15
#define	M_COND_USAGE	"Usage error"

#define	C_COND_SYSTEM	16
#define	M_COND_SYSTEM	"Bad system call"

#define	C_COND_BADFILE	17
#define	M_COND_BADFILE	"No such file or directory"

#define	C_COND_BADFILE_SERVER	18
#define	M_COND_BADFILE_SERVER	"No such file or directory on server"

#define	C_COND_PERM	19
#define	M_COND_PERM	"No file permissions"

#define	C_COND_BADNET	20
#define	M_COND_BADNET	"Bad network connection"

#define	C_COND_BADPROTO	21
#define	M_COND_BADPROTO	"Protocol was misused"

#define	C_COND_BADOPT	22
#define	M_COND_BADOPT	"Invalid option specified"

#define	C_COND_BADHOST	23
#define	M_COND_BADHOST	"Invalid host in location specification"

#define	C_COND_INTR	24
#define	M_COND_INTR	"Execution was interrupted by client"

#define	C_BADOPT_VAL	25
#define	M_BADOPT_VAL	"%s not a valid value for %s\n"

#define	C_BADKEY_2	26
#define	M_BADKEY_2	"%s ... Not recognized. Expecting %s or %s\n"

#define	C_BADKEY_1	27
#define	M_BADKEY_1	"%s ... Not recognized. Expecting %s\n"

#define	C_BADKEY_OPT	28
#define	M_BADKEY_OPT	"%s ... Not recognized. Expecting option\n"

#define	C_BADKEY_SVC	29
#define	M_BADKEY_SVC	"SERVICE %s ... Not recognized.\n"

#define	C_STARTOPT	30
#define	M_STARTOPT	"Start options input; end with %s\n"

#define	C_HELLO		31
#define	M_HELLO		"Hello %s, ready for software action.\n"

#define	C_READY		32
#define	M_READY		"Ready to do %s\n"

#define	C_COND_SOURCE_INVAL	33
#define	M_COND_SOURCE_INVAL	"Invalid source specification"

#define	C_COND_TARGET_INVAL	34
#define	M_COND_TARGET_INVAL	"Invalid target specification"

#define	C_COND_CLOSED	35
#define	M_COND_CLOSED	"Datastream closed by client"

#define	C_COND_SUCCESS	36
#define	M_COND_SUCCESS	"Successful execution"

#define	C_COND_UNK	37
#define	M_COND_UNK	"Unspecified condition"

#define	C_ERR_UNAME	38
#define	M_ERR_UNAME	"Call to uname() failed"

#define	C_ERR_POPEN	39
#define	M_ERR_POPEN	"Call to popen() failed"

#define	C_ERR_DATA	40
#define	M_ERR_DATA	"Could not get data on %s from %s:%s.\n"

#define	C_ERR_NOTFOUND	41
#define	M_ERR_NOTFOUND	"Service terminated; %s/%s not found.\n"

#define	C_ERR_HOST	42
#define	M_ERR_HOST	"%s is an invalid host specification\n"

#define	C_ERR_PROXY	43
#define	M_ERR_PROXY	"Target host did not request service; giving up.\n"

#define	C_CMD_RET	44
#define	M_CMD_RET	"Command %s returned %d\n"

#define	C_LOG_SOURCE	45
#define	M_LOG_SOURCE	"Source is <%s:%s>\n"

#define	C_LOG_TARGET	46
#define	M_LOG_TARGET	"Target is <%s:%s>\n"

#define	C_LOG_REQUEST	47
#define	M_LOG_REQUEST	"Host %s requested %s\n"

#define	C_LOG_BEGIN	48
#define	M_LOG_BEGIN	"=== %s BEGIN %s.\n"

#define	C_LOG_END	49
#define	M_LOG_END	"=== %s END %s.\n"

#define	C_MAIL_LOG	50
#define	M_MAIL_LOG	"Log of"

#define	C_FOUND_NET	51
#define	M_FOUND_NET	"Found network: %s\n"

#define	C_ERR_FOUND_NET	52
#define	M_ERR_FOUND_NET	"Requested network %s not found.\n"

#define	C_ERR_TOPEN	53
#define	M_ERR_TOPEN	"Cannot t_open %s: error %d (errno=%d)\n"

#define	C_ERR_TBIND	54
#define	M_ERR_TBIND	"Cannot t_bind %s: error %d\n"

#define	C_ERR_TALLOC	55
#define	M_ERR_TALLOC	"Cannot t_alloc %s: error %d\n"

#define	C_ERR_TCONNECT	56
#define	M_ERR_TCONNECT	"Cannot t_connect: error %d\n"

#define	C_ERR_IPOP	57
#define	M_ERR_IPOP	"Cannot pop STREAMS module: error %d\n"

#define	C_ERR_IPUSH	58
#define	M_ERR_IPUSH	"Cannot push STREAMS module %s: error %d\n"

#define	C_ERR_DUP2	59
#define	M_ERR_DUP2	"Cannot dup2() %s: error %d\n"

#define	C_LOG_SENT	60
#define	M_LOG_SENT	"SENT: %s"

#define	C_LOG_RCVD	61
#define	M_LOG_RCVD	"RCVD: %s"

#define	C_LOG_OPTS	62
#define	M_LOG_OPTS	"Options used:\n"

