/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dthelp:dtaudit/Refs.c	1.1"

/*
 * Refs.c
 *
 */

#include <stdio.h>
#include <string.h>

static void ProcessFile();

main(argc, argv)
int argc;
char * argv[];
{

   char ** p;

   for (p = &argv[1]; p < &argv[argc]; p++)
   {
      ProcessFile(*p);
   }

} /* end of main */
/*
 * ProcessFile 
 *
 * This procedure opens the given file and parses it to
 * find help file keywords expected to be in the form:
 *
 *   \k(<extracted data>)
 *
 * Whenever a keyword is found, the name of the file and 
 * the extracted data is written to stdout for processing 
 * by \fIMatch\fP.
 *
 */

static void
ProcessFile(p)
char * p;
{

   FILE * fp;
   int    c;
   int    beg;
   int    end;

   if ((fp = fopen(p, "r")) == NULL)
      fprintf(stderr, "Error: Can't process file '%s'\n", p);
   else
   {
/*
      fprintf (stderr, "processing file '%s'\n", p);
*/
      p = strrchr(p, '/') + 1;
      while ((c = fgetc(fp)) != EOF)
      {
         if (c == '\\')
         {
            if ((c = fgetc(fp)) == 'k')
            {
               if ((c = fgetc(fp)) == '(' || c == '{')
               {
				beg = c;
				if (beg == '(')
					end = ')';
				else
					end = '}';

                  fprintf(stdout, "%s: ", p);
                  while ((c = fgetc(fp)) != EOF && c != end)
                     fprintf(stdout,"%c", c == beg ? ' ' : c);
                  fprintf(stdout, "\n");
                  fflush(stdout);
               }
            }
         }
      }
      fclose(fp);
   }

} /* end of ProcessFile */
