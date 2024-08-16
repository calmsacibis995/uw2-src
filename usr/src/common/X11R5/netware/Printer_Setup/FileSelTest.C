#include <memory.h>
#include "FileSel.h"
#include <Xm/PushB.h>
#include <Xm/Text.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/ScrollBar.h>
#include <Xm/Separator.h>
#include <string.h> 
#include "MultiPList.h"
#include <unistd.h>
#include <assert.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <sys/types.h>
#include <sys/stat.h>


//
// Pixmap files to use
//

char *pixFiles[] = {
					"/usr/X/lib/pixmaps/spipe.icon",
					"/usr/X/lib/pixmaps/schrdev.icon",
					"/usr/X/lib/pixmaps/sdir.icon",
					"/usr/X/lib/pixmaps/sdatafile.icon",
					"/usr/X/lib/pixmaps/sblkdev.icon",
					"/usr/X/lib/pixmaps/sdatafile.icon",
					"/usr/X/lib/pixmaps/ssem.icon",
					"/usr/X/lib/pixmaps/sunk.icon",
					"/usr/X/lib/pixmaps/sexec.icon",
};

FileSelWindow::FileSelWindow ( char *name )
{
}

Widget FileSelWindow::createWorkArea ( Widget parent, char *startPath,
								char *copyTxt, char *asTxt, char *copyName )
{
	int i,y;
	int x,numOfItems;
	Directory *dir_;
    XmString *xmstr; 
	char tcwd[512];
	char tn[512];
	char td[512];
	Widget rowcol;
	Widget form;
	Widget copy_label;
	Widget path_label;
	Widget form1;
	int pixmapR;
	struct stat st_buf;
	XmString		str;
	char			*tmp;


    form = XtCreateManagedWidget("FileSel_form", xmFormWidgetClass, parent,NULL,0);
  
	str = XmStringCreateLocalized( copyTxt ); 
  	copy_label = XtVaCreateManagedWidget( "copyLabel", xmLabelWidgetClass, 
					form,
					XmNtopAttachment,XmATTACH_FORM,
					XmNleftAttachment,XmATTACH_FORM,
					XmNleftOffset, 20,
					XmNalignment,XmALIGNMENT_END,
					XmNlabelString, str,
					NULL);
	XmStringFree( str );

	str = XmStringCreateLocalized( asTxt );
  	as_label = XtVaCreateManagedWidget( "asLabel", xmLabelWidgetClass, 
					form,
					XmNtopAttachment,XmATTACH_WIDGET,
					XmNtopWidget, copy_label,
					XmNleftAttachment,XmATTACH_FORM,
					XmNleftOffset, 20,
					XmNalignment, XmALIGNMENT_END,
					XmNlabelString, str,
					NULL);
	XmStringFree( str );


	sep = XtVaCreateManagedWidget( "sep", xmSeparatorWidgetClass,
					form, 
					XmNorientation, XmVERTICAL,
					XmNseparatorType, XmNO_LINE,
					XmNtopAttachment,XmATTACH_FORM,
					NULL );

	str = XmStringCreateLocalized( copyName );
	copyTxt_label = XtVaCreateManagedWidget( "copyTxtLbl", xmLabelWidgetClass,
					form,
					XmNtopAttachment, XmATTACH_FORM,
					XmNrightAttachment,XmATTACH_FORM,
					XmNalignment, XmALIGNMENT_BEGINNING,
					XmNlabelString, str,
					NULL );
	XmStringFree( str );

	str = XmStringCreateLocalized("");
  	path_lbl = XtVaCreateManagedWidget( "pathLabel", xmLabelWidgetClass, 
					form,
					XmNtopAttachment,XmATTACH_WIDGET,
					XmNtopWidget, copyTxt_label,
					XmNrightAttachment,XmATTACH_FORM,
					XmNalignment, XmALIGNMENT_BEGINNING,
					XmNlabelString, str,
					NULL);
	XmStringFree( str );


    //
	// Create the MultiPList object , needs it own form widget
	//
    form1 = XtCreateManagedWidget("M_form", xmFormWidgetClass, form,NULL,0);
    ml = new MultiPList(form1,2,&FileSelWindow::ListCallback,(void *)this);
	//
	// Read in and save the pixmap images
	//
	for(i=0;i< 9;i++ )
		ml->AddPixmap(pixFiles[i]);
	//
	// Load the pixmaps to the pixmap lists
	//
	ml->LoadListPixmaps();

	XtVaSetValues(form1,
					XmNtopAttachment,XmATTACH_WIDGET,
					XmNtopWidget, as_label,
					XmNtopOffset, 15,
					XmNleftAttachment,XmATTACH_FORM,
					XmNrightAttachment,XmATTACH_FORM,
					NULL);

	XtVaSetValues( sep, XmNbottomAttachment, XmATTACH_WIDGET,
					XmNbottomWidget, form1, 
					XmNleftAttachment, XmATTACH_WIDGET,
					XmNleftWidget, 
					strlen( copyTxt ) > strlen( asTxt ) ? copy_label : as_label,
					NULL );	

	XtVaSetValues( copyTxt_label,
					XmNleftAttachment, XmATTACH_WIDGET,
					XmNleftWidget, sep, NULL );
	XtVaSetValues( path_lbl,
					XmNleftAttachment, XmATTACH_WIDGET,
					XmNleftWidget, sep, NULL );

	//
	// Create the directory path text widget
	//
    path_text = XtVaCreateManagedWidget("path_text",
                     xmTextWidgetClass,
                     form,
					 XmNbottomAttachment,XmATTACH_FORM,
					 XmNleftAttachment,XmATTACH_FORM,
					 XmNrightAttachment,XmATTACH_FORM,
                     NULL );
  	path_label = XtVaCreateManagedWidget("Selection", xmLabelWidgetClass, form,
					 XmNbottomAttachment, XmATTACH_WIDGET,
                     XmNbottomWidget, path_text,
					 XmNleftAttachment,XmATTACH_FORM,
					 XmNrightAttachment,XmATTACH_FORM,
					 XmNalignment,XmALIGNMENT_BEGINNING,
                     NULL );
	XtVaSetValues( form1, XmNbottomAttachment, XmATTACH_WIDGET,
						XmNbottomWidget, path_label, NULL );
	XtAddCallback(path_text, XmNactivateCallback,
                   &FileSelWindow::PathCallback, (XtPointer) this);

	strcpy(tn," ");
	strcpy(td,"/");
    
    //
	// Get the path to the current directory
    //
	//cwd = getcwd(NULL,512);
	tmp = startPath;
	cwd = XtMalloc( 512 );
	strcpy( cwd, tmp );
	if ( !cwd )
		cwd = "";
	printf("CWD is %s\n", cwd );
	path = cwd;
	strcpy(wd,cwd);
	strcat(wd,"/");
	int dircount = GetPathElemCount(wd);

    for ( i = 0; i <= dircount; i++) 
    {
		strcpy(tcwd,cwd);
		TruncDirString(tcwd,i);
    	dir_ = Directory::open(tcwd);
    	if (dir_ == NULL) {
        	break;
    	}
		// Load an initial dir
    	xmstr = (XmString *)XtMalloc (sizeof(XmString) * dir_->count());
		y =  dir_->count();
		for( numOfItems =  x = 0; x < y;x++ )
		{
    		const UString& s = *dir_->name(x);
			char *t = (char *)s.string();
			if (strcmp(t,".") != NULL && strcmp(t,"..") != NULL && t[0] != '.')
			{
				strcpy(tn,tcwd);
				strcat(tn,"/");
				strcat(tn,t);
				strcpy(tn," ");
				strcpy(&tn[1],t);
				if (dir_->is_directory(x))
				{
					pixmapR = 2;
					xmstr[numOfItems] = 
						XmStringCreate(t,XmSTRING_DEFAULT_CHARSET);
					ml->AddListItem(xmstr[numOfItems],pixmapR);
					numOfItems++;
				}
			}
		}
   		ml->NextList();
		delete dir_;
	}
	XmTextSetString(path_text,cwd);
	SetSelectedItems(cwd,dircount);
	XtFree(cwd);
	printf("BEFORE\n");
	ml->DisplayLists();
	printf("AFTER\n" );
	XtManageChild(form);
//    return (ml->GetTopWidget());

	tmp = strdup( GetPath() );
	tmp[strlen(tmp) - 1] = '\0';
	str = XmStringCreateLocalized( tmp );
	XtVaSetValues( path_lbl, XmNlabelString, str, NULL );
	XmStringFree( str );
	XtFree( tmp );

    return (form);
}

void 
FileSelWindow::ListCallback ( Widget  w,
                      XtPointer item,XtPointer pathPos )
{
    FileSelWindow * obj = (FileSelWindow *) w;
    printf("In the FileSelList item: = %s %d\n",item,pathPos);
	obj->SetNewPath((char *)item,(int)pathPos);
}
void
FileSelWindow::SetNewPath(char * item,int pathpos)
{
	int y;
	int numOfItems,x;
    XmString *xmstr; 
	struct stat st_buf;
	char tcwd[512];
	char tn[512];
	char td[512];
	Directory *dir_;
	int pixmapR;
	XmString str;
	char *tmp;

	strcpy(tn," ");
	strcpy(td,"/");
    //
	// Get the path to the current directory
    //
	strcpy(tcwd,wd);
	TruncDirString(tcwd,pathpos);
	strcat(tcwd,item);
	strcpy(wd,tcwd);
	path = wd;
	strcat(wd,"/");
	int dircount = GetPathElemCount(wd);
printf("tcwd = %s\n",tcwd);
   	dir_ = Directory::open(tcwd);
   	if (dir_ == NULL) {
       	return;
   	}
	// Load an initial dir
   	xmstr = (XmString *)XtMalloc (sizeof(XmString) * dir_->count());
	y= dir_->count();
	ml->ClearLists(pathpos+1);
	for( numOfItems =  x = 0; x < y;x++ )
	{
   		const UString& s = *dir_->name(x);
		char *t = (char *)s.string();
		if (strcmp(t,".") != NULL && strcmp(t,"..") != NULL && t[0] != '.')
		{
			strcpy(tn,tcwd);
			strcat(tn,"/");
			strcat(tn,t);
			strcpy(tn," ");
			strcpy(&tn[1],t);
			if (dir_->is_directory(x))
			{
				pixmapR = 2;
				xmstr[numOfItems] = 
					XmStringCreate(t,XmSTRING_DEFAULT_CHARSET);
				ml->AddListItem(xmstr[numOfItems],pixmapR);
				numOfItems++;
			}
		}
	}
#if 0
	for(numOfItems = x = 0; x < y; x++  )
	{
   		const UString& s = *dir_->name(x);
		char *t = (char *)s.string();
		if (strcmp(t,".") != NULL && strcmp(t,"..") != NULL && t[0] != '.')
		{
			strcpy(tn,tcwd);
			strcat(tn,"/");
			strcat(tn,t);
			stat(tn,&st_buf);
			strcpy(tn," ");
			strcpy(&tn[1],t);
			switch(  st_buf.st_mode & S_IFMT)
			{
				case S_IFIFO:     // 0x1000  /* fifo */
					pixmapR = 0;
					t = tn;
					break;
				case S_IFCHR:     // 0x2000  /* character special */
					pixmapR = 1;
					t = tn;
					break;
				case S_IFDIR:     // 0x4000  /* directory */
					pixmapR = 2;
					strcpy(&td[1],t);
					t = td;
					break;
				case S_IFNAM:     // 0x5000  /* XENIX 
					pixmapR = 3;
					t = tn;
					break;
				case S_IFBLK:     // 0x6000  /* block special */
					pixmapR = 4;
					t = tn;
					break;
				case S_IFREG:     // 0x8000  /* regular */
					pixmapR = 5;
					if (( st_buf.st_mode & S_IEXEC) || 
						st_buf.st_mode & S_IXUSR || 
							st_buf.st_mode & S_IXGRP )
						pixmapR = 8;  // execute
					t = tn;
					break;
				case S_IFLNK:     // 0xA000  /* symbolic link */
					pixmapR = 6;
					t = tn;
					break;
				case S_IFSOCK:    // 0xC000  /* socket */
					pixmapR = 7;
					t = tn;
					break;
				default:
					pixmapR = 5;
					t = tn;
					break;
				}
				if( ( st_buf.st_mode & S_IFMT ) == S_IFDIR )
				{
					xmstr[numOfItems] = 
						XmStringCreate(t,XmSTRING_DEFAULT_CHARSET);
					ml->AddListItem(xmstr[numOfItems],pixmapR);
					numOfItems++;
				}
#if 0
			if (dir_->is_directory(x))
			{
				strcpy(&td[1],t);
				t = td;
				xmstr[numOfItems] = XmStringCreate(t,XmSTRING_DEFAULT_CHARSET);
				ml->AddListItem(xmstr[numOfItems],0);
				numOfItems++;
			}
			else
			{
				if ( filter.length() > 0 && dir_->match(s, filter))
				{
					strcpy(&tn[1],t);
					t = tn;
					xmstr[numOfItems] = XmStringCreate(t,XmSTRING_DEFAULT_CHARSET);
					ml->AddListItem(xmstr[numOfItems],1);
					numOfItems++;
				}
			}
#endif
		}
	}
#endif
	delete dir_;
	ml->NextList();
	SetSelectedItems(tcwd,dircount);
	ml->DisplayLists();
	XmTextSetString(path_text,tcwd);

	tmp = strdup( GetPath() );
	tmp[strlen(tmp) - 1] = '\0';
	str = XmStringCreateLocalized( tmp );
	XtVaSetValues( path_lbl, XmNlabelString, str, NULL );
	XmStringFree( str );
	XtFree( tmp );
}
//
// Name: GetPathElemCount
//
// Description:  This member functions is used to determine the number
//               of path name components in a path name string. Called 
//               when initial lists are loaded and when the user selects 
//               a list item.
//
// Purpose: Get the number of path elements in a path string
//
// Returns: Number of path elements
//
// Side Effects:  NONE
//
int
FileSelWindow::GetPathElemCount(char * path)
{
	int i = 0;
	path++;				// past the root
	while ( *path != NULL )
	{
		while( *path != '/' && *cwd != NULL)
		{
			path++;
		}
		path++;
		i++;
	}
	return(i);
}
//
// Name: TruncDirString
//
// Description:  This member functions will change the argumented string
//               to contain only element_count path elements.  Called when 
//               the application wishes to change the directory path.
//               All path names start at the root of the tree.
//
// Purpose: Truncate a directory path name to contain only
//          element_count elements
//
// Returns:
//
// Side Effects:  Changes 'path' to contain only element_count
//                elements
void
FileSelWindow::TruncDirString(char * path,int element_count)
{
	int i = 0;
	path++;				// past the root
	while ( i != element_count && *cwd != NULL )
	{
		while( *path != '/' && *path != NULL)
		{
			path++;
		}
		if ( *path == NULL )				// Error 
		{
			assert("element_count > cwd's element_count");
		}
		path++;
		i++;
	}
	*path = NULL;
}
void
FileSelWindow::SetSelectedItems(char * cwd,int dircount)
{
	int i;
	int x,numOfItems;
	char *p;
	char tcwd[256];
	Directory *dir_;
	char *tt;
	int dirCount = 0;
//	String *s;


printf("cwd = %s\n",cwd);
	char *pcwd = strdup(cwd);
	tt = pcwd;
	pcwd++;
printf("pcwd = %s\n",pcwd);
	p = strtok(pcwd,"/");
printf("p = %s\n",p);
	pcwd++;

    for ( i = 0; i < dircount; i++) 
    {
		strcpy(tcwd,cwd);
		TruncDirString(tcwd,i);
    	dir_ = Directory::open(tcwd);
    	if (dir_ == NULL) {
        	break;
    	}
		for(numOfItems = x = 0; x < dir_->count(); x++ )
		{
    		//s = dir_->name(x);
    		const UString& s = *dir_->name(x);
			char *t = (char *)s.string();
			if (strcmp(t,".") != NULL && strcmp(t,"..") != NULL && t[0] != '.')
			{
				if (dir_->is_directory(x))
				{
					printf("dir is p=%s t=%s \n", p,t );
					if ( strcmp(p,t) == NULL )
					{
						ml->SelectItem(i,numOfItems);
						printf("The directory is %s list_index = %d %d\n\n", t,i,numOfItems );
					}
					numOfItems++;
				}
			}
		}
		p = strtok(NULL,"/");
		delete dir_;
	}
	XtFree(tt);
}

void FileSelWindow::PathCallback ( Widget    w,
                      XtPointer clientData,
                      XtPointer callData )
{
    FileSelWindow * obj = (FileSelWindow *) clientData;
	obj->GetPathText();
}
void FileSelWindow::GetPathText()
{
	char *buffer;
	printf("Int the Path callback\n");
	buffer = XmTextGetString(path_text);
	path = buffer;
	ChangePath();
}
void
FileSelWindow::ChangePath()
{
	int i,y;
    XmString *xmstr; 
	struct stat st_buf;
	int x,numOfItems;
	char tcwd[512];
	char tn[512];
	char td[512];
	Directory *dir_;
	int pixmapR;
	char * p;
	XmString str;
	char *tmp;

	strcpy(tn," ");
	strcpy(td,"/");

	ml->ClearLists();
	p = (char *)path.string();
	strcpy(wd,p);
printf("path = %s\n",wd);
	if ( wd[strlen(wd)-1] != '/')
		strcat(wd,"/");
printf("path = %s\n",wd);
    //
	// Get the path to the current directory
    //
	int dircount = GetPathElemCount(wd);
printf("dircount = %d\n",dircount);

    for ( i = 0; i <= dircount; i++) 
    {
		strcpy(tcwd,p);
		TruncDirString(tcwd,i);
    	dir_ = Directory::open(tcwd);
    	if (dir_ == NULL) {
        	break;
    	}
		// Load an initial dir
    	xmstr = (XmString *)XtMalloc (sizeof(XmString) * dir_->count());
		y =  dir_->count();
		for( numOfItems =  x = 0; x < y;x++ )
		{
   			const UString& s = *dir_->name(x);
			char *t = (char *)s.string();
			if (strcmp(t,".") != NULL && strcmp(t,"..") != NULL && t[0] != '.')
			{
				strcpy(tn,tcwd);
				strcat(tn,"/");
				strcat(tn,t);
				strcpy(tn," ");
				strcpy(&tn[1],t);
				if (dir_->is_directory(x))
				{
					pixmapR = 2;
					xmstr[numOfItems] = 
						XmStringCreate(t,XmSTRING_DEFAULT_CHARSET);
					ml->AddListItem(xmstr[numOfItems],pixmapR);
					numOfItems++;
				}
			}
		}
#if 0
		for(numOfItems =  x = 0; x < y;x++ )
		{
    		const UString& s = *dir_->name(x);
			char *t = (char *)s.string();
			if (strcmp(t,".") != NULL && strcmp(t,"..") != NULL && t[0] != '.')
			{
				strcpy(tn,tcwd);
				strcat(tn,"/");
				strcat(tn,t);
				stat(tn,&st_buf);
				strcpy(tn," ");
				strcpy(&tn[1],t);
				switch(  st_buf.st_mode & S_IFMT)
				{
					case S_IFIFO:     // 0x1000  /* fifo */
						pixmapR = 0;
						t = tn;
						break;
					case S_IFCHR:     // 0x2000  /* character special */
						pixmapR = 1;
						t = tn;
						break;
					case S_IFDIR:     // 0x4000  /* directory */
						pixmapR = 2;
						strcpy(&td[1],t);
						t = td;
						break;
					case S_IFNAM:     // 0x5000  /* XENIX 
						pixmapR = 3;
						t = tn;
						break;
					case S_IFBLK:     // 0x6000  /* block special */
						pixmapR = 4;
						t = tn;
						break;
					case S_IFREG:     // 0x8000  /* regular */
						pixmapR = 5;
						if (( st_buf.st_mode & S_IEXEC) || 
								st_buf.st_mode & S_IXUSR || 
									st_buf.st_mode & S_IXGRP )
							pixmapR = 8;  // execute
						t = tn;
						break;
					case S_IFLNK:     // 0xA000  /* symbolic link */
						pixmapR = 6;
						t = tn;
						break;
					case S_IFSOCK:    // 0xC000  /* socket */
						pixmapR = 7;
						t = tn;
						break;
					default:
						pixmapR = 5;
						t = tn;
						break;
				}
#if 0
				if (dir_->is_directory(x))
				{
					strcpy(&td[1],t);
					t = td;
					pixmapR = 0;
				}
				else
				{
					strcpy(&tn[1],t);
					t = tn;
					pixmapR = 1;
				}
#endif
				if( ( st_buf.st_mode & S_IFMT ) == S_IFDIR )
				{
					xmstr[numOfItems] = 
						XmStringCreate(t,XmSTRING_DEFAULT_CHARSET);
					ml->AddListItem(xmstr[numOfItems],pixmapR);
					numOfItems++;
				}
			}
		}
#endif
		ml->NextList();
		delete dir_;
	}
	XmTextSetString(path_text,p);
	SetSelectedItems(p,dircount);
	ml->DisplayLists();

	tmp = strdup( GetPath() );
	tmp[strlen(tmp) - 1] = '\0';
	str = XmStringCreateLocalized( tmp );
	XtVaSetValues( path_lbl, XmNlabelString, str, NULL );
	XmStringFree( str );
	XtFree( tmp );
}
