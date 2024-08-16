#ident	"@(#)prtsetup2:ps_openlook.C	1.3"
//--------------------------------------------------------------
// Filename: ps_openlook.c
//
// Description: This file contains functions needed to interface
//		with the "old" UNIXWARE desktop.
//
// Functions: GetCharProperty
//--------------------------------------------------------------

//--------------------------------------------------------------
//                         I N C L U D E S
//--------------------------------------------------------------
#include <stdlib.h>
#include <X11/Xmd.h>
#undef X_WCHAR
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <Xol/OlClients.h>

extern "C" char *GetCharProperty( Display *dpy, Window w, Atom property, int *length );
extern "C" void EnqueueCharProperty( Display *dpy, Window w, Atom atom, char *data, int len );
/*extern "C" char *malloc( int );
extern "C" void free( char * );*/



//--------------------------------------------------------------
// Function Name: GetCharProperty
//
// Description: 
//
// Parameters:
//
// Return:
//--------------------------------------------------------------
char *GetCharProperty( Display *dpy, Window w, Atom property, int *length )
{
   Atom			actual_type;
   int			actual_format;
   unsigned long 	num_items; 
   unsigned long	bytes_remaining = 1;
   char  		*buffer;
   char			*Buffer;
   int			Buffersize = 0;
   int			EndOfBuffer;

   Buffer = ( char * ) malloc( 1 );
   Buffer[0] = '\0';
   EndOfBuffer = 0;
   do
   {
	if ( (XGetWindowProperty( dpy, w, property,
			( long ) ( ( Buffersize + 3 ) / 4 ),
			( int ) ( ( bytes_remaining + 3 ) / 4 ), True,
			XA_STRING, &actual_type, &actual_format,
			&num_items, &bytes_remaining,
			( unsigned char ** ) &buffer ) ) != Success )
			{
			    if ( buffer ) 
				free( buffer );
			    if ( Buffer )
				free( Buffer );
			    *length = 0;
			    return ( NULL );
			}
	if ( buffer )
        {
	    register int i;
	    Buffersize += ( int ) num_items;
	    Buffer = ( char * ) realloc( Buffer, Buffersize + 1 );
	    if ( Buffer == NULL )
	    { 
		free( buffer );
		*length = 0;
		return ( NULL );
	    }
	    for ( i = 0; i < num_items; i++ )
		Buffer[EndOfBuffer++] = '\0';
	    Buffer[EndOfBuffer] = '\0';
	    free( buffer );
	}

   } while( bytes_remaining > 0 );

   *length = Buffersize;
   if ( Buffersize == 0 && Buffer != NULL )
   {
	free( Buffer );
	return( NULL );
   }	
   else
	return( Buffer );
}


//--------------------------------------------------------------
// Function Name: EnqueueCharProperty
//
// Description: 
//
// Parameters:
//
// Return:
//--------------------------------------------------------------
void EnqueueCharProperty( Display *dpy, Window w, Atom atom, char *data, int len )
{
    XChangeProperty( dpy, w, atom, XA_STRING, 8, PropModeAppend,
		( unsigned char * ) data, len );
    XFlush( dpy );
}

