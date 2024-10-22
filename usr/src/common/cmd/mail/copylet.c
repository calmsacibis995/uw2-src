/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/copylet.c	1.13.5.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)copylet.c	2.36 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	copylet - copy a given letter to a file pointer

    SYNOPSIS
	int copylet(Letinfo *pletinfo, int letnum, FILE *f, CopyLetFlags type, int pflg, int Pflg)

    DESCRIPTION
	Copylet() will copy the letter "letnum" to the
	given file pointer.

		pletinfo -> letter info
		letnum	 -> index into: letter table
		f	 -> file pointer to copy file to
		type	 -> copy type
		pflg	 -> user typed 'p' (print binary)
		Pflg	 -> user typed 'P' (print all headers)

	Returns TRUE on a completely successful copy.
*/

int copylet(pletinfo, letnum, f, type, pflg, Pflg)
Letinfo		*pletinfo;
int		letnum;
register FILE	*f;
CopyLetFlags	type;
int		pflg;
int		Pflg;
{
	static const char pn[] = "copylet";
	static const char binmsg[] =
		":366:\n*** Message content is not printable: print, delete, write or save it to a file ***\n";
	static const char localemsg[] =
		":389:\n*** Message locale '%s' does not match current locale '%s': print, delete, write or save it to a file ***\n";
	char	buf[LSIZE], lastc;
	int	n;
	long	k;
	int	num;
	int	rtrncont = 1;	/* True: nondelivery&content included, or regular mail */
	int	suppress = FALSE;
	int	print_from_struct = FALSE; /* print from hdrlines struct */
					   /* rather than fgets() buffer */
	int	pushrest = FALSE;
	int	ctf = FALSE;
	int	didafflines = FALSE;	/* Did we already put out any */
					/* H_AFWDFROM lines? */
	int	didrcvlines = FALSE;	/* Did we already put out any */
					/* H_RECEIVED lines? */
	long	clen = -1L;
	int	htype;			/* header type */
	Hdrs	*hptr;

	/* Clear out any saved header info from previous message */
	clr_hdrinfo(pletinfo->phdrinfo);

	fseek(pletinfo->tmpfile.tmpf, pletinfo->let[letnum].adr, 0);
	/* Get size of message as stored into tempfile by copymt() */
	k = pletinfo->let[letnum+1].adr - pletinfo->let[letnum].adr;
	Dout(pn, 1, "(letnum = %d, type = %d), k = %ld\n", letnum, (int)type, k);
	while (k>0) {	/* process header */
		num = ((k < sizeof(buf)) ? k+1 : sizeof(buf));
		if (fgets (buf, num, pletinfo->tmpfile.tmpf) == NULL) {
			return (FALSE);
		}
		if ((n = strlen (buf)) == 0) {
			k = 0;
			break;
		}
		k -= n;
		lastc = buf[n-1];
		if (pushrest) {
			pushrest = (lastc != '\n');
			continue;
		}
		htype = isheader (buf, &ctf, 0, pletinfo->phdrinfo->fnuhdrtype, 0);
		Dout(pn, 5, "loop 1: buf = %s, htype= %d/%s\n", buf, htype, header[htype].tag);
		if (htype == H_CLEN) {
			save_a_txthdr(pletinfo->phdrinfo, buf, htype);
			if ((hptr = pletinfo->phdrinfo->hdrs[H_CLEN]) != (Hdrs *)NULL) {
				clen = atol (s_to_c(hptr->value));
			}
		}
		if (type == ZAP) {
			if (htype != FALSE) {
				pushrest = (lastc != '\n');
				continue;
			}
			/* end of header.  Print non-blank line and bail. */
			Dout(pn, 5, "ZAP end header; n=%d, buf[0] = %d\n", n, buf[0]);
			if (buf[0] != '\n') {
				if (fwrite(buf,1,n,f) != n) {
					sav_errno = errno;
					return(FALSE);
				}
			} else {
				n = 0;
			}
			break;
		}
		/* Copy From line appropriately */
		if (fwrite(buf,1,n-1,f) != n-1)  {
			sav_errno = errno;
			return(FALSE);
		}
		if (lastc != '\n') {
			if (fwrite(&lastc,1,1,f) != 1) {
				sav_errno = errno;
				return(FALSE);
			}
			continue;
		}
		switch(type) {
			case REMOTE:
				fprintf(f, " remote from %s\n", remotefrom);
				break;

			case TTY:
			case ORDINARY:
			default:
				fprintf(f, "\n");
				break;
		}
		fflush(f);
		break;
	}

	/* if not ZAP, copy balance of header */
	n = 0;
	if ((type != ZAP) && rtrncont)
		while (k>0 || n>0) {
			if ((n > 0) && !suppress) {
				if (print_from_struct == TRUE) {
					if (printhdr (type, htype, hptr, f, Pflg) < 0) {
						return (FALSE);
					}
				} else {
				    if (sel_disp(type, htype, buf, Pflg) >= 0) {
					if (fwrite(buf,1,n,f) != n)  {
						sav_errno = errno;
						return(FALSE);
					}
				    }
				}
				if (htype == H_DATE) {
					dumprcv(pletinfo->phdrinfo, type, htype,&didrcvlines,&suppress,f,Pflg);
					dumpaff(pletinfo->phdrinfo, type, htype,&didafflines,&suppress,f,Pflg);
				}
			}
			if (k <= 0) {
				/* Can only get here if k=0 && n>0, which occurs */
				/* in a message with header lines but no content. */
				/* If we haven't already done it, force out any */
				/* H_AFWDFROM or H_RECEIVED lines */
				dumprcv(pletinfo->phdrinfo, type, -1,&didrcvlines,&suppress,f,Pflg);
				dumpaff(pletinfo->phdrinfo, type, -1,&didafflines,&suppress,f,Pflg);
				break;
			}
			num = ((k < sizeof(buf)) ? k+1 : sizeof(buf));
			if (fgets (buf, num, pletinfo->tmpfile.tmpf) == NULL) {
				return (FALSE);
			}
			n = strlen (buf);
			k -= n;
			lastc = buf[n-1];

			if (pushrest) {
				pushrest = (lastc != '\n');
				continue;
			}
			suppress = FALSE;
			print_from_struct = FALSE;
			htype = isheader (buf, &ctf, 0, pletinfo->phdrinfo->fnuhdrtype, 0);
			Dout(pn, 5, "loop 2: buf = %s, htype= %d/%s\n", buf, htype, header[htype].tag);
			/* The following order is defined in the MTA documents. */
			switch (htype) {
			case H_CONT:
			    continue;
			case H_TCOPY:
			case H_CTYPE:
			case H_CLEN:
				save_a_txthdr(pletinfo->phdrinfo, buf, htype);
				hptr = pletinfo->phdrinfo->hdrs[htype];
				if (htype == H_CLEN) {
					clen = atol (s_to_c(hptr->value));
				}
				/*
				 * Use values saved in hdrs[] structure
				 * rather than what was read from tmp file.
				 */
				print_from_struct = TRUE;
				/* FALLTHROUGH */
			case H_EOH:
			case H_AFWDFROM:
			case H_AFWDCNT:
			case H_RECEIVED:
				dumprcv(pletinfo->phdrinfo, type, htype,&didrcvlines,&suppress,f,Pflg);
				dumpaff(pletinfo->phdrinfo, type, htype,&didafflines,&suppress,f,Pflg);
				continue;	/* next header line */
			default:
				pushrest = (lastc != '\n');
				continue;	/* next header line */
			case FALSE:	/* end of header */
				break;
			}

			/* Found the blank line after the headers. */
			if (n > 0) {
				if (fwrite(buf,1,n,f) != n)  {
					return(FALSE);
				}
			}

			Dout(pn, 3,", let[%d].text = %s\n",
				letnum, (pletinfo->let[letnum].binflag == C_Text) ?  "Text" :
					(pletinfo->let[letnum].binflag == C_GText) ? "Generic-Text" :
										     "Binary");

			if ((type == TTY) && (pletinfo->let[letnum].binflag != C_Text) && !pflg) {
				if (pletinfo->let[letnum].binflag == C_GText) {
					char *locale = getenv("LC_CTYPE");
					char *letlocale = pletinfo->let[letnum].encoding_type;
					if (!letlocale) letlocale = "Unknown";
					if (!locale)
					    locale = getenv("LANG");
					if (!locale || (strcmp(letlocale, locale) != SAME)) {
					    pfmt(f, MM_NOSTD, localemsg, letlocale,
						locale ? locale : "Unknown");
					    return (TRUE);
					}
				} else {
					pfmt (f, MM_NOSTD, binmsg);
					return (TRUE);
				}
			}

			if (n == 1 && buf[0] == '\n') {
				n = 0;
			}
			break;
		}

	Dout(pn, 1, "header processed, clen/k/n = %ld/%ld/%d\n", clen, k, n);

	if (clen >= 0) {
		if (((clen - n) == k) || ((clen - n) == (k - 1))) {
			k = clen - n;
		} else {
			/* probable content-length mismatch. show it ALL! */
			Dout(pn, 1, "clen conflict. using k = %ld\n", k);
		}
	}

	/* copy balance of message */
	if (rtrncont)
		while (k > 0) {
			num = ((k < sizeof(buf)) ? k : sizeof(buf));
			if ((n = fread (buf, 1, num, pletinfo->tmpfile.tmpf)) <= 0) {
				Dout(pn, 1, "content-length mismatch. return(FALSE)\n");
				return(FALSE);
			}
			k -= n;
			if (fwrite(buf,1,n,f) != n)  {
				sav_errno = errno;
				return(FALSE);
			}
		}

	Dout(pn, 3, "body processed, k=%ld\n", k);

	if (rtrncont && type != ZAP && type != REMOTE) {
		if (fwrite("\n",1,1,f) != 1)  {
			sav_errno = errno;
			return(FALSE);
		}
	}

	return(TRUE);
}
