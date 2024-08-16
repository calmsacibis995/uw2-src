/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_SCSI_H	/* wrapper symbol for kernel use */
#define _IO_SCSI_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:io/scsi.h	1.6"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Supported SCSI commands and data structures.
 */

/* 
 * Class 00 (common) SCSI commands.
 */
#define SCSI_TEST		0x00	/* Test unit ready */
#define SCSI_REZERO		0x01	/* Rezero unit */
#define SCSI_RSENSE		0x03	/* Request sense */
#define SCSI_FORMAT		0x04	/* Format unit */
#define SCSI_REASS		0x07	/* Reassign blocks */
#define SCSI_READ		0x08	/* Read */
#define SCSI_WRITE		0x0a	/* Write */
#define SCSI_SEEK		0x0b	/* Seek */
#define SCSI_TRAN		0x0f	/* Translate logical to phys */
#define SCSI_INQUIRY		0x12	/* Do inquiry */
#define SCSI_WRITEB		0x13	/* Write buffer */
#define SCSI_READB		0x14	/* Read buffer */
#define SCSI_MODES		0x15	/* Mode select */
#define SCSI_RESRV		0x16	/* Reserve unit */
#define SCSI_RELSE		0x17	/* Release unit */
#define SCSI_MSENSE		0x1a	/* Mode sense */
#define SCSI_STARTOP		0x1b	/* Start/stop unit */
#define 	SCSI_START_UNIT  	0x01
#define 	SCSI_STOP_UNIT  	0x00
#define SCSI_RDIAG		0x1c	/* Receive diagnostic */
#define SCSI_SDIAG		0x1d	/* Send diagnostic */
#define		SCSI_SDIAG_UNITOFFLINE	0x01
#define		SCSI_SDIAG_DEVOFFLINE	0x02
#define		SCSI_SDIAG_SELFTEST	0x04
#define 	SCSI_SDIAG_REINIT	0x60
#define 	SCSI_SDIAG_DUMP_HW	0x61
#define 	SCSI_SDIAG_DUMP_RAM	0x62
#define 	SCSI_SDIAG_PATCH_HW	0x63
#define 	SCSI_SDIAG_PATCH_RAM	0x64
#define 	SCSI_SDIAG_SET_RD_ERR   0x65
#define 		SCSI_SDIAG_RD_ERR_DEF	0x00
#define 		SCSI_SDIAG_RD_ERR_RPT	0x01
#define 		SCSI_SDIAG_RD_ERR_NOC	0x02
#define SCSI_PA_REMOVAL		0x1E    /* Prevent or allow media removal */
#define 	SCSI_REMOVAL_ALLOW	0x00	/* Allow media removal */
#define 	SCSI_REMOVAL_PREVENT	0x01	/* Prevent media removal */

/* 
 * Class 01 SCSI commands.
 */
#define SCSI_READC		0x25	/* Read capacity */
#define 	SCSI_FULL_CAP		0x00
#define 	SCSI_PART_CAP		0x01
#define SCSI_READ_EXTENDED	0x28
#define SCSI_WRITE_EXTENDED	0x2A
#define SCSI_READ_DEFECTS	0x37

/* 
 * Unsupported class 01 SCSI commands.
 */
#define SCSI_SET_THRESHOLD  	0x10
#define SCSI_RD_USAGE_CTRS  	0x11
#define SCSI_SEEK_EXTENDED  	0x2B
#define SCSI_WRITE_AND_VERIFY	0x2E
#define SCSI_VERIFY_EXTENDED	0x2F
#define SCSI_SEARCH_DATA_LOW	0x30
#define SCSI_SEARCH_DATA_EQUAL	0x31
#define SCSI_SEARCH_DATA_HIGH	0x32
#define SCSI_SET_LIMITS		0x33

/*
 * Sequential access device SCSI commands (i.e. tape).  
 */
#define SCSI_REWIND		0x01	/* Rewind command */
#define SCSI_RETENSION  	0x02	/* Retension a tape */
#define SCSI_READ_BLK_LIMITS    0x05    /* Read limits for fixed block sizes */
#define SCSI_READ_REVERSE	0x0f	/* Read tape while backing up */
#define SCSI_WFM		0x10	/* Write a file mark */
#define SCSI_SPACE		0x11	/* Space (default blocks) fwd */
#define 	SCSI_SPACE_BLOCKS       0x00
#define 	SCSI_SPACE_FILEMARKS    0x01
#define 	SCSI_SPACE_SFILEMARKS   0x02
#define 	SCSI_SPACE_ENDOFDATA    0x03
#define 	SCSI_SPACE_CODE		0x03
#define SCSI_VERIFY		0x13	/* Verify data on the media */
#define 	SCSI_VERIFY_BYTES	0x00	/* Verify against data */
#define 	SCSI_VERIFY_MEDIA	0x02	/* Verify media can be read */
#define SCSI_ERASE		0x19	/* Erase a tape */
#define 	SCSI_ERASE_SHORT	0x00  /* Create a blank gap */
#define 	SCSI_ERASE_LONG 	0x01  /* Completely erase media */
#define SCSI_LOAD_UNLOAD 	0x1b	/* Position media for load/unload */
#define 	SCSI_LOAD_MEDIA 	0x01
#define 	SCSI_RETEN_MEDIA 	0x02
#define SCSI_VARIABLE_BLOCKS    0x00	/* Qualifier for read and write */
#define SCSI_FIXED_BLOCKS	0x01	/* Qualifier for read and write */
#define SCSI_IMMEDIATE  	0x01	/* Return prior to completion of cmd */

/*
 * Sizes of data transferred for some standard commands.
 */
#define SIZE_CAP		8	/* #bytes in Read Capacity input data */
#define SIZE_TRANS		8	/* #bytes in Translate input data */
#define SIZE_INQ		4	/* #bytes in Inquiry output data */
#define SIZE_INQ_XTND   	36	/* #bytes in extended Inquiry data */
#define SIZE_BDESC		12	/* #bytes in SCSI_MODES block descr. */
#define SIZE_MAXDATA		36	/* #bytes in largest data transfer */

#define SCSI_CMD6SZ		6	/* SCSI command length */
#define SCSI_CMD10SZ		10	/* SCSI command length */
#define SCSI_CMD12SZ		12	/* SCSI command length */
#define SCSI_MAXCMDSZ   	12	/* Maximum SCSI command length */

/*
 * The following data structure collects generic 
 * information about a SCSI command into one place, 
 * which can be passed to and filled in by SCSI library 
 * functions. 
 */
struct scsi_cmd {
	unchar dir;			/* Direction of data xfer, if any */
	unchar clen;			/* Amount of cmd being used */
	unchar cmd[SCSI_MAXCMDSZ];	/* The SCSI command to execute */
};

typedef struct scsi_cmd scsicmd_t;

/*
 *  Definitions for scsi_cmd.dir
 */
#define SDIR_NONE		0	/* No data transfered */
#define SDIR_HTOD		1	/* From host memory to device */
#define SDIR_DTOH		2	/* From device to host memory */

/*
 * Generic SCSI command termination values.
 */
#define SSTAT_OK        	0	/* Good termination */
#define SSTAT_NODEV     	1	/* Unrecognized device */
#define SSTAT_BUSERR    	2	/* SCSI Bus error or reset */
#define SSTAT_NOTARGET  	3	/* Target Adapter does not respond */
#define SSTAT_BUSYTARGET	4	/* Target adapter is busy */
#define SSTAT_BUSYLUN   	5	/* Logical unit is busy */
#define SSTAT_CCHECK    	6	/* Check condition occurred.  Current
					 * sense data in extended format */
#define SSTAT_DCHECK    	7	/* Check condition occurred.  Deferred
					 * sense data in extended format */
#define SSTAT_UCHECK    	8	/* Check condition occurred.  Sense
					 * data in unrecognized format */
#define SSTAT_RESERVED  	9	/* Reserved termination value */

/*
 * Length of data returned from SCSI commands.
 */
#define SCSI_L_TEST		0
#define SCSI_L_INQ		5
#define SCSI_L_READC		8
#define SCSI_L_REQSEN   	13
#define SCSI_L_MODE		20

/* 
 * Structures for SCSI format command.
 * The data for the format command is called the
 * defect list.  For a typical device it consists
 * of a parameter list header followed by zero or
 * more defect list entries.
 */

/* Basic command structure */
struct	scfmt_cmd {
	unchar	f_type; 		/* Command type */
	unchar	f_misc; 		/* 3 bit logical unit, FmtData flag, 
					 * CmpLst flag, defect list format */ 
	unchar	f_vendor; 		/* Vendor unique data */
	unchar	f_ileave[2];		/* Interleave */
	unchar	f_control;		/* Control byte */
};

/* scfmt_cmd.f_misc flags */
#define FMT_BBL_DATA		0x10	/* Bad block list exists */
#define FMT_CMPLT		0x08	/* Bad block list is complete */
#define FMT_USER_FMT		0x04	/* Use user-supplied format data */
#define FMT_DATA		0x02	/* Use user-supplied data pattern */
#define FMT_ALL 	(FMT_BBL_DATA | FMT_CMPLT | FMT_USER_FMT | FMT_DATA)

/* scfmt_cmd.f_vendor */
#define FMT_PAT 		0x6D	/* Worst winchester data pattern
					 * for diagnostic area. */

/* Defect list header; only one of these */
struct	scfmt_hdr {
	unchar	fh_full;		/* Full or cylinder flag */
	unchar	fh_spares;		/* Number of spares sectors/cylinder */
	unchar	fh_dlen[2];		/* Length of defect list blocks */
};

/* scfmt_hdr.f_full flags */
#define FMT_FULL		0x00	/* Format the complete drive */
#define FMT_CYL 		0x01	/* Format a single cylinder */

/* Defect list entries (zero or more of these) - physical sector format */
struct scfmt_dlist {			
	unchar	fd_cyls[3];		/* Cylinder of defect */
	unchar	fd_heads;		/* Head number */
	unchar	fd_bytes[4];		/* Bytes from index */
};

/* Here is the defect list definition for a typical disk */
#define FORMAT_BUF		1024	/* Max bytes for Format Data */
#define MAX_DEFECTS		(FORMAT_BUF / sizeof(struct scfmt_dlist))

struct scsi_format_disk {
	struct scfmt_hdr d_hdr; 	/* Defect list header */
	struct scfmt_dlist d_dlist[MAX_DEFECTS];  /* Defect list entries */
};

/*
 * Data returned from the SCSI Inquiry command.
 * Its format consists of a header followed by
 * vendor unique data, the size in bytes of which 
 * is described in the header.
 */

/* Inquiry data header */
struct scinq_hdr {
	unchar	ih_devtype;		/* Type SCSI device */
	unchar	ih_qualif;		/* Dev type qualifier */
	unchar	ih_version;		/* SCSI spec version */
	unchar	ih_reserved;		/* 0 for adaptec, 1 for CCS */
	unchar	ih_vlength;		/* Length of vendor unique data */
};

/* For scinq_hdr.ih_devtype */
#define INQ_DIRECT		0x00	/* Direct-access device */
#define INQ_SEQ 		0x01	/* Sequential access device */
#define INQ_PRINT		0x02	/* Printer device */
#define INQ_PROC		0x03	/* Processor device */
#define INQ_WRONCE		0x04	/* Write-once read multiple times */
#define INQ_READONLY		0x05	/* Read only medium device */
#define INQ_NOTFOUND		0x7F	/* Logical device not found */

/* For scinq_hdr.ih_qualif  */
#define INQ_REMOVABLE   	0x80	/* Has removable media */

/* For scinq_hdr.ih_reserved for SCSI disks */
#define INQ_RES_SCSI2		0x02	/* SCSI-2 command set */
#define INQ_RES_CCS		0x01	/* CCS */
#define INQ_RES_TARG		0x00	/* Adaptec */

/* Here is a parameter list description for a typical device */
#define INQ_VEND		8	/* Length of vendor name field */
#define INQ_PROD		16	/* Length of product i.d. field */
#define INQ_REV 		4	/* Length of revision field */

struct scinq_dev {
	struct scinq_hdr sc_hdr;	/* Inquiry data header */
	unchar	sc_pad[3];
	char	sc_vendor[INQ_VEND];	/* Name of the vendor */
	char	sc_product[INQ_PROD];  /* Product ID */
	unchar	sc_revision[INQ_REV];  /* Product fw rev level */
};

#define INQ_LEN_DEV		sizeof(struct scinq_dev)

/*
 * Structures for SCSI Mode Select and Mode Sense commands.
 * The data for both commands is a parameter list.  For a 
 * typical device it consists of a header followed by zero 
 * or more block descriptors, optionally followed by a vendor 
 * unique parameter list.
 */

/* Basic command structure */
struct scmode_cmd {
	unchar	m_cmd;			/* Command type */
	unchar	m_unit; 		/* Upper 3 bits are logical unit */
	unchar	m_pad1[2];		/* Reserved */
	unchar	m_plen; 		/* Parameter list length in bytes */
	unchar	m_cont; 		/* Control byte */
};

/* For scmode_cmd.m_unit; special flags for disks */
#define MODE_UN_SMP   	0x01		/* Save Mode Parameters */
#define MODE_UN_PF   	0x10		/* Page Format */

/* Parameter list header */
struct scmode_hdr {
	unchar	mh_sdlen;		/* Mode sense data length */
	unchar	mh_mtype;		/* Media type; for direct access only */
	unchar	mh_mode;		/* Mode select: Buffered mode and speed
					 * for sequential access devices.
					 * Mode sense: write protected media. */
	unchar	mh_dlen;		/* Descriptor list length */
};

/* For scmode_hdr.mh_mtype */
#define MODE_TYPE_DEF   	0x00	/* Default; use current medium type */
#define MODE_TYPE_FLEX1 	0x01	/* Flex disk, single-sided */
#define MODE_TYPE_FLEX2 	0x02	/* Flex disk, double-sided */

/* For Mode Select buffered mode portion of the scmode_hdr.mh_mode */
#define MSEL_BFM_SYNC   	0x00	/* Report write completions after
					 * its termination. */
#define MSEL_BFM_ASYNC  	0x10	/* Report write completions once
					 * its data has been copied. */

/* For Mode Select speed portion of the scmode_hdr.mh_mode */
#define MSEL_SPD_DEFAULT 	0x00	/* Use peripheral's default speed */
#define MSEL_SPD_LOWEST 	0x01	/* Use peripheral's lowest speed */

/* For scmode_hdr.mh_mode on Mode Sense */
#define MSENSE_WP 	 	0x80	/* Media is protected against writes */

/* Block descriptor; there may be zero or more of these */
struct scmode_blkd {
	unchar	mb_density;		/* Density code */
	unchar	mb_numblks[3];   	/* Number of blocks */
	unchar	mb_pad1;		/* Reserved */
	unchar	mb_bsize[3];		/* Block size */
};

/* Tape density codes for scmode_blkd.mb_density */
#define MODE_DEN_DEFAULT	0x00	/* Use peripheral's default density */
#define MODE_DEN_X322   	0x01	/* 800 CPI, NRZI */
#define MODE_DEN_X339   	0x02	/* 1600 CPI, PE */
#define MODE_DEN_X354   	0x03	/* 6250 CPI, GCR */
#define MODE_DEN_QIC11_4  	0x04	/* 1/4 inch, QIC-11, 4-track format */
#define MODE_DEN_QIC24  	0x05	/* 1/4 inch, QIC-24 format */
#define MODE_DEN_X385   	0x06	/* 3200 CPI, PE */
#define MODE_DEN_QIC120_ECC	0x0D	/* 1/4 inch, QIC-120 w/ECC */
#define MODE_DEN_QIC150_ECC	0x0E	/* 1/4 inch, QIC-150 w/ECC */
#define MODE_DEN_QIC120		0x0F	/* 1/4 inch, QIC-120 format */
#define MODE_DEN_QIC150		0x10	/* 1/4 inch, QIC-150 format */
#define MODE_DEN_QIC525		0x11	/* 1/4 inch, QIC-525 format */
#define MODE_DEN_NOOP		0x7F	/* No change from last density */
#define MODE_DEN_QIC11_9  	0x84	/* 1/4 inch, QIC-11, 9-track format */
#define MODE_DEN_HPCOMP   	0xC3	/* H.P. specific compressed 6250 */

/* Here is the parameter list description for typical tape */
struct scsi_mode_tape {
	struct scmode_hdr t_hdr;	/* Parameter list header */
	struct scmode_blkd t_bd;	/* Block descriptor list */
};

/* Here is the mode select parameter list description for typical disks */
struct scsi_msel_disk {
	struct scmode_hdr d_hdr;	/* Parameter list header */
	struct scmode_blkd d_bd;	/* Block descriptor list */

		/* Device specific parameters list */
	unchar	d_fcode;		/* Format code */
	unchar	d_cyls[2];		/* Cylinder count */
	unchar	d_heads;		/* Data head count */
	unchar	d_rwcc[2];		/* Reduced write current cylinder */
	unchar	d_wpc[2];		/* Write precompensation cylinder */
	char	d_lzone;		/* Landing zone position */
	unchar	d_srate;		/* Step pulse output rate code */
};

/* For scsi_msel_disk.d_fcode */
#define MSEL_FCODE		0x01	/* Must be 1 */

/* Here is the mode sense data description for typical disks */
struct scsi_msense_disk {
	struct scmode_hdr d_hdr;	/* Sense data header */
	struct scmode_blkd d_blkd;	/* Block descriptor list length */

		/* Device specific parameters list */
	unchar d_pgcode;		/* Page code 1 - error recovery page */
	unchar d_pglength;		/* Page length */
	unchar d_bits;  		/* Various error-recovery bits */
	unchar d_retry; 		/* Retry count */
	unchar d_corr;  		/* Correction span */
	unchar d_headoff;		/* Head offset count */
	unchar d_dsoff; 		/* Data strobe offset count */
	unchar d_recov; 		/* Recovery time limit */
};

/* For scsi_msense_disk.d_pgcode */
#define MSENSE_MODES		0x0	/* Just return block descriptor */
#define MSENSE_ERROR		0x1	/* Error recovery page */
#define MSENSE_CONN		0x2	/* Disconnect/reconnect page */
#define MSENSE_FORMAT   	0x3	/* Format parameter page */
#define MSENSE_GEOM		0x4	/* Rigid disk drive geometry page */
#define MSENSE_ALL		0x3f	/* Return all of the above pages */

/* For scsi_msense_disk.d_bits */
#define MSENSE_ERR_DCR  	0x1	/* Disable Correction */
#define MSENSE_ERR_DTE  	0x2	/* Disable transfer on error */
#define MSENSE_ERR_PER  	0x4	/* Post error */
#define MSENSE_ERR_EEC  	0x8	/* Enable early correction */
#define MSENSE_ERR_RC   	0x10	/* Read continuous */
#define MSENSE_ERR_TB   	0x20	/* Transfer block */
#define MSENSE_ERR_ARRE 	0x40	/* Automatic read realloc. enabled */
#define MSENSE_ERR_AWRE 	0x80	/* Automatic write reallo. enabled */

/*
 * Read Block Limits command returned data.
 */
struct screadblk {
	unchar	rb_pad1;      	/* Reserved */
	unchar	rb_max_len[3];	/* Maximum block length (MSB...LSB) */
	unchar	rb_min_len[2];	/* Minimum block length (MSB...LSB) */
};

/*
 * Read Capacity command returned data.
 */
struct screadcap {
	                     	/* Highest addressable block on disk: */
	unchar rc_nblocks0;  	/* MSB */
	unchar rc_nblocks1;
	unchar rc_nblocks2;
	unchar rc_nblocks3;  	/* LSB */
	                     	/* Formatted size of disk blocks: */
	unchar rc_bsize0;    	/* MSB */
	unchar rc_bsize1;
	unchar rc_bsize2;
	unchar rc_bsize3;    	/* LSB */
};

/*
 * Minimum data structure returned for SCSI Request 
 * Sense command (extended format). Its format consist 
 * of a header followed by vendor unique data, the size 
 * in bytes of which is described in the header.
 */
struct scrsense {
	unchar	rs_error;		/* Error code and valid bit */
	unchar	rs_seg;  		/* Segment Number */
	unchar	rs_key;  		/* Filemark, EOM, ILI, and Sense Key */
	unchar	rs_info[4];		/* Information bytes, kludged to 
					 * preserve alignment */
	unchar	rs_addlen;		/* Additional length in bytes */
};

/* For scrsense.rs_error  */
#define RS_VALID		0x80	/* Bit indicates error code is valid */
#define RS_ERRCLASS		0x70	/* Mask for error class */
#define RS_ERRCODE		0x0f	/* Mask for error code */
#define RS_DEFERR		0x71	/* Deferred error */
#define RS_VENDERR		0x7f	/* Vendor unique error code */
#define RS_CLASS_EXTEND 	0x70	/* Extended class of error codes */
#define RS_CODE_EXTEND 		0x00	/* Extended sense data format code */

/* For scrsense.rs_key */
#define RS_FILEMARK		0x80	/* Filemark has just been read */
#define RS_EOM     		0x40	/* End of media encountered */
#define RS_ILI     		0x20	/* Incorrect block length indicator */
#define RS_RES     		0x10	/* Reserved for future use */
#define RS_SENSEKEYMASK 	0x0f	/* Mask for Sense Key codes */

/* Sense Key codes for scrsense.rs_key & RS_SENSEKEYMASK  */
#define RS_NOSENSE		0x00	/* No Sense information available */
#define RS_RECERR		0x01	/* Recovered from error */
#define RS_NOTRDY		0x02	/* Addressed unit not accessible */
#define RS_MEDERR		0x03	/* Error in medium encountered */
#define RS_HRDERR		0x04	/* Target detects hardware failure */
#define RS_ILLREQ		0x05	/* Illegal request */
#define RS_UNITATTN		0x06	/* Media changed or target reset */
#define RS_PROTECT		0x07	/* Media protected against operation */
#define RS_BLANK		0x08	/* Blank check medium error */
#define RS_VENDUNIQ		0x09	/* Vendor unique code */
#define RS_CPABORT		0x0a	/* Copy command aborted */
#define RS_ABORT		0x0b	/* Command aborted */
#define RS_EQUAL		0x0c	/* Search data found equal comparison */
#define RS_OVFLOW		0x0d	/* Volume overflow */
#define RS_MISCMP		0x0e	/* Source and media data mis-compare */
#define RS_RESKEY		0x0f	/* Reserved for future use */

/*
 * Macros for extacting frequently accessed information 
 * from SCSI request sense data fields.
 */
#define SCSI_RS_INFO_VALID(rsp)	(((struct scrsense *)rsp)->rs_error & RS_VALID)
#define SCSI_RS_ERR_CLASS(rsp)	\
		(((struct scrsense *)rsp)->rs_error & RS_ERRCLASS)
#define SCSI_RS_ERR_CODE(rsp)	\
		(((struct scrsense *)rsp)->rs_error & RS_ERRCODE)
#define SCSI_RS_EOM_SET(rsp)	(((struct scrsense *)rsp)->rs_key & RS_EOM)
#define SCSI_RS_FILEMARK_SET(rsp) \
		(((struct scrsense *)rsp)->rs_key & RS_FILEMARK)
#define SCSI_RS_ILI_SET(rsp)	(((struct scrsense *)rsp)->rs_key & RS_ILI)
#define SCSI_RS_SENSE_KEY(rsp)	\
		(((struct scrsense *)rsp)->rs_key & RS_SENSEKEYMASK)
#define SCSI_RS_ADDLEN(rsp, rbuflen) min((rbuflen) - sizeof(struct scrsense), \
		(uint)((struct srcsense *)rsp)->rs_addlen);

/*
 * SCSI device unit Macros (for standalone addressing):
 * 	bits 0-2: logical unit number on the target adapter (up to 8)
 *	bits 3-5: target adapter number (up to 8)
 *	bits 6-8: drive type (up to 8), index into configuration table 
 *	bits 9-11: SCSI adapter controller board number
 *
 *	On embedded SCSI, the logical unit number is always 0.
 */
#define SCSI_MAXDEVNO	63      	/* Maximum SCSI device number; a 
					 * combination of target and unit */
#define SCSI_MAX_LUN	7		/* Maximum SCSI logical unit number */
#define SCSI_MAX_TARGET	7		/* Maximum SCSI target adapter number */

#define SCSI_UNIT(x)  	((x)&7)
#define SCSI_TARGET(x)	(((x)>>3)&7)
#define SCSI_TYPE(x)  	(((x)>>6)&7)
#define SCSI_BOARD(x) 	(((x)>>9)&7)

#define SCSI_LUNSHFT	5		/* For shifting logical unit numbers
					 * to put into a SCSI command. */

/*
 * SCSI command termination status byte macros.
 * Flexible for usage in "if" and "switch" statements.
 */

/* Masking for termination code field */
#define STERM_CODE(x)		((x) & 0x1E)

/* Values for the code field; others are reserved */
#define STERM_GOOD		0x00
#define STERM_CHECK_CONDITION   0x02
#define STERM_CONDITION_MET	0x04
#define STERM_BUSY		0x08
#define STERM_INTERMEDIATE	0x10
#define STERM_INTERMEDIATE_COND 0x12
#define STERM_RES_CONFLICT	0x18

/* Conditional expressions for checking termination */
#define SCSI_GOOD(x) 		  (STERM_CODE(x) == STERM_GOOD)
#define	SCSI_CHECK_CONDITION(x)   (STERM_CODE(x) == STERM_CHECK_CONDITION)
#define	SCSI_CONDITION_MET(x) 	  (STERM_CODE(x) == STERM_CONDITION_MET)
#define	SCSI_BUSY(x)		  (STERM_CODE(x) == STERM_BUSY)
#define SCSI_INTERMEDIATE(x)	  (STERM_CODE(x) == STERM_INTERMEDIATE)
#define SCSI_INTERMEDIATE_GOOD(x) (STERM_CODE(x) == STERM_INTERMEDIATE_GOOD)
#define	SCSI_RES_CONFLICT(x)	  (STERM_CODE(x) == STERM_RES_CONFLICT)	

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_SCSI_H_ */
