/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/message.c	1.7"
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<string.h>
#include	<malloc.h>
#include	<time.h>
#include	<sys/types.h>
#include	<sys/wait.h>
#include	<mail/list.h>
#include	<mail/table.h>

#define	MESSAGE_OBJ

#include	"smtpType.h"

typedef struct badAddr_s
    {
    struct badAddr_s
	*ba_next;

    char
	*ba_address,
	*ba_machine,
	*ba_error;
    }	badAddr_t;

typedef struct message_s
    {
    table_t
	*msg_machines;

    badAddr_t
	*msg_badAddrs;

    time_t
	msg_timeStamp;

    list_t
	*msg_laterRecipients;

    int
	msg_rank,
	msg_useDns,
	msg_retry,
	msg_inFree,
	msg_nlinks;

    char
	*msg_sender,
	*msg_controlPath,
	*msg_dataPath;
    }	message_t;

#include	"smtp.h"

extern char
    DomainName[];

static table_t
    *MessageTable;

static char
*stripHost(char *address)
    {
#if 1
    static char
	result[512];

    char
	*atsign,
	*p;

    if(address == NULL || *address == '\0')
	{
	*result = '\0';
	}
    else if(*address != '@')
	{
	if((atsign = strchr(address, '@')) != NULL)
	    {
	    *atsign = '\0';
	    strncpy(result, address, sizeof(result));
	    *atsign = '@';
	    }
	else
	    {
	    strncpy(result, address, sizeof(result));
	    }
	}
    else
	{
	for
	    (
	    p = address;
	    *p != ',' && *p != ':' && p[1] != '\0';
	    p++
	    );
	
	strncpy(result, p + 1, sizeof(result));
	}

    return(result);
#else
    return(address);
#endif
    }

static void
messageReturnError(message_t *msg_p)
    {
    char
	*tmpFilePath,
	*argVector[32];

    int
	argIndex,
	status;

    pid_t
	pid,
	waitPid;

    badAddr_t
	*curAddr_p;

    FILE
	*fp_tmp;

    if((tmpFilePath = tmpnam(NULL)) == NULL)
	{
	/* ERROR No File. */
	}
    else if((fp_tmp = fopen(tmpFilePath, "w")) == NULL)
	{
	perror(tmpFilePath);
	}
    else
	{
	while((curAddr_p = msg_p->msg_badAddrs) != NULL)
	    {
	    (void) fprintf
		(
		fp_tmp,
		"%s:%s:%s\n",
		curAddr_p->ba_address,
		curAddr_p->ba_machine,
		curAddr_p->ba_error
		);

	    msg_p->msg_badAddrs = curAddr_p->ba_next;
	    if(curAddr_p->ba_address != NULL) free(curAddr_p->ba_address);
	    if(curAddr_p->ba_error != NULL) free(curAddr_p->ba_error);
	    if(curAddr_p->ba_machine != NULL) free(curAddr_p->ba_machine);
	    free(curAddr_p);
	    }

	(void) fclose(fp_tmp);
	if((pid = fork()) == 0)
	    {
	    /* Child process */
	    argIndex = 0;
	    argVector[argIndex++] = "mailWrapper";
	    argVector[argIndex++] = "-f";
	    argVector[argIndex++] = "Postmaster";
	    argVector[argIndex++] = "-r";
	    argVector[argIndex++] = msg_p->msg_sender;
	    
	    argVector[argIndex++] = tmpFilePath;
	    argVector[argIndex++] = msg_p->msg_dataPath;
	    argVector[argIndex++] = NULL;

	    (void) execv("/usr/lib/mail/surrcmd/mailWrapper", argVector);
	    /*perror("EXECV RETURNED!!!!!");*/
	    exit(1);
	    }
	else while((waitPid = wait(&status)) >= 0)
	    {
	    /* Wait for mailer wrapper to exit and send response. */
	    if(waitPid != pid)
		{
		/* Some other child.  STRANGE There are no others!!!! */
		}
	    else if(!WIFEXITED(status))
		{
		/* Not a normal Exit */
		}
	    else if(WEXITSTATUS(status) == 0)
		{
		(void) unlink(tmpFilePath);
		break;
		}
	    else
		{
		(void) unlink(tmpFilePath);
		break;
		}
	    }
	}
    }

void
messageFree(message_t *msg_p)
    {
    FILE
	*fp_control;

    char
	buffer[256],
	*recipient;

    if(DebugLevel > 4)
	{
	(void) fprintf(stderr, "messageFree(0x%x) Entered.\n", (int) msg_p);
	}

    if(msg_p == NULL)
	{
	/* Leave it alone */
	}
    else if(--msg_p->msg_nlinks > 0)
	{
	/* Leave it alone */
	}
    else if(tableDeleteEntryByValue(MessageTable, msg_p))
	{
	}
    else if(msg_p->msg_inFree++)
	{
	}
    else if(msg_p->msg_retry)
	{
	if((fp_control = fopen(msg_p->msg_controlPath, "w")) == NULL)
	    {
	    perror(msg_p->msg_controlPath);
	    }
	else
	    {
	    (void) fprintf(fp_control, "RANK\t%d\n", msg_p->msg_rank + 1);
	    (void) fprintf(fp_control, "DNS\t%d\n", msg_p->msg_useDns);
	    (void) fprintf(fp_control, "DATA\t%s\n", msg_p->msg_dataPath);
	    (void) fprintf
		(
		fp_control,
		"SENDER\t%s\n",
		stripHost(msg_p->msg_sender)
		);

	    (void) fprintf
		(
		fp_control,
		"TIMESTAMP\t%d\n",
		(int) msg_p->msg_timeStamp
		);

	    while(!listGetNext(msg_p->msg_laterRecipients, &recipient))
		{
		(void) fprintf(fp_control, "RCPT\t%s\n", recipient);
		free(recipient);
		}

	    (void) fclose(fp_control);
	    }

	if(msg_p->msg_badAddrs != NULL) messageReturnError(msg_p);
	if(msg_p->msg_controlPath != NULL) free(msg_p->msg_controlPath);
	if(msg_p->msg_dataPath != NULL) free(msg_p->msg_dataPath);
	if(msg_p->msg_sender != NULL) free(msg_p->msg_sender);
	if(msg_p->msg_machines) tableFree(msg_p->msg_machines);

	free(msg_p);
	}
    else
	{
	if(msg_p->msg_badAddrs != NULL) messageReturnError(msg_p);
	if(msg_p->msg_controlPath != NULL)
	    {
	    sprintf
		(
		buffer,
		"control file %s successfully finished.",
		msg_p->msg_controlPath
		);

	    doLog(buffer);
	    (void) unlink(msg_p->msg_controlPath);
	    free(msg_p->msg_controlPath);
	    }

	if(msg_p->msg_dataPath != NULL)
	    {
	    (void) unlink(msg_p->msg_dataPath);
	    free(msg_p->msg_dataPath);
	    }

	if(msg_p->msg_sender != NULL) free(msg_p->msg_sender);
	if(msg_p->msg_machines) tableFree(msg_p->msg_machines);

	free(msg_p);
	}

    if(DebugLevel > 4) (void) fprintf(stderr, "messageFree() Exited.\n");
    }

int
messageRcptAdd(message_t *msg_p, char *machine, char *recipient)
    {
    msgConn_t
	*msgConn_p;

    int
	result;

    char
	*address,
	*atSign_p;

    if(DebugLevel > 4)
	{
	(void) fprintf(stderr, "messageRcptAdd(0x%x, %s) Entered.\n", (int) msg_p, recipient);
	}

    if(machine != NULL)
	{
	address = recipient;
	}
    else if(*recipient == '@')
	{
	machine = strtok(recipient + 1, ":,");
	address = strtok(NULL, "\r\n");
	}
    else if((atSign_p = strchr(recipient, '@')) == NULL)
	{
	/* No Machine in address */
	machine = NULL;
	address = recipient;
	}
    else
	{
	address = recipient;
	*atSign_p++ = '\0';
	machine = atSign_p;
	}

    if(machine == NULL)
	{
	result = 1;
	}
    else if
	(
	    (
	    msgConn_p = msgConnNew(machine, msg_p->msg_machines, msg_p)
	    ) == NULL
	)
	{
	messageBadRecipient
	    (
	    msg_p,
	    machine,
	    address,
	    "No such machine"
	    );

	result = 1;
	}
    else
	{
	result = 0;
	msgConnAddRecipient(msgConn_p, address);
	}

    if(DebugLevel > 4) (void) fprintf(stderr, "messageRcptAdd()  = %d Exited.\n", result);
    return(result);
    }

message_t
*messageNew(char *controlPath)
    {
    FILE
	*fp = NULL;

    message_t
	*result;
    
    int
	allBadRecips = 1;

    char
	buffer[256],
	*systemName = NULL,
	*name,
	*data;

    if(DebugLevel > 4)
	{
	(void) fprintf(stderr, "messageNew(%s) Entered.\n", controlPath);
	}

    if(MessageTable == NULL) MessageTable = tableNew();

    if(controlPath == NULL)
	{
	result = NULL;
	}
    else if
	(
	    (
	    result = (message_t *)tableGetValueByString
		(
		MessageTable,
		controlPath
		)
	    ) != NULL
	)
	{
	/* Already being processed */
	allBadRecips = 0;
	}
    else if((fp = fopen(controlPath, "r")) == NULL)
	{
	perror(controlPath);
	result = NULL;
	}
    else if((result = (message_t *)calloc(sizeof(*result), 1)) == NULL)
	{
	}
    else if((result->msg_controlPath = strdup(controlPath)) == NULL)
	{
	messageFree(result);
	result = NULL;
	}
    else if((result->msg_laterRecipients = listNew()) == NULL)
	{
	messageFree(result);
	result = NULL;
	}
    else if((result->msg_machines = tableNew()) == NULL)
	{
	messageFree(result);
	result = NULL;
	}
    else
	{
	sprintf(buffer, "control file %s being processed.", controlPath);
	doLog(buffer);
	while(result != NULL && fgets(buffer, sizeof(buffer), fp) != NULL)
	    {
	    if((name = strtok(buffer, "\r\n\t ")) == NULL)
		{
		}
	    else if((data = strtok(NULL, "\r\n\t ")) == NULL)
		{
		}
	    else if(*name == '#' || *name == ';')
		{
		}
	    else if(!strcmp(name, "DATA"))
		{
		if((result->msg_dataPath = strdup(data)) == NULL)
		    {
		    if(DebugLevel > 0) (void) fprintf(stderr, "\tdata not duped.\n");
		    messageFree(result);
		    result = NULL;
		    }
		}
	    else if(!strcmp(name, "TIMESTAMP"))
		{
		result->msg_timeStamp = atoi(data);
		}
	    else if(!strcmp(name, "SYSTEM"))
		{
		systemName = strdup(data);
		}
	    else if(!strcmp(name, "RCPT"))
		{
		allBadRecips = (messageRcptAdd(result, systemName, data))?
		    allBadRecips:
		    0;
		}
	    else if(!strcmp(name, "SENDER"))
		{
		if
		    (
			(
			result->msg_sender = malloc
			    (
			    strlen(data) + strlen(DomainName) + 3
			    )
			) == NULL
		    )
		    {
		    if(DebugLevel > 0) (void) fprintf(stderr, "\tsender not duped.\n");
		    messageFree(result);
		    result = NULL;
		    }
		else if(strchr(data, '@') != NULL)
		    {
		    strcpy(result->msg_sender, "@");
		    strcat(result->msg_sender, DomainName);
		    strcat(result->msg_sender, (*data == '@')? ",": ":");
		    strcat(result->msg_sender, data);
		    }
		else
		    {
		    strcpy(result->msg_sender, data);
		    strcat(result->msg_sender, "@");
		    strcat(result->msg_sender, DomainName);
		    }
		}
	    else if(!strcmp(name, "DNS"))
		{
		result->msg_useDns = atoi(data);
		}
	    else if(!strcmp(name, "RANK"))
		{
		result->msg_rank = atoi(data);
		}
	    }

	if(DebugLevel > 5) (void) fprintf(stderr, "\tFinished with file.\n");
	if(result != NULL)
	    {
	    if(DebugLevel > 5)
		{
		(void) fprintf
		    (
		    stderr,
		    "\tMessageTable = 0x%x, controlPath = %s, result = 0x%x.\n",
		    (int) MessageTable,
		    controlPath,
		    (int) result
		    );
		}

	    tableAddEntry(MessageTable, controlPath, result, messageFree);
	    }

	if(DebugLevel > 5) (void) fprintf(stderr, "\tAdded to table.\n");
	if(systemName != NULL) free(systemName);
	if(DebugLevel > 5) (void) fprintf(stderr, "\tFreed system name.\n");

	tableDoForEachEntry(result->msg_machines, msgConnDispatch, NULL);
	}

    if(fp != NULL) (void) fclose(fp);

    if(result == NULL)
	{
	}
    else if
	(
	result->msg_sender == NULL
	    || result->msg_dataPath == NULL
	    || allBadRecips
	)
	{
	if(DebugLevel > 5)
	    {
	    (void) fprintf
		(
		stderr,
		"\tsender = %s, dataPath = %s, badRecips = %d.\n",
		result->msg_sender,
		result->msg_dataPath,
		allBadRecips
		);
	    }

	messageFree(result);
	result = NULL;
	}
    
    if(DebugLevel > 4) (void) fprintf(stderr, "messageNew()  = 0x%x Exited.\n", (int) result);
    return(result);
    }

void
messageBadRecipient(message_t *msg_p, char *machine, char *address, char *error)
    {
    badAddr_t
	*badAddr_p;
    
    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "messageBadRecipient(0x%x, %s, %s, %s) Entered.\n",
	    (int) msg_p,
	    machine,
	    address,
	    error
	    );
	}

    if((badAddr_p = (badAddr_t *)calloc(sizeof(*badAddr_p), 1)) == NULL)
	{
	free(error);
	free(address);
	free(machine);
	}
    else
	{
	badAddr_p->ba_address = address;
	badAddr_p->ba_error = error;
	badAddr_p->ba_machine = machine;
	badAddr_p->ba_next = msg_p->msg_badAddrs;
	msg_p->msg_badAddrs = badAddr_p;
	}

    if(DebugLevel > 4) (void) fprintf(stderr, "messageBadRecipient() Exited.\n");
    }

void
messageLink(message_t *msg_p)
    {
    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "messageLink(0x%x) Entered.\n",
	    (int) msg_p
	    );
	}

    if(msg_p != NULL)
	{
	msg_p->msg_nlinks++;
	}

    if(DebugLevel > 4) (void) fprintf(stderr, "messageLink() Exited.\n");
    }

void
messageLaterRecipientAdd(message_t *msg_p, char *recipient)
    {
    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "messageLaterRecipient(0x%x, %s) Entered.\n",
	    (int) msg_p,
	    recipient
	    );
	}

    listAdd(msg_p->msg_laterRecipients, recipient);
    msg_p->msg_retry = 1;
    if(DebugLevel > 4) (void) fprintf(stderr, "messageLaterRecipientAdd() Exited.\n");
    }

int
messageRank(message_t *msg_p)
    {
    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "messageRank(0x%x) Entered/Exited.\n",
	    (int) msg_p
	    );
	}

    return((msg_p == NULL)? 0: msg_p->msg_rank);
    }

char
*messageSender(message_t *msg_p)
    {
    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "messageSender(0x%x) Entered/Exited.\n",
	    (int) msg_p
	    );
	}

    return((msg_p == NULL)? NULL: msg_p->msg_sender);
    }

char
*messageDataPath(message_t *msg_p)
    {
    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "messageDataPath(0x%x) Entered/Exited.\n",
	    (int) msg_p
	    );
	}

    return((msg_p == NULL)? NULL: msg_p->msg_dataPath);
    }

int
    messageUseDns(message_t *msg_p)
	{
	return(msg_p->msg_useDns);
	}
