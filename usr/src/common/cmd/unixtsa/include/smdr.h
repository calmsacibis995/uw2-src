/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/include/smdr.h	1.2"

#if !defined(_SMDR_H_INCLUDED)
#define _SMDR_H_INCLUDED
#include <smstypes.h>
#ifndef UNIX
#include <tispxipx.h>
#endif

#if !defined(STATIC)
#if defined(DEBUG_CODE)
#define STATIC
#else
#define STATIC static
#endif
#endif

#if defined(NLM) && defined(NETWARE_V320)
#define STRCHR NWLstrchr
#define STRUPR NWLstrupr
#define SPRINTF NWsprintf
#ifdef NETWARE_V320
#include <nwlocale.h>
#endif
#else
#define STRCHR strchr
#define STRUPR strupr
#define SPRINTF sprintf
#endif

//#if defined(DEBUG_CODE) && defined(NLM)
//char *GetMessage(int);
//#else
#define GetMessage(n) (programMesgTable[n])
//#endif

// 0x8000L is 32K
// define SMS_BUFFER_SIZE	0x8000L
#define STACK_SIZE			0x8000L

#define ALL_TYPES			0
#define TSA_TYPE			1
#define SDI_TYPE			2
#define SME_TYPE			3
#define SMDR_TYPE			4
#define FILTER_TYPE		5

#define NOT_CONNECTED		0
#define CONNECTED			1
#define LOGGED_IN			2

#define BYTE_SIZE			0xFFFFFFFFL
#define WORD_SIZE			0xFFFFFFFEL
#define LONG_SIZE			0xFFFFFFFDL
#define UINT8_ARRAY		0xFFFFFFFCL
#define UINT16_ARRAY		0xFFFFFFFBL
#define STRUCT_SIZE		0

#define SMDR_SAP_TYPE		0x23F
#define WS_TSA_SAP_TYPE	0x23E

#define PACKAGE_SIZE		512

typedef struct
{
	UINT32 
		size;

	void 
		*address;
} 
SMSP_FRAG;

typedef struct
{
	char
		*buffer;

	UINT32	
		physicalSize,
		logicalSize,
		writePosition;
} 
SMDR_FRAG;

typedef struct
{
	CCODE 
		(*fptr)();

	UINT16 
		smspcode,
		pad;
} 
RESOURCE_PROCEDURES;

typedef
CCODE SERVICE_SMSP(
	UINT16		smsp,
	struct smdr_thread *thread);

#include <tiuser.h>

typedef struct netbuf NET_ADDRESS;
typedef struct t_info TINFO;
typedef struct t_call TCALL;
typedef struct t_bind TBIND;

#define SPX	 	1
#define TCPIP		2
#define ADSP		4

typedef struct
{
	UINT32 
		smdrLevel;

	char
		moduleScanName[80],	
		lockdownIdentifier[32],
		smsType, 
		functionCount;
	
	SERVICE_SMSP
		*ServiceSMSP;

	RESOURCE_PROCEDURES
		*privateFunctions;

	NWSM_MODULE_VERSION_INFO
		info;

	UINT32
		lockingHandle_311,
		bufferMax;
} 
SMDR_MODULE_TABLE;

typedef struct
{
	UINT32
		transferSize,
		protocolSize,
		actualConnection,
		protocolType,
		loggedIn;
		
	void
		*fptr;

	RESOURCE_PROCEDURES
		*privateFunctions;

	int
		pipe;

	char
		defragger[PACKAGE_SIZE];

	SMDR_MODULE_TABLE
		*moduleTable;

	NET_ADDRESS
		netAddress;
}
SMDR_CONNECTION;

typedef
void DECODE_PASSWORD(
	UINT8 *key,
	UINT8 *encryption,
	UINT8 *password);

typedef struct 
{
	int 	 sessionPipe;
	TCALL 	*tCall;
}
SMDR_THREAD_INFO;


typedef struct smdr_thread
{
	int
		pipe;

	UINT32 
		connectionID;

	INT16  
		seedX, 
		seedY,
		seedZ,
		pad1;

	char
		key[8];

	CCODE
		*ccode;

	SMDR_FRAG
		input,
		output,
		data;

	CCODE (*LoadSMDR)(struct smdr_thread *thread, void *buffer, UINT32 size);

	DECODE_PASSWORD 
		*DecryptPassword;

	SMDR_MODULE_TABLE 
		*moduleTable;

	UINT32
		groupSendSize,
		threadIsLockedToModule;
#ifndef UNIX
	IPX_ADDR clientConnectionAddr;
#endif
}
SMDR_THREAD;

typedef 
CCODE SCAN_SMDR_MODULE(
	 char *pattern,
	 UINT8 scanType,
	 UINT32 *sequence,
	 char *moduleName);

typedef 
CCODE LINK_SMDR_MODULE(
	 char *name,
	 char smsType,
	 RESOURCE_PROCEDURES **procs,
	 SMDR_MODULE_TABLE **table);

typedef
CCODE UNLINK_SMDR_MODULE(
	 RESOURCE_PROCEDURES *procs);

typedef 
CCODE LOGIN_SMDR(
	 SMDR_CONNECTION *connection);

typedef 
CCODE LOGOUT_SMDR(
	 SMDR_CONNECTION *connection);

typedef 
CCODE SMDR_REQUEST(
	 SMDR_CONNECTION *connection,
	 int n_req,
	 SMSP_FRAG *requests,
	 int m_rep,
	 SMSP_FRAG *replies);

typedef
CCODE TRANSFER_DATA(
	SMDR_CONNECTION *connection,
	void *buffer,
	UINT32 size);

typedef
CCODE ALLOCATE_MEMORY(
	void **ptr,
	UINT32 size);

typedef
void ENCODE_PASSWORD(
	UINT8 *key,
	UINT8 *authenication,
	UINT8 *encryption);

typedef
CCODE GET_VERSION_INFO(
	SMDR_CONNECTION *connection,
	UINT32 infoSelector,
	NWSM_MODULE_VERSION_INFO *info);

typedef struct
{
	UINT32 smdrLevel;

	SCAN_SMDR_MODULE	*ScanSMDRModule;
	LINK_SMDR_MODULE	*LinkSMDRModule;
	UNLINK_SMDR_MODULE  *UnlinkSMDRModule;
	LOGIN_SMDR			*LoginSMDR;
	LOGOUT_SMDR			*LogoutSMDR;
	SMDR_REQUEST		*SMDRRequest;
	TRANSFER_DATA		*ReadData,
						*WriteData;
	ALLOCATE_MEMORY		*SMDRClearAlloc;
	ENCODE_PASSWORD		*EncodePassword;
	GET_VERSION_INFO	*GetVersionInfo;
} 
SMDR_PROCEDURES;

CCODE NWSMExportModuleToSMDR(SMDR_MODULE_TABLE *table);
CCODE NWSMRetractModuleFromSMDR(SMDR_MODULE_TABLE *table);
CCODE NWSMImportSMDRProcedures(SMDR_PROCEDURES *smdrProcs);
SMDR_CONNECTION *ValidConnection(UINT32 connectionID);
UINT32 NewConnectionHandle(SMDR_CONNECTION *addition);
CCODE DeleteConnectionHandle(SMDR_CONNECTION *deletion);
CCODE EnableServiceAdvertising(char SMDR[], UINT16 *socket);
CCODE DisableServiceAdvertising(void);
#endif

int _t_rcv(int, char*, unsigned, int*);
int _t_snd(int, char*, unsigned, int);
