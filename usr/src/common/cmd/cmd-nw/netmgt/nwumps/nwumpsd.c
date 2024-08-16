/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/netmgt/nwumps/nwumpsd.c	1.16"
/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

#if !defined(NO_SCCS_ID) && !defined(lint) && !defined(SABER)
static char rcsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/netmgt/nwumps/nwumpsd.c,v 1.22.2.2 1994/10/21 22:52:31 rbell Exp $";
#endif

/****************************************************************************
** Source file:  nwumpsd.c
**
** Description:   
**
** Contained functions:
**                      nwuArgInit()
**                      nwuEnvInit()
**                      nwuFDInit()
**                      nwuMibInit()
**
**                      start_smux()
**                      doit_smux()
**                      get_smux()
**                      set_smux()
**
**                      PollForEvents()
**
** Author:   Rick Bell
**
** Date Created:  January 25, 1993
**
** COPYRIGHT STATEMENT: (C) COPYRIGHT 1993 by Novell, Inc.
**                      Property of Novell, Inc.  All Rights Reserved.
**
****************************************************************************/

#include <stdio.h>
#include <sys/nwportable.h>

#ifndef OS_AIX
#include <stdlib.h>
#endif
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif
#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

#include <stdarg.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <stropts.h>
#include <signal.h>
#include <tiuser.h>  /* for diags */
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "nps.h"
#include "nwconfig.h"

#include "sys/ipx_app.h"
#include "sap_lib.h"

#include "nwmsg.h"
#include "netmgtmsgtable.h"
#include "nwumpsd.h"

/* including isode and snmp header files */
#ifdef OS_UNIXWARE
#include "snmp.h"
#include "objects.h"
#else
#ifdef PEPYPATH
#include "smux.h"
#include "objects.h"
#else
#include <isode/snmp/smux.h>
#include <isode/snmp/objects.h>
#endif
#endif

/* Include NetWare for Unix headers */
#ifdef OS_UNIXWARE
#include "nwsmuxsvr4.h"
#else
#include "nwsmux.h"
#endif

/* Include NetWare for Unix headers */
#ifdef BITTEST
#undef BITTEST
#endif

/* External Variables */
char  errorStr[MSG_MAX_LEN];
char  nwumpsTitleStr[] = "nwumpsd";    /* Uppercase Daemon name */
int   mode = 0;                        /* This is the normal mode */

char  *ripxDevice = "/dev/ripx";
char  *ipxDevice = "/dev/ipx";
char  *spxDevice = "/dev/nspx";

int   ripxFd = -1;
int   ipxFd = -1;
int   spxFd = -1;

int   spxFlag = TRUE;
int   diagFlag = TRUE;
int   sapFlag = TRUE;

IpxNetAddr_t    MyIPXNetAddr = {0};
IpxNodeAddr_t   MyIPXNodeAddr = {0};
char            MyServerName[NWUMPS_NAME_SIZE];
char            IPXCircIndex[24];


static	char *optstr = "dv";
uint32	verbose = FALSE;
int	Cret;                /* Status from NWCM calls */
int	debug = 0;

char nwumpsDaemonName[NWCM_MAX_STRING_SIZE]; /* Lowercase Daemon name */

extern int NumLans;
extern int NumRouters;
extern int NumSPXConnections;
extern uint32 NumServers;
extern uint32 NumDestServ;
extern ServerEntry_t **ServerTable;

/*
**  shared memory information
*/
int           MemoryMapped = 0;  /* Has MapSAPMemory been called? */
int           shmid = -1;        /* Shared memory id */
key_t         shmkey = -1;       /* Shared memory key */

SapShmHdr_t   *ShmBase = NULL;   /* Shared memory address */
SAPD          SAPStats;          /* SAP Statistics */
ServerEntry_t *SrvBase;          /* Server entry address base */

/* DATA */
static   int   got_at_least_one = 0;
static   int   rock_and_roll = 0;
static   int   dont_bother_anymore = 0;
static   int   smuxPollErrorCount = 0;

static   OID   subtree = NULLOID;
static   struct NWsmuxEntry *se = NULL;

/* Forward References */
static   nwuArgInit(int argCh, char *argVar[]);
static   nwuEnvInit();
static   nwuMibInit();

static void start_smux(void);
static void doit_smux(void);
static void get_smux(register struct type_SNMP_GetRequest__PDU *pdu, int offset);
static void set_smux(struct NWtype_SNMP_SMUX__PDUs *event);

void  PollForEvents(int);
void  nwuRefresh(int);
void  nwumpsExit(int status);

/* info on file descriptors that need to be polled */
struct   StructPollFds  
               {
               int   initialized;
               int   nElements;
               struct pollfd  *pollFds;
               } polls = {0};

static struct subgroup
   {
   char *t_tree;
   OID t_name;
   int t_access;
   void (*t_init)(void);
   IFP t_sync;
   } subgroups[] = {
   "ipx", NULL, readOnly , nwuIPXInit, NULL,
   "nwuSPX", NULL, readOnly, nwuSPXInit, NULL,
   "nwuDiag", NULL, readOnly, nwuDiagInit, NULL,
   "ripsap", NULL, readOnly, nwuRIPSAPInit, NULL,
   NULL
   };

static struct subgroup * tc;

/* Main */

/* ARGSUSED */

main(int argc, char *argv[], char *envp[])
   {
   int   sd;

#ifndef  sun
   /*
    **  Close all open file descriptors
    */
   for(sd = 4; sd < 20; sd++)
      {
      close(sd);
      }
   
   errno = 0;  /* clear probable EBADF from bogus close */

#endif

#ifdef OS_UNIXWARE
   /* This is only for UnixWare 2.0 SNMP agent. Check if SNMP agent is up */
   {
     struct stat tmpbuff;
     int errorcount = 0;

     while((stat("/tmp/snmpd.pid", &tmpbuff) == -1) && (errorcount < 60))
	{
	sleep(5);
	errorcount++;
	}

     if(errorcount >= 60)
       exit(EXIT_SUCCESS);
   }
#endif

   nwuArgInit(argc, argv);

   nwuEnvInit();

   nwuFDInit();
  
   nwuMibInit();

   fprintf(stderr, MsgGetStr(NWUMPSD_START));

  /* set fd's for other purposes here... */

   for(;;)
      {
      int   n;
      int   secs;

      secs = NOTOK;

      if(mode == NWUMPS_DEBUG)
         fprintf(stderr, "%s: main () smux fd = %d in the for loop.\n", 
               nwumpsTitleStr, polls.pollFds[SMUX_NWUMPS_FD].fd);

      if(polls.pollFds[SMUX_NWUMPS_FD].fd == NOTOK && !dont_bother_anymore)
         {
         if(smuxPollErrorCount < NWUMPS_POLL_ERROR_COUNT)
            smuxPollErrorCount ++;
         else
            nwumpsExit(EXIT_FAILURE);
               
         secs = 5 * 60L;
         fprintf(stderr, "%s: main() The timeout has been set.\n", nwumpsTitleStr);
         }
      else if(rock_and_roll)
         {
         if(smuxPollErrorCount != 0)
            smuxPollErrorCount = 0;

         polls.pollFds[SMUX_NWUMPS_FD].events = POLLIN;
         }
      else
         {
         if(smuxPollErrorCount != 0)
            smuxPollErrorCount = 0;

         polls.pollFds[SMUX_NWUMPS_FD].events = POLLOUT;
         }

      PollForEvents(secs);
      }
   }

/* Init */

static nwuArgInit(int argCh, char *argVar[])
   {
   int   c;                

   if((Cret = NWCMGetParam(NWUMPS_PROGRAM, NWCP_STRING, nwumpsDaemonName)) != SUCCESS) 
      {
      NWCMPerror(Cret, MsgGetStr(NWUMPSD_CFG_ERROR), NWUMPS_PROGRAM);
      return FAILURE;
      }

   /* Checks the switches. */

   while((c = getopt(argCh, argVar, optstr )) != -1) 
      {
      switch(c) 
         {
         case 'd' :
            mode = NWUMPS_DEBUG;
            debug = NWUMPS_DEBUG;
         break;

         case 'v' :
            verbose = TRUE;
         break;

         default :
            fprintf(stderr, "%s: -%s Unknown switch.\n", nwumpsTitleStr, c);
         break;
         }
      }
  }

static nwuEnvInit()
   {
   pid_t    pid = 0;
   char     file[BUFSIZ];
   FILE     *fp;

   int      i;
   int      forkvalue;

   const char *configDir;

   if(mode == NWUMPS_DEBUG)
      fprintf(stderr, "%s: Starting the EnvInit.\n", nwumpsTitleStr);

   if(MsgBindDomain(MSG_DOMAIN_NWUMPSD, MSG_DOMAIN_NETMGT_FILE, MSG_NETMGT_REV_STR) != SUCCESS) 
      {
      fprintf(stderr, "%s: Cannot get message catalogue, Error Exit.\n", nwumpsTitleStr);
      nwumpsExit(EXIT_FAILURE);
      }

   if(verbose == TRUE)
      {
      if((configDir = NWCMGetConfigFilePath()) == NULL) 
         {
         fprintf(stderr, MsgGetStr(NWUMPSD_BAD_CONFIG));
         fprintf(stderr, MsgGetStr(NWUMPSD_ERROR_EXIT));
         exit(EXIT_FAILURE);
         }

      fprintf(stderr, MsgGetStr(NWUMPSD_IDENTITY));
      fprintf(stderr, MsgGetStr(NWUMPSD_CONFIGURATION), configDir);
      }

   if(mode == NWUMPS_DEBUG)
      {
      fprintf(stderr, "%s: Checking mode %d.\n", nwumpsTitleStr, mode);
      fprintf(stderr, "%s: Starting the Fork.\n", nwumpsTitleStr);
      }

   forkvalue = (int)fork();

   switch(forkvalue) 
      {
      case NOTOK: 
         if(mode == NWUMPS_DEBUG)
            fprintf(stderr, "%s: Fork not Ok.\n", nwumpsTitleStr);

	 sprintf(errorStr, MsgGetStr(NWUMPSD_FORK_FAIL));
         perror(errorStr);
         fprintf(stderr, MsgGetStr(NWUMPSD_ERROR_EXIT));
         nwumpsExit(EXIT_FAILURE);
   /*NOTREACHED*/

      case OK: 
         if(mode == NWUMPS_DEBUG)
            fprintf(stderr, "%s: Fork Ok.\n", nwumpsTitleStr);

         if(setpgrp() == -1) 
            {
            sprintf(errorStr, MsgGetStr(NWUMPSD_SESSION));
            perror(errorStr);
            fprintf(stderr, MsgGetStr(NWUMPSD_ERROR_EXIT));
            nwumpsExit(EXIT_FAILURE);
      /*NOTREACHED*/
            }

         else if(mode == NWUMPS_DEBUG)
            fprintf(stderr, "%s: setgrp() Ok.\n", nwumpsTitleStr);
 
         if((pid = fork()) < 0) 
            {
            sprintf(errorStr, MsgGetStr(NWUMPSD_FORK_FAIL));
            perror(errorStr);
            fprintf(stderr, MsgGetStr(NWUMPSD_ERROR_EXIT));
            nwumpsExit(EXIT_FAILURE);
          /*NOTREACHED*/
            }
         else if(pid > 0)
            exit(-1);    /* second child */

         umask(022);
      break;

      default:
         if(mode == NWUMPS_DEBUG)
            fprintf(stderr, "%s: Fork default. Fork value: %d.\n", nwumpsTitleStr, forkvalue);
      exit(EXIT_SUCCESS);
      }

   if(mode == NWUMPS_DEBUG)
      fprintf(stderr, "%s: Ending the Fork.\n", nwumpsTitleStr);

   sigignore(SIGCLD);
   sigset(SIGALRM, nwuRefresh);
   alarm(NWUMPS_REFRESH);

   if(mode == NWUMPS_DEBUG)
      fprintf(stderr, "%s: Ending the EnvInit.\n", nwumpsTitleStr);
   }

/* MIB Init */

static  nwuMibInit() 
   {
   OT       ot;
   extern char  *snmp_address;

   char nwumpsPathName[NWCM_MAX_STRING_SIZE]; /* Lowercase Daemon name */
   
   if(mode == NWUMPS_DEBUG)
      fprintf(stderr, "%s: Starting the MibInit.\n", nwumpsTitleStr);

   if((se = NWgetsmuxEntrybyname(nwumpsDaemonName)) == NULL)
      {
      if(mode == NWUMPS_DEBUG)
         fprintf(stderr, MsgGetStr(NWUMPSD_SMUX_PEER_ERROR), nwumpsDaemonName);
      }

   if((Cret = NWCMGetParam(NWUMPS_ETC_DIR, NWCP_STRING, nwumpsPathName)) != SUCCESS)
      {
      NWCMPerror(Cret, MsgGetStr(NWUMPSD_CFG_ERROR), NWUMPS_ETC_DIR);
      return FAILURE;
      }

   strcat(nwumpsPathName, "/");
   strcat(nwumpsPathName, "nwumpsd.defs");

   if(readobjects(nwumpsPathName) == NOTOK)
      {
      fprintf(stderr, MsgGetStr(NWUMPSD_READOBJECT_ERROR));
      fprintf(stderr, MsgGetStr(NWUMPSD_DEF_PATH), nwumpsPathName);
      }

   if(mode == NWUMPS_DEBUG)
      fprintf(stderr, "%s: Starting the Initialization of agent.\n", nwumpsTitleStr);

   for(tc = subgroups; tc->t_tree; tc++)
      if(ot = text2obj(tc->t_tree))
         {
         tc->t_name=ot->ot_name;
         (void)(*tc->t_init)();
         }
      else
         fprintf(stderr, MsgGetStr(NWUMPSD_TEXT2OBJ_FAILED), tc->t_tree);

   if(mode == NWUMPS_DEBUG)
      fprintf(stderr, "%s: Ending the Initialization.\n", nwumpsTitleStr);

    /* Specify the IP address for this SMUX peer.  This IP address
     * must be the same one specified in the smux entry in the
     * /etc/snmpd.conf file.  If this variable is not set, then snmpd
     * will only authenticate the SMUX peer if the SMUX entry in the
     * /etc/snmpd.conf file is 127.0.0.1.  The variable, snmp_address,
     * is an external variable, so snmpd gets the value.  In the libisode.a,
     * the value of snmp_address is "loopback".  You override this value 
     * by setting snmp_address here.
     */
#if 0
   snmp_address = "129.35.129.227";
#endif

   if(mode == NWUMPS_DEBUG)
      fprintf(stderr, "%s: Ending the MibInit.\n", nwumpsTitleStr);
   }

static nwuFDInit()
   {
   int   fd;
   int   status;
   struct t_info  info;

   if(mode == NWUMPS_DEBUG)
      fprintf(stderr, "%s: Starting the FDInit.\n", nwumpsTitleStr);
   
   if(!(polls.pollFds = 
         (struct pollfd *)malloc( sizeof(struct pollfd) * NUM_NWUMPS_FD))) 
      {
      perror("");
      nwumpsExit(EXIT_FAILURE);
      /*NOTREACHED*/
      }

   polls.pollFds[SMUX_NWUMPS_FD].fd = NOTOK;

   if((fd = NWsmux_init(mode)) == NOTOK)
      fprintf(stderr, MsgGetStr(NWUMPSD_SMUX_INIT_ERROR), NWsmux_error(smux_errno), smux_info);
   else
      rock_and_roll = 0;

   ++polls.nElements;
   polls.pollFds[SMUX_NWUMPS_FD].fd = fd;
   polls.pollFds[SMUX_NWUMPS_FD].events = POLLOUT;
   polls.pollFds[SMUX_NWUMPS_FD].revents = 0;

   if(mode == NWUMPS_DEBUG)
      fprintf(stderr, "%s: smux fd = %d in the nwuFDInit.\n", nwumpsTitleStr, 
               polls.pollFds[SMUX_NWUMPS_FD].fd);

   if(ripxFd == -1)
      {
      if((ripxFd = open(ripxDevice, O_RDWR)) < 0)
         {
         sprintf(errorStr, MsgGetStr(NWUMPSD_OPEN_FAIL), ripxDevice);
         perror(errorStr);
         nwumpsExit(EXIT_FAILURE);
   /*NOTREACHED*/
         }

   if(mode == NWUMPS_DEBUG)
      fprintf(stderr, "%s: ripmx_fd = %d.\n", nwumpsTitleStr, ripxFd);
      }

   if((fd = t_open(ipxDevice, O_RDWR, &info)) < 0) 
      {
      sprintf(errorStr, MsgGetStr(NWUMPSD_TOPEN_FAIL), ipxDevice);
      perror(errorStr);
      nwumpsExit(EXIT_FAILURE);
      }

   if(mode == NWUMPS_DEBUG)
       fprintf(stderr, "%s: IPX_fd = %d.\n", nwumpsTitleStr, fd);

   if(t_bind(fd, NULL, NULL) < 0)
      {
      fprintf(stderr, MsgGetStr(NWUMPSD_TBIND_FAIL), ipxDevice);
      t_error("");
      nwumpsExit(EXIT_FAILURE);
      }

   ++polls.nElements;
   polls.pollFds[IPX_NWUMPS_FD].fd = fd;
   ipxFd = fd;
   polls.pollFds[IPX_NWUMPS_FD].events = POLLIN;
   polls.pollFds[IPX_NWUMPS_FD].revents = 0;

   if((Cret = NWCMGetParam(NWUMPS_SERVER_NAME, NWCP_STRING,
      &MyServerName)) != SUCCESS)
      {
      bzero(MyServerName, NWUMPS_NAME_SIZE);
      NWCMPerror(Cret, MsgGetStr(NWUMPSD_CFG_ERROR), NWUMPS_SERVER_NAME);
      }

   nwuGetNetNodeAddress(ipxFd);

   if(spxFd == -1)
      {
      if((spxFd = open(spxDevice, O_RDWR)) < 0)
         {
	 spxFlag = FALSE;
         fprintf(stderr,"SPX Deamon is down.\n");
         }

   if(mode == NWUMPS_DEBUG)
      fprintf(stderr, "%s: spx fd = %d.\n", nwumpsTitleStr, spxFd);
      }

   if((Cret = NWCMGetParam(NWUMPS_DIAG_FLAG, NWCP_BOOLEAN, &diagFlag)) != SUCCESS)
      {
      diagFlag = FALSE;
      NWCMPerror(Cret, MsgGetStr(NWUMPSD_CFG_ERROR), NWUMPS_DIAG_FLAG);
      }

   if(diagFlag == FALSE)
      fprintf(stderr,"Diagnostic Deamon is down.\n");

   if((status = MapSAPMemory()) < 0)
      {
      sapFlag = FALSE;
      fprintf(stderr,"SAP Deamon is down.\n");
      }

   if(mode == NWUMPS_DEBUG)
      fprintf(stderr, "%s: Ending the FDInit.\n", nwumpsTitleStr);
   }

/* SMUX */

static void start_smux(void) 
   {
   int index;
   struct NWtype_SNMP_SMUX__PDUs *event;

   if(NWsmux_simple_open(&se->se_identity, nwumpsTitleStr,
         se->se_password, strlen(se->se_password)) == NOTOK)
      {
      if(smux_errno == inProgress)
         return;

losing: 
      ;
      polls.pollFds[SMUX_NWUMPS_FD].fd = NOTOK;
      fprintf(stderr, MsgGetStr(NWUMPSD_SMUX_SIMPLE_OPEN_FAILED), 
		NWsmux_error(smux_errno), smux_info);
      return;
      }

   if(mode == NWUMPS_DEBUG)
   	fprintf(stderr, MsgGetStr(NWUMPSD_SMUX_SIMPLE_OPEN), 
		oid2ode(&se->se_identity), se->se_name);

   rock_and_roll = 1;

   for(tc = subgroups; tc->t_tree; tc++)
      if(tc->t_name)
         {
         if(NWsmux_register(tc->t_name, -1, tc->t_access) == NOTOK) 
            {
            fprintf(stderr, MsgGetStr(NWUMPSD_SMUX_REGISTER), NWsmux_error(smux_errno), smux_info);

            goto losing;
            }


        /* Waiting for answer */
         smux_wait(&event, 300);

         if(event->offset == NWtype_SNMP_SMUX__PDUs_registerResponse)
           {
           struct type_SNMP_RRspPDU *rsp = event->un.registerResponse;

           if(rsp->parm == int_SNMP_RRspPDU_failure)
             fprintf(stderr, "%s: smux_register failed.%s\n", nwumpsTitleStr, 
			tc->t_tree);
           }
         else
          fprintf(stderr, "%s: smux_register failed %s.\n", nwumpsTitleStr, 
			tc->t_tree);
         }

   if(smux_trap(int_SNMP_generic__trap_coldStart,
                 0, (struct type_SNMP_VarBindList *) 0) == NOTOK)
     fprintf(stderr, "%s: smux_trap %s, %s.\n", nwumpsTitleStr,
            smux_error(smux_errno), smux_info);

   for(index = 0; index < NumLans; index ++)
      {
      bzero(IPXCircIndex, 24);
      sprintf(IPXCircIndex, "ipxCircIndex.%d", index);

   /* Currently there is only one instance of IPX. This will need to be 
      changed if multiple IPX are to be supported */

      nwumpsSendTrap(NWUMPS_IPX_TRAP_CIRCUIT_UP,
                        "ipxCircSysInstance.0",
                        0,
                        IPXCircIndex,
                        index,
                        NULL);
      }

   }

static void doit_smux(void)
   {
   struct NWtype_SNMP_SMUX__PDUs *event;

   if(mode == NWUMPS_DEBUG)
       fprintf(stderr, "%s: doit_smux() calling NWsmux_wait().\n", nwumpsTitleStr);

   if(NWsmux_wait(&event, NOTOK) == NOTOK)
      {
      if(smux_errno == inProgress)
         return;

      fprintf(stderr, MsgGetStr(NWUMPSD_SMUX_WAIT), NWsmux_error(smux_errno), smux_info);

losing: 
     ;
      polls.pollFds[SMUX_NWUMPS_FD].fd = NOTOK;
      return;
      }

   switch(event->offset)
      {
      case NWtype_SNMP_SMUX__PDUs_get__request:
         if(mode == NWUMPS_DEBUG)
             fprintf(stderr, "%s: doit_smux() receive a get request.\n", 
                     nwumpsTitleStr);

         get_smux(event->un.get__request, event->offset);
      break;

      case NWtype_SNMP_SMUX__PDUs_get__next__request:
         if(mode == NWUMPS_DEBUG)
             fprintf(stderr, "%s: doit_smux() receive a get_next request.\n", 
                     nwumpsTitleStr);

         get_smux(event->un.get__request, event->offset);
      break;

      case NWtype_SNMP_SMUX__PDUs_close:
      	 fprintf(stderr, MsgGetStr(NWUMPSD_SMUX_CLOSE), NWsmux_error(event->un.close->parm));

      goto losing;

      case NWtype_SNMP_SMUX__PDUs_registerResponse:
      case NWtype_SNMP_SMUX__PDUs_simple:
      case NWtype_SNMP_SMUX__PDUs_registerRequest:
      case NWtype_SNMP_SMUX__PDUs_get__response:
      case NWtype_SNMP_SMUX__PDUs_trap:
unexpected: ;
         fprintf(stderr, MsgGetStr(NWUMPSD_SMUX_UNEXPECTED_OPERATIONS), event->offset);

         (void) NWsmux_close(protocolError);
      goto losing;


      case NWtype_SNMP_SMUX__PDUs_set__request:
      case NWtype_SNMP_SMUX__PDUs_commitOrRollback:
         if(mode == NWUMPS_DEBUG)
             fprintf(stderr, "%s: doit_smux() receive a set request.\n", 
                     nwumpsTitleStr);

         set_smux(event);
      break;

      default:
         fprintf(stderr, MsgGetStr(NWUMPSD_SMUX_BAD_OPERATIONS), event->offset);
         (void) NWsmux_close(protocolError);
      goto losing;
      }
   }

static void get_smux(register struct type_SNMP_GetRequest__PDU *pdu, int offset)
   {
   int   idx;
   int   status;

   object_instance ois;
   register struct type_SNMP_VarBindList *vp;

   idx = 0;

   for(vp = pdu->variable__bindings; vp; vp = vp->next)
      {
      register OI oi;
      register OT ot;
      register struct type_SNMP_VarBind *v = vp->VarBind;

      idx++;

      if(offset == NWtype_SNMP_SMUX__PDUs_get__next__request) 
         {
         if(mode == NWUMPS_DEBUG)
             fprintf(stderr, "%s: get_smux() receive a get_next request.\n", nwumpsTitleStr);

         if((oi = name2inst(v->name)) == NULLOI
               && (oi = next2inst(v->name)) == NULLOI)
            {
            if(mode == NWUMPS_DEBUG)
               fprintf(stderr, "%s: get_smux() goto no_name.\n", nwumpsTitleStr);

            goto no_name;
            }

         if((ot = oi->oi_type)->ot_getfnx == NULLIFP)
            {
            if(mode == NWUMPS_DEBUG)
               fprintf(stderr, "%s: get_smux() goto get_next.\n", nwumpsTitleStr);

            goto get_next;
            }
         }
      else if((oi = name2inst(v->name)) == NULLOI
               || (ot = oi->oi_type)->ot_getfnx == NULLIFP)
         {
no_name: ;
         pdu->error__status = int_SNMP_error__status_noSuchName;
         goto out;
         }

try_again: ;
      switch(ot->ot_access) 
         {
         case OT_NONE:
            if(mode == NWUMPS_DEBUG)
               fprintf(stderr, "%s: get_smux() OT_NONE.\n", nwumpsTitleStr);

            if(offset == NWtype_SNMP_SMUX__PDUs_get__next__request)
               goto get_next;
         goto no_name;

         case OT_RDONLY:
            if(mode == NWUMPS_DEBUG)
               fprintf(stderr, "%s: get_smux() OT_RDONLY.\n", nwumpsTitleStr);

            if(offset == NWtype_SNMP_SMUX__PDUs_set__request)
               {
               pdu->error__status = int_SNMP_error__status_noSuchName;
               goto out;
               }
         break;

         case OT_RDWRITE:
            if(mode == NWUMPS_DEBUG)
               fprintf(stderr, "%s: get_smux() OT_RDWRITE.\n", nwumpsTitleStr);
         break;
         }
      
      switch(status = (*ot->ot_getfnx) (oi, v, offset)) 
         {
         case NOTOK:       /* get-next wants a bump */
get_next: ;
            oi = &ois;
            for(;;) 
               {
               if((ot = ot->ot_next) == NULLOT) 
                  {
                  pdu->error__status = int_SNMP_error__status_noSuchName;
                  goto out;
                  }

               oi->oi_name = (oi->oi_type = ot)->ot_name;

               if(ot->ot_getfnx)
                  goto try_again;
               }

         case int_SNMP_error__status_noError:
         break;

         default:
            pdu->error__status = status;
         goto out;
         }
      }

   idx = 0;

out: ;
   pdu->error__index = idx;

   if(NWsmux_response(pdu) == NOTOK) 
      {
      fprintf(stderr, MsgGetStr(NWUMPSD_SMUX_RESPONSE), NWsmux_error(smux_errno), smux_info);
      polls.pollFds[SMUX_NWUMPS_FD].fd = NOTOK;
      }
   }

static void set_smux(struct type_SNMP_SMUX__PDUs *event)
   {
   struct type_SNMP_VarBindList *vp;
   register struct type_SNMP_GetRequest__PDU *pdu = event->un.get__response;

   for(vp = event->un.set__request->variable__bindings; vp; vp = vp->next)
      {
      switch(event->offset)
        {
        case SMUX__PDUs_set__request:
            {
            OI oi;
            OT ot;
            if((oi = name2inst (vp->vb_ptr->name)) == NULLOI)
                pdu->error__status = error__status_noSuchName;
            else
                {
                ot = oi->oi_type;

                if(ot->ot_setfnx)
                   pdu->error__status = (ot->ot_setfnx)(oi, vp->vb_ptr, event->offset);
                else
                  pdu->error__status = error__status_noSuchName;
                }

            if(smux_response(pdu) == NOTOK)
                {
                fprintf(stderr, MsgGetStr(NWUMPSD_SMUX_RESPONSE), NWsmux_error(smux_errno), smux_info);
                polls.pollFds[SMUX_NWUMPS_FD].fd = NOTOK;
                }
            }
        break;
        }
      }
   }

/*********************************************************************/
/* PollForEvents: Poll all active file descriptors for events. When
 * something interesting happens on a given fd, respond appropriately.
 *********************************************************************/
void PollForEvents(int timeout)
   {  
   int index;

   int   heard;

#ifdef HARDDEBUG
   int i;
#endif

POLL_AGAIN:
   if((heard = poll(polls.pollFds, polls.nElements, timeout)) == NOTOK)
      {
      if(errno == EINTR)
         goto POLL_AGAIN;

      fprintf(stderr, MsgGetStr(NWUMPSD_POLL), heard);
      nwumpsExit(EXIT_FAILURE);
      /*NOTREACHED*/
      }

#ifdef HARDDEBUG
   for(i = 0; i < polls.nElements; i++) 
      {
      if(polls.pollFds[i].revents & POLLERR)
         fprintf(stderr, "[%d].revents & POLLERR\n", i);

      if(polls.pollFds[i].revents & POLLHUP)
         fprintf(stderr, "[%d].revents & POLLHUP\n", i);

      if(polls.pollFds[i].revents & POLLNVAL)
         fprintf(stderr, "[%d].revents & POLLNVAL\n", i);
      else
         fprintf(stderr, "[%d].revents = 0x%X   [%d].fd = %d\n",
               i, polls.pollFds[i].revents, i, polls.pollFds[i].fd);
      }
#endif

      /* check fd's for other purposes here... */

   if(polls.pollFds[SMUX_NWUMPS_FD].fd == NOTOK && !dont_bother_anymore) 
      {
      if(heard == 0)
         {
         if((polls.pollFds[SMUX_NWUMPS_FD].fd = NWsmux_init(mode)) == NOTOK)
            {
            fprintf(stderr, MsgGetStr(NWUMPSD_POLL_SMUX), polls.pollFds[SMUX_NWUMPS_FD].fd);
            }
         else
            rock_and_roll = 0;
         }
      }
   else if(rock_and_roll)
      {
      if(polls.pollFds[SMUX_NWUMPS_FD].revents & POLLIN) 
         {
         if(mode == NWUMPS_DEBUG)
             fprintf(stderr, "%s: PollForEvents() The read bit has been set for the smux fd. doit_smux!!\n", 
                     nwumpsTitleStr);
         doit_smux();
         }
      }
   else if(polls.pollFds[SMUX_NWUMPS_FD].revents & POLLOUT)
      {
      if(mode == NWUMPS_DEBUG)
         fprintf(stderr, "%s: PollForEvents() The write bit has been tested for the smux fd. start_smux!!\n",
                  nwumpsTitleStr);
      start_smux();
      }

   /* If HANGUP we are done */

   if(polls.pollFds[IPX_NWUMPS_FD].revents & POLLHUP) 
      {
      if(mode == NWUMPS_DEBUG)
         fprintf(stderr, "%s: Received hangup signal.\n");

      fprintf(stderr, MsgGetStr(NWUMPSD_HANGUP_SIG));
   
      for(index = 0; index < NumLans; index ++)
         {
         bzero(IPXCircIndex, 24);
         sprintf(IPXCircIndex, "ipxCircIndex.%d", index);

   /* Currently there is only one instance of IPX. This will need to be 
      changed if multiple IPX are to be supported */

         nwumpsSendTrap(NWUMPS_IPX_TRAP_CIRCUIT_DOWN,
                           "ipxCircSysInstance.0",
                           0,
                           IPXCircIndex,
                           index,
                           NULL);
         }

      nwumpsExit(EXIT_SUCCESS);
      }

   return;
   }

void nwumpsExit(int status)
   {
   if(status) 
      {
      fprintf(stderr, MsgGetStr(NWUMPSD_ERROR_EXIT));
      } 
   else 
      fprintf(stderr, MsgGetStr(NWUMPSD_NORMAL_EXIT));

   exit(status);
   }

void nwuRefresh(int sig)
   {
   NumRouters = nwuIPXGetRouterTable();

   if(sapFlag == TRUE)
      {
      SAPStats = ShmBase->D;

      nwuIPXGetServerTable(&NumServers);
      nwuIPXGetDestServTable(&NumDestServ);
      }

   if(spxFlag == TRUE)
      NumSPXConnections = nwuSPXGetNumConn();

   if(mode == NWUMPS_DEBUG)
      {
      time_t   CurrentTime;

      time(&CurrentTime);
      fprintf(stderr, "%s: Refreshing Data %s.\n", nwumpsTitleStr, ctime(&CurrentTime));

      printf("Refresh Number of Routers: %d.\n", NumRouters);
      printf("Refresh Number of Servers: %d.\n", NumServers);
      printf("Refresh Number of Destination Servers: %d.\n", NumDestServ);
      printf("Refresh Number of SPX Connections %d.\n", NumSPXConnections);
      }

   alarm(NWUMPS_REFRESH);
   }
/******************************************************************/
static int SetUpMappedSAPMemory()
   {
   const char *configDir;

   if(shmid == -1) 
      {
      if((configDir = NWCMGetConfigFilePath()) == NULL) 
         {
         return(-SAPL_NWCM);
         }

      if((shmkey = ftok(configDir, SAP_PACKET_TYPE)) == -1) 
         {
         return(-SAPL_FTOK);
         }

      if((shmid = shmget(shmkey, 0, 0444)) == -1) 
         {
         return(-SAPL_SHMGET);
         }
      }

   if((ShmBase = (SapShmHdr_t *)shmat(shmid, NULL, SHM_RDONLY)) == (void *)-1)
      {
      return(-SAPL_SHMAT);
      }

   SAPStats = ShmBase->D;
   SrvBase = (ServerEntry_t *)((char *)ShmBase + ShmBase->ServerPool);

   return(0);
   }

/******************************************************************/
/*
** The MapSAPMemory function attaches to shared memory
*/
int MapSAPMemory(void)
   {
   int ret;

   if(MemoryMapped == FALSE)
      {
      if(ShmBase == NULL)
         {
         ret = SetUpMappedSAPMemory();

         if(ret)  
            {
            return(ret);
            } 
         else 
            {
            MemoryMapped = TRUE;
            }
         }
      }

   return(0);
   }

/******************************************************************/
/*
** The UnMapSAPMemory function detaches from shared memory
*/
void UnMapSAPMemory(void)
   {
   if(ShmBase == NULL) 
      {
      return;
      }

   shmdt((char *)ShmBase);
   ShmBase = NULL;
   MemoryMapped = FALSE;

   return;
   }

int nwuGetNetNodeAddress(int ipxfd)
   {
   struct   strioctl ioc;

   int      status;

   if(mode == NWUMPS_DEBUG)
      printf("%s: Setting up ioc structure for IPX_GET_NET.\n", nwumpsTitleStr);

   ioc.ic_cmd = IPX_GET_NET;
   ioc.ic_timout = 3;
   ioc.ic_len = sizeof(MyIPXNetAddr);
   ioc.ic_dp = (char *)&MyIPXNetAddr;

   if(mode == NWUMPS_DEBUG)
      printf("%s: Doing ioctl IPX_GET_NET.\n", nwumpsTitleStr);

   if(ioctl(ipxfd, I_STR, &ioc) < 0)
      {
      sprintf(errorStr, MsgGetStr(NWUMPSD_IOCTL_TO_FAILED), nwumpsTitleStr,
                  "IPX_GET_NET", ipxDevice, MyIPXNetAddr);
      perror(errorStr);
      return FAILURE;
      }

   if(mode == NWUMPS_DEBUG)
      printf("%s: Setting up ioc structure for IPX_GET_NODE_ADDR.\n", nwumpsTitleStr);

   ioc.ic_cmd = IPX_GET_NODE_ADDR;
   ioc.ic_timout = 3;
   ioc.ic_len = sizeof(MyIPXNodeAddr);
   ioc.ic_dp = (char *)&MyIPXNodeAddr;

   if(mode == NWUMPS_DEBUG)
      printf("%s: Doing ioctl IPX_GET_NODE_ADDR.\n", nwumpsTitleStr);

   if(ioctl(ipxfd, I_STR, &ioc) < 0)
      {
      sprintf(errorStr, MsgGetStr(NWUMPSD_IOCTL_TO_FAILED), nwumpsTitleStr,
                  "IPX_GET_NODE_ADDR", ipxDevice, MyIPXNodeAddr);
      perror(errorStr);
      return FAILURE;
      }

   return(SUCCESS);
   }


void nwumpsSendTrap(int trap_number, ... )
   {
   va_list ap;
   char * text;
   int value;

   struct type_SNMP_VarBindList *bind_list=NULL;
   struct type_SNMP_VarBindList *temp_bind_list=NULL;
   struct type_SNMP_VarBind    *vb;
   OI oi;
  
   va_start(ap, trap_number);

   while(text = va_arg(ap, char *))
      {
      /* 
       * Create a variable binding list structure to build up a list of 
       * variable bindings.
       */
      if(bind_list)
	{
	temp_bind_list->next = (struct type_SNMP_VarBindList *)
	    	calloc(1, sizeof(struct type_SNMP_VarBindList));
	temp_bind_list = temp_bind_list->next;
	}
      else
	{
	bind_list = temp_bind_list = (struct type_SNMP_VarBindList *)
	    	calloc(1, sizeof(struct type_SNMP_VarBindList));
	}

      /* 
       * Create a variable binding structure for the variable and hook this 
       * VarBind into the VarBindList.
       */
      vb = (struct type_SNMP_VarBind *) 
		calloc(1, sizeof(struct type_SNMP_VarBind));

      temp_bind_list->VarBind = vb;

      /*
       * Create an object instance for the first variable.
       * Put the name of this variable into the name of the variable binding 
       * structure.
       */
      
      oi = text2inst(text);

      vb->name = oid_cpy(oi->oi_name);
      
      vb->value = (struct type_SNMP_ObjectSyntax *) 
		calloc(1, sizeof(struct type_SNMP_ObjectSyntax));
      
      /*
       * Put the value of the variable into the variable binding structure.
       */

      if((strcmp(oi->oi_type->ot_syntax->os_name,"DisplayString")==0) ||
	  (strcmp(oi->oi_type->ot_syntax->os_name,"OctetString")==0) ||
	  (strcmp(oi->oi_type->ot_syntax->os_name,"PhysAddress")==0))
	{
	static char * value;
	value = va_arg(ap, char *);
	o_string(oi, vb, value, strlen(value));
	}
      else
	{
	static int value;
	value = va_arg(ap, int);
	o_integer(oi, vb, value);
	}
    }
  
   if(smux_trap(int_SNMP_generic__trap_enterpriseSpecific, trap_number, 
		bind_list) == NOTOK)
      fprintf(stderr, MsgGetStr(NWUMPSD_SMUX_TRAP), NWsmux_error(smux_errno), smux_info);
  
   if(bind_list)
      free_SNMP_VarBindList(bind_list);

   va_end(ap);
   return;
   }
