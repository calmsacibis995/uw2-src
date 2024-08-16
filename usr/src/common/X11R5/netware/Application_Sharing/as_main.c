/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)appshare:as_main.c	1.4"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Application_Sharing/as_main.c,v 1.3 1994/06/21 17:58:06 plc Exp $"

/*--------------------------------------------------------------------
** Filename : as_main.h
**
** Description : This file contains the main function for the 
**               App_Sharing program.
**
** Functions : main
**             BuildButtonItems
**------------------------------------------------------------------*/




/*--------------------------------------------------------------------
**                         I N C L U D E S  
**------------------------------------------------------------------*/
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <elf.h>
#include <libelf.h>

#include <locale.h>

#include "as_listhdr.h"

/*--------------------------------------------------------------------
**                          D E F I N E S
**------------------------------------------------------------------*/
#define M_TYPE      EM_386
#define M_DATA      ELFDATA2LSB
#define M_CLASS     ELFCLASS32
#define ELF_EHDR    Elf32_Ehdr
#define ELF_PHDR    Elf32_Phdr
#define elf_getehdr elf32_getehdr
#define elf_getphdr elf32_getphdr

/*--------------------------------------------------------------------
** Function : main
**
** Description : This is the main function for the App_Sharing
**               program
**
** Parameters : argc and argv
**
** Return : 0 
**------------------------------------------------------------------*/
int main( int argc, char **argv )
{
    static   WnStruc      win;
    static   buttonItems  *buttons;
    int                   numButtons;
    extern XtAppContext          appContext;

   XtSetLanguageProc(NULL, NULL, NULL);
   OlToolkitInitialize( argc, argv, ( XtPointer ) NULL );
   widg.toplevel = XtAppInitialize( &appContext, ( char * )APP_NAME, NULL, 0,
                               argc, argv, NULL, NULL, 0 );

    numButtons = BuildButtonItems( &buttons, buttonsTemplate, numButtonItems );  
    CopyInterStr( TXT_TITLE_DAY1,     &win.title, 0 );
    CopyInterStr( TXT_ICON_NAME_DAY1, &win.iconName, 0 );
    CopyInterStr( TXT_APP_LABEL,      &win.appListLabel, 0 );
    CopyInterStr( TXT_FOOT_LABEL,     &win.footerLabel, 0 );
    win.initHeight = DEFAULT_HEIGHT;
    win.initWidth  = DEFAULT_WIDTH;
    win.buttons          = buttons;
    win.numButtonItems   = numButtons;

    BuildListWindow( &win, &argc, argv );
    return( 0 );
}


/*--------------------------------------------------------------------
** Function : BuildButtonItems
**
** Description : This function builds an array of button items from 
**               the button templates of internationalized strings.
**
** Parameters : buttons - structure to put new button items in
**              buttonsTemplate - contains the internationalized 
**                                string definitions for the buttons.
**              numButtonItems  - the number of buttons 
**
** Return : The number of button items created.
**------------------------------------------------------------------*/
int    BuildButtonItems( buttonItems **buttons, 
                         buttonItems *buttonsTemplate, 
                         int         numButtonItems )  
{
    buttonItems     *tempPtr;
    int              i;
    unsigned char   *temp;

    *buttons = tempPtr = ( buttonItems * ) XtMalloc
                               ( sizeof( buttonItems ) * numButtonItems );
    for ( i = 0; i < numButtonItems; i++ )
    {
        CopyInterStr( buttonsTemplate[i].label, 
                    ( char ** )&(tempPtr->label), 0 );
        CopyInterStr( buttonsTemplate[i].mnemonic, &temp, 0 );
        tempPtr->mnemonic = ( char * )temp[0];
        XtFree( ( XtPointer )temp );
        tempPtr->select = buttonsTemplate[i].select;
        tempPtr->sensitive = buttonsTemplate[i].sensitive;
        tempPtr++; 
    }
    return( i ); 
}


/*--------------------------------------------------------------------
** Function : IsXApp 
**
** Description : This function attempts to determine if an application is
**               an X or a Text Application. It uses code derived from the
**               ldd command. If we can't determine assume Text.
**
** Parameters : char *filePath - filepath of the file to test.
**
** Return : TRUE if it is an X application, FALSE if it is not an X
**          Application.
**------------------------------------------------------------------*/
Boolean IsXApp( unsigned char *filePath )
{
   static char *trace     = "LD_TRACE_LOADED_OBJECTS=1";
   static char *blazy     = "LD_BIND_NOW=";
   Elf         *elf       = NULL;
   ELF_EHDR    *ehdr      = NULL;
   ELF_PHDR    *phdr      = NULL;
   int          fd        = -1;
   Boolean      retCode   = FALSE;
   int          i;
   char         *lddFileName = NULL;
   char         *command     = { "%s > %s" };
   char         *commandBuf  = NULL;
   FILE         *lddOutput   = NULL;
   char         *line        = NULL;
   int           dynamic     = 0;
   static Boolean first      = TRUE;

   if ( first )
   {
      if ( putenv( trace ) != 0 ) /* Trace loaded objects */
          goto EXIT_ISXAPP; 
      if ( putenv( blazy ) != 0 )
          goto EXIT_ISXAPP; 
      first = FALSE;
   }
   else
   {
      trace[0] = 'L';
      blazy[0] = 'L';
   }

   if ( elf_version( EV_CURRENT ) == EV_NONE )
       goto EXIT_ISXAPP;    /* Assume Text */
   if ( ( fd = open( (char *) filePath, O_RDONLY ) ) == -1 )
       goto EXIT_ISXAPP;    /* Assume Text */
   if ( ( elf = elf_begin( fd, ELF_C_READ, ( Elf * ) 0 ) ) == ( Elf * ) 0 )
       goto EXIT_ISXAPP;    /* Assume Text */
   if ( elf_kind( elf ) != ELF_K_ELF )
       goto EXIT_ISXAPP;    /* Assume Text */
   if ( ( ehdr = elf_getehdr( elf ) ) == ( ELF_EHDR * ) 0 )
       goto EXIT_ISXAPP;
   if ( ehdr->e_ident[EI_CLASS] != M_CLASS ||
        ehdr->e_ident[EI_DATA] != M_DATA )
            goto EXIT_ISXAPP;
   if ( ehdr->e_type != ET_EXEC )
       goto EXIT_ISXAPP;
   if ( ehdr->e_machine != M_TYPE )
       goto EXIT_ISXAPP;
   if ( ( phdr = elf_getphdr( elf ) ) == ( ELF_PHDR * ) 0 )
       goto EXIT_ISXAPP;
   for ( i = 0; i < (int) ehdr->e_phnum; i++ )
   {
       if ( phdr->p_type == PT_DYNAMIC )
       {
           dynamic = 1;
           break;
       }
       phdr = ( ELF_PHDR * ) ( (unsigned long) phdr + ehdr->e_phentsize );
   }    
   if ( !dynamic )
       goto EXIT_ISXAPP;
   if ( close ( fd ) == -1 )
       goto EXIT_ISXAPP;

   /*-------------------------------------------------------------
   ** Setup all the variables to make the fork call and then fork
   **-----------------------------------------------------------*/
   GenRandomTempFName( &lddFileName ); 
   mktemp( lddFileName );
   commandBuf = XtMalloc( strlen( command ) + 
                          strlen( (char *)filePath ) +
                          strlen( lddFileName ) + 1 );
   sprintf( commandBuf, command, filePath, lddFileName );
   system( commandBuf );
   XtFree( (XtPointer) commandBuf );
   lddOutput = fopen( lddFileName, "r" );
   line = (char *) XtMalloc( MAX_LINE_LEN );
   while ( fgets( (char *) line, MAX_LINE_LEN-1, lddOutput ) != NULL )
   {
      for ( i=0; Xlibs[i] != NULL; i++ )
      if ( strstr( (char *) line, Xlibs[i] ) != NULL )
      {
         retCode = TRUE;
         break;
      }
   }
   if ( lddFileName != NULL )
   {
      unlink( lddFileName );
      XtFree( lddFileName );
   }
   if ( lddOutput )
      fclose( lddOutput );
   if ( line )
      XtFree( line );

EXIT_ISXAPP: /* Clean up and return. The return code should already be set */
   trace[0] = '\0';
   blazy[0] = '\0';

   if ( fd != -1 ) 
       close( fd );
   return( retCode );


}
