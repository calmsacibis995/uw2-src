/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lp:cmd/lpsched/lpNet/nucChild/printreq.c	1.9"
#ident	"$Header: $"

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "lp.h"			/* includes fcntl.h sys/types.h sys/stat.h */
#include "lpNet.h"
#include "requests.h"
#include "printers.h"
#include "secure.h"
#include "lpd.h"
#include <pwd.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/nwportable.h>
#include <nw/nwcalls.h>
#include <nw/nwprint.h>

#include <limits.h>

#include <cpscommon.h>
#include <cpsservice.h>


#if defined (__STDC__)
static	char	* r_print_request(int, char *, int, char *);
#else
static	char	* r_print_request();
#endif

static char buff1[PATH_MAX];

#define	SPOOL_BUF_SZ	1024
/*
 * Send print request to NetWare file server
 * (S_PRINT_REQUEST processing)
 */
char *
#if defined (__STDC__)
s_nuc_print_request(char *msg)
#else
s_nuc_print_request(msg)
char	*msg;
#endif
{
	static	NWPrintJobStruct	qmsPrintRequest;
	static	NWQueueJobStruct	qmsJobRequest;
	static	char				spoolBuffer[SPOOL_BUF_SZ];

	int			unSpoolFd;
	REQUEST		*rp;
	SECURE		*sp = NULL;
	PRINTER		*pr = NULL;
	QMS_INFO_T	qmsInfo;
	QMS_SERVICE_T	*qmsHandle;
    NWFILE_HANDLE	spoolHandle;

	char		*rqfile;
	char		*cf = NULL;
	char		**lpnm;
	char		*JobRequest = "65536";
	char		*bannerNameField;
	char		*bannerFileField;
	char		*headerFileName;
	char 		*save_rem_reqid;
	char		*PrintServer;
	char		*PrintQueue = NULL;
	char		*hostName;
	char		**options = NULL, **modes;
	char		*title;
	char		*filename;
	char		*flist;
	char		*files[MAX_LPD_FILES];
	char		*cp;
	char		*tmp_user;

	int		c;
	int		i;
	int		spoolBytes;
	int		authentication_attempts = 0;
	int32		spoolEof;
	int32		startingOffset = 0;
	short		status = SUCCESS;
	int		is_sv = TRUE;
	int 		nfiles;
	int		len;
	int		IsPS;
	struct passwd	*pw;
	uid_t		uid, saved_uid, tmp_uid;
	int			pid;
	nuint8		binderyAccessLevel;
	nuint32		objectID;
	NWCCODE		ccode;
	uint8		*packetPtr;

	saved_uid = geteuid();

        (void)getmessage(msg, S_PRINT_REQUEST, &rqfile);
	logit(LOG_DEBUG, "S_PRINT_REQUEST(rqfile=\"%s\")", rqfile);

	if (!(rp = getrequest(rqfile)) || !(sp = getsecure(rqfile))) {
		status = MNOINFO;
		goto out;
	}
	if (!(pr = getprinter(rp->destination)) || !pr->remote) {
		status = MNOINFO;
		goto out;
	}
	if (Printer = strchr(pr->remote, '!'))
		Printer++;			/* remote name of printer */
	else
		Printer = rp->destination;	/* local name of printer */

	if (PrintServer = strchr(Printer, '!')) {
		*PrintServer = NULL;
		PrintQueue = strdup(Printer);	/* free this one */
		*PrintServer++ = '!';		/* don't free this one */
	}
	else {
		PrintQueue = strdup(Printer);	/* free this one */
		PrintServer = NULL;
	}

	qmsInfo.qmsServer = Rhost;
	qmsInfo.qmsQueue = PrintQueue;
	qmsInfo.pServer = PrintServer;

	/*
	 *  If pr->user is set, then lpNet runs as pr->user when
	 *  connecting to a NetWare file server.  This allows many users
	 *  to print to a file server using only one connection.
	 *
	 *  If pr->user is not set, then lpNet runs as the user that
	 *  submitted the print job.
	*/
	if ((pw = getpwnam(pr->user ? pr->user : rp->user)) == NULL) {
		logit(LOG_WARNING, "can't find %s in password file",
		pr->user ? pr->user : rp->user);
		status = MSEELOG;
		goto out;
	}
	uid = pw->pw_uid;
	logit(LOG_DEBUG, "changing effective UID to %d", uid);
	if (seteuid(uid) == -1) {
		logit(LOG_WARNING, "can't change effective UID to %d", uid);
		status = MSEELOG;
		goto out;
	}
	if (NWcpsAttachQMS(&qmsInfo, &qmsHandle) != SUCCESS) {
		logit(LOG_WARNING, "can't attach to file server \"%s\"", Rhost);
		status = MNOATTACH;
		goto out;
	}

	logit(LOG_DEBUG, "%s attached to \"%s\" queue on \"%s\" for print server \"%s\"",
		Name, PrintQueue, Rhost, PrintServer ? PrintServer : "ANY");

	/*
	 * Much of the following code was stolen library routines
	 * written by Gary B. Tomlinson.
	 */

	IsPS = IsPSPrinter( pr->name );
	/*
	 	if the printjob is postscript then the number of copies has already
		been specified in the postscript encapsulation. There is no need
		to specify them again to NetWare.
	*/

	qmsPrintRequest.clientJobInfoVer	= 0;
	qmsPrintRequest.tabSize			= 8;

	if (IsPS && strcmp(rp->input_type, "PS") 
		 && strcmp(rp->input_type, "postscript")) 
		qmsPrintRequest.numberCopies	= 1;
	else
		qmsPrintRequest.numberCopies	= rp->copies;

	qmsPrintRequest.printFlags	= 0;
	qmsPrintRequest.maxLines	= 66;
	qmsPrintRequest.maxChars	= 80;

	options = dashos(rp->options);

	if (pr->banner & BAN_ALWAYS) {
		if ((pr->banner & BAN_OFF) == 0)
			qmsPrintRequest.printFlags |= NWPCF_PRINT_BANNER;
	} else {
		if (find_listfld("banner", options)) {
			qmsPrintRequest.printFlags |= NWPCF_PRINT_BANNER;
		} else if (find_listfld(NOBANNER, options)) {
			/* do nothing */
		} else { /* no user supplied options - use default */
			if ((pr->banner & BAN_OFF) == 0)
				qmsPrintRequest.printFlags |=
					NWPCF_PRINT_BANNER; 
		}
	}

	if (pr->nw_flags & FF_ALWAYS) {
		if (pr->nw_flags & FF_OFF)
			qmsPrintRequest.printFlags |= NWPCF_SUPPRESS_FF;
	} else {
		if (find_listfld("noformfeed", options)) {
			qmsPrintRequest.printFlags |= NWPCF_SUPPRESS_FF;
		} else if (find_listfld("formfeed", options)) {
			/* do nothing */
		} else { /* no user supplied options - use default */
			if (pr->nw_flags & FF_OFF)
				qmsPrintRequest.printFlags |=
					NWPCF_SUPPRESS_FF;
		}
	}

	strcpy((char *)qmsPrintRequest.formName, "");

	bannerNameField = rp->user;

	memset(qmsPrintRequest.bannerUserName, '\0',
		 NWMAX_BANNER_NAME_FIELD_LENGTH);
	strncpy((char *)qmsPrintRequest.bannerUserName, bannerNameField, 
		(NWMAX_BANNER_NAME_FIELD_LENGTH - 1));

	memset(qmsPrintRequest.bannerFileName, '\0',
		NWMAX_BANNER_FILE_FIELD_LENGTH);

	/*
	 * Unfortunately, we have to figure out whether the job
	 * was spooled by lp(1) or lpr(1).  If spooled by lp(1),
	 * the title, if present, will be a real user specified title,
	 * and the list of files to be printed will be in rp->file_list.
	 * If spooled by lpr(1), the title will be in the 'Y' field,
	 * and the file list will be in both the options field and
	 * rp->file_list.  The difference being that rp->file_list 
	 * contains copies of files listed in the options field.
	 * 
	 */

	if (flist = find_listfld(FLIST, options))
		is_sv = FALSE;

	title = NULL;
	filename = NULL;

	if (is_sv) {

		title = rp->title;

		if (rp->file_list[0])
			filename = rp->file_list[0];
		else
			filename = NUCPS_UNKNOWN_FILE_NAME;
	}
	else {
		modes = dashos(rp->modes);
		if (cp = find_listfld(PRTITLE, modes)) {
			rmesc(cp += STRSIZE(PRTITLE));
			title = cp;
		}
		nfiles = lenlist(rp->file_list);
		nfiles = MIN(nfiles, MAX_LPD_FILES);
		if (parseflist(flist+STRSIZE(FLIST), nfiles, files, NULL) != nfiles)
			goto detach;

		if (*files[0])
			filename = files[0];
		else
			filename = "stdin";
	}

	if (title)
		bannerFileField = title;
	else
		bannerFileField = sp->req_id;

	headerFileName = basename(filename);

	strncpy((char *)qmsPrintRequest.bannerFileName, bannerFileField, 
		(NWMAX_BANNER_FILE_FIELD_LENGTH - 1));

	memset(qmsPrintRequest.bannerHeaderFileName, '\0',
		NWMAX_HEADER_FILE_NAME_LENGTH);
	strncpy((char *)qmsPrintRequest.bannerHeaderFileName, headerFileName, 
			(NWMAX_HEADER_FILE_NAME_LENGTH - 1));

	/*
	 * The qmsPrintRecord.directoryPath pertains to printing a file
	 * on the NetWare Server, not applicable in this case.
	 */
	strcpy((char *)qmsPrintRequest.filePathName,"");

	/*
	 * Put the QMS Print Record into the QMS Job Structure Client Record Area
	 */


	packetPtr = (uint8 *) qmsJobRequest.clientRecordArea;

	*packetPtr++ = qmsPrintRequest.clientJobInfoVer;
	*packetPtr++ = qmsPrintRequest.tabSize;

	NCopyHiLo16(packetPtr, &qmsPrintRequest.numberCopies);
	packetPtr += sizeof(nuint16);

	NCopyHiLo16(packetPtr, &qmsPrintRequest.printFlags);
	packetPtr += sizeof(nuint16);

	NCopyHiLo16(packetPtr, &qmsPrintRequest.maxLines);
	packetPtr += sizeof(nuint16);

	NCopyHiLo16(packetPtr, &qmsPrintRequest.maxChars);
	packetPtr += sizeof(nuint16);

	memcpy((char *)packetPtr, (char *)qmsPrintRequest.formName, 
		   NWMAX_FORM_NAME_LENGTH );
	packetPtr += NWMAX_FORM_NAME_LENGTH;

	memset( (char *)packetPtr, 0, 6);  /* zero reserve 6 bytes */
	packetPtr += 6;

	memcpy((char *)packetPtr, (char *)qmsPrintRequest.bannerUserName,
					NWMAX_BANNER_NAME_FIELD_LENGTH );
	packetPtr += NWMAX_BANNER_NAME_FIELD_LENGTH;

	memcpy((char *)packetPtr, (char *)qmsPrintRequest.bannerFileName, 
					NWMAX_BANNER_FILE_FIELD_LENGTH );
	packetPtr += NWMAX_BANNER_FILE_FIELD_LENGTH;

	memcpy((char *)packetPtr, (char *)qmsPrintRequest.bannerHeaderFileName, 
					NWMAX_HEADER_FILE_NAME_LENGTH );
	packetPtr += NWMAX_HEADER_FILE_NAME_LENGTH;

	memcpy((char *)packetPtr, (char *)qmsPrintRequest.filePathName, 
					NWMAX_JOB_DIR_PATH_LENGTH );
	packetPtr += NWMAX_JOB_DIR_PATH_LENGTH;


	/*
	 * Complete the QMS Job Request
	 */
	qmsJobRequest.targetServerID = qmsHandle->pServerID;
	for ( i = 0; i < NWMAX_QUEUE_JOB_TIME_SIZE; i++ )
		qmsJobRequest.targetExecutionTime[i] = SCHEDULE_ASAP;
	qmsJobRequest.jobType = 0;

	memset(qmsJobRequest.jobFileName, '\0', NWMAX_JOB_FILE_NAME_LENGTH);
	strncpy((char *)qmsJobRequest.jobFileName, filename, 
		(NWMAX_JOB_FILE_NAME_LENGTH - 1));

	/*
	 * Fabricate description for use by status queries to indicate the
	 * NUC is the creator, and then the originating file name
	 */
	memset( (char *)(qmsJobRequest.jobDescription), '\0',
		NWMAX_JOB_DESCRIPTION_LENGTH);
	hostName = Lhost;
	sprintf((char *) (qmsJobRequest.jobDescription), "%s@%s:",
		rp->user, hostName);
	strncat( (char *)(qmsJobRequest.jobDescription), filename,
		(NWMAX_JOB_DESCRIPTION_LENGTH -
			strlen((char *)qmsJobRequest.jobDescription) - 1));

	/*
	 * Create the Print Job, and associated Spool File
	 */

	while ((ccode = callAPI(CREATE_QUEUE_FILE_2, qmsHandle->connID, 
									   qmsHandle->queueID,
									   &qmsJobRequest, 
									   &spoolHandle)) != SUCCESS) {
	    switch(ccode) {

		case ERR_NO_Q_RIGHTS:

		    /*
		     *  Distinguish between having no queue rights
		     *  and not being logged in.
		     */

		    if (callAPI(GET_BINDERY_ACCESS_LEVEL, qmsHandle->connID,
			&binderyAccessLevel, &objectID))
		    {
			logit(LOG_WARNING, "NWGetBinderyAccessLevel failed");
			status = MSEELOG;
			goto detach;
		    }
		    if ((int)(binderyAccessLevel & 0x0f)
				>= (int)BS_LOGGED_READ )
		    {
			logit(LOG_DEBUG, "no queue rights");
			status = MNORIGHTS;
			goto detach;
		    }

		    if (++authentication_attempts > MAX_AUTHENTICATION_ATTEMPTS) {
			logit(LOG_WARNING, "Login to %s failed", Rhost);
			status = MNOLOGIN;
			goto detach;
		    }
			pid = 0;
		    authenticateToServer(Rhost, pr->user ? pr->user : rp->user,
								 &uid, &pid);
		    break;

		default:
		    logit(LOG_WARNING, "NWCreateQueueFile2 failed, Return Value = %x",
				  ccode);
		    status = MSEELOG;
		    goto detach;
	    }
	}

	/*
	 * Unspool file from UNIX, and spool it to QMS
	 */
	len = strlen( Lp_Tmp ) + 1;
	for (i=0, lpnm = rp->file_list; i<MAX_LPD_FILES && *lpnm; i++, lpnm++){
		if ( ( strncmp( *lpnm, Lp_Tmp, strlen( Lp_Tmp ) ) ) != 0 )
		{
			char temp[PATH_MAX];
			char *ptr;
			FILE *fd;

			strcpy( temp, rqfile );
			ptr = strrchr( temp, '-' ); 	
			if ( ptr != NULL )
			{
				ptr++;	
				sprintf( ptr, "%d", i + 1 );
				sprintf( buff1, "%s/tmp/%s", Lp_NetTmp, temp );
			}
			else sprintf( buff1, "%s", *lpnm );
		}
		else
			sprintf( buff1, "%s/tmp/%s", Lp_NetTmp, (*lpnm)+len);	
		if ((unSpoolFd = open(buff1, O_RDONLY)) == -1) {
			logit(LOG_DEBUG,
				"can't open spool file: changing UID to %d", saved_uid);
			(void) seteuid(saved_uid);
			if ((unSpoolFd = open(buff1, O_RDONLY)) == -1) {
				logit(LOG_WARNING, "can't open spool file: %s", buff1);
				status = MSEELOG;
				(void) seteuid(uid);
				continue;
			}
			(void) seteuid(uid);
		}
		spoolEof = 0;
		logit(LOG_DEBUG, "spooled file: %s", *lpnm);
		do {
retry:
			if ((spoolBytes = read(unSpoolFd,spoolBuffer,SPOOL_BUF_SZ)) == -1) {
				/*
				 * handle error; in case of EAGAIN,retry.
				 */
				if( errno = EAGAIN)
					goto retry;

				/*
				 * Handle failure error
				 */
				status = ~(SUCCESS);
				break;
			}

			/*
			 * Reading less than we expected means we are at EOF
			 */
			if( spoolBytes < SPOOL_BUF_SZ)
				spoolEof = 1;

			if ( status == SUCCESS ) {

				/*
				 * Spool buffer to QMS
				 */
				if ( callAPI(WRITE_FILE, spoolHandle, spoolBytes, (pnuint8)spoolBuffer)
						!= SUCCESS) {
					status = ~(SUCCESS);
				}
	
				/*
				 *  if there are more than one file to print in this job
				 *  and FF_OFF is not set, insert a formfeed.
				 */

				if ( status == SUCCESS && spoolEof && ( (i+1) != nfiles)) {
					if ( !(pr->nw_flags & FF_OFF) ) {
						if ( !IsPS) {
							/*
							 * Non-PostScript formfeed...
							 */
							if ( callAPI(WRITE_FILE, spoolHandle, 1, (pnuint8)"\f")
									!= SUCCESS) {
								status = ~(SUCCESS);
							}
						}
					}
				}

			}
		} while ( (status == SUCCESS) && !spoolEof );

		if ( IsPS )
		{
			spoolBuffer[0] = 0x04;
			spoolBytes = 1;
			if ( callAPI(WRITE_FILE, spoolHandle, spoolBytes, (pnuint8)spoolBuffer)
					!= SUCCESS) 
				status = ~(SUCCESS);
		}
		close(unSpoolFd);
		if (status != SUCCESS)
			break;
	}

	if (status != SUCCESS) {
		/*
		 * Something went wrong with the unspool/spool process
		 *	Abort the QMS Request, and cleanup
		 *
		 * Note:
		 * 	Also, handle the case where we received a NULL only,
		 *	which is a Spool Stream of zero bytes.
		 */
		callAPI(CLOSE_FILE_AND_ABORT_QUEUE_JOB_2, qmsHandle->connID,
									 qmsHandle->queueID,
									 qmsJobRequest.jobNumber,
									 spoolHandle);
		logit(LOG_WARNING, "can't spool job to %s", Rhost);
		status = MSEELOG;
		goto detach;
	}
	else status = MOK;

	/*
	 * Request the spool file be serviced by the PSERVER
	 */
	if (callAPI(CLOSE_FILE_AND_START_QUEUE_JOB_2, qmsHandle->connID, 
									qmsHandle->queueID,
									qmsJobRequest.jobNumber, 
									spoolHandle) != SUCCESS) {
		logit(LOG_WARNING, "NWCreateQueueFile failed");
		status = MSEELOG;
		goto detach;
	}

	logit(LOG_DEBUG, "QMS job request: %d",
		qmsJobRequest.jobNumber & 0xffff);

	sprintf(JobRequest, "%d", qmsJobRequest.jobNumber & 0xffff);

	/* temporarily replace rem_reqid while we update disk copy */
	save_rem_reqid = sp->rem_reqid;
	sp->rem_reqid = JobRequest;

	(void) seteuid(saved_uid);
	if (putsecure(rqfile, sp) == -1)
		logit(LOG_WARNING, "can't write secure file: %s", rqfile);

	sp->rem_reqid = save_rem_reqid;

detach:
	(void) seteuid(uid);
	NWcpsDetachQMS(qmsHandle);
	logit(LOG_DEBUG, "%s detached from queue \"%s\" on \"%s\"",
		Name, PrintQueue, Rhost);
out:
	(void) seteuid(saved_uid);
	freerequest(rp);
	freeprinter(pr);
	if (PrintQueue)
		free(PrintQueue);
	if (options)
		freelist(options);
	if (status == REPRINT)
		cf = (char *)NULL;
	else
		cf = r_print_request(status, (sp ? sp->req_id : ""), 0,
								JobRequest);
	freesecure(sp);
	return(cf);
}

static char *
#if defined (__STDC__)
r_print_request(int status, char *reqid, int chkbits, char *remote_reqid)
#else
r_print_request(status, reqid, chkbits, remote_reqid)
int	 status;
char	*reqid;
int	 chkbits;
char	*remote_reqid;
#endif
{
	logit(LOG_DEBUG, "R_PRINT_REQUEST(%d, \"%s\")", status, reqid);
	if (putmessage(Msg, R_PRINT_REQUEST, status, reqid, chkbits,
							remote_reqid) < 0) {
		logit(LOG_DEBUG, "R_PRINT_REQUEST returns -1");
		return(NULL);
	}
	else
		return(Msg);
}

/*---------------------------------------------------------------------------
* Function: IsPSPrinter
*
* Description: This function determines if a printer is a postscript printer
*
* Returns: 0 if printer is not a PS printer, or we can't determine what kind
*               of printer
*          1 if printer is a PS printer
*---------------------------------------------------------------------------*/
int IsPSPrinter( char *printerName )
{
	char *printerPath = { "/etc/lp/printers/%s/configuration" };
	char *fileBuffer = NULL;
	int  retCode = 0; /* return failure unless we determine it's PS */    
	struct stat Buf;
	char buf[100];
	FILE *fp = NULL;


	fileBuffer = (char *)malloc(strlen(printerName) + strlen(printerPath));
	if ( fileBuffer != NULL )
	{
		sprintf( fileBuffer, printerPath, printerName );
		if ( stat( fileBuffer, &Buf ) == 0 )
		{
			fp = fopen( fileBuffer, "r" );
			if ( fp != NULL )
			{
			    while ( fgets( buf, sizeof(buf) - 1, fp ) != NULL)
			    {
				if (strstr( buf, "Printer type:" ) != NULL)
				{
				    if ( strstr( buf, "PS" ) != NULL )
					retCode = 1;   
				    break;
				}
			    }	
			}
		}
	}
	if ( fp )
		fclose( fp );
	if ( fileBuffer )
		free( fileBuffer );
	return( retCode );
}




