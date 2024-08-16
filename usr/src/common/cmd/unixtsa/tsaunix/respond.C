#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/respond.C	1.6"
/**********************************************************************
Filename:			respond.c
**********************************************************************/

#include <tsad.h>
#include <smdr.h>
#include <smsdrerr.h>
#include <error.h>
#include <smspcode.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include <smstypes.h>
#include "tsasmsp.h"
#include "respond.h"
#include "tsaglobl.h"

/**********************************************************************
Global Variables
**********************************************************************/

#define SMSP_HEADER_SIZE	6

extern int t_errno;
extern char *t_errlist[];

int		SessionPipeParm;

#ifdef DEBUG
extern void show_buf(char *buf, int len);
extern void fshow_buf(FILE *outp, char *buf, int len);
extern FILE *logfp ;
#endif
 
void dump_buf(char *buf, int len, char *dumpBuf);
char dumpBuffer[512] ;
 
STATIC CCODE SendBuffer(int pipe, void *buffer, UINT32 size, 
								UINT32 groupsize) ;

STATIC CCODE ReceiveBuffer(int pipe, void *buffer, UINT32 size, 
					UINT32 groupsize) ;

STATIC void TrapThreadError(SMDR_THREAD *thread, CCODE ccode, 
					UINT8 send) ;
STATIC CCODE InitializeBuffers(SMDR_THREAD *thread);

STATIC void InitializeSeeds(SMDR_THREAD *thread);

STATIC UINT32 RandomUINT32(SMDR_THREAD *thread) ;

STATIC UINT8 RandomChar(SMDR_THREAD *thread, UINT8 max) ;

STATIC void TerminateProcess(SMDR_THREAD *thread) ;

STATIC CCODE SMDRGetSMSP(int pipe, UINT32 size, 
				SMDR_THREAD *thread, UINT32 group) ;

STATIC UINT32 GetProtocolSize(int pipe) ;

STATIC UINT32 RandomUINT32(SMDR_THREAD *thread)
{
	UINT32 seedX, seedY, seedZ;

	thread->seedX = 0xAB * (thread->seedX % 0xB1) - 0x02 * (thread->seedX / 0xB1);

	if (thread->seedX < 0)
		thread->seedX += 0x763D;

	thread->seedY = 0xAC * (thread->seedY % 0xB0) - 0x23 * (thread->seedY / 0xB0);

	if (thread->seedY < 0)
		thread->seedY += 0x7663;

	thread->seedZ = 0xAA * (thread->seedZ % 0xB2) - 0x3F * (thread->seedZ / 0xB2);

	if (thread->seedZ < 0)
		thread->seedZ += 0x7673;

	seedX = thread->seedX;
	seedY = thread->seedY;
	seedZ = thread->seedZ;

	seedX <<= 16;
	seedY <<= 16;
	seedZ <<= 16;

	seedX /= 0x763D;
	seedY /= 0x7663;
	seedZ /= 0x7673;

	return((seedX + seedY + seedZ) & 0x0FFFF);
}

STATIC UINT8 RandomChar(SMDR_THREAD *thread, UINT8 max)
{
	UINT32 temp;

	temp = (RandomUINT32(thread) * max) >> 16;

	return ((UINT8)temp);
}

STATIC void InitializeSeeds(SMDR_THREAD *thread)
{
	thread->seedZ = (short)time(NULL);
	thread->seedX = thread->seedZ >> 2;
	thread->seedY = thread->seedZ << 3;
}

#define SMSP_POLL_TIMEOUT	3 * 24 * 60 * 60 /* 3 days */

/*********************************************************************
Function:	ServiceRequest

Purpose:	This the thread that actually handles the servicing of a
            request from an SME.  It receives the SMSP on the wire and
			responds appropriately, either handling the request, or
			passing it on to ServiceSMSP.

Calls:			InitializeBuffers
				TrapThreadErorr
				GetProtocolSize
				ReceiveBuffer
				SendBuffer
				SMDRGetSMSP
				ServiceSMSP
				TerminateProcess

**********************************************************************/
void ServiceRequest(void)
{
	CCODE			ccode;
	UINT32	groupsize, *transfersize, *rcode ;
	INT32	thistime, unread;
	int flags ;
	int pipe ;
	SMDR_THREAD	smdrThread;
	SMSP_HEADER request, requestBuffer;
	char *ptr = (char *)&requestBuffer ;

#ifdef DEBUG
	logerror(PRIORITYWARN,"In ServiceRequest() sessionPipe = %d\n",SessionPipeParm);

	FLUSHLOG ;
#endif
	memset(&smdrThread, '\0', sizeof(SMDR_THREAD));
	pipe = smdrThread.pipe = SessionPipeParm;

	ccode = InitializeBuffers(&smdrThread);

	TrapThreadError(&smdrThread, ccode, NOSEND);

	InitializeSeeds(&smdrThread) ;

	rcode = (UINT32*)smdrThread.output.buffer ;
	
	smdrThread.groupSendSize = groupsize = GetProtocolSize(pipe);

	for(;;)
	{	
		unread = SMSP_HEADER_SIZE ;
		while ( unread ) {
			thistime = t_rcv (pipe,ptr, unread,&flags);
			if (thistime == -1 ) {
				if ( t_errno == TNODATA ) { /* start polling */
					if ( PollTransportEndPoint(pipe, POLLIN, 
							SMSP_POLL_TIMEOUT, 1) == -1 ) {
						logerror(PRIORITYWARN,
							TSAGetMessage(POLL_FAILED), errno);
						TerminateProcess(&smdrThread);
						return;
					}
					if ( t_look(pipe) != T_DATA ) {
						break ;
					}
					continue ;
				}
				if ( t_errno == TLOOK ) {
					logerror(PRIORITYWARN,
						TSAGetMessage(T_RCV_FAILED), t_look(pipe));
				}
				logerror(PRIORITYERROR,
						TSAGetMessage(T_RCV_ERROR), t_errno);
				FLUSHLOG ;
				TerminateProcess(&smdrThread);
				return;
			}
			unread -= thistime ;
			ptr += thistime ;
		}
		ptr = (char *)&requestBuffer ;

		smdrThread.output.logicalSize = sizeof(UINT32);
		smdrThread.output.writePosition =  sizeof(UINT32);

		request.size = SwapUINT32p(ptr) - SMSP_HEADER_SIZE ;
		request.fcode = SwapUINT16p(ptr + sizeof(UINT32));

#ifdef DEBUG
		logerror(PRIORITYWARN,"\n------------------------\n\nReceived SMSP %x, size %d\n",
			request.fcode, request.size);
		fshow_buf(logfp, (char *)&request,SMSP_HEADER_SIZE);
		FLUSHLOG ;
#endif

		switch(request.fcode)
		{
			case SMSP_GetEncryptionKey:
			{
				int i;

				for (i = 0; i < 8; i++)
					smdrThread.key[i] = RandomChar(&smdrThread, 255);
				*rcode = 0L;
				LoadReplyBuffer(&smdrThread, smdrThread.key, 8);
				goto TRANSFER;
			}
		
			case SMSP_SendData:
				ccode = ReceiveBuffer(pipe, smdrThread.data.buffer, request.size,
					groupsize);
				TrapThreadError(&smdrThread, ccode, NOSEND);
				*rcode = SwapUINT32(ccode);
				goto TRANSFER;

			case SMSP_ReceiveData:
				ccode = 0L;
				ccode = SendBuffer(pipe, &ccode, sizeof(CCODE), groupsize);
				TrapThreadError(&smdrThread, ccode, NOSEND);

				ccode = SendBuffer(pipe, smdrThread.data.buffer, smdrThread.data.logicalSize,
							groupsize);

				TrapThreadError(&smdrThread, ccode, NOSEND);

				continue;

			default:
				ccode = 0L ;
				if ( request.size ) {
					ccode = SMDRGetSMSP(pipe, request.size, &smdrThread, groupsize);
					TrapThreadError(&smdrThread, ccode, SEND);
				}
				if (ccode)
					continue;
				else
					break;
		}

		switch(request.fcode)
		{
			case SMSP_ConnectToRemoteResource:
			{
				*rcode = 0;
				transfersize = rcode;
				transfersize++;
				*transfersize = SwapUINT32(THREAD_BUFFER_SIZE);
				smdrThread.output.logicalSize = 8 ;
				break;
			}	

			default:
				ServiceSMSP(request.fcode, &smdrThread);
				break;
		}
TRANSFER:
		ccode = SendBuffer(pipe, smdrThread.output.buffer,
				smdrThread.output.logicalSize, groupsize);

		TrapThreadError(&smdrThread, ccode, NOSEND);

		if ( request.fcode == SMSP_ReleaseTargetService ) {
			if ( PollTransportEndPoint(pipe, POLLIN, 
				SMSP_POLL_TIMEOUT, 1) == -1 || t_look(pipe) != T_DATA) {
				break ;
			}
		}
	}


	TerminateProcess(&smdrThread);
}


/*********************************************************************

Function:		TerminateProcess

Purpose:		This function terminates the ServiceRequest thread by
      			freeing up previously allocated resources, closing the
				communication pipe, and causing the thread to exit.

Parameters:	thread	input/output	Thread data structure

Design Notes:	
*********************************************************************/
STATIC void TerminateProcess(SMDR_THREAD *thread)
{
	if ( thread->input.buffer != NULL ) {
		FreeMemory(thread->input.buffer);
	}
	if ( thread->output.buffer != NULL ) {
		FreeMemory(thread->output.buffer);
	}
	if ( thread->data.buffer != NULL ) {
		FreeMemory(thread->data.buffer);
	}

	CloseTransportEndpoint(thread->pipe);
	fclose(logfp);
	exit(0);
}


/*********************************************************************

Function:	InitializeBuffers

Purpose:	This function initializes the three send and receive buffers
  			and sets up the thread structure to hold them.

Parameters:	thread	input/output	Thread data structure

**********************************************************************/
STATIC CCODE InitializeBuffers(SMDR_THREAD *thread)
{
	ERROR_VARS;
	CCODE 	ccode;
	char *temp;


	/*  data stuff */
	temp = (char *)AllocateMemory(THREAD_BUFFER_SIZE+1024);
	ERRORNZ(temp==NULL, 1);
	thread->data.buffer = temp ;
	memset(thread->data.buffer, '\0', THREAD_BUFFER_SIZE+1024);
	thread->data.physicalSize = THREAD_BUFFER_SIZE;

	/*  input stuff */
	temp = (char *)AllocateMemory(INPUT_OUTPUT_SIZE);
	ERRORNZ(temp==NULL, 2);
	thread->input.buffer = temp ;
	memset(thread->input.buffer, '\0', INPUT_OUTPUT_SIZE);
	thread->input.physicalSize = INPUT_OUTPUT_SIZE;

	/*  output stuff */
	temp = (char *)AllocateMemory(INPUT_OUTPUT_SIZE);
	ERRORNZ(temp==NULL, 3);
	thread->output.buffer = temp ;
	memset(thread->output.buffer, '\0', INPUT_OUTPUT_SIZE);
	thread->output.physicalSize = INPUT_OUTPUT_SIZE;

	thread->ccode = (CCODE*)thread->output.buffer;

	return 0;

	ERRORS
	{
		default:
			ccode = NWSMDR_OUT_OF_MEMORY;
			break;
	}

	CLEANUP
	{
		case 3:
			FreeMemory(thread->input.buffer);
		case 2:
			FreeMemory(thread->data.buffer);
		case 1:;
	}

	thread->data.buffer = NULL;
	thread->data.physicalSize = 0L;

	thread->input.buffer = NULL;
	thread->input.physicalSize = 0L;

	thread->output.buffer = NULL;
	thread->output.physicalSize = 0L;

	return ccode;
}


/********************************************************************

Function:	SMDRGetSMSP

Purpose:	This function is called by ServiceRequest when a packet is
			received, and it is not immediately requesting either a 
			Send or Receive.  This function then receives an SMSP into
			the thread buffer area.

Parameters:	pipe	input				Session pipe from connection
			size		input				Requested size
			thread	input/output	Thread data structure
			group		input				Acceptable size to receive

*********************************************************************/
STATIC CCODE SMDRGetSMSP(int pipe, UINT32 size, 
				SMDR_THREAD *thread, UINT32 group)
{
	ERROR_VARS;
	CCODE		ccode;
	UINT32	newsize;
	char *temp;


#ifdef DEBUG
	logerror(PRIORITYWARN, "In SMDRGetSMSP:\n") ;
	logerror(PRIORITYWARN, "size = %d, input.physicalSize = %d\n", 
					size, thread->input.physicalSize) ;
	FLUSHLOG ;
#endif

	if (size > thread->input.physicalSize)
	{
		FreeMemory(thread->input.buffer);
		thread->input.buffer = NULL;
		newsize = (size + 0x1FF) / 0x200; /* give buffersize a multiple of 512 */
		newsize *= 0x200;

		temp = (char *)AllocateMemory((unsigned int)newsize);
		ERRORNZ(temp==NULL, 1);

		thread->input.buffer = temp ;
		memset(thread->input.buffer, '\0', (int)newsize);
		thread->input.physicalSize = newsize;
	}

	thread->input.logicalSize = size;
	
	ccode = ReceiveBuffer(pipe, thread->input.buffer, size, group);
	ERRORNZ(ccode, 2);
#ifdef DEBUG
	fshow_buf(logfp, (char *)thread->input.buffer, size) ;
#endif

	return 0;

	ERRORS
	{
		case 1:
			ccode = NWSMDR_OUT_OF_MEMORY;
			break;

		case 2:
			ccode = NWSMDR_TRANSPORT_FAILURE;
			break;
	}

	return ccode;
}


/********************************************************************

Function:	TrapThreadError

Purpose:	This function gets called when an error may necessitate
			shutting down the TSA.  The SendBuffer condition only gets
			called after a SMDRGetSMSP call fails due to running out
			of memory.

Parameters:	thread	input/output	Thread data structure
			ccode	input			Error code to examine
			send	input			TRUE or FALSE whether to send

Design Notes:	

**********************************************************************/
STATIC void TrapThreadError(SMDR_THREAD *thread, CCODE ccode, UINT8 send)
{
	UINT32	sendCode ;
	switch(ccode)
	{
		case NWSMDR_OUT_OF_MEMORY:
			if (send)
			{
				sendCode = SwapUINT32(ccode);
				SendBuffer(thread->pipe, &sendCode, sizeof(UINT32), 
										sizeof(UINT32));
				break;
			}

		case NWSMDR_TRANSPORT_FAILURE:
			TerminateProcess(thread);
			break;
	}
}


/********************************************************************
Function:		LoadReplyBuffer

Purpose:This function only gets called from inside the DRAPI.  The
		DRAPI calls use this as a transparent mechanism for getting
		their stuff to the SME.  Here the actual sends are performed
		and physical size limits are dealt with.

Parameters:	thread	input/output	Thread data structure
		buffer	input				Pointer to data to send
		size		input				Size to send

Design Notes:

**********************************************************************/
CCODE LoadReplyBuffer(SMDR_THREAD *thread, void *buffer, UINT32 size)
{
	ERROR_VARS;
	CCODE ccode;
	SMDR_FRAG *reply = &thread->output;
	char *ptr;
	int pipe = thread->pipe;
	UINT32 groupsize = thread->groupSendSize;

	if (size + reply->logicalSize > reply->physicalSize)
	{
		ccode = SendBuffer(pipe, thread->output.buffer,
			thread->output.logicalSize, groupsize);

		ERRORNZ(ccode, 1);

		reply->logicalSize = 0;
	}

	if (size > reply->physicalSize)
	{
		ccode = SendBuffer(pipe, buffer, size, groupsize);
		ERRORNZ(ccode, 1);
	}
	else
	{
		ptr = reply->buffer + reply->logicalSize;
		memcpy(ptr, buffer, (UINT16) size);
		reply->logicalSize += size;
	}

	return 0;

	ERRORS
	{
		case 1:
			ccode = NWSMDR_TRANSPORT_FAILURE;
			TerminateProcess(thread);
			break;
	}

	return ccode;
}


/********************************************************************

Function:	GetProtocolSize

Purpose:	This function provides transport specific information about
			the maximum size of packets allowed.  This information is
			stored in the thread data space, and used when sending and
			receiving.

Parameters:	pipe	input		TLI Pipe to query for size information

Design Notes:	
*********************************************************************/
STATIC UINT32 GetProtocolSize(int pipe)
{
	struct t_info info;

	t_getinfo(pipe, &info);

	if ((info.tsdu == 0) || (info.tsdu == -1) || (info.tsdu > 0x7FFF))
		return 0x7FFF;
	else
		return info.tsdu;
}


/***********************************************************************

Function:	ReceiveBuffer

Purpose:	This call is used to actually receive information from the
			SME.  It reads information from a TLI pipe until the pipe
			is empty, and places the information in the buffer address 
			passed to it.

Parameters:	pipe		input				Session pipe from connection
			size		input				Number of bytes to send
			thread	input/output	Thread data structure
			group		input				Acceptable size to send

Design Notes:	
*********************************************************************/
STATIC CCODE ReceiveBuffer(int pipe, void *buffer, UINT32 size, 
					UINT32 groupsize)
{
	ERROR_VARS;
	CCODE ccode;
	int flags;
	INT32 unread, thistime;
	char *ptr;

	ERROREZ(size, 1);

	if (groupsize > 0x7FFF)
		groupsize = 0x7FFF;

	ptr = (char *)buffer;
	unread = size;

#ifdef DEBUG
	logerror(PRIORITYWARN,"In ReceiveBuffer() size = %d pipe = %d\n",
			size, pipe);
#endif

	do
	{
		if (unread < groupsize)
			groupsize = unread;

		thistime = t_rcv (pipe,ptr, (int)groupsize,&flags);

		if ( thistime == - 1 ) {
			if ( t_errno == TNODATA ){/*start polling*/
				if ( PollTransportEndPoint(pipe, POLLIN, 
						SEND_RCV_TIME_OUT, NO_OF_TRIES) == -1 ) {
					logerror(PRIORITYWARN,
							TSAGetMessage(POLL_FAILED), errno);
					ccode = NWSMDR_TRANSPORT_FAILURE;
					return ccode;
				}
				continue ;
			}
			if ( t_errno == TLOOK ) {
				logerror(PRIORITYWARN,
						TSAGetMessage(T_RCV_FAILED), t_look(pipe));
			}
			logerror(PRIORITYERROR,
						TSAGetMessage(T_RCV_ERROR), t_errno);
			FLUSHLOG ;
			ccode = NWSMDR_TRANSPORT_FAILURE;
			return ccode;
		}
		ERRORLZ(thistime, 2);

		unread -= thistime;

		ptr += thistime;
	}
	while (unread > 0);
#ifdef DEBUG
	dump_buf((char *)buffer,size, dumpBuffer);
	logerror(PRIORITYWARN,"ReceiveBuffer\n");
	logerror(PRIORITYWARN,"%s\n",dumpBuffer);
#endif

	return 0;

	ERRORS
	{
		case 1:
			ccode = 0;
			break;

		case 2:
			ccode = NWSMDR_TRANSPORT_FAILURE;
			break;
	}

	return ccode;
}


/********************************************************************

Function: SendBuffer

Purpose:	This function is used to actually send data to the SME.
			Information is written to a TLI pipe until the buffer of
			information has been completely sent.

Parameters:	pipe		input				Session pipe from connection
			size		input				Requested size
			thread	input/output	Thread data structure
			group		input				Acceptable size to receive

Design Notes:	
*********************************************************************/
STATIC CCODE SendBuffer(int pipe, void *buffer, UINT32 size, 
								UINT32 groupsize)
{
	ERROR_VARS;
	CCODE ccode;
	INT32 unwritten, thistime;
	char *ptr;

	ERROREZ(size, 1);

	if (groupsize > 0x7FFF)
		groupsize = 0x7FFF;

	ptr = (char *)buffer;
	unwritten = size;

#ifdef DEBUG
		logerror(PRIORITYWARN,"\nSending data:\n") ;
		fshow_buf(logfp, (char *)buffer,size);
		FLUSHLOG ;
#endif

	do
	{
		if (unwritten < groupsize)
			groupsize = unwritten;

		thistime = (UINT32) t_snd(pipe, ptr, (int)groupsize, 0);

		if ( thistime == - 1 ) {
			if ( t_errno == TFLOW ){/*start polling*/
				if ( PollTransportEndPoint(pipe, POLLOUT, 
						SEND_RCV_TIME_OUT, NO_OF_TRIES) == -1 ) {
					logerror(PRIORITYWARN,
							TSAGetMessage(POLL_FAILED), errno);
					ccode = NWSMDR_TRANSPORT_FAILURE;
					return ccode;
				}
				continue ;
			}
			if ( t_errno == TLOOK ) {
				logerror(PRIORITYWARN,
						TSAGetMessage(T_SND_FAILED), t_look(pipe));
			}
			logerror(PRIORITYERROR,
						TSAGetMessage(T_SND_ERROR), t_errno);
			FLUSHLOG ;
			ccode = NWSMDR_TRANSPORT_FAILURE;
			return ccode;
		}

		ERRORLZ(thistime, 2);

		unwritten -= thistime;
		ptr += thistime;
	}
	while (unwritten > 0);

	return 0;

	ERRORS
	{
		case 1:
			ccode = 0;
			break;

		case 2:
			ccode = NWSMDR_TRANSPORT_FAILURE;
			break;
	}

	return ccode;
}
