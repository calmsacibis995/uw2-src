/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:npt.h	1.1"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Printer_Setup/npt.h,v 1.1 1994/03/21 20:15:37 doug Exp $"

/*
 * Copyright 1989, 1991 Unpublished Work of Novell, Inc. All Rights Reserved.
 *
 * THIS WORK IS AN UNPUBLISHED WORK AND CONTAINS CONFIDENTIAL,
 * PROPRIETARY AND TRADE SECRET INFORMATION OF NOVELL, INC. ACCESS
 * TO THIS WORK IS RESTRICTED TO (I) NOVELL EMPLOYEES WHO HAVE A
 * NEED TO KNOW TO PERFORM TASKS WITHIN THE SCOPE OF THEIR
 * ASSIGNMENTS AND (II) ENTITIES OTHER THAN NOVELL WHO HAVE
 * ENTERED INTO APPROPRIATE AGREEMENTS.
 * NO PART OF THIS WORK MAY BE USED, PRACTICED, PERFORMED,
 * COPIED, DISTRIBUTED, REVISED, MODIFIED, TRANSLATED, ABRIDGED,
 * CONDENSED, EXPANDED, COLLECTED, COMPILED, LINKED, RECAST,
 * TRANSFORMED OR ADAPTED WITHOUT THE PRIOR WRITTEN CONSENT
 * OF NOVELL.  ANY USE OR EXPLOITATION OF THIS WORK WITHOUT
 * AUTHORIZATION COULD SUBJECT THE PERPETRATOR TO CRIMINAL AND
 * CIVIL LIABILITY.
 *
 *    lib/libnpt/npt.h 1.5 (Novell) 7/30/91
 */

/********************************************************************
 * 			D E F I N E S 
 *******************************************************************/
#define NWMAX_JOB_FILE_NAME_LENGTH	14
#define NWMAX_JOB_DESCRIPTION_LENGTH	50
#define NWMAX_QUEUE_JOB_TIME_SIZE	6
#define NWMAX_FORM_NAME_LENGTH		16
#define NWMAX_OBJECT_NAME_LENGTH	48
#define NWMAX_PROPERTY_NAME_LENGTH	16
#define NWMAX_SERVER_NAME_LENGTH	48
#define NWMAX_FILE_NAME_LENGTH		14
#define NWMAX_PROPERTY_VALUE_LENGTH	128
#define NWMAX_QUEUE_NAME_LENGTH		48
#define NWMAX_BANNER_NAME_FIELD_LENGTH  13
#define NWMAX_BANNER_FILE_FIELD_LENGTH	13
#define NWMAX_HEADER_FILE_NAME_LENGTH	14
#define NWMAX_JOB_DIR_PATH_LENGTH	80



/********************************************************************
 *
 * Program Name:  Novell Print Transport (NPT) library include file
 *
 * Filename:	  NPT.H
 *
 * Date Created:  October 27, 1988
 *
 * Date Modified: December 3, 1990
 *
 * Version:		  1.01
 *
 * Programmers:	  Lloyd Honomichl, Dale Bethers, Chuck Liu, Joe Ivie
 *
 * COPYRIGHT (c) 1988 by Novell, Inc.  All Rights Reserved.
 *
 ********************************************************************/

typedef struct print_job {
	uint8		clientStation;
	uint8		clientTaskNumber;
	uint32		clientIDNumber;
	uint32		targetServerIDNumber;
	uint8		targetExecutionTime[NWMAX_QUEUE_JOB_TIME_SIZE];
	uint8		entryTime[NWMAX_QUEUE_JOB_TIME_SIZE];
	uint16		jobNumber;
	uint16		formNumber;
	uint8		jobPosition;
	uint8		jobControlFlags;
	uint8		fileName[NWMAX_HEADER_FILE_NAME_LENGTH];
	uint8		fileHandle[NWMAX_QUEUE_JOB_TIME_SIZE];
	uint8		serverStation;
	uint8		serverTaskNumber;
	uint32		serverIDNumber;
	uint8		jobDescription[NWMAX_JOB_DESCRIPTION_LENGTH];
	uint8		versionNumber;
	uint8		tabSize;
	uint16		numberOfCopies;
	uint16		printControlFlags;
	uint16		maxLinesPerPage;
	uint16		maxCharsPerLine;
	char		formName[NWMAX_FORM_NAME_LENGTH];
/*	uint8		reserve[6];	*/
	char    	bannerNameField[NWMAX_BANNER_NAME_FIELD_LENGTH];
	char		bannerFileField[NWMAX_BANNER_FILE_FIELD_LENGTH];
	char		headerFileName[NWMAX_HEADER_FILE_NAME_LENGTH];
	char		directoryPath[NWMAX_JOB_DIR_PATH_LENGTH];
} PRINT_JOB;

/*
	Printer configuration file structure (lo-high)
*/
typedef	struct pconfig {
	uint8 name[NWMAX_OBJECT_NAME_LENGTH];
								/* Name of printer					*/
	uint16 printerSubtype,		/* Subtype of printer				*/
		 useInterrupts,			/* Use interrupts or polling?		*/
		 irqNumber,				/* IRQ number for printer interrupt	*/
		 serviceMode,			/* Queue service mode				*/
		 bufferSize,			/* Buffer size in K					*/
		 baudRate,				/* Baud rate						*/
		 dataBits,				/* Data bits						*/
		 stopBits,				/* Stop bits						*/
		 parity,				/* Parity type						*/
		 useXonXoff,			/* Use X-On/X-Off?					*/
		 currentForm;			/* Currently mounted form			*/
} PCONFIG;

/*
	Print Server information structure returned by
	PSGetPrintServerInfo (uint8)
*/
typedef struct psinfo {
	uint8	status,				/* Print server status				*/
			numPrinters,		/* Number of attached printers		*/
			numModes,			/* Number of queue service modes	*/
			majorVersion,		/* Print server, major version		*/
			minorVersion,		/* Print server, minor version		*/
			revision,			/* Print server, revision			*/
			serialNumber[4],	/* Serial number in BCD				*/
			serverType,			/* Print Server Type				*/
			futureUse[9];		/* Reserved for future use			*/
} PS_INFO;

/*
	Possible values of server status (uint8)
*/
#define RUNNING			0x00	/* Running							*/
#define	GOING_DOWN		0x01	/* Ready to quit when jobs finish	*/
#define DOWN			0x02	/* Ready to quit					*/
#define INITIALIZING	0x03	/* Initialization in progress		*/

/*
	Possible types of print servers (uint8)
*/
#define PS_TYPE_UNKNOWN 	0x00 /* Unknown print server type(pre 1.1)*/
#define PS_TYPE_EXE			0x01 /* Dedicate print server for DOS	*/
#define PS_TYPE_NLM			0x02 /* NetWare Loadable Module			*/
#define PS_TYPE_SERVER_VAP	0x03 /* Value added process, file server*/
#define PS_TYPE_BRIDGE_VAP	0x04 /* Value added process, Bridge		*/
#define PS_TYPE_PORT		0x05 /* Portable Netware Server			*/

/*
	Well known sockets (uint16 high-lo)
*/
#define PS_CLIENT_SOCKET	0x8060	/* Print Server client socket	*/
#define PS_REMOTE_SOCKET	0x811E	/* Print Server remote socket	*/

/*
	Values of printer status (uint8)
*/
#define PRINTER_RUNNING		0x00 /* Printer is running				*/
#define	PRINTER_OFFLINE		0x01 /* Printer is offline				*/
#define PRINTER_PAPER_OUT	0x02 /* Printer is out of paper			*/

/*
	Flags used for printControlFlags (uint16 lo-high)
*/
#define SUPPRESS_FF		0x0008
#define NOTIFY_USER		0x0010
#define TEXT_MODE		0x0040
#define	PRINT_BANNER	0x0080

/********************************************************************/

/*
	Queue service modes (uint16 lo-high)
*/
#define	QUEUE_ONLY		 	0x0000
#define	QUEUE_BEFORE_FORM	0x0001
#define	FORM_ONLY		 	0x0002
#define	FORM_BEFORE_QUEUE	0x0003

typedef	struct					/* Remote printer info structure	*/
{
	uint16	printerType,		/* Type of remote printer			*/
			useInterrupts,		/* Should we use interrupts?		*/
			irqNumber,			/* IRQ number for printer			*/
			numBlocks,			/* Number of blocks in buffer		*/
			useXonXoff,			/* Use Xon/Xoff?					*/
			baudRate,			/* Baud rate						*/
			dataBits,			/* Number of data bits				*/
			stopBits,			/* Number of stop bits				*/
			parity,				/* Parity type						*/
			socket;				/* Socket number for remote printers*/
} REMOTE_INFO;

/*
	Status packet from a remote printer
*/
typedef struct
{
	uint8	printerNumber,		/* Printer number request is from	*/
			needBlocks,			/* Number of blocks needed to print	*/
			finishedBlocks,		/* Number of blocks that are printed*/
			status,				/* Printer online/offline, etc...	*/
			inSideband;			/* Remote printer printing sideband?*/
} REMOTE_STATUS;

/*
	Values for serial port control stored in config files
	(uint16 lo-high)
*/
#define BAUD_RATE_0300	0x0000	/* Baud rates				*/
#define BAUD_RATE_0600	0x0001
#define BAUD_RATE_1200	0x0002
#define BAUD_RATE_2400	0x0003
#define BAUD_RATE_4800	0x0004
#define BAUD_RATE_9600	0x0005

#define STOP_BITS_1		0x0000	/* Stop bits				*/
#define STOP_BITS_1_5	0x0001
#define STOP_BITS_2		0x0002

#define PARITY_NONE		0x0000	/* Parity type				*/
#define PARITY_EVEN		0x0001
#define PARITY_ODD		0x0002

/*
	Data stream types of data sent to remote printers (uint8)
*/
#define	DST_DATA		0x00	/* Packet contains data		*/
#define DST_FLUSH		0x01	/* Flush print buffers packet*/
#define	DST_PAUSE		0x02	/* Pause packet				*/
#define	DST_START		0x03	/* End pause packet			*/
#define DST_SIDEBAND	0x04	/* Print sideband packet	*/
#define DST_NEW_JOB		0x05	/* Start new job packet		*/
#define DST_RELEASE		0x06	/* Release printer packet	*/
#define DST_RECLAIM		0x07	/* Reclaim printer packet	*/

/*
	Printer types (uint16 lo-high)
*/
#define P_PAR_1			0x0000	/* Parallel port 1			*/
#define P_PAR_2			0x0001	/* Parallel port 2	 		*/
#define P_PAR_3			0x0002	/* Parallel port 3	 		*/
#define P_SER_1			0x0003	/* Serial port 1	 		*/
#define P_SER_2			0x0004	/* Serial port 2	 		*/
#define P_SER_3			0x0005	/* Serial port 3	 		*/
#define P_SER_4			0x0006	/* Serial port 4	 		*/
#define P_REM_PAR_1		0x0007	/* Rprinter using parallel 1*/
#define P_REM_PAR_2		0x0008	/* Rprinter using parallel 2*/
#define P_REM_PAR_3		0x0009	/* Rprinter using parallel 3*/
#define P_REM_SER_1		0x000a	/* Rprinter using serial 1	*/
#define P_REM_SER_2		0x000b	/* Rprinter using serial 2	*/
#define P_REM_SER_3		0x000c	/* Rprinter using serial 3	*/
#define P_REM_SER_4		0x000d	/* Rprinter using serial 4	*/
#define P_REM_OTHER		0x000e	/* Rprinter of undefined type*/
#define P_ELSEWHERE		0x000f	/* Printer defined elsewhere*/

/*
	For operator notification purposes, notify job owner (uint16)
*/
#define JOB_OWNER		0xFFFF

/*
	Job outcomes (uint8)
*/
#define	PLACE_ON_HOLD		0x00 /* Pause job						*/
#define	RETURN_TO_QUEUE		0x01 /* Return job to queue				*/
#define	THROW_AWAY			0x02 /* Throw job away					*/

/*
	Remote printer modes (uint8)
*/
#define REMOTE_SHARED		0x00 /* Rprinter is shared with network	*/
#define REMOTE_PRIVATE		0x01 /* Rprinter is private				*/

/********************************************************************/

/*
	Print server request codes (uint8)
*/
#define CMD_LOGIN_TO_PRINT_SERVER	0x01
#define CMD_GET_PRINT_SERVER_INFO	0x02
#define CMD_DOWN					0x03
#define CMD_CANCEL_DOWN				0x04
#define CMD_GET_PRINTER_STATUS		0x05
#define CMD_STOP_PRINTER			0x06
#define CMD_START_PRINTER			0x07
#define CMD_MOUNT_FORM				0x08
#define CMD_REWIND_PRINT_JOB		0x09
#define CMD_EJECT_PAGE				0x0A
#define CMD_MARK_PAGE				0x0B
#define CMD_CHANGE_SERVICE_MODE		0x0C
#define CMD_GET_JOB_STATUS			0x0D
#define CMD_ABORT_JOB				0x0E
#define CMD_SCAN_QUEUE_LIST			0x0F
#define CMD_CHANGE_QUEUE_PRIORITY	0x10
#define CMD_ADD_QUEUE				0x11
#define CMD_DELETE_QUEUE			0x12
#define CMD_GET_PRINTERS_FOR_QUEUE	0x13
#define CMD_SCAN_NOTIFY_LIST		0x14
#define CMD_CHANGE_NOTIFY			0x15
#define CMD_ADD_NOTIFY				0x16
#define CMD_DELETE_NOTIFY			0x17
#define CMD_ATTACH_TO_FILE_SERVER  	0x18
#define CMD_DETACH_FROM_FILE_SERVER	0x19
#define CMD_GET_ATTACHED_SERVERS	0x1A
#define CMD_GET_REMOTE				0x80
#define CMD_CONNECT_REMOTE			0x81
#define CMD_SET_REMOTE_MODE			0x82

/********************************************************************/

/*
	Printer status codes (uint8)
*/
#define PSTAT_WAITING_FOR_JOB		0x00
#define PSTAT_WAITING_FOR_FORM		0x01
#define PSTAT_PRINTING_JOB			0x02
#define PSTAT_PAUSED				0x03
#define PSTAT_STOPPED				0x04
#define PSTAT_MARK_EJECT			0x05
#define	PSTAT_READY_TO_GO_DOWN		0x06
#define	PSTAT_NOT_CONNECTED			0x07
#define PSTAT_PRIVATE				0x08

/********************************************************************/

/*
	Print server communication errors (uint16 local-only)
*/
#define PSC_NO_AVAILABLE_SPX_CONNECTIONS	0x0040
#define PSC_SPX_NOT_INITIALIZED				0x0041
#define PSC_NO_SUCH_PRINT_SERVER			0x0042
#define PSC_UNABLE_TO_GET_SERVER_ADDRESS	0x0043
#define PSC_UNABLE_TO_CONNECT_TO_SERVER		0x0044
#define PSC_NO_AVAILABLE_IPX_SOCKETS		0x0045
#define PSC_ALREADY_ATTACH_TO_A_PRINTSERVER 0x0046
#define PSC_IPX_NOT_INITIALIZED				0x0047
#define PSC_CONNECTION_TERMINATED			0x00ED

/*
	Print server error codes (uint16 lo-high)
*/
#define PSE_SUCCESSFUL						0x0000
#define PSE_TOO_MANY_FILE_SERVERS			0x0101
#define PSE_UNKNOWN_FILE_SERVER				0x0102
#define PSE_BINDERY_LOCKED					0x0103
#define PSE_SERVER_MAXED_OUT				0x0104
#define PSE_NO_RESPONSE						0x0105
#define PSE_ALREADY_ATTACHED				0x0106
#define PSE_CANT_ATTACH						0x0107
#define PSE_NO_ACCOUNT_BALANCE				0x0108
#define PSE_NO_CREDIT_LEFT					0x0109
#define PSE_INTRUDER_DETECTION_LOCK			0x010A
#define PSE_TOO_MANY_CONNECTIONS			0x010B
#define PSE_ACCOUNT_DISABLED				0x010C
#define PSE_UNAUTHORIZED_TIME				0x010D
#define PSE_UNAUTHORIZED_STATION			0x010E
#define PSE_NO_MORE_GRACE					0x010F
#define PSE_LOGIN_DISABLED					0x0110
#define PSE_ILLEGAL_ACCT_NAME				0x0111
#define PSE_PASSWORD_HAS_EXPIRED			0x0112
#define PSE_ACCESS_DENIED					0x0113
#define PSE_CANT_LOGIN						0x0114
#define	PSE_PRINTER_ALREADY_INSTALLED		0x0115
#define PSE_CANT_OPEN_CONFIG_FILE			0x0116
#define PSE_CANT_READ_CONFIG_FILE			0x0117
#define PSE_UNKNOWN_PRINTER_TYPE			0x0118
#define PSE_NO_SUCH_QUEUE					0x0200
#define PSE_NOT_AUTHORIZED_FOR_QUEUE		0x0201
#define PSE_QUEUE_HALTED					0x0202
#define PSE_UNABLE_TO_ATTACH_TO_QUEUE		0x0203
#define PSE_TOO_MANY_QUEUE_SERVERS			0x0204
#define PSE_INVALID_REQUEST					0x0300
#define PSE_NOT_ENOUGH_MEMORY				0x0301
#define	PSE_NO_SUCH_PRINTER					0x0302
#define PSE_INVALID_PARAMETER				0x0303
#define PSE_PRINTER_BUSY					0x0304
#define PSE_CANT_DETACH_PRIMARY_SERVER		0x0305
#define PSE_GOING_DOWN						0x0306
#define PSE_NOT_CONNECTED					0x0307
#define PSE_ALREADY_IN_USE					0x0308
#define PSE_NO_JOB_ACTIVE					0x0309
#define PSE_NOT_ATTACHED_TO_SERVER			0x030A
#define PSE_ALREADY_IN_LIST					0x030B
#define PSE_DOWN							0x030C
#define PSE_NOT_IN_LIST						0x030D
#define PSE_NO_RIGHTS						0x030E
#define PSE_UNABLE_TO_VERIFY_IDENTITY		0x0400
#define	PSE_NOT_REMOTE_PRINTER				0x0401
#ifndef SUCCESSFUL
#define SUCCESSFUL							PSE_SUCCESSFUL
#endif

/*
	Client privilege levels for print server (uint8)
*/
#define PS_LIMITED		0x00	/* Limited access only				*/
#define PS_USER			0x01	/* User access						*/
#define PS_OPERATOR		0x02	/* Operator access					*/

/********************************************************************/

#ifdef PROTO
/* Job Control */
extern uint16 PSAbortPrintJob(
	uint16,						/* SPX Connection number			*/
	uint8,						/* Printer number					*/
	uint8);						/* Job outcome						*/

extern uint16 PSEjectForm(
	uint16,						/* SPX Connection number			*/
	uint8);						/* Printer number					*/

extern uint16 PSGetPrintJobStatus(
	uint16,						/* SPX Connection number			*/
	uint8,						/* Printer number					*/
	uint8 *,					/* File server name					*/	
	uint8 *,					/* Queue name						*/	
	uint16 *,					/* Job number						*/
	uint8 *,					/* Description of job				*/
	uint16 *,					/* Number of copies to be printed	*/
	uint32 *,					/* Size of print job				*/
	uint16 *,					/* Copies finished		 			*/
	uint32 *,					/* Bytes into current copy			*/
	uint16 *,					/* Form number for job				*/
	uint8 *);					/* Is job text?						*/

extern uint16 PSMarkTopOfForm(
	uint16,						/* SPX Connection number			*/
	uint8,						/* Printer number					*/
	uint8);						/* Character to mark form with		*/

extern uint16 PSRewindPrintJob(
	uint16,						/* SPX Connection number			*/
	uint8,						/* Printer number					*/
	uint8,						/* Rewind by page?					*/
	uint8,						/* Rewind relative to current position*/
	uint16,						/* Copy to rewind to (if absolute)	*/
	uint32);					/* Offset							*/

/* Notify Functions */
extern uint16 PSAddNotifyObject(
	uint16,						/* SPX Connection number			*/
	uint8,  					/* Printer number					*/
	uint8 *,					/* File server name					*/
	uint8 *,					/* Object name						*/
	uint16,						/* Object type						*/
	uint16,						/* First notify delay				*/
	uint16);					/* Notify interval					*/

extern uint16 PSChangeNotifyInterval(
	uint16,						/* SPX Connection number			*/
	uint8,  					/* Printer number					*/
	uint8 *,					/* File server name					*/
	uint8 *,					/* Object name						*/
	uint16,						/* Object type						*/
	uint16,						/* First notify delay				*/
	uint16);					/* Notify interval					*/

extern uint16 PSDeleteNotifyObject(
	uint16,						/* SPX Connection number			*/
	uint8,						/* Printer number					*/
	uint8 *,					/* File server name					*/
	uint8 *,					/* Object name						*/
	uint16);					/* Object type						*/

extern uint16 PSGetNotifyObject(
	uint16,						/* SPX Connection number			*/
	uint8,  					/* Printer number					*/
	uint16 *,					/* Sequence number.  0 first time	*/
	uint8 *,					/* File server name					*/
	uint8 *,					/* Object name						*/
	uint16 *,					/* Object type						*/
	uint16 *,					/* First notify delay				*/
	uint16 *);					/* Notify interval					*/

/* Printer Control */
extern uint16 PSChangeServiceMode(
	uint16,						/* SPX Connection number			*/
	uint8,						/* Printer number					*/
	uint8);						/* New service mode					*/

extern uint16 PSGetPrinterStatus(
	uint16,						/* SPX Connection number			*/
	uint8,						/* Printer number					*/
	uint8 *,					/* Printer status					*/	
	uint8 *,					/* On line/Off line/Out of paper	*/	
	uint8 *,					/* Printer has an active job		*/	
	uint8 *,					/* Queue service mode				*/
	uint16 *,					/* Mounted form	number				*/
	uint8 *,					/* Mounted form name				*/
	uint8 *);					/* Printer name						*/

extern uint16 PSSetMountedForm(
	uint16,						/* SPX Connection number			*/
	uint8,						/* Printer number					*/
	uint8);						/* Form number						*/

extern uint16 PSStartPrinter(
	uint16,						/* SPX Connection number			*/
	uint8);						/* Printer number					*/

extern uint16 PSStopPrinter(
	uint16,						/* SPX Connection number			*/
	uint8,						/* Printer number					*/
	uint8);						/* Job outcome						*/

/* Queue Control */
extern uint16 PSAddQueueToPrinter(
	uint16,						/* SPX Connection number			*/
	uint8,						/* Printer number					*/
	uint8 *,					/* File server name					*/
	uint8 *,					/* Queue name						*/
	uint8);						/* Priority							*/

extern uint16 PSChangeQueuePriority(
	uint16,						/* SPX Connection number			*/
	uint8,						/* Printer number					*/
	uint8 *,					/* File server name					*/
	uint8 *,					/* Queue name						*/
	uint8);						/* New priority						*/

extern uint16 PSDeleteQueueFromPrinter(
	uint16,						/* SPX Connection number			*/
	uint8,						/* Printer number					*/
	uint8 *,					/* File server name					*/
	uint8 *,					/* Queue name						*/
	uint8,						/* Detach immediately?				*/
	uint8);						/* Outcome of current job			*/

extern uint16 PSGetPrintersServicingQueue(
	uint16,						/* SPX Connection number			*/
	uint8 *,					/* File server name					*/
	uint8 *,					/* Queue name						*/
	uint8,						/* Max number of printers to return	*/
	uint8 *,					/* Actual number of printers returned*/
	uint8 *);					/* Array where printer numbs returned*/
	
extern uint16 PSGetQueuesServiced(
	uint16,						/* SPX Connection number			*/
	uint8,						/* Printer number					*/
	uint16 *,					/* Sequence number.  0 first time	*/
	uint8 *,					/* File server name					*/
	uint8 *,					/* Queue name						*/
	uint8 *);					/* Priority							*/

/* Remote Printer Control */
extern uint16 PSGetNextRemotePrinter(
	uint16,						/* SPX Connection number			*/
	uint8 *,					/* Printer number					*/
	int *,						/* Printer type						*/
	uint8 *);					/* Name of printer					*/

extern uint16 PSRequestRemotePrinter(
	uint16 ,					/* SPX Connection number			*/
	uint8,						/* Printer number					*/
    REMOTE_INFO *);				/* Rprinter information structure	*/

extern uint16 PSSetRemoteMode(
	uint16,						/* SPX Connection number			*/
	uint8,						/* Printer number					*/
	uint8);						/* New mode							*/

/* Server Control Calls */
extern uint16 PSAttachPrintServerToFileServer(
	uint16,						/* SPX Connection number			*/
	uint8 *,					/* File server name					*/
	uint8 *);					/* Password							*/

extern uint16 PSAttachToPrintServer(
	uint8 *,					/* Print server name				*/
	uint16 *);					/* SPX Connection number			*/

extern uint16 PSCancelDownRequest(
	uint16);					/* SPX Connection number			*/

extern uint16 PSDetachFromPrintServer(
	uint16);					/* SPX Connection number			*/

extern uint16 PSDetachPrintServerFromFileServer(
	uint16,						/* SPX Connection number 			*/
	uint8 *,					/* File server name					*/
	uint8,						/* Detach immediately?				*/
	uint8);						/* Outcome of current jobs			*/

extern uint16 PSDownPrintServer(
	uint16,						/* SPX Connection number			*/
	uint8,						/* Go down immediately?				*/
	uint8);						/* Outcome of current jobs			*/

extern uint16 PSGetAttachedServers(
	uint16,						/* SPX Connection number			*/
	uint8 *,					/* Sequence number. 0 first time	*/
	uint8 *);					/* File server name 				*/

extern uint16 PSGetPrintServerInfo(
	uint16,						/* SPX Connection number			*/
	PS_INFO *,					/* Server info structure			*/
	uint16);					/* Size of information requested	*/

extern uint16 PSLoginToPrintServer(
	uint16,						/* SPX Connection number			*/
	uint8 *);					/* Client's access level			*/

/* Extended Remote Printer calls (undefined) */

/* Common Routines */
extern uint16 PSGetPreferredConnectionID();
extern void   PSSetPreferredConnectionID(
	uint16);					/* Connection ID					*/

#else /* PROTO */

/* Job Control */
extern uint16 PSAbortPrintJob();
extern uint16 PSEjectForm();
extern uint16 PSGetPrintJobStatus();
extern uint16 PSMarkTopOfForm();
extern uint16 PSRewindPrintJob();

/* Notify Functions */
extern uint16 PSAddNotifyObject();
extern uint16 PSChangeNotifyInterval();
extern uint16 PSDeleteNotifyObject();
extern uint16 PSGetNotifyObject();

/* Printer Control */
extern uint16 PSChangeServiceMode();
extern uint16 PSGetPrinterStatus();
extern uint16 PSSetMountedForm();
extern uint16 PSStartPrinter();
extern uint16 PSStopPrinter();

/* Queue Control */
extern uint16 PSAddQueueToPrinter();
extern uint16 PSChangeQueuePriority();
extern uint16 PSDeleteQueueFromPrinter();
extern uint16 PSGetPrintersServicingQueue();
extern uint16 PSGetQueuesServiced();

/* Remote Printer Control */
extern uint16 PSGetNextRemotePrinter();
extern uint16 PSRequestRemotePrinter();
extern uint16 PSSetRemoteMode();

/* Server Control Calls */
extern uint16 PSAttachPrintServerToFileServer();
extern uint16 PSAttachToPrintServer();
extern uint16 PSCancelDownRequest();
extern uint16 PSDetachFromPrintServer();
extern uint16 PSDetachPrintServerFromFileServer();
extern uint16 PSDownPrintServer();
extern uint16 PSGetAttachedServers();
extern uint16 PSGetPrintServerInfo();
extern uint16 PSLoginToPrintServer();

/* Extended Remote Printer calls (undefined) */

/* Common Routines */
extern uint16 PSGetPreferredConnectionID();
extern void   PSSetPreferredConnectionID();

#endif /* PROTO */

/********************************************************************/
/********************************************************************/

