/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamintf:common/cmd/oamintf/security/audit/bin/valevt.c	1.1.3.2"
#ident  "$Header: valevt.c 2.0 91/07/12 $"
/* valevt.c - validation for pre, object, and post selection events */
#include	<stdio.h>
#include	<string.h>
#include	<sys/types.h>
#include	<sys/param.h>
#include	<audit.h>

#define	ALL	"all"
#define	NONE	"none"
#define	PRE	"pre"
#define	OBJ	"obj"
#define	POST	"post"

main(argc,argv)
int argc;
char **argv;
{
   int i;

   /* PRE-SELECTION HERE with arg1=pre */
   if (strcmp(argv[1],PRE)== 0)
   {
      if(argv[2])
      {
         for (i=1;i<=ADT_NUMOFEVTS;i++)
         {
            if ( (strcmp(argv[2],ALL)== 0)
              || (strcmp(argv[2],NONE)== 0) )
               exit(0);      
            if ( (strcmp(argv[2],adtevt[i].a_evtnamp)== 0)
              && ((adtevt[i].a_fixed)==0) ) 
               exit(0);      
         }
      }
      else
      {
         fprintf(stdout,"%s\n",ALL);
         fprintf(stdout,"%s\n",NONE);
         for (i=1;i<=ADT_NUMOFEVTS;i++)
         {
            if ((adtevt[i].a_fixed)==0)
               fprintf(stdout,"%s\n",adtevt[i].a_evtnamp);
         }
      }
   }
   /* OBJECT-SELECTION HERE with arg1=obj */
   else if (strcmp(argv[1],OBJ)== 0)
   {
      if(argv[2])
      {
         for (i=1;i<=ADT_NUMOFEVTS;i++)
         {
            if ( (strcmp(argv[2],ALL)== 0)
              || (strcmp(argv[2],NONE)== 0) )
               exit(0);      
            if ( (strcmp(argv[2],adtevt[i].a_evtnamp)== 0)
              && ((adtevt[i].a_objt)==1) ) 
               exit(0);      
         }
      }
      else
      {
         fprintf(stdout,"%s\n",ALL);
         fprintf(stdout,"%s\n",NONE);
         for (i=1;i<=ADT_NUMOFEVTS;i++) 
         {
            if ((adtevt[i].a_objt)==1)
               fprintf(stdout,"%s\n",adtevt[i].a_evtnamp);
         }
      }
   }
   /* POST-SELECTION HERE with arg1=post */
   else if (strcmp(argv[1],POST)== 0)
   {
      for (i=1;i<=ADT_NUMOFEVTS;i++)
      {
         if (argv[2])
         {
            if (strcmp(argv[2],adtevt[i].a_evtnamp)== 0)
               exit(0);      
         }
         else
            fprintf(stdout,"%s\n",adtevt[i].a_evtnamp);
      }
   }
   /* If didn't exit already, exit with status 1 */
   exit(1);
}
