/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:common/cmd/crash/abuf.c	1.2"
#ident	"$Header: abuf.c 1.1 91/07/23 $"

/*
 * This file contains code for the crash function: abuf.
 */

#include <a.out.h>
#include <stdlib.h>
#include <stdio.h>
#include "crash.h"
#include <sys/types.h>
#include <sys/param.h>
#include <fcntl.h>
#include <malloc.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/mac.h>
#include <audit.h>
#include <sys/stat.h>
#include <sys/privilege.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <sys/stropts.h>
#include <sys/sysmacros.h>
#include <sys/auditrec.h>

#define BSZ  1		/* byte size field */
#define SSZ  2		/* short size field */
#define LSZ  4		/* long size field */

static struct syment *Abuf=NULL;	/* namelist symbol pointer */
static struct syment *Alog=NULL;	/* namelist symbol pointer */
static struct syment *Actl=NULL;	/* namelist symbol pointer */
static struct syment *Amac=NULL;	/* namelist symbol pointer */
static struct syment *Anbuf=NULL;	/* namelist symbol pointer */
static struct syment *Utsnm=NULL;	/* namelist symbol pointer */

static abufctl_t adtbuf;	/* internal audit buffer structure */
static kabuf_t a_buf;		/* current audit buffer structure */
static kabuf_t a_fbuf;		/* free audit buffer structure */
static kabuf_t a_dbuf;		/* dirty audit buffer structure */
static arecbuf_t a_rec;		/* audit record structure */

static alogctl_t adtlog;	/* internal audit log structure */
static actlctl_t adtctl;	/* internal audit control structure */

static int type = LSZ;		/* default abuf type */
static char mode = 'x';		/* default abuf mode */
static char *logfile;		
static kabuf_t *dbufp, *cbufp, *tbufp;
static arecbuf_t *arecp; 
static arecbuf_t *rbuffer = NULL;
static kabuf_t *dbuffer = NULL;
static kabuf_t *cbuffer = NULL;
static void prabuf();		/* write audit buffer in specified format*/
static void prbinary();		/* write audit buffer in binary format*/
static void bytehead();		/* write byte order and header record info */
static boolean_t file_redir = B_FALSE;	/* default no file redirection */

extern char *dumpfile;

/*
 * get data in audit buffer 
 */
int
getabuf()
{

	int b, c, flagcnt;
	int nbuf, data;
	int bufusecnt;
	int recusecnt;
	kabuf_t *bufp;

	b = flagcnt = 0;
	nbuf = data = 0;
	bufusecnt = 0;
	recusecnt = 0;

	if (!(Abuf = symsrch("adt_bufctl")))
		error("audit buffer structure not found in symbol table\n");
	readmem(Abuf->n_value, 1, -1, (char *)&adtbuf, sizeof(adtbuf),
		"audit buffer control structure");
	if (adtbuf.a_vhigh == 0) 
		fprintf(fp,"abuf: audit buffer mechanism bypassed\n");

	if (!Anbuf && !(Anbuf = symsrch("adt_nbuf")))
		error("number of audit buffers not found\n");
	readmem(Anbuf->n_value, 1, -1,
		(char *)&nbuf, sizeof(int),"number of audit buffers");

	/* Dirty Buffers? */
	if (dbuffer) {
		free(dbuffer);
		dbuffer = NULL;
	}
	if ((dbufp = adtbuf.a_dbufp) != NULL) {
		if ((dbuffer=(kabuf_t *)calloc(sizeof(kabuf_t) * nbuf,1))==NULL)
			error("getabuf: calloc() failed\n");
		tbufp = dbuffer;
		do {
			readmem((long)dbufp, 1, -1, (kabuf_t *)tbufp,
				sizeof(kabuf_t), "dirty audit buffer");
			tbufp++;
		} while (dbufp = dbufp->ab_next);
	}

	/* Current Buffer? */
	if (cbuffer) {
		free(cbuffer);
		cbuffer = NULL;
	}
	if ((cbufp = adtbuf.a_bufp) != NULL) {
		if ((cbuffer = (kabuf_t *)malloc(sizeof(kabuf_t))) == NULL)
			error("getabuf: malloc() failed\n");
		readmem((long)cbufp, 1, -1, (kabuf_t *)cbuffer,
			sizeof(kabuf_t), "current audit buffer");
	}

	optind = 1;
	while((c = getopt(argcnt, args, "bcdoxw:")) != EOF) {
		switch(c) {
			/* character format */
                        case 'c' :      mode = 'c';
                                        type = BSZ;
                                        flagcnt++;
                                        break;
			/* decimal format */
                        case 'd' :      mode = 'd';
                                        type = LSZ;
                                        flagcnt++;
                                        break;
			/* octal format */
                        case 'o' :      mode = 'o';
                                        type = LSZ;
                                        flagcnt++;
                                        break;
			/* hexadecimal format */
                        case 'x' :      mode = 'x';
                                        type = LSZ;
                                        flagcnt++;
                                        break;
			/* binary format */
			case 'b' :      mode = 'b';
                                        flagcnt++;
                                        break;
			/* file redirection */
			case 'w' :	file_redir = B_TRUE;
					if (mode != 'b')
						redirect();
					if ((logfile=(char *)calloc
					    (strlen(optarg)+1,1))==NULL)
						error("getabuf: calloc() failed\n");
					strcpy(logfile, optarg);
                                        break;
                        default  :      longjmp(syn,0);
                                        break;
		}
	}
	if (flagcnt > 1) 
	   error("only one mode may be specified: b, c, d, o, or x.\n");

	if (mode == 'b') {
		if (file_redir) {
			bytehead();
			if (tbufp = dbuffer) {
				/* go thru dirty buffer list */
				do {
					prbinary(tbufp->ab_bufp,
						 tbufp->ab_inuse);
					tbufp++;
				} while (tbufp = dbufp->ab_next);
			}
			if (tbufp = cbuffer)
				prbinary(tbufp->ab_bufp, tbufp->ab_inuse);
			
			if ((strcmp(dumpfile, "/dev/mem") != 0)
			 && (arecp = adtbuf.a_recp) != NULL) {
				do { /* go thru write-thru-record list */
					if (rbuffer)
						free(rbuffer);
					if ((rbuffer = (arecbuf_t *)calloc
					    (sizeof(arecbuf_t), 1)) == NULL)
						error("getabuf: calloc() failed\n");
					readmem((long)arecp, 1, -1, rbuffer,
						sizeof(arecbuf_t),
						"write-thru record buffer");
					prbinary(rbuffer->ar_bufp,
						 rbuffer->ar_inuse);
				} while (arecp = rbuffer->ar_next);
			}
		}else
			error("unable to display in binary format\n");
	}else {
		fprintf(fp,"\tAUDIT BUFFER CONTROL STRUCTURE\n");
		fprintf(fp,"\ta_vhigh  = 0x%x\n",adtbuf.a_vhigh);
		fprintf(fp,"\ta_bsize  = 0x%x\n",adtbuf.a_bsize);
		fprintf(fp,"\ta_mutex  = 0x%x\n",adtbuf.a_mutex);
		fprintf(fp,"\ta_off_sv = 0x%x\n",adtbuf.a_off_sv);
		fprintf(fp,"\ta_buf_sv = 0x%x\n",adtbuf.a_buf_sv);
		fprintf(fp,"\ta_flags  = 0x%x\n",adtbuf.a_flags);
		fprintf(fp,"\ta_addrp  = 0x%x\n",adtbuf.a_addrp);
		fprintf(fp,"\ta_abufp  = 0x%x\n",adtbuf.a_bufp);
		fprintf(fp,"\ta_fbufp  = 0x%x\n",adtbuf.a_fbufp);
		fprintf(fp,"\ta_dbufp  = 0x%x\n",adtbuf.a_dbufp);
		fprintf(fp,"\ta_recp   = 0x%x\n",adtbuf.a_recp);
		fprintf(fp,"\n\tNUMBER OF AUDIT BUFFERS = %d\n\n", nbuf);

		fprintf(fp,"\tBUFFER\t\tADDRESS\t\tINUSE\t\tNEXT\n");
		bufusecnt = b = 0;
		if (tbufp = dbuffer) {
			do {
				b++;
				fprintf(fp, "\t%08d\t%08x\t%08d\t0x%08x\n", b,
					tbufp->ab_bufp, tbufp->ab_inuse,
					tbufp->ab_next);
				bufusecnt += tbufp->ab_inuse;
				tbufp++;
			} while (tbufp = dbufp->ab_next);
		}
		if (tbufp = cbuffer) {
			b++;
			fprintf(fp, "\t%08d\t%08x\t%08d\t0x%08x\n", b,
				tbufp->ab_bufp, tbufp->ab_inuse, tbufp->ab_next);
			bufusecnt += tbufp->ab_inuse;
		}
		if (!b)
			fprintf(fp, "\t0\t\t0x0\t\t0\t\t0x0\n");

		(void)fprintf(fp, "\n\tAudit Buffer Size: %d bytes\n",
			adtbuf.a_bsize);
		(void)fprintf(fp, "\tAmount of Data: %d bytes\n", bufusecnt);
		if (tbufp = dbuffer) {
			do { /* print dirty buffer list */
				prabuf(tbufp->ab_bufp, tbufp->ab_inuse);
				tbufp++;
			} while (tbufp = dbufp->ab_next);
		}
		if (tbufp = cbuffer) /* print current buffer */
			prabuf(tbufp->ab_bufp, tbufp->ab_inuse);
		if ((strcmp(dumpfile, "/dev/mem") != 0)
		 && (arecp = adtbuf.a_recp) != NULL) {
			do { /* print write-thru record list */
				if (rbuffer)
					free(rbuffer);
				if ((rbuffer = (arecbuf_t *)calloc
				    (sizeof(arecbuf_t), 1)) == NULL)
					error("getabuf: calloc() failed\n");
				readmem((long)arecp, 1, -1, rbuffer,
					sizeof(arecbuf_t),
					"write-thru record buffer");
				prabuf(rbuffer->ar_bufp, rbuffer->ar_inuse);
			} while (arecp = rbuffer->ar_next);
		}
	}
	return(0);
}

/*
 * Print or display contents of the audit buffer.
 */
static void
prabuf(addr, size)
long	addr;
int	size;
{
	int	i;
	char	ch;
	long	lnum;
	long	value;
	char	*format;
	int	precision;

	for (i = 0; i < size; i++) {
		switch(type) {
			case BSZ :  readmem(addr, 1, 0, &ch,
					sizeof(ch), "audit buffer");
			            value = ch & 0377;
				    break;
			case LSZ :
				    readmem(addr, 1, 0, (char *)&lnum,
					sizeof(lnum), "audit buffer");
				    value = lnum;
				    break;
		}
		if (((mode == 'c') && ((i % 16) == 0)) 
		 || ((mode != 'c') && ((i % 4)  == 0))) {
				if (i != 0) 
					(void)fprintf(fp,"\n");
				(void)fprintf(fp, "\t%8.8x:  ", addr);
			}
		switch(mode) {
		case 'c' :  switch(type) {
				case BSZ :  putch(ch);
					    break;
				case LSZ :  putch((char)lnum);
					    break;
			    }
			    break;
		case 'o' :  format = "%.*o   ";
			    switch(type) {
				case BSZ :  precision = 3;
					    break;
				case LSZ :  precision = 11;
					    break;
		   		}
		 	    (void)fprintf(fp, format, precision, value);
		 	    break;
		case 'd' :  format = "%.*d   ";
			   switch(type) {
				case BSZ :  precision = 3;
					    break;
				case LSZ :  precision = 10;
					    break;
			    }
		 	    (void)fprintf(fp, format, precision, value);
		   	    break;
		case 'x' :  format = "%.*x   ";
			    switch(type) {
				case BSZ :  precision = 2;
					    break;
				case LSZ :  precision = 8;
					    break;
			    }
		 	    (void)fprintf(fp, format, precision, value);
			    break;
		}
		addr += type;
	}
	(void)fprintf(fp, "\n");
}


/*
 * create a file that contains the contents
 * of the audit buffer in binary format.
 */
static void
prbinary(addr, size)
long	addr;
int	size;
{
	char	*buf;
	int	fd;

	if ((fd = open(logfile, O_WRONLY | O_APPEND | O_CREAT, 0600)) > 0) {
		if ((buf = (char * )malloc(sizeof(int) * size)) == NULL)
			error("abuf: unable to allocate space for reading audit buffer\n");
		readmem((long)addr, 1, -1, (char *)buf, size, " audit_buffer");
		if ((write(fd, buf, size)) == -1) {
			close(fd);
			error("error writing buffer to file\n");
		}
	} else
		error("error opening file\n");
	close(fd);
}

static void
bytehead()
{
 	char		*ap, *wap, *byordbuf;
	int		fd, size, mac;
 	idrec_t		*id;
	struct utsname	utsname;
	struct stat	statbuf;
	char		sp[]={' '};
	int		ts, ss, rs;

	ss = ts = rs = 0;
	/* check if the file exists */
	if (stat(logfile, &statbuf) == -1) {
		if ((fd = open(logfile, O_WRONLY | O_CREAT, 0600)) > 0) {
			/* write out machine byte ordering info */
			if ((byordbuf = (char *)calloc(sizeof(char),
				ADT_BYORDLEN + ADT_VERLEN)) == NULL) {
					close(fd);
					error("bytehead: byordbuf calloc failed\n");
			}
			strncpy(byordbuf, ADT_BYORD, ADT_BYORDLEN);
		
			if (!Actl && !(Actl = symsrch("adt_ctl")))
				error("audit control structure not found in symbol table\n");
			readmem(Actl->n_value, 1, -1, (char *)&adtctl,
				sizeof(adtctl), "audit control structure");
			strncpy(byordbuf + ADT_BYORDLEN, adtctl.a_version, 
				ADT_VERLEN);

			if ((write(fd, byordbuf, sizeof(char) * ADT_BYORDLEN + 
				ADT_VERLEN)) != (sizeof(char) * ADT_BYORDLEN +
				ADT_VERLEN)) {
					close(fd);
					error("error writing buffer to file \"%s\"\n",logfile);
			}
		        size = sizeof(idrec_t) + sizeof(struct utsname);
			if ((ap = (char *)malloc(size)) == NULL) {
				close(fd);
				error("bytehead: ap malloc failed\n");
			}

			(void)memset(ap, '\0', size);
		
			/* write out audit log header record info */
			wap = ap;
			id = (idrec_t *)wap;
			id->cmn.c_rtype = id->cmn.c_event = FILEID_R;
		
			if (!Alog && !(Alog = symsrch("adt_logctl")))
				error("audit log structure not found in symbol table\n");
			readmem(Alog->n_value, 1, -1, (char *)&adtlog,
				sizeof(adtlog), "audit log control structure");
			id->cmn.c_seqnum = adtlog.a_seqnum;
			id->cmn.c_crseqnum = FILEID_R;
			strncpy(id->spec.i_mmp, adtlog.a_mmp, ADT_DATESZ);
			strncpy(id->spec.i_ddp, adtlog.a_ddp, ADT_DATESZ);
		
			id->cmn.c_pid = 0;
			id->cmn.c_time.tv_sec = 0;
			id->cmn.c_time.tv_nsec = 0;
			id->cmn.c_status = 0;
		
			if (!Amac && !(Amac = symsrch("mac_installed")))
				error("MAC installed flag not found in symbol table\n");
			id->spec.i_flags = ADT_ON;
			readmem(Amac->n_value, 1, -1,
				(char *)&mac, sizeof(int),"MAC installed flag");
			if (mac)
				id->spec.i_flags |= ADT_MAC_INSTALLED;
			wap += sizeof(idrec_t);
		
			if (!Utsnm && !(Utsnm = symsrch("utsname")))
				error("utsname not found in symbol table\n");
			readmem(Utsnm->n_value, 1, -1, (char *)&utsname,
				sizeof(utsname),"utsname structure");
			ss = strlen(utsname.sysname);
			strcpy(wap,utsname.sysname);
			wap += ss;
			*wap = *sp;
			wap++;
			ts += ss + 1;

			ss = strlen(utsname.nodename);
			strcpy(wap, utsname.nodename);
			wap += ss;
			*wap = *sp;
			wap++;
			ts += ss + 1;

			ss = strlen(utsname.release);
			strcpy(wap,utsname.release);
			wap += ss;
			*wap = *sp;
			wap++;
			ts += ss + 1;

			ss = strlen(utsname.version);
			strcpy(wap, utsname.version);
			wap += ss;
			*wap = *sp;
			wap++;
			ts += ss + 1;

			ss = strlen(utsname.machine);
			strcpy(wap, utsname.machine);
			wap += ss;
			*wap = '\0';
			wap++;
			ts += ss + 1;

			rs = ROUND2WORD(ts);
			id->cmn.c_size = (sizeof(idrec_t) + rs);	
			if ((write(fd, ap, id->cmn.c_size)) != id->cmn.c_size) {
				close(fd);
				error("error writing buffer to file \"%s\"\n",logfile);
			}
		} else 
			error("error opening file \"%s\"\n",logfile);
	} else 
              		error("abuf: file \"%s\" already exists, try another!\n",logfile);
}


