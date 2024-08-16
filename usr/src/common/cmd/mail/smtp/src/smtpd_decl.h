/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/smtpd_decl.h	1.1.3.2"
#ident "@(#)smtpd_decl.h	1.4 'attmail mail(1) command'"

/* Function prototypes for the smtpd process */
extern char *convertaddr proto((char *));
extern int alarmsend proto(());
extern int cmdparse proto((char *, int));
extern int death proto((int));
extern int do_mail proto((FILE *, FILE *));
extern int do_rcpt proto((FILE *, FILE *));
extern int doit proto((int, int));
extern int gotodir proto((char *));
extern int init_xfr proto(());
extern int mkctlfile proto((char, char *, char *));
extern int mkdatafile proto((char *));
extern int parse_rcpt proto((char *, int));
extern int shellchars proto((char *));
extern int tgets proto((char *, int, FILE *));
extern int tputs proto((char *, FILE *));
extern void bitch proto((char *, FILE *));
extern void converse proto((FILE *, FILE *, int));
extern void do_data proto((FILE *, FILE *));
extern void do_helo proto((FILE *, FILE *));
extern void from822 proto((char *, char *(*)(), FILE *, FILE *, char *, char *));
extern void process proto((void));
extern void quit proto((FILE *, FILE *));
extern void smtplog proto((char *));
extern void smtpsched proto((char *, char *, int));
