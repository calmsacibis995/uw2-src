/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sccs:lib/cassi/deltack.c	6.8.1.1"
#include <errno.h>
#include "../../hdr/had.h"
#include "../../hdr/defines.h"
#include "../../hdr/filehand.h"
#include <ccstypes.h>
#include <pfmt.h>
#include <locale.h>
#include <sys/euc.h>
#include <limits.h>
/*#include <stdlib.h>*/

#define FOREVER 1
#define MAXLIST 15
#define MAXLIST2 20
#define MAXLENCMR 12
char *strtok(),*strcpy(),*strchr(),*logname();
static char errorlog[FILESIZE];		 /* log cmts errors here */
static FILE *efd;
static int	promdelt(), msg(), verif(), getinfo();

/*
*
*deltack(pfile,mrs,nsid,apl) performs the necessary validations on a delta
*involving the CMF FRED file of active CMR's
*
*/

deltack(pfile,mrs,nsid,apl)
	char pfile[];	/* the pfile name */

	char *mrs;		/* list of mrs from pfile */
	char *nsid;		/* sid id */
	char *apl;		/* application from the file */
	{
	 static char type[10],sthold[10];
	 char hold[302],*h,*fred, *strcat();
	 unsigned	strlen();
	 void	error();

	/*check for the existence of the p.file*/
	if(!pfile)
		{
		 error(":186:Pfile non existent at deltack ");
		 return(0);
		}
	/*if no application serious error */
	if(!apl)
		{
		 error (":187:no application found with -fz flag");
		 return(0);
		}
	/*check for and retrieve FRED given the application name*/
	if(!getinfo(&fred,apl))
		{
		 error(":188:no FRED file or system name in admin directory ");

		 return(0);
		}
	strcpy(errorlog,fred);
	*(errorlog + strlen(errorlog) - 11) = NULL;
	strcat(errorlog,"LOG");
	if(!(mrs))
		{
		 if (efd=fopen(errorlog,"a"))
			{fprintf(efd,"***CASSI REPORTS ERROR: no CMRS in pfile: %s\n",pfile);
			 (void) fclose(efd);
			}
		 error(":189:CMRs not on P.file -serious inconsistency");

		 return(0);
		}
	 if(!promdelt(mrs,sthold,type,fred))
		{
		 return(0);
		}
	/*now build the chpost line  */
	(void) strcpy(hold,mrs);
	h=strtok(hold,",\0");
	(void) msg(apl,pfile,h,sthold,type,nsid,fred);
	while(h=strtok(0,",\0 "))
		{
		 (void) msg(apl,pfile,h,sthold,type,nsid,fred);
		}
	return(1);
	}



/*
*
*promdelt(cmrs,stat,type,fred) allows one to modify the cmrs list and 
*enrter the type and status for the input to the MR system via the net
*
*/

static int
promdelt(cmrs,stat,type,fred)
char *cmrs,*stat,*type,*fred;
{
	 extern char had[];
	 extern char * Cassin;
	 extern char * Sflags[];
	 static char hold[300],nold[300], *cmrlist[MAXLIST2 + 1];
	 char answ[100];
	 int i,j,numcmrs,fdflag=0,eqflag=0,badflag=0;
	 char *h;
	 int	strcmp(), fatal();
	 unsigned	strlen();

	 /*place the cmrs list in the array of pointers and remove the commas */
	 strcpy(hold,cmrs);
	 (void) strtok(hold,",");
   	 cmrlist[0]=hold;
	 for(i=1;i<MAXLIST;i++)
		{
		 if((cmrlist[i]=strtok(0,",\0"))==(char *)NULL)
			{
			 break;
			}
		 }
	numcmrs=i;
	/* remove invalid cmrs from the list if now none set flag */
	for(i=0;cmrlist[i];i++)
		{
		 if(!verif(cmrlist[i],fred))
			{
			 /* a 'sd' cmr has been found */
			 if(numcmrs > 1)
				{
				 for(j=i;j<numcmrs;j++)
					{
					 cmrlist[j] = cmrlist[j + 1];
					}
				 numcmrs--;
				}
			 else /* there is a last cmr to delete set flag */
				{
				 cmrlist[0] = NULL;
				 badflag = 1;
				 numcmrs--;
				}
			}
		}
	if(HADZ) /*force delta flag on */
	{
		if (badflag) /* no legal cmrs in list */
		{
		 if (efd=fopen(errorlog,"a"))
			{fprintf(efd,"***CASSI REPORTS ERROR: no CMRS at sd\n");
			 (void) fclose(efd);
			}
			(void) fatal(":190:no CMR's left, delta forbidden\n");

		}
		(void) strcpy(stat,"sd");
		if (Sflags[TYPEFLAG - 'a'])
			strcpy(type,Sflags[TYPEFLAG - 'a']);
		else
			strcpy(type,"sw");
			/* rebuild cmr comma separated list*/
		cat(nold,cmrlist[0],0);
		for(i=1;i<numcmrs;i++)
		{
			cat(nold,",",cmrlist[i],0);
		}
		strcpy(cmrs,nold);
		return(1);
	}
	while(FOREVER)
	  {
	   if(!badflag)
		{
		 pfmt(stdout, MM_NOSTD,
			":191:the CMRs for this delta now are:\n");
		 for(i=0;cmrlist[i + 1];i++)
			{
			 printf(" %s,",cmrlist[i]);
			}
		 printf("%s\n",cmrlist[i]);
		 printf("OK ??");
		 (void) gets(answ);  
		 if((!strcmp(answ,"y")) || (!strcmp(answ,"ye")) || (!strcmp(answ,"yes")))
			{
			 break;
			}
		 }
	   else
		 {
		   pfmt(stdout, MM_NOSTD,
			":192:you must input a least 1 valid cmr number\n");
		 }
		 /*now prompt for new cmrs to add to the list*/
		while(FOREVER)
			{
			 eqflag = 0;
			 pfmt(stdout, MM_NOSTD,
				":193:enter new CMR number or 'CR' ");
			 (void) gets(answ);
			 if(answ[0] == NULL)
				if(!badflag)
					{
					 break;
					}
				else
					{
					 continue;
					}
			 h=(char *) malloc((unsigned)(strlen(answ) + 6));
			 strcpy(h,answ);
			 /*check for duplicate */
			 for (i=0;i<numcmrs;i++)
				{
				 if(!strcmp(h,cmrlist[i]))
					{
					 eqflag=1;
					 break;
					}
				}
			 if(eqflag==1)
				{
				 pfmt(stdout, MM_NOSTD,
					":194:\n duplicate CMR number ignored\n");
				 continue;
				}
			 /*now verify that the cmr is in FRED */
			 if(!verif(h,fred))
				{
				 pfmt(stdout, MM_NOSTD,
					":195:\n invalid CMR ignored \n");
				 continue;
				}
			 /*the addition is valid*/
			cmrlist[numcmrs] = h;
			badflag = 0; /* turn off the no cmrs found indicator */
			if(++numcmrs > MAXLIST2)
				{
				 pfmt(stdout, MM_NOSTD,
					":196:\n too many CMRs added no more allowed \n");
				 break;
				}
			}
		/*now delete mrs from list */
		while(FOREVER)
			{
			 fdflag = 0;
			 pfmt(stdout, MM_NOSTD,
				":197:\n CMR number to delete or (CR) ? ");
			 (void) gets(answ);
			 if(!(*answ))
				{
				 break;
				}
			 /*if one left break */
			 if(numcmrs==1)
				{
				 pfmt(stdout, MM_NOSTD,
					":198:\n only one CMR left can't delete more\n");
				 break;
				}
			 /*check if request is on list */
			 for(i=0;i<numcmrs;i++)
				{
				 if (!strcmp(answ,cmrlist[i]))
					{
					 fdflag=1;
					 for(j=i;j<numcmrs;j++)
						{
						 cmrlist[j]=cmrlist[j+1];
						}
					 break;
					}
				}
			if(fdflag==0)
				{
				 pfmt(stdout, MM_NOSTD,
					":199:\n not on list request ignored\n");
 				continue;
				}
			else
				{
				 numcmrs--;
				 /* we have oneless cmr */
				}
			}
		}
		/*here ends the cmr loop */
		/*set type to proper value*/
		if ( Sflags[TYPEFLAG - 'a'])
			strcpy(type,Sflags[TYPEFLAG - 'a']);
		else
			strcpy(type,"sw");
		/*set status*/
		strcpy(stat,"sd");
	/*reformat the cmrlist into a comma separated cmr list*/
	cat(nold,cmrlist[0],0);
	for(i=1;i<numcmrs;i++)
		{
		 cat(nold,nold,",",cmrlist[i],0);
		}
	 strcpy(cmrs,nold);
	return(1);
	}



/*
*
*msg(syst,cmrs,stats,types,sids) formats a message and calls cmrpost
*
*/
static int
msg(syst,name,cmrs,stats,types,sids,fred)
	char *syst,*name,*cmrs,*stats,*types,*sids,*fred;
	{
	 FILE *fd;
	 extern char *Sflags[];
	 char *k;
	 char pname[FILESIZE],*ptr,holdfred[100],dir[100],path[FILESIZE];
	 struct stat stbuf;
	 int noexist = 0;
	 char *strchr(), *strcat();
	 char *getcwd(), *abspath();
	 int	stat(), chmod(), chown(), fatal();
	
	/* if -fm flag contains a value substitute a the value for name */
	 if(k=Sflags[MODFLAG - 'a'])
	 {
		name = k;
	 }
	 if(*name != '/') /* not full path name */
		{
		 if(getcwd(path,sizeof(path)) == NULL)
			(void) fatal(":87:getcwd() failed (ge20)");
		 cat(pname,path,"/",name,0);
		}
	else
		{
		 strcpy(pname,name);
		}
	(void) abspath(pname);				/* get rid of . and .. */
/******** the net is replaced by psudonet ******
*	  sprintf(holdit,"netq %s chpost  %s q %s %s MID=%s MFS=%s q q",syst,cmrs,pname,types,sids,stats); 
*	 system(holdit);
************************************************/
	 /* build the name of the  termLOG file */
	 strcpy(holdfred,fred);
	 ptr=strchr(holdfred,'.');
	 *ptr=NULL;
	 strcat(holdfred,"source");
	 strcpy(dir,holdfred);
	 strcat(holdfred,"/termLOG");
	 if(stat(holdfred,&stbuf) == -1)
		noexist = 1; /*new termLOG */
	 if(!(fd=fopen(holdfred,"a")))
		{
		 if (efd=fopen(errorlog,"a"))
			{fprintf(efd,"***CASSI REPORTS ERROR: can't write to FRED : %s\n",pname);
			 (void) fclose(efd);
			}
		 (void) fatal(":200:Cassi Interface Msg not writable\n");

		 return(0);
		}
	 fprintf(fd,"%s chpost %s q %s %s MID=%s MFS=%s MPA=%s q q\n",syst,cmrs,pname,types,sids,stats,logname());
	 (void) fclose(fd);
	 if(noexist) /*new termLOG make owner of /BD/source owner of file */
	 {
		if(stat(dir,&stbuf) == -1)
		{
		 if (efd=fopen(errorlog,"a"))
			{fprintf(efd,"***CASSI REPORTS ERROR: can't write to BD/source : %s\n",pname);
			 (void) fclose(efd);
			}
			(void) fatal(":145:Cassi BD/source not writable\n");

		}
		(void) chmod(holdfred,(mode_t)0666);
		(void) chown(holdfred,stbuf.st_uid,stbuf.st_gid); 
	}
	 return(1);
}

/*
*
*verif(cmr,fred) calls the verification prog and returns 0 if failed
* 
*/
static int
verif(cmr,fred)
	char *cmr,*fred;
	{
	int sweep();
	int res;
	char *cmrpass[2];
	unsigned strlen();

	/* if length of cmr number not = MAXLENCMR error 
	*    all cmr numbers are 12 characters long 
	*/
	if (strlen(cmr) != MAXLENCMR)
		{
			return(0);
		}
	cmrpass[0] = cmr;
	cmrpass[1] = NULL;
	res=sweep(SEQVERIFY,fred,NULL,'\n',WHITE,40,cmrpass,NULL,NULL,
		 (int(*)()) NULL, (int (*)()) NULL);
	if(res != FOUND)
		{
		 return(0);
		}
	else
		{
		 return(1);
		}
	}
/*
*
*getinfo(freddy,sys)
*    get the name of the fred file and the system name 
*/
static int
getinfo(freddy,sys)
	char **freddy,*sys;
	{
	extern int errno;
	extern char *gf();
	struct stat buf;
	*freddy=gf(sys);
	if(!(**freddy))
		{
		 pfmt(stdout, MM_NOSTD,
			":201:got to bad FRED file %s\n",*freddy);
		 return(0);
		}
	 if(stat(*freddy,&buf))
		{
		return(0);
		}
	 return(1);
	}
