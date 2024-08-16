/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:mail/table.h	1.2"
#if	!defined(TABLE_H)
#define	TABLE_H

#if	!defined(TABLE_OBJ)
typedef void table_t;
#endif

table_t
    *tableNew(void);

void
    tableClear(table_t *table),
    tableFree(table_t *table),
    tableAddEntry(table_t *table, char *string, void *value, void (*freeFunc)()),
    *tableGetValueByString(table_t *table, char *string),
    *tableGetValueByNoCaseString(table_t *table, char *string),
    tableSetDebugLevel(int debugLevel),
    tableDoForEachEntry(table_t *table, void (*func)(), void *localData_p);

int
    tableDeleteEntryByValue(table_t *table, void *data_p);

#endif
