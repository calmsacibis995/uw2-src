/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtpd/command.c	1.2"
#include	<stdio.h>
#include	<malloc.h>
#include	<string.h>
#include	<mail/list.h>
#include	<mail/link.h>
#include	<mail/table.h>

#include	"smtp.h"
#include	"help.h"

extern void
    setTimeout(connData_t *conn_p, int delay);

static int
    DebugLevel = 0;

void
    *tableNew();

typedef struct cmd_s
    {
    void
	*cmd_help;
    char
	*cmd_name,
	cmd_allow[(int)sss_end];
    int
	cmd_wordCount;
    smtpServerState_t
	(*cmd_error)(),
	(*cmd_func)();
    }	cmd_t;

static void
    cmdFree(cmd_t *cmd_p)
	{
	void
	    *curLink_p;

	if(cmd_p != NULL)
	    {
	    if(cmd_p->cmd_name != NULL) free(cmd_p->cmd_name);
	    if(cmd_p->cmd_help != NULL)
		{
		while
		    (
		    (curLink_p = linkNext(cmd_p->cmd_help)) != cmd_p->cmd_help
		    )
		    {
		    free((char *)linkOwner(curLink_p));
		    linkFree(curLink_p);
		    }

		linkFree(cmd_p->cmd_help);
		}

	    free(cmd_p);
	    }
	}

static void
    *CmdTable = NULL;

static cmd_t
    *cmdNew
	(
	char *name,
	smtpServerState_t (*func)(),
	smtpServerState_t (*errorFunc)(),
	int wordCount,
	char *help
	)
	{
	void
	    *curLink_p;
	
	char
	    *curLine_p;

	cmd_t
	    *result;
	
	if(name == NULL || func == NULL)
	    {
	    result = NULL;
	    }
	else if((result = (cmd_t *)calloc(sizeof(*result), 1)) == NULL)
	    {
	    }
	else if((result->cmd_help = linkNew(NULL)) == NULL)
	    {
	    cmdFree(result);
	    result = NULL;
	    }
	else if((result->cmd_name = strdup(name)) == NULL)
	    {
	    cmdFree(result);
	    result = NULL;
	    }
	else
	    {
	    result->cmd_func = func;
	    result->cmd_error = errorFunc;
	    result->cmd_wordCount = wordCount;
	    for
		(
		curLine_p = strtok(help, "\n");
		curLine_p != NULL;
		curLine_p = strtok(NULL, "\n")
		)
		{
		if((curLine_p = strdup(curLine_p)) == NULL)
		    {
		    /* ERROR No Memory */
		    }
		else if((curLink_p = linkNew(curLine_p)) == NULL)
		    {
		    /* ERROR No Memory */
		    free(curLine_p);
		    }
		else
		    {
		    linkAppend(result->cmd_help, curLink_p);
		    }
		}

	    if(CmdTable == NULL) CmdTable = tableNew();
	    tableAddEntry(CmdTable, name, result, cmdFree);
	    }
	
	return(result);
	}

static void
    cmdAllowState(cmd_t *cmd_p, smtpServerState_t state)
	{
	cmd_p->cmd_allow[(int) state] = 1;
	}

void
    *cmdHelp(char *name)
	{
	cmd_t
	    *cmd_p;
	
	void
	    *result;

	if(name == NULL)
	    {
	    result = NULL;
	    }
	else if
	    (
		(
		cmd_p = (cmd_t *)tableGetValueByNoCaseString(CmdTable, name)
		) == NULL
	    )
	    {
	    result = NULL;
	    }
	else
	    {
	    result = cmd_p->cmd_help;
	    }

	return(result);
	}

smtpServerState_t
    cmdExec
	(
	char *line,
	smtpServerState_t state,
	void *connData_p
	)
	{
	cmd_t
	    *cmd_p;

	list_t
	    *list_p;

	int
	    cmdWordCount;

	char
	    *argument,
	    *name;

	if((name = strtok(line, "\t \r\n:")) == NULL)
	    {
	    /* No Command. */
	    if(DebugLevel > 0)(void) fprintf(stderr, "\tNo Name\n");
	    }
	else if
	    (
		(
		cmd_p = (cmd_t *)tableGetValueByNoCaseString(CmdTable, name)
		) == NULL
	    )
	    {
	    if(DebugLevel > 0)(void) fprintf(stderr, "\tNo command for %s\n", name);
	    }
	else if((list_p = listNew()) == NULL)
	    {
	    /* ERROR No Memory */
	    if(DebugLevel > 0)(void) fprintf(stderr, "\tNo Memory\n");
	    }
	else
	    {
	    if(DebugLevel > 0)(void) fprintf(stderr, "\tcommand = %s", name);
	    cmdWordCount = cmd_p->cmd_wordCount - 1;
	    while(cmdWordCount-- > 0 && (argument = strtok(NULL, " \t\r\n:")) != NULL)
		{
		if(DebugLevel > 0)(void) fprintf(stderr, " %s", argument);
		}

	    while((argument = strtok(NULL, " \t\r\n")) != NULL)
		{
		if(cmdWordCount-- > 0)
		    {
		    }
		else if((argument = strdup(argument)) == NULL)
		    {
		    /* ERROR No Memory */
		    }
		else
		    {
		    if(DebugLevel > 0)(void) fprintf(stderr, " %s", argument);
		    listAdd(list_p, argument);
		    }
		}

	    if(DebugLevel > 0)(void) fprintf(stderr, "\n");
	    if(cmd_p->cmd_allow[(int) state])
		{
		state = cmd_p->cmd_func
		    (
		    name,
		    state,
		    connData_p,
		    list_p
		    );
		}
	    else if(cmd_p->cmd_error)
		{
		state = cmd_p->cmd_error
		    (
		    name,
		    state,
		    connData_p,
		    list_p
		    );
		}
	    else
		{
		}

	    listFree(list_p);
	    }

	return(state);
	}

void
    cmdInit(int debugLevel)
	{
	cmd_t
	    *cmd_p;

	DebugLevel = debugLevel;

	if((cmd_p = cmdNew("HELO", heloFunc, heloError, 1, HELO_HELP)) != NULL)
	    {
	    cmdAllowState(cmd_p, sss_init);
	    }

	if((cmd_p = cmdNew("MAIL", mailFunc, mailError, 2, MAIL_HELP)) != NULL)
	    {
	    cmdAllowState(cmd_p, sss_ready);
	    }

	if((cmd_p = cmdNew("RCPT", rcptFunc, rcptError, 2, RCPT_HELP)) != NULL)
	    {
	    cmdAllowState(cmd_p, sss_mail);
	    cmdAllowState(cmd_p, sss_rcpt);
	    }

	if((cmd_p = cmdNew("DATA", dataFunc, dataError, 1, DATA_HELP)) != NULL)
	    {
	    cmdAllowState(cmd_p, sss_rcpt);
	    }

	if((cmd_p = cmdNew("SAML", mailFunc, mailError, 2, SAML_HELP)) != NULL)
	    {
	    cmdAllowState(cmd_p, sss_ready);
	    }

	if((cmd_p = cmdNew("SOML", mailFunc, mailError, 2, SOML_HELP)) != NULL)
	    {
	    cmdAllowState(cmd_p, sss_ready);
	    }

	if((cmd_p = cmdNew("SEND", blnkFunc, blnkError, 2, SEND_HELP)) != NULL)
	    {
	    cmdAllowState(cmd_p, sss_ready);
	    }

	if((cmd_p = cmdNew("TURN", blnkFunc, blnkError, 2, TURN_HELP)) != NULL)
	    {
	    cmdAllowState(cmd_p, sss_ready);
	    }

	if((cmd_p = cmdNew("RSET", rsetFunc, rsetError, 1, RSET_HELP)) != NULL)
	    {
	    cmdAllowState(cmd_p, sss_init);
	    cmdAllowState(cmd_p, sss_ready);
	    cmdAllowState(cmd_p, sss_mail);
	    cmdAllowState(cmd_p, sss_rcpt);
	    }

	if((cmd_p = cmdNew("EXPN", expnFunc, expnError, 1, EXPN_HELP)) != NULL)
	    {
	    cmdAllowState(cmd_p, sss_init);
	    cmdAllowState(cmd_p, sss_ready);
	    cmdAllowState(cmd_p, sss_mail);
	    cmdAllowState(cmd_p, sss_rcpt);
	    }

	if((cmd_p = cmdNew("VRFY", blnkFunc, blnkError, 1, VRFY_HELP)) != NULL)
	    {
	    cmdAllowState(cmd_p, sss_init);
	    cmdAllowState(cmd_p, sss_ready);
	    cmdAllowState(cmd_p, sss_mail);
	    cmdAllowState(cmd_p, sss_rcpt);
	    }

	if((cmd_p = cmdNew("HELP", helpFunc, helpError, 1, HELP_HELP)) != NULL)
	    {
	    cmdAllowState(cmd_p, sss_init);
	    cmdAllowState(cmd_p, sss_ready);
	    cmdAllowState(cmd_p, sss_mail);
	    cmdAllowState(cmd_p, sss_rcpt);
	    }

	if((cmd_p = cmdNew("NOOP", noopFunc, noopError, 1, NOOP_HELP)) != NULL)
	    {
	    cmdAllowState(cmd_p, sss_init);
	    cmdAllowState(cmd_p, sss_ready);
	    cmdAllowState(cmd_p, sss_mail);
	    cmdAllowState(cmd_p, sss_rcpt);
	    }

	if((cmd_p = cmdNew("QUIT", quitFunc, quitError, 1, QUIT_HELP)) != NULL)
	    {
	    cmdAllowState(cmd_p, sss_init);
	    cmdAllowState(cmd_p, sss_ready);
	    cmdAllowState(cmd_p, sss_mail);
	    cmdAllowState(cmd_p, sss_rcpt);
	    }

	(void) cmdNew(NULL, unrcFunc, unrcFunc, 1, NULL);
	}
