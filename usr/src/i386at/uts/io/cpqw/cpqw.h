/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/cpqw/cpqw.h	1.5"

/*
 *	Copyright (c) 1992, 1993  Compaq Computer Corporation
 */

#define SPL()			splhi()/* protection from interrupts */

#define OK			0
#define NOT_OK			(-1)

#define ENABLED			1
#define DISABLED		0

/* minor numbers for sub-drivers */
#define CPQH_DRIVER		0	/* CPQH driver */
#define ASR_DRIVER		1	/* ASR driver */
#define ECC_DRIVER		2	/* ECC driver */
#define CSM_DRIVER		3	/* CSM driver */

#define CPQ1501			0x0115110E	/* SYSTEMPRO/XL */
#define CPQ1509			0x0915110E	/* SYSTEMPRO/XL */
#define CPQ0601			0x0106110E	/* PROSIGNIA */
#define CPQ0701			0x0107110E	/* ANCHORAGE */
#define CPQ7000			0x0070110E	/* SERVER MANAGER */

#define SERVER_MGR_PRESENT	0x80;	/* Server Manager board present */

#define ERROR_ENTRY_LENGTH	8	/* crit/corr error entry length */

#define NCP_STATUS		0xCC9	/* coprocessor control/status register*/

#define CORRECTABLE_MEM_ERROR	0x10	/* correctable memory error */
#define NCP_SETUP		0x01	/* coprocessor control/status setup */
#define ECC_DISABLE		0x20	/* disable ECC error notification */
#define ROM_CALL_ERROR		0x01	/* ROM call error bit */
#define CALL_NOT_SUPPORTED	0x86	/* ROM call not supported */
#define ERROR_NOT_LOGGED	0x87	/* ROM call not logged */
#define EV_NOT_FOUND		0x88	/* EV not found with ROM call */
#define LOG_SYSTEM_ERROR	0xD8A3	/* log system error INT 15 ROM call */

/* EV values */
#define PASSED_POST_TEST	'1'	/* passed POST */
#define ENABLE_CPR_COUNTER	'1'	/* ASR enabled */
#define ASR_COUNT_VALUE		30	/* asr count value in minutes */

/* asr control word (timer 0, LSB first, mode 4, binary) */
#define ASR_CONTROL_WORD	0x38

#define ASR_NMI_ENABLE		0x04	/* enable ASR NMI */
#define ASR_NMI_DISABLE		0x08	/* disable ASR NMI */

/* error type */
#define NMI			0x0	/* critical error get and log */
#define ECC			0x1	/*correctable memory error get and log*/
#define CRITICAL_ERROR		0x80	/* critical error log */

/* display a post error message if an error occurs */
#define POST_MESSAGE		0x20

/* ioctl options */
#define ASR_HANDSHAKE		0x01
#define ASR_RESET		0x02
#define ASR_STOP		0x03
#define SOFT_NMI		0x11
#define ECC_ENABLE		0x12
#define ECC_SET			0x13
#define POST_MSG		0x14
#define CNMI_DEBUG		0x15
#define EISA_BUS_UTILIZATION	0x20
#define	FTPS_GET_STATUS		0x30

/* Number of pages to allocate for the EISA buffer.  (16 * 4k) */
#define EISA_BUF_PAGES		16
#define EISA_BUFFER_SIZE	65536 

/* NMI types */
#define NCMERR			1	/* non-correctable memory error */
#define BUSMTIM			2	/* bus master timeout */
#define CMDTIM			3	/* expansion bus cycle timeout */
#define IOCHK			4	/* expansion bus error */
/* */
#define CPERR			6	/* cashe parity error */
#define PPERR			7	/* processor parity error */
#define EHMNMI			8	/* EHM line buffer snoop read hit */
/* */
#define FAILTIM			1	/* failsafe timer timeout */
#define NMIPORT			1	/* software NMI via I/O port */

#define CSM_OBF			0x1	/* output buffer - 1 = full */
#define CSM_IBF			0x2	/* input buffer - 1 = full */
#define CSM_ID			0x00	/* ID register read command */
#define CSM_MODEM_R		0x10	/* modem control register read cmd */
#define CSM_MODEM_W		0x90	/* modem control register write cmd */
#define CSM_TEMP1_STATUS_R	0x20	/* temperature #1 status read command */
#define CSM_TEMP1_ENABLE_R	0x21	/* temperature #1 enable read command */
#define CSM_TEMP2_STATUS_R	0x22	/* temperature #2 status read command */
#define	CSM_FTPS_STATUS_R	CSM_TEMP2_STATUS_R

#define CSM_TEMP2_ENABLE_R	0x23	/* temperature #2 enable read command */
#define CSM_FTPS_ENABLE_R	CSM_TEMP2_ENABLE_R
#define	CSM_EXPANSION_INPUT_R	0x71	/* Expansion Input/Redun. Pwr Spply */
#define CSM_TEMP1_STATUS_W	0xA0	/* temperature #1 status write command*/
#define CSM_TEMP1_ENABLE_W	0xA1	/* temperature #1 enable write command*/
#define CSM_TEMP2_STATUS_W	0xA2	/* temperature #2 status write command*/
#define CSM_TEMP2_ENABLE_W	0xA3	/* temperature #2 enable write command*/
#define	CSM_FTPS_ENABLE_W	CSM_TEMP2_ENABLE_W
#define CSM_FAN_STATUS		0x30	/* fan status command */
#define CSM_FAN_ENABLE_R	0x31	/* fan enable read command */
#define CSM_FAN_ENABLE_W	0xB1	/* fan enable write command */
#define CSM_POLLING_MODE	0xAD	/* place CSM in polling mode */
#define CSM_INTERRUPT_MODE	0xAE	/* place CSM in interrupt mode */
#define CSM_EISA_BUS_COUNTER	0x60	/* EISA bus counter command */
#define CSM_POWER_SHUTDOWN_W	0xE8	/* CSM Power Shutdown */
#define CSM_EBBETS_MISC_CTRL_R	0x40	/* Ebbets misc. ctrl register - read */
#define CSM_EBBETS_MISC_CTRL_W	0xC0	/* Ebbets misc. ctrl register - write */

#define CSM_TEMP_ENABLE		0x01	/* enable temperature command */
#define	CSM_FTPS_ENABLE		CSM_TEMP_ENABLE
#define CSM_TEMP1_DISABLE	0x00	/* disable temperature #1 */
#define CSM_TEMP2_DISABLE	0x00	/* disable temperature #2 */
#define CSM_DISABLE_ALL_FANS	0x00	/* disable all the fans */
#define CSM_POWER_SHUTDOWN_CMD	0xB2	/* CSM Power Shutdown command */
#define CSM_ECC_DISABLE		0x20	/* CSM disable ECC errors */

#define THERMAL	0x01			/* thermal shutdown */
#define UPS	0x02			/* UPS shutdown */
#define ASR	0x04			/* ASR shutdown */

#define MAX_CSM_COUNTER		128	/* the max. number of trys at the CSM */

#define CSM_ID_VAL		0x01	/* ID register value for CSM */
#define	ISM_ID_VAL		0x02	/* ID register value for ISM */

#pragma pack(1)

/* critical error log structure */
typedef struct CRITICAL_ERROR_T {
	unsigned long	type		:7,
			corrected	:1,
			hour		:5,
			day		:5,
			month		:4,
			year		:7,
			reserved	:3;
	unsigned long	error_info;
} critical_error_t;

/* correctable error log structure */
typedef struct CORRECTABLE_ERROR_T {
	unsigned long	count		:8,
			hour		:5,
			day		:5,
			month		:4,
			year		:7,
			reserved	:3;
	unsigned short	DDR;
	unsigned short	syndrome;
} correctable_error_t;

/* ASR freeform data structure */
typedef struct asr_freeform {
	unsigned char num_bytes;
	unsigned char major_version;
	unsigned char minor_version;
	unsigned char timeout_value;
	unsigned char reserved;
	unsigned short base_addr;
	unsigned short scale_value;
} asr_t;

/* eisa bus utilization structure */
typedef struct EISA_BUS_UTIL_T {
	char bclkp;			/* BCLK period - from CPHCSM EV */
	unsigned long icount;		/* number of idle BCLK's */
	long interval;			/* time since last counter read */
} eisa_bus_util_t;

/* CSM type string structure */
typedef struct CSM_TYPE_STRING_T {
	char length;			/* length of string */
	char csm_interrupt;		/* CSM interrupt value */
	char fan_slots;			/* bit denotes if a fan slot exists */
	char fans_required;		/* bit denotes fans required for */
					/*   corresponding slot */
	char fans_present;		/* bit denotes fans installed */
	char processor_fans;		/* bit denotes if the fan is a */
					/*   processor fan */
	char bclk_period;		/* BCLK period in nano-seconds */
	short max_eisa_util_interval;	/* maximum EISA bus util. interval */
					/*   between two samples (in seconds) */
	short csm_base_addr;		/* base address of the CSM */
} csm_type_string_t;

/* Server Management EV - CPHCSM */
typedef struct CQHCSM_T {
	char eaas;			/* EAAS byte */
	char quicktest_rom_date[3];	/* Quicktest ROM date */
	char confirm_recovery_required;	/* EAAS/ASR conformation of recovery */
					/*   required */
	char eaas_shutdown_occurred;	/* eaas shutdown occurred */
} cqhcsm_t;

/* CSM failure structure */
typedef struct CSM_FAILURE_T {
	unsigned char temp_over_run;	/* counter */
	unsigned char failed_fan;	/* bitmap */
/*
	struct lockb lock_csmtab;	/* lock code to a single processor */
	int oldspl;     		/* old interrupt priority level */
#define	FTPS_INSTEAD_OF_TEMP2
#ifdef	FTPS_INSTEAD_OF_TEMP2
	unsigned char ftps;		/* Redundant Power Supply */
#endif
} csm_failure_t;

#pragma pack()

/* ECC setup structure */
typedef struct ECC_SETUP_T {
	long max_ECC_errors;		/* maximum ECC errors per interval */
	long ECC_interval;		/* time in seconds */
} ecc_setup_t;

/*
 * Fault Tolerant Power Supply
 */
 
/* cpqHeFltTolPwrSupplyCondition  */
#define	FTPSCONDITION_DONT_EXIST	1	/* FTPS not supported or
						 * not installed */
#define	FTPSCONDITION_OK		2	/* FTPS detected */
#define	FTPSCONDITION_DEGRADED		3	/* FTPS not detected */
#define	FTPSCONDITION_DONTKNOW		4	/* can't determine */
 
#define	FTPSSTATUS_DONTKNOW		1	/* can't determine */
#define	FTPSSTATUS_NOTSUPPORTED		2	/* No CSM, therefore no FTPS */
#define	FTPSSTATUS_NOTINSTALLED		3	/* FTPS not installed */
#define	FTPSSTATUS_INSTALLED		4	/* FTPS installed & detected */
 
typedef struct {
	int	ftps_status;
	int	ftps_condition;
} snmp_ftps_t;
