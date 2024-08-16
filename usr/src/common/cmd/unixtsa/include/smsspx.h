/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/include/smsspx.h	1.1"

#define ERR_MCCS_NO_MEMORY           0xffd0
#define ERR_MCCS_CONNECTION_OVERFLOW 0xffd1
#define ERR_MCCS_LOOK_FOR_PACKET     0xffd2
#define ERR_MCCS_NO_SERVER           0xffd3

short SwapShort(unsigned short i);
#define SERVER_TYPE 3
#define MAXIMUM_ALLOWED_USERS 100
#define NUMBER_RECEIVE_ECBS 4
#define NUMBER_SEND_ECBS 1
#define MAX_BLOCK_SIZE 522
#define MAX_BLOCK_RECEIVED ( 65535 / MAX_BLOCK_SIZE) + 1

int IPXInitialize(void) ;
void IPXRelinquishControl(void) ;
int InitSPXFunctions(void);
int InitMCCSSpx(unsigned short socketValue,long *session,int NumberUsers);
void DeInitMCCSSpx(long *MccsSpxHandle);
int SignIntoServer(long MccsSpxHandle,int login,char *name,char *password,short type);
int LocateServer(long MccsSpxHandle,int login,char *name,char *password,short type);
void CloseConnection(long *communicationHandle);
int WaitForConnection(long MccsSpxHandle,long *communicationHandle);
int MakeConnection(long MccsSpxHandle,long *communicationHandle);

#if NLM == 1
unsigned ReadConnection(long communicationHandle,char *buffer,unsigned bytes);
void ClearReadConnection(long communicationHandle);
unsigned WriteConnection(long communicationHandle,char *buffer,unsigned bytes);
#else
unsigned ReadConnection(long communicationHandle,char far *buffer,unsigned bytes);
unsigned WriteConnection(long communicationHandle,char far *buffer,unsigned bytes);
#endif	

void ClearUserConnections(long MccsSpxHandle);
short GetMCCSTaskNumber(long communicationHandle);
void ReleaseUserConnection(long commHandle);
int GetCommunicationHandle(long MccsSpxHandle, long Network, char *Node,
                                                    long *communicationHandle);
#if OS2 == 1
HSEM GetResponseSemaphore(long communicationHandle);
#else
long GetResponseSemaphore(long communicationHandle);
#endif

int WaitForResponse(long communicationHandle);
