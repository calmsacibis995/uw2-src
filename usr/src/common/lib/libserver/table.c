/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libserver:table.c	1.2"
static char SccsID[] = "@(#)table.c	1.2 91/09/0310:26:26 92/01/1014:55:22";
/*
**  Netware Unix Client 
**	Copyright Novell Inc. 1991
**
**	 Author:  Scott Harrison
**	 Created: 4-16-91
**
**      SCCS: 
**
**	MODULE:
**	   table.c -	The table generator/lookup package.
**				Maps a string onto a pointer to an
**				arbitrary type.
**
**	ABSTRACT:
*/
#define	TABLE_OBJ

#include	<stdio.h>
#include	<string.h>
#include	<malloc.h>
#include	<ctype.h>
#include	<mail/link.h>

typedef struct table_t
    {
    void
	*tbl_freeHead,
	*tbl_head;

    unsigned
	tbl_lock,
	tbl_free: 1;
    }	table_t;

#include	<mail/table.h>

typedef struct tableEntry_s
    {
    table_t
	*tbl_owner;
    char
	*tbl_string;	/* Pointer to string to be mapped into data below. */
    void
	*tbl_link,
	*tbl_freeLink,
	*tbl_value,	/* Pointer to opaque data type. */
	(*tbl_free)();	/* Pointer to function to free above data type. */
    }	tableEntry_t;

static int
    DebugLevel = 0;

static void
    tableEntryFree(tableEntry_t *entry_p)
	{
	if(entry_p == NULL)
	    {
	    }
	else if(!entry_p->tbl_owner->tbl_lock)
	    {
	    if(entry_p->tbl_link != NULL) linkFree(entry_p->tbl_link);
	    if(entry_p->tbl_freeLink != NULL) linkFree(entry_p->tbl_freeLink);
	    if(entry_p->tbl_string != NULL) free(entry_p->tbl_string);
	    if(entry_p->tbl_free != NULL) entry_p->tbl_free(entry_p->tbl_value);
	    free(entry_p);
	    }
	else if(entry_p->tbl_freeLink)
	    {
	    }
	else if((entry_p->tbl_freeLink = linkNew(entry_p)) == NULL)
	    {
	    if(entry_p->tbl_link != NULL) linkFree(entry_p->tbl_link);
	    if(entry_p->tbl_freeLink != NULL) linkFree(entry_p->tbl_freeLink);
	    if(entry_p->tbl_string != NULL) free(entry_p->tbl_string);
	    if(entry_p->tbl_free != NULL) entry_p->tbl_free(entry_p->tbl_value);
	    free(entry_p);
	    }
	else
	    {
	    (void)linkAppend(entry_p->tbl_owner->tbl_freeHead, entry_p->tbl_freeLink);
	    }
	}

static tableEntry_t
    *tableEntryNew
	(
	table_t *owner,
	char *string,
	void *value,
	void (*freeFunc)()
	)
	{
	tableEntry_t
	    *result;
	
	if((result = (tableEntry_t *)calloc(sizeof(*result), 1)) == NULL)
	    {
	    }
	else if((result->tbl_link = linkNew(result)) == NULL)
	    {
	    result->tbl_value = value;
	    result->tbl_free = freeFunc;
	    result->tbl_owner = owner;
	    tableEntryFree(result);
	    result = NULL;
	    }
	else if
	    (
	    string != NULL && (result->tbl_string = strdup(string)) == NULL
	    )
	    {
	    result->tbl_value = value;
	    result->tbl_free = freeFunc;
	    result->tbl_owner = owner;
	    tableEntryFree(result);
	    result = NULL;
	    }
	else
	    {
	    result->tbl_value = value;
	    result->tbl_free = freeFunc;
	    result->tbl_owner = owner;
	    (void)linkAppend(owner->tbl_head, result->tbl_link);
	    if(DebugLevel > 9)
		{
		fprintf
		    (
		    stderr,
		    "\tvalue = 0x%x, freeFunc = 0x%x, owner = 0x%x, link = 0x%x, entry = 0x%x.\n",
		    (int) value,
		    (int) freeFunc,
		    (int) owner,
		    (int) result->tbl_link,
		    (int) result
		    );
		}
	    }

	return(result);
	}

/*
 * BEGIN_MANUAL_ENTRY(tableNew(3T), \
 *			./man/man3/tableNew)
 * NAME
 *	tableNew	- Creation of a new empty table.
 *
 * SYNOPSIS
 *	public void
 *	    *tableNew()
 *
 * INPUT
 *	None.
 *
 * OUTPUT
 *	None.
 *
 * RETURN VALUES
 *	Opaque pointer to the new table..
 *
 * DESCRIPTION
 *	
 *	
 *
 * NOTES
 *	
 *
 * SEE ALSO
 *	
 *
 * END_MANUAL_ENTRY
 */
table_t
    *tableNew()
	{
	table_t
	    *result;
	
	if((result = (table_t *) calloc(sizeof(*result), 1)) == NULL)
	    {
	    }
	else if((result->tbl_head = linkNew(NULL)) == NULL)
	    {
	    tableFree(result);
	    result = NULL;
	    }
	else if((result->tbl_freeHead = linkNew(NULL)) == NULL)
	    {
	    tableFree(result);
	    result = NULL;
	    }

	return(result);
	}

/*
 * BEGIN_MANUAL_ENTRY(tableClear(3T), \
 *			./man/man3/tableClear)
 * NAME
 *	tableClear	- Clear all entries in the table
 *
 * SYNOPSIS
 *	public void
 *	    tableClear()
 *
 * INPUT
 *	Pointer to the table to be freed.
 *
 * OUTPUT
 *	None.
 *
 * RETURN VALUES
 *	None.
 *
 * DESCRIPTION
 *	
 *	
 *
 * NOTES
 *	
 *
 * SEE ALSO
 *	
 *
 * END_MANUAL_ENTRY
 */
void
    tableClear(table_t *table)
	{
	tableEntry_t
	    *cur_e;
	
	if(table == NULL)
	    {
	    }
	else while
	    (
		(
		cur_e = (tableEntry_t *)linkOwner(linkNext(table->tbl_head))
		) != NULL
	    )
	    {
	    tableEntryFree(cur_e);
	    }
	}

/*
 * BEGIN_MANUAL_ENTRY(tableFree(3T), \
 *			./man/man3/tableFree)
 * NAME
 *	tableFree	- Free a table along with all entries.
 *
 * SYNOPSIS
 *	public void
 *	    tableFree()
 *
 * INPUT
 *	Pointer to the table to be freed.
 *
 * OUTPUT
 *	None.
 *
 * RETURN VALUES
 *	None.
 *
 * DESCRIPTION
 *	
 *	
 *
 * NOTES
 *	
 *
 * SEE ALSO
 *	
 *
 * END_MANUAL_ENTRY
 */
void
    tableFree(table_t *table)
	{
	if(table == NULL)
	    {
	    }
	else if(table->tbl_lock)
	    {
	    if(DebugLevel > 2) (void)fprintf(stderr, "tableFree() locked.\n");

	    table->tbl_free = 1;
	    }
	else
	    {
	    if(DebugLevel > 2) (void)fprintf(stderr, "tableFree() ok.\n");
	    tableClear(table);
	    linkFree((void *)table->tbl_head);
	    free(table);
	    }
	}


/*
 * BEGIN_MANUAL_ENTRY(tableAddEntry(3T), \
 *			./man/man3/tableAddEntry)
 * NAME
 *	tableAddEntry	- Add a string to data mapping to a table.
 *
 * SYNOPSIS
 *	public void
 *	    tableAddEntry()
 *
 * INPUT
 *	Pointer to table to add entry to.
 *	Pointer to a character string.
 *	Pointer to an opaque data type.
 *	Pointer to a function to free the data pointed to by the above.
 *
 * OUTPUT
 *	None.
 *
 * RETURN VALUES
 *	None.
 *
 * DESCRIPTION
 *	
 *	
 *
 * NOTES
 *	
 *
 * SEE ALSO
 *	
 *
 * END_MANUAL_ENTRY
 */
void
    tableAddEntry(table_t *table, char *string, void *value, void (*freeFunc)())
	{
	if(DebugLevel > 4)
	    {
	    (void)fprintf
		(
		stderr,
		"tableAddEntry(0x%x, %s, 0x%x, 0x%x) Entered.\n",
		(int)table,
		string,
		(int)value,
		(int)freeFunc
		);
	    }

	(void)tableEntryNew(table, string, value, freeFunc);
	if(DebugLevel > 4) (void)fprintf(stderr, "tableAddEntry() Exited.\n");
	}

/*
 * BEGIN_MANUAL_ENTRY(tableGetValueByString(3T), \
 *			./man/man3/tableGetValueByString)
 * NAME
 *	tableGetValueByString	- Do a table lookup.
 *
 * SYNOPSIS
 *	public void
 *	    *tableGetValueByString()
 *
 * INPUT
 *	Pointer to table of string to data mappings.
 *	Pointer to character string.
 *
 * OUTPUT
 *	None.
 *
 * RETURN VALUES
 *	Pointer to data object mapped to string in table.
 *	NULL if no such string.
 *
 * DESCRIPTION
 *	
 *	
 *
 * NOTES
 *	
 *
 * SEE ALSO
 *	
 *
 * END_MANUAL_ENTRY
 */
void
    *tableGetValueByString(table_t *table, char *string)
	{
	void
	    *cur_p;
	
	if(table == NULL)
	    {
	    }
	else for
	    (
	    cur_p = linkNext((void *)table->tbl_head);
	    linkOwner(cur_p) != NULL
		&& strcmp
		    (
		    ((tableEntry_t *) linkOwner(cur_p))->tbl_string,
		    string
		    );

	    cur_p = linkNext(cur_p)
	    );
	
	return
	    (
	    (table == NULL)?
		NULL:
		(linkOwner(cur_p) == NULL)?
		    NULL:
		    ((tableEntry_t *) linkOwner(cur_p))->tbl_value
	    );
	}

/*
 * BEGIN_MANUAL_ENTRY(tableGetValueByNoCaseString(3T), \
 *			./man/man3/tableGetValueByNoCaseString)
 * NAME
 *	tableGetValueByNoCaseString	- Do a table lookup.
 *
 * SYNOPSIS
 *	public void
 *	    *tableGetValueByNoCaseString()
 *
 * INPUT
 *	Pointer to table of string to data mappings.
 *	Pointer to character string.
 *
 * OUTPUT
 *	None.
 *
 * RETURN VALUES
 *	Pointer to data object mapped to string in table.
 *	NULL if no such string.
 *
 * DESCRIPTION
 *	
 *	
 *
 * NOTES
 *	
 *
 * SEE ALSO
 *	
 *
 * END_MANUAL_ENTRY
 */
void
    *tableGetValueByNoCaseString(table_t *table, char *string)
	{
	register char
	    *p,
	    *q;
	
	tableEntry_t
	    *curEntry;
	    
	void
	    *cur_p,
	    *result;
	
	if(DebugLevel > 4)
	    {
	    (void) fprintf
		(
		stderr,
		"tableGetValueByNoCaseString(0x%x, %s) Entered.\n",
		(int) table,
		string
		);
	    }

	if(table == NULL)
	    {
	    result = NULL;
	    }
	else for
	    (
	    cur_p = linkNext((void *)table->tbl_head),
	        curEntry = (tableEntry_t *)linkOwner(cur_p),
		result = NULL;
	    curEntry != NULL && result == NULL;
	    cur_p = linkNext(cur_p),
	        curEntry = (tableEntry_t *)linkOwner(cur_p)
	    )
	    {
	    if(DebugLevel > 9)
		{
		(void) fprintf
		    (
		    stderr,
		    "\tcurEntry = 0x%x, tbl_string = %s.\n",
		    (int) curEntry,
		    (curEntry->tbl_string == NULL)? "NIL": curEntry->tbl_string
		    );
		}

	    if(curEntry->tbl_string == NULL)
		{
		result = curEntry->tbl_value;
		break;
		}

	    for
		(
		p = curEntry->tbl_string,
		    q = string;
		toupper(*p) == toupper(*q) && *p != '\0';
		p++,
		    q++
		);
	    
	    if(*p == '\0' && *q == '\0')
		{
		result = curEntry->tbl_value;
		break;
		}
	    }
	
	if(DebugLevel > 4)
	    {
	    (void) fprintf
		(
		stderr,
		"tableGetValueByNoCaseString() = 0x%x Exited.\n",
		(int) result
		);
	    }

	return result;
	}

/*
 * BEGIN_MANUAL_ENTRY(tableDeleteByValue(3T), \
 *			./man/man3/tableDeleteByValue)
 * NAME
 *	tableDeleteByValue	- Delete all references to an object
 *				from the table.
 *
 * SYNOPSIS
 *	public void
 *	    tableDeleteByValue()
 *
 * INPUT
 *	Pointer to table of string to data mappings.
 *	Pointer to data object.
 *
 * OUTPUT
 *	None.
 *
 * RETURN VALUES
 *
 * DESCRIPTION
 *	
 *	
 *
 * NOTES
 *	
 *
 * SEE ALSO
 *	
 *
 * END_MANUAL_ENTRY
 */
int
    tableDeleteEntryByValue(table_t *table, void *data_p)
	{
	tableEntry_t
	    *curEntry_p;
	    
	int
	    result = 0;

	void
	    *cur_p;
	
	if(DebugLevel > 4)
	    {
	    (void) fprintf
		(
		stderr,
		"tableDeleteEntryByValue(0x%x, 0x%x) Entered.\n",
		(int) table,
		(int) data_p
		);
	    }

	if(table == NULL)
	    {
	    }
	else for
	    (
	    cur_p = linkNext((void *)table->tbl_head);
	    (curEntry_p  = (tableEntry_t *)linkOwner(cur_p)) != NULL;
	    )
	    {
	    cur_p = linkNext(cur_p);
	    if(DebugLevel > 9)
		{
		fprintf
		    (
		    stderr,
		    "\tEntry = 0x%x, value = 0x%x, cmp value = 0x%x.\n",
		    (int) curEntry_p,
		    (int) curEntry_p->tbl_value,
		    (int) data_p
		    );
		}

	    if(curEntry_p->tbl_value == data_p)
		{
		tableEntryFree(curEntry_p);
		result = 1;
		}
	    }

	if(DebugLevel > 4)
	    {
	    fprintf
		(
		stderr,
		"tableDeleteEntryByValue() = %d Exited.\n",
		result
		);
	    }

	return(result);
	}

static void
    tableLock(table_t *table)
	{
	table->tbl_lock++;
	}

static void
    tableUnlock(table_t *table)
	{
	void
	    *curLink_p;

	tableEntry_t
	    *entry_p;

	if(table->tbl_lock > 0)
	    {
	    table->tbl_lock--;
	    }
	
	if(table->tbl_lock <= 0)
	    {
	    while((curLink_p = linkNext(table->tbl_freeHead)) != table->tbl_freeHead)
		{
		entry_p = (tableEntry_t *)linkOwner(curLink_p);
		tableEntryFree(entry_p);
		}

	    if(table->tbl_free)
		{
		tableFree(table);
		}
	    }
	}

void
    tableDoForEachEntry(table_t *table, void (*func)(), void *localData_p)
	{
	tableEntry_t
	    *curEntry_p;
	    
	void
	    *cur_p;

	if(table == NULL)
	    {
	    }
	else
	    {
	    tableLock(table);
	    for
		(
		cur_p = linkNext((void *)table->tbl_head);
		(curEntry_p  = (tableEntry_t *)linkOwner(cur_p)) != NULL;
		cur_p = linkNext(cur_p)
		)
		{
		func(curEntry_p->tbl_value, localData_p);
		}

	    tableUnlock(table);
	    }
	}

void
    tableSetDebugLevel(int debugLevel)
	{
	DebugLevel = debugLevel;
	if(DebugLevel != 0)
	    {
	    fprintf
		(
		stderr,
		"tableSetDebugLevel(%d) Entered & Exited.\n",
		debugLevel
		);
	    }
	}
