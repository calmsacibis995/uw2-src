/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)cs:cs/extern.h	1.10.2.6"
#ident	"$Header: $"

extern	int	check_device_privs(char *);
extern	int	tli_connect(void);
extern	void	update_cache_list(char *);
extern	int	blank(char *);
extern	int	comment(char *);
extern	int	checkscheme(char *, char *, char *);
extern	void	opendebug();
extern	void	debug();
extern	void	check_log_size();
extern	void	check_debug_size();
extern  void	*setnetpath();
extern 	struct	netconfig *getnetpath();
extern	int	getscheme();
extern	int	get_alias();
extern	CALL 	*read_dialrequest();	
extern	int 	write_dialrequest();	
extern	int 	dev_stat();	
extern	int 	devalloc();	
extern	char	Scratch[];
extern	char	msg[];
extern 	int 	Debug;
extern 	int 	Verbose;
extern	CALL	Call;
extern	CALL	*Callp;
extern	int 	Debugging;
extern	int 	netfd;		/* fd into the network	*/
extern	int 	returnfd;	/* authenticated fd to return */
extern 	struct	nd_hostserv	Nd_hostserv;
extern 	struct	nd_hostserv	*Nd_hostservp;
extern	int	Pid;

/* in order for DUMP to work properly "x" must be of the form
 *	(msg, "string_with_printf_options", [args])
 * where args is optional
 */
#define DUMP(x) if (Debugging) {sprintf x; debug(msg);}
#define CS_LOG(x) {sprintf x; logmsg(msg); if (Debugging) debug(msg);}

/* a special define for debug_dial */
#define D_DIAL(x) {sprintf x; debug(msg);}

