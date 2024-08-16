/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:server/xyzext.c	1.1"


#ifndef XYZEXT
/*
 * It doesn't make sense to not compile this file without -DXYZEXT;
 * so let's always turn it on!
 */
#define XYZEXT
#endif

/*
 * NOTE: There is an issue as to what memory allocation routines to use.
 * We would like to use the generic server routines Xalloc and Xfree but
 * since we are probably very interested in using XYZ with these routines,
 * we can't use them for XYZ's internal memory allocation - if we did
 * double entry into the XYZ subsystem could result.  So we use the
 * standard Unix malloc and free routines.  This might be a problem if
 * the Unix memory allocation routines shouldn't be used.  An override
 * is supplied by defining the XYZalloc and XYZfree macros to "use the
 * right allocator."
 *
 * NOTE: We also don't use ALLOCATE_LOCAL and DEALLOCATE_LOCAL since
 * they might rely on X's memory allocation functions.  Instead we
 * use XYZalloc and XYZfree.
 */
#ifndef XYZalloc
#define XYZalloc(size) malloc(size)
#endif
#ifndef XYZfree
#define XYZfree(ptr) free(ptr)
#endif

#include <string.h>

#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "extnsionst.h"

#define XK_LATIN1
#include <X11/keysymdef.h>

#define _XAMINE_YOUR_ZERVER_SERVER_
#include "xyzstr.h"
#include "xyz.h"

#define XYZ_NUM_BUCKETS 300

#define TAGNAME_IS_STATIC 0
#define SHOULD_FREE_TAGNAME 1

typedef struct _TagRec TagRec, *Tag;
struct _TagRec {
   char *tagname;
   int allocated_tagname;
   unsigned char tracelevel;
   int value;
   Tag next;
   struct XYZ_marker *marker;
};

static Bool xyz_instrument = FALSE;
static Bool xyz_trace = FALSE; 
static int xyz_status = XYZ_NO_ERROR;
static int current_tracelevel = XYZ_DEFAULT_TRACE_LEVEL;
static Tag tag_hash_table[XYZ_NUM_BUCKETS];

static
invalidate_markers(marker)
struct XYZ_marker *marker;
{
   while(marker != NULL) {
      marker->tag = NULL;
      marker = marker->next;
   }
}

static unsigned int
hash_function(string)
char *string;
{
   unsigned int value;

   value = 0;
   while(*string != '\0') {
      value <<= 1;
      value += *string;
      string++;
   }
   return value % XYZ_NUM_BUCKETS;
}

static Tag
tag_find_or_add(tagname, if_should_free_tagname)
char *tagname;
int if_should_free_tagname;
{
   Tag tag;
   unsigned int hash_num;

   hash_num = hash_function(tagname);
   tag = tag_hash_table[hash_num];
   while(tag != NULL) {
      if(!strcmp(tagname, tag->tagname)) {
	 if(if_should_free_tagname == SHOULD_FREE_TAGNAME) {
	    XYZfree(tagname);
	 }
	 /* in table; return tag */
	 return tag;
      }
      /* keep looking though bucket list */
      tag = tag->next;
   }
   /* tagname not in table; put in table */
   tag = (Tag) XYZalloc(sizeof(TagRec));
   if(tag == NULL) {
      /* memory allocation failure; stop doing XYZ */
      xyz_status = XYZ_ERROR;
      if(if_should_free_tagname == SHOULD_FREE_TAGNAME) {
	 XYZfree(tagname);
      }
      return NULL;
   }
   tag->tagname = tagname;
   /* we don't free in this case; we just remember if we should for later */
   tag->allocated_tagname = if_should_free_tagname;
   tag->value = 0;
   tag->tracelevel = XYZ_DEFAULT_TRACE_LEVEL;
   tag->next = tag_hash_table[hash_num];
   tag->marker = NULL;
   tag_hash_table[hash_num] = tag;
   /* return newly created tag */
   return tag;
}

static Tag
tag_find(tagname)
char *tagname;
{
   Tag tag;
   unsigned int hash_num;

   hash_num = hash_function(tagname);
   tag = tag_hash_table[hash_num];
   while(tag != NULL) {
      if(!strcmp(tagname, tag->tagname)) {
	 /* in table; return tag */
	 return tag;
      }
      /* keep looking though bucket list */
      tag = tag->next;
   }
   return NULL;
}

static void
tag_hash_table_init()
{
   int i;

   for(i=0;i<XYZ_NUM_BUCKETS;i++) {
      tag_hash_table[i] = NULL;
   }
}

#ifdef UNUSED_CODE

static void
tag_hash_table_flush()
{
   int i;
   Tag tag;
   Tag next;

   for(i=0;i<XYZ_NUM_BUCKETS;i++) {
      tag = tag_hash_table[i];
      while(tag != NULL) {
         next = tag->next;
	 invalidate_markers(tag->marker);
	 if(tag->allocated_tagname) {
	    XYZfree(tag->tagname);
	 }
	 XYZfree(tag);
	 tag = next;
      }
      tag_hash_table[i] = NULL;
   }
}

#endif /* UNUSED_CODE */

static void
reset_values()
{
   Tag tag;
   Tag next;
   Tag *tag_ptr;
   int i;

   for(i=0;i<XYZ_NUM_BUCKETS;i++) {
      tag_ptr = &tag_hash_table[i];
      tag = tag_hash_table[i];
      while(tag != NULL) {
	 next = tag->next;
	 if(tag->tracelevel == XYZ_DEFAULT_TRACE_LEVEL) {
	    *tag_ptr = tag->next;
	    invalidate_markers(tag->marker);
	    if(tag->allocated_tagname == SHOULD_FREE_TAGNAME) {
               XYZfree(tag->tagname);
	    }
	    XYZfree(tag);
	 } else {
	    tag->value = 0;
	    tag_ptr = &tag->next;
         }
	 tag = next;
      }
   }
}

static void
reset_trace_levels()
{
   Tag tag;
   Tag next;
   Tag *tag_ptr;
   int i;

   for(i=0;i<XYZ_NUM_BUCKETS;i++) {
      tag_ptr = &tag_hash_table[i];
      tag = tag_hash_table[i];
      while(tag != NULL) {
	 next = tag->next;
	 if(tag->value == 0) {
	    *tag_ptr = tag->next;
	    invalidate_markers(tag->marker);
	    if(tag->allocated_tagname == SHOULD_FREE_TAGNAME) {
	       XYZfree(tag->tagname);
	    }
	    XYZfree(tag);
	 } else {
	    tag->tracelevel = 0;
	    tag_ptr = &tag->next;
         }
	 tag = next;
      }
   }
}

static int
ProcXYZ_Instrument(client)
ClientPtr client;
{
   REQUEST(xXYZ_InstrumentReq);

   REQUEST_SIZE_MATCH(xXYZ_InstrumentReq);
   xyz_instrument = stuff->instrument;
   return client->noClientException;
}

static int
ProcXYZ_Trace(client)
ClientPtr client;
{
   REQUEST(xXYZ_TraceReq);

   REQUEST_SIZE_MATCH(xXYZ_TraceReq);
   xyz_trace = stuff->trace;
   return client->noClientException;
}

static int
ProcXYZ_SetCurTraceLevel(client)
ClientPtr client;
{
   REQUEST(xXYZ_SetCurTraceLevelReq);

   REQUEST_SIZE_MATCH(xXYZ_SetCurTraceLevelReq);
   current_tracelevel = stuff->tracelevel;
   return client->noClientException;
}

static int
ProcXYZ_QueryState(client)
ClientPtr client;
{
   REQUEST(xXYZ_QueryStateReq);
   xXYZ_QueryStateReply rep;
   int n;

   REQUEST_SIZE_MATCH(xXYZ_QueryStateReq);
   rep.type = X_Reply;
   rep.length = 0;
   rep.sequenceNumber = client->sequence;
   rep.instrument = xyz_instrument;
   rep.trace = xyz_trace;
   rep.tracelevel = current_tracelevel;
   rep.status = xyz_status;
   if(client->swapped) {
      swaps(&rep.sequenceNumber, n);
      swapl(&rep.length, n);
   }
   WriteToClient(client, sizeof(xXYZ_QueryStateReply), 
      (char *) &rep);
   return client->noClientException;
}

static int
ProcXYZ_GetTag(client)
ClientPtr client;
{
   REQUEST(xXYZ_GetTagReq);
   xXYZ_GetTagReply rep;
   Tag tag;
   char *tagname;
   int n;

   REQUEST_FIXED_SIZE(xXYZ_GetTagReq, stuff->nChars);
   tagname = (char *) XYZalloc(sizeof(char) * stuff->nChars + 1);
   if(!tagname) {
      return BadAlloc;
   }
   strncpy(tagname, (char *) &stuff[1], stuff->nChars);
   tagname[stuff->nChars] = '\0';
   tag = tag_find(tagname);

   rep.type = X_Reply;
   rep.length = 0;
   rep.sequenceNumber = client->sequence;
   if(tag != NULL) {
      rep.tracelevel = tag->tracelevel;
      rep.value = tag->value;
   } else {
      rep.tracelevel = XYZ_DEFAULT_TRACE_LEVEL;
      rep.value = 0;
   }
   if(client->swapped) {
      swaps(&rep.sequenceNumber, n);
      swapl(&rep.length, n);
      swapl(&rep.value, n);
   }
   WriteToClient(client, sizeof(xXYZ_GetTagReply), (char *) &rep);
   XYZfree(tagname);
   return client->noClientException;
}

static int
ProcXYZ_SetValue(client)
ClientPtr client;
{
   REQUEST(xXYZ_SetValueReq);
   Tag tag;
   char *tagname;

   REQUEST_FIXED_SIZE(xXYZ_SetValueReq, stuff->nChars);
   tagname = (char *) XYZalloc(sizeof(char) * stuff->nChars + 1);
   if(!tagname) {
      return BadAlloc;
   }
   /*
    * the tagname we just allocate will be freed by the
    * hash table management code; this is not a leak.
    */
   strncpy(tagname, (char *) &stuff[1], stuff->nChars);
   tagname[stuff->nChars] = '\0';
   tag = tag_find_or_add(tagname, SHOULD_FREE_TAGNAME);
   if(tag != NULL) {
      tag->value = stuff->value;
   } else {
      return BadAlloc;
   }
   return client->noClientException;
}

static int
ProcXYZ_SetTraceLevel(client)
ClientPtr client;
{
   REQUEST(xXYZ_SetTraceLevelReq);
   Tag tag;
   char *tagname;

   REQUEST_FIXED_SIZE(xXYZ_SetTraceLevelReq, stuff->nChars);
   tagname = (char *) XYZalloc(sizeof(char) * stuff->nChars + 1);
   if(!tagname) {
      return BadAlloc;
   }
   /*
    * the tagname we just allocate will be freed by the
    * hash table management code; this is not a leak.
    */
   strncpy(tagname, (char *) &stuff[1], stuff->nChars);
   tagname[stuff->nChars] = '\0';
   tag = tag_find_or_add(tagname, SHOULD_FREE_TAGNAME);
   if(tag != NULL) {
      tag->tracelevel = stuff->tracelevel;
   } else {
      return BadAlloc;
   }
   return client->noClientException;
}

static Bool
TagNameMatch(pattern, tagname)
char *pattern;
char *tagname;
{
   while(*pattern != '\0') {
      if(*pattern == XK_asterisk) {
	 pattern++;
	 if(*pattern == '\0') {
	    return TRUE;
	 }
	 while(!TagNameMatch(pattern, tagname)) {
	    if(*tagname++ == '\0') {
	       return FALSE;
	    }
	 }
	 return TRUE;
      } else {
	 if(*tagname == '\0') {
	    return FALSE;
	 } else {
	    if((*pattern != XK_question) && (*pattern != *tagname)) {
	       return FALSE;
	    }
	 }
      }
      pattern++;
      tagname++;
   }
   return *tagname == '\0';
}

static Bool
TagNameMatchPatterns(patterns, numpats, tagname)
char **patterns;
int numpats;
char *tagname;
{
   int i;

   for(i=0;i<numpats;i++) {
      if(TagNameMatch(patterns[i], tagname)) {
	 return TRUE;
      }
   }
   return FALSE;
}

static int
ProcXYZ_ListValues(client)
ClientPtr client;
{
   REQUEST(xXYZ_ListValuesReq);
   xXYZ_ListValuesReply rep;
   Tag tag;
   int npats;
   char **patterns;
   char *pattern;
   int patlen;
   char **tagnames;
   int *values;
   unsigned int maxtags;
   int count;
   int rc;
   int i;
   char *buffer;
   char *bufptr;
   int total_length;
   int length;
   int n;

   REQUEST_AT_LEAST_SIZE(xXYZ_ListValuesReq);
   maxtags = stuff->maxtags;
   npats = stuff->npats;
   patterns = (char **) XYZalloc(sizeof(char*) * npats);
   if(!patterns) {
      return BadAlloc;
   }
   /* 
    * buffer serves double duty for extracting patterns and
    * building reply.
    */
   buffer = (char *) &stuff[1];
   for(i=0; i<npats; i++) {
      /* notice that each pattern starts at an aligned 4-byte boundary */
      patlen = *((unsigned short *) buffer);
      if(client->swapped) {
	 /* gross and ugly that we do this here! */
	 swaps(&patlen, n);
      }
      pattern = (char *) XYZalloc(sizeof(char) * patlen + 1);
      if(pattern == NULL) {
	 /* deallocate all the patterns we have allocated so far */
	 for(n=0; n<i; n++) {
	   XYZfree(patterns[n]);
	 }
	 XYZfree(patterns);
	 return BadAlloc;
      }
      /* +2 advances past patlen field */
      strncpy(pattern, buffer + 2, patlen);
      pattern[patlen] = '\0';
      patterns[i] = pattern;
      buffer += patlen;
      buffer = (char *) ((2 + (int)buffer + 3) & ~3);
   }

   rep.type = X_Reply;
   rep.sequenceNumber = client->sequence;

   values = (int *) XYZalloc(sizeof(int) * maxtags);
   tagnames = (char **) XYZalloc(sizeof(char*) * maxtags);
   if((!values) || (!tagnames)) {
      if(values) XYZfree(values);
      if(tagnames) XYZfree(tagnames);
      return BadAlloc;
   }
   count = 0;
   total_length = 0;
   for(i=0;i<XYZ_NUM_BUCKETS;i++) {
      tag = tag_hash_table[i];
      while(tag != NULL) {
	 if(tag->value != 0) {
	    rc = TagNameMatchPatterns(patterns, npats, tag->tagname);
	    if(rc == 1) {
               /* matches */
	       if(count < maxtags) {
	          tagnames[count] = tag->tagname;
	          values[count] = tag->value;
	          total_length += (int)(4 + 2 + strlen(tag->tagname) + 3) >> 2;
	       }
               count++;
	    }
	 }
	 tag = tag->next;
      }
   }
   rep.length = total_length;
   rep.returned = min(maxtags, count);
   rep.total = count;
   if(total_length > 0) {
      buffer = (char *) XYZalloc(total_length << 2);
      if(!buffer) {
         for(i=0;i<npats;i++) {
            XYZfree(patterns[i]);
         }
	 XYZfree(patterns);
         XYZfree(values);
         XYZfree(tagnames);
         return BadAlloc;
      }
      bufptr = buffer;
      for(i=0; i < (int)rep.returned; i++) {
         length = strlen(tagnames[i]);
         *((unsigned short *)bufptr) = length;
         if(client->swapped) {
            swaps((unsigned short *)bufptr, n);
         }
         bufptr += sizeof(unsigned short);
         bcopy(tagnames[i], bufptr, length);
         bufptr += length;
         bufptr = (char *) ((int) (bufptr + 3) & ~3);
         *((int *) bufptr) = values[i];
         if(client->swapped) {
            swapl((int *)bufptr, n);
         }
         bufptr += sizeof(int);
      }
   }

   if(client->swapped) {
      swaps(&rep.sequenceNumber, n);
      swapl(&rep.length, n);
      swaps(&rep.returned, n);
      swaps(&rep.total, n);
   }
   WriteToClient(client, sizeof(xXYZ_ListValuesReply), &rep);
   if(rep.length) {
      WriteToClient(client, total_length << 2, buffer);
      XYZfree(buffer);
   }
   for(i=0;i<npats;i++) {
      XYZfree(patterns[i]);
   }
   XYZfree(patterns);
   XYZfree(values);
   XYZfree(tagnames);
   return client->noClientException;
}

static int
ProcXYZ_ResetValues(client)
ClientPtr client;
{
   REQUEST(xXYZ_ResetValuesReq);

   REQUEST_SIZE_MATCH(xXYZ_ResetValuesReq);
   reset_values();
   xyz_status = XYZ_NO_ERROR;
   return client->noClientException;
}

static int
ProcXYZ_ResetTraceLevels(client)
ClientPtr client;
{
   REQUEST(xXYZ_ResetTraceLevelsReq);

   REQUEST_SIZE_MATCH(xXYZ_ResetTraceLevelsReq);
   reset_trace_levels();
   xyz_status = XYZ_NO_ERROR;
   return client->noClientException;
}

static int
ProcXYZ_Dispatch(client)
ClientPtr client;
{
   REQUEST(xReq);
   switch(stuff->data) {
   case X_XYZ_Instrument:
      return ProcXYZ_Instrument(client);
   case X_XYZ_Trace:
      return ProcXYZ_Trace(client);
   case X_XYZ_SetCurTraceLevel:
      return ProcXYZ_SetCurTraceLevel(client);
   case X_XYZ_QueryState:
      return ProcXYZ_QueryState(client);
   case X_XYZ_GetTag:
      return ProcXYZ_GetTag(client);
   case X_XYZ_SetValue:
      return ProcXYZ_SetValue(client);
   case X_XYZ_SetTraceLevel:
      return ProcXYZ_SetTraceLevel(client);
   case X_XYZ_ListValues:
      return ProcXYZ_ListValues(client);
   case X_XYZ_ResetValues:
      return ProcXYZ_ResetValues(client);
   case X_XYZ_ResetTraceLevels:
      return ProcXYZ_ResetTraceLevels(client);
   default:
      return BadRequest;
   }
}

static int
SProcXYZ_Instrument(client)
ClientPtr client;
{
   REQUEST(xXYZ_InstrumentReq);
   int n;

   swaps(&stuff->length, n);
   return ProcXYZ_Instrument(client);
}

static int
SProcXYZ_Trace(client)
ClientPtr client;
{
   REQUEST(xXYZ_TraceReq);
   int n;

   swaps(&stuff->length, n);
   return ProcXYZ_Trace(client);
}

static int
SProcXYZ_SetCurTraceLevel(client)
ClientPtr client;
{
   REQUEST(xXYZ_SetCurTraceLevelReq);
   int n;

   swaps(&stuff->length, n);
   return ProcXYZ_SetCurTraceLevel(client);
}

static int
SProcXYZ_QueryState(client)
ClientPtr client;
{
   REQUEST(xXYZ_QueryStateReq);
   int n;

   swaps(&stuff->length, n);
   return ProcXYZ_QueryState(client);
}

static int
SProcXYZ_GetTag(client)
ClientPtr client;
{
   REQUEST(xXYZ_GetTagReq);
   int n;

   swaps(&stuff->length, n);
   swaps(&stuff->nChars, n);
   return ProcXYZ_GetTag(client);
}

static int
SProcXYZ_SetValue(client)
ClientPtr client;
{
   REQUEST(xXYZ_SetValueReq);
   int n;

   swaps(&stuff->length, n);
   swaps(&stuff->nChars, n);
   swapl(&stuff->value, n);
   return ProcXYZ_SetValue(client);
}

static int
SProcXYZ_SetTraceLevel(client)
ClientPtr client;
{
   REQUEST(xXYZ_SetTraceLevelReq);
   int n;

   swaps(&stuff->length, n);
   swaps(&stuff->nChars, n);
   return ProcXYZ_SetTraceLevel(client);
}

static int
SProcXYZ_ListValues(client)
ClientPtr client;
{
   REQUEST(xXYZ_ListValuesReq);
   int n;

   swaps(&stuff->length, n);
   swaps(&stuff->npats, n);
   swaps(&stuff->maxtags, n);
   return ProcXYZ_ListValues(client);
}

static int
SProcXYZ_ResetValues(client)
ClientPtr client;
{
   REQUEST(xXYZ_ResetValuesReq);
   int n;

   swaps(&stuff->length, n);
   return ProcXYZ_ResetValues(client);
}

static int
SProcXYZ_ResetTraceLevels(client)
ClientPtr client;
{
   REQUEST(xXYZ_ResetTraceLevelsReq);
   int n;

   swaps(&stuff->length, n);
   return ProcXYZ_ResetTraceLevels(client);
}

static int
SProcXYZ_Dispatch(client)
ClientPtr client;
{
   REQUEST(xReq);
   switch(stuff->data) {
   case X_XYZ_Instrument:
      return SProcXYZ_Instrument(client);
   case X_XYZ_Trace:
      return SProcXYZ_Trace(client);
   case X_XYZ_SetCurTraceLevel:
      return SProcXYZ_SetCurTraceLevel(client);
   case X_XYZ_QueryState:
      return SProcXYZ_QueryState(client);
   case X_XYZ_GetTag:
      return SProcXYZ_GetTag(client);
   case X_XYZ_SetValue:
      return SProcXYZ_SetValue(client);
   case X_XYZ_SetTraceLevel:
      return SProcXYZ_SetTraceLevel(client);
   case X_XYZ_ListValues:
      return SProcXYZ_ListValues(client);
   case X_XYZ_ResetValues:
      return SProcXYZ_ResetValues(client);
   case X_XYZ_ResetTraceLevels:
      return SProcXYZ_ResetTraceLevels(client);
   default:
      return BadRequest;
   }
}

static void
XYZ_Reset(extEntry)
ExtensionEntry *extEntry;
{
}

void
XamineYourZerverInit(argc, argv)
int argc;
char *argv[];
{
   ExtensionEntry *extEntry;
   int i;
   static int beenhere = FALSE;

   extEntry = AddExtension(XAMINE_YOUR_ZERVER_NAME,
      XYZ_NumberEvents, XYZ_NumberErrors,
      ProcXYZ_Dispatch, SProcXYZ_Dispatch, 
      XYZ_Reset, StandardMinorOpcode);

   if(!beenhere) {
      beenhere = TRUE;
      tag_hash_table_init();

      xyz_instrument = FALSE;
      xyz_trace = FALSE;
      xyz_status = XYZ_NO_ERROR;

      /* look for -xyz command line arg to start with XYZ on */
      if(extEntry != NULL) {
         for(i=1;i<argc;i++) {
	    if(!strcmp(argv[i], "-xyz")) {
               xyz_instrument = TRUE;
	       return;
	    }
	    if(!strcmp(argv[i], "-xyztrace")) {
	       xyz_trace = TRUE;
	       return;
	    }
	    if(!strcmp(argv[i], "-xyzboth")) {
	       xyz_instrument = TRUE;
	       xyz_trace = TRUE;
               return;
   	    }
         }
      }
   }
}

void
XamineYourZerver(tagname, delta, marker)
char *tagname;
int delta;
struct XYZ_marker *marker;
{
   Tag tag;
   int tracelevel;

   tag = (Tag) ((marker->tag != NULL) ? marker->tag : NULL);
   if(xyz_instrument) {
      if(xyz_status == XYZ_NO_ERROR) {
	 if(tag == NULL) {
            tag = tag_find_or_add(tagname, TAGNAME_IS_STATIC); 
	    marker->tag = (void *) tag;
	    marker->next = tag->marker;
	    tag->marker = marker;
	 }
         tag->value += delta;
      }
   }
   if(xyz_trace) {
      if(tag == NULL) {
         tag = tag_find(tagname); 
	 if(tag != NULL) {
	    marker->tag = (void *) tag;
	    marker->next = tag->marker;
	    tag->marker = marker;
	 }
      }
      tracelevel = (tag != NULL) ? tag->tracelevel : XYZ_DEFAULT_TRACE_LEVEL;
      if(tracelevel <= current_tracelevel) {
         ErrorF("XYZ: %s = %d\n", tagname, tag == NULL ? 0 : tag->value);
      }
   }
}

