/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dthelp:dtaudit/Sects.c	1.1"

/*
 * Tags.c
 *
 */

#include <stdio.h>
#include <string.h>

static void ProcessFile();

/*
 * main
 *
 */

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
 * This procedure parses the given file to extract section
 * names expected to be in the form 
 *
 *      ^<digits>^<section name[=section alias]>
 *
 * Whenever such a line is found, the name of the file
 * and the extracted data is written to stdout for
 * processing by \fIMatch\fP.
 *
 */

static void
ProcessFile(p)
char * p;
{

   FILE * fp;
   int    c;

   if ((fp = fopen(p, "r")) == NULL)
      fprintf(stderr, "Can't process file '%s'\n", p);
   else
   {
      fprintf (stderr, "processing file '%s'\n", p);
      p = strrchr(p, '/') + 1;
      while ((c = fgetc(fp)) != EOF)
      {
         if (c == '\n')
            ;
         else
            if (c == '^')
            {
               while (strchr("0123456789", (c = fgetc(fp))) != NULL)   ;
               if (c == '^')
               {
                  fprintf(stdout, "%s: ", p);
                  while ((c = fgetc(fp)) != EOF && c != '\n')
                     fprintf(stdout,"%c", c);

                  while ((c = fgetc(fp)) != EOF) {
				if (c == '^' && (c = fgetc(fp)) == '$') {
                      fprintf(stdout,"^");
                     while ((c = fgetc(fp)) != EOF && c != '\n')
                       fprintf(stdout,"%c", c);
                     fprintf(stdout, "\n");
				}
			   }
                  fflush(stdout);
               }
               while (c != '\n' && (c = fgetc(fp)) != EOF && c != '\n')    ;
            }
            else
               while ((c = fgetc(fp)) != EOF && c != '\n')    ;
      }
      fclose(fp);
   }

} /* end of ProcessFile */
