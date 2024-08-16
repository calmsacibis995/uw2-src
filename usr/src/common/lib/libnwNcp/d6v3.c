/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d6v3.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS6v3Search
         (
            pNWAccess pNetAccess,
            nflag32  flFlags,
            pnuint32 pluIterationHandle,
            nuint32  luBaseEntryID,
            nuint32  luScope,
            nuint32  luNodesToSearch,
            nuint32  luInfoType,
            nuint32  luInfoFlags,
            nuint32  blAllAttributes,
            nuint32  luAttrNamesLen,
            pnuint8  pAttrNames,
            nuint32  luFilterLen,
            pnuint8  pFilter,
            pnuint32 pluNumberOfNodesSearched,
            pnuint32 pluSearchInfoLen,
            pnuint8  pSearchInfo
         )

REMARKS:

DSV_SEARCH_ENTRIES                              6 (0x06)

Request

  nuint32      version;
  nuint32      flags;
  nuint32      iterationHandle;
  nuint32      baseEntryID;
  nuint32      scope;
  nuint32      NodesToSearch;
  nuint32      infoType;
  nuint32      infoFlags;
  boolean      allAttributes;
  unicode      attrNames[];
  SearchFilter filter;      //defined below

Response

  nuint32         iterationHandle;
  nuint32         numberOfNodesSearched;
  nuint32         infotype
  nuint32         lengthOfSearchInfoEntries;
  SearchInfo      entries[];             //defined below
  nuint32         LengthOfSearchReferrals;
  SearchReferral  referral[];            //defined below
Definitions of Parameter Types

(Types used in Request)

  union   SearchItem
  {
     Attribute  equal;          //tag #7 //defined below
     Attribute  greaterOrEqual; //tag #8
     Attribute  lessOrEqual;    //tag #9
     unicode    present;        //tag #15
     Attribute  approximateMatch;//tag #10
     any_t      searchRDN;
     any_t      searchBaseClass;
  };

  struct  Attribute
  {
     unicode attrName;
     any_t   attrValue;
  }

  union   SearchFilter
  {
     SearchItem Item;
     SearchFilterAnd[];    //recursive
     SearchFilterOr[];     //recursive
     SearchFilterNot;      //recursive
  }

(Types used in Response)

  struct  SearchInfo
  {
     nuint32   entryFlags;
     nuint32   subordinateCount;
     nuint32   modificationTime;
     unicode   baseClass;
     unicode   dn;
     AttrInfo  entryInfo; //defined below
  };

  The entryFlags field defines the following bits:

  DS_ALIAS_ENTRY        Set if the entry is an alias entry  DS_PARTITION_ROOT     Set if the entry is the root entry of a partition

  DS_CONTAINER_ENTRY    Set if the entry is allowed to have subordinates in the
                        Directory.

  The subordinateCount field contains the number of entries which are immediately
  subordinate to this entry in the Directory. If the subordinate count is unknown,
  0xFFFFFFFF is returned. (The subordinate count may not be known if the entry is an
  external entry.)

  The modificationTime field contains the time of the last modification to the entry. If the
  modification time is unknown, 0 is returned. (The modification time may not be known if
  the entry is an external entry.)

  The baseClass field contains the name of the entry class which was used to create the
  entry.

  The DN field contains the DN of the subordinate entry.

  The entryInfo field holds the requested information. This union either contains a list of
  attribute names or an array of structures containing the attribute name, attribute syntax
  ID, and value set. The type of information returned depends on the infoType in the
  request.

  union    AttrInfo
  {
     unicode      attrNames[];
     SearchAttr   attributes[];//defined below
  };

  struct  SearchAttr
  {
     nuint32   attrSyntaxID;
     unicode   attrName;
     any_t     attrValue[];
  };

  The type definition for attrValue uses the any_t base type. This type means that the
  encoding for an attribute value is ambiguously defined. In this instance, the encoding of
  an attribute value depends on the accompanying attribute name. The unicode decoding
  routine will have to interpret the syntax in order to determine decoding of the
  accompanying attribute value array. If a client agent doesn't know how to decode a
  particular attribute, that attribute can be bypassed simply by skipping the array count as
  the size of the undecoded value.  struct  SearchReferral
  {
     nuint32   referralType;
     unicode   searchReferral;
  }

Remarks

This verb allows you to search a portion of the Directory for specific entries and return
selected information from those entries.

The following Request parameters are defined:

version
flags
iterationHandle
baseEntryID
scope
numberOfNodesToSearch
infoType
infoFlags          //used only in version 3
allAttributes
attrNames[]
filter

The iterationHandle is used to continue a previous request where the response was too large
to fit in a single message. It should be set initially to 0xFFFFFFFF and thereafter to the value
returned by the previous response.

The baseEntryID identifies the entry relative to which the search is to take place.

The scope indicates whether the search is to be applied to the following:

DS_SEARCH_ENTRY         0
DS_SEARCH_SUBORDINATES  1
DS_SEARCH_SUBTREE       2
The infoType, allAttributes,and attrNames parameters indicate what information from the
entry is requested. As listed below, the infoType specifies a request for attribute name
information only, or for both attribute name and attribute value information:

ValueInfoType

0    DS_ATTRIBUTE_NAME
1    DS_ATTRIBUTE_NAME and DS_ATTRIBUTE_VALUE

If no attribute information is to be returned, the infoType is not meaningful.

If allAttributes is TRUE (1), information about all attributes of the entry is requested.

If allAttributes is FALSE (0), information about attributes named in the attrNames is
requested. If allAttributes is FALSE and attrNames is empty, no attribute information will be
returned, only entry information.

If the request is version 3, the request message also includes the infoFlags. In this case, the
response will include different information depending on the value of infoFlags as shown
below:

Flag

DS1_ENTRY_ID                  nuint32    entryID;
DS1_ENTRY_FLAGS               nuint32    flags;
DS1_SUBORDINATE_COUNT         nuint32    subordinateCount;
DS1_MODIFICATION_TIME         nuint32    modificationTime;
DS1_MODIFICATION_TIMESTAMP    TIMESTAMP  modificationTimeStamp;
DS1_CREATION_TIMESTAMP        TIMESTAMP  creationTimeStamp;
DS1_PARTITION_ROOT_ID         nuint32    partitionID;
DS1_PARENT_ID                 nuint32    parentID;
DS1_REVISION_COUNT            nuint32    revisionCount;
DS1_BASECLASS                 unicode    baseClass;
DS1_ENTRY_RDN                 unicode    rdn;
DS1_ENTRY_DN                  unicode    dn;
DS1_PARTITION_ROOT_DN         unicode    partitionDN;
DS1_PARENT_DN                 unicode    parentDN;

  struct  TIMESTAMP
  {
     nuint32seconds;
     int16replicaNumber;
     int16event;
  }

The infoFlags, therefore, allows you to request only certain information about an entry such
as the entry ID, the entry flags, the subordinate count, the modification time, etc.

The filter is used to eliminate entries from the search space which are not of interest.
Information will only be returned on entries which satisfy the filter. A search filter is either a
search item or an expression involving simpler search filters composed together using the
logical operators ITEM, AND, OR, and NOT.

The SearchFilter is undefined if it is a search item which is undefined or if it involves one or
more simpler filters, all of which are undefined. Otherwise, for the following conditions the
search filter produces the following results:

ITEM      TRUE if and only if the corresponding SearchItem is true.

AND       TRUE unless any of the nested SearchFilters is FALSE. Also, if there are no
          nested search filters, the result is TRUE.

OR        FALSE unless any of the nested SearchFilters is TRUE. Also, if there are no
          nested search filters, the result is FALSE.

NOT       TRUE if and only if the nested SearchFilter is FALSE.

A SearchItem is an assertion about the presence or value(s) of an attribute of a particular type
in the entry under test. Each such assertion is TRUE, FALSE, or undefined.

Every SearchItem (except those for RNN and baseClass) includes an attribute name which
identifies the particular attribute concerned. Any assertion about the value of such an attribute
is only defined if the attribute name is known, and the purported attribute value(s) conform to
the attribute syntax defined for that attribute.

Notes:

1) Where the conditions above are not met, the search filter is undefined.

2) Access control restrictions may require that the filter item be considered undefined.

Assertions about the value of an attribute are evaluated using the matching rules associated
with the attribute syntax defined for that attribute. A matching rule not defined for a particular
attribute syntax cannot be used to make assertions about that attribute.

Note: Where the condition above is not met, the search item is not defined.
A search item may be undefined (as described above). Otherwise, for the assertions (equality,
substrings, greaterOrEqual, lessOrEqual, present, approximateMatch), the search item
produces the following results, assuming the syntax supports type of comparison:

Equality        TRUE if and only if there is a value of the attribute equal to the
                assertion.

Substrings      TRUE if and only if there is a value of the attribute in which the
                specified substring pattern matches. The substrings indicated by the
                pattern shall be non-overlapping, and may (but need not) be separated
                from the ends of the attribute value and from one another by zero or
                more string elements. The substring pattern consists of a sequence of
                characters and wildcards. The wildcard character is the asterisk ('*').
                The wildcard can be escaped by the backslash ('\'), and the backslash
                can escape itself. The wild card is interpreted as matching the shortest
                sequence of characters from the point already matched in the target
                string to the start of the next non-wild sequence of characters in the
                search pattern. Any non-wild characters in the search pattern must
                match a corresponding sequence of characters in the target string. A
                trailing wild card matches to the end of the target string.

GreaterOrEqual  TRUE if and only if the relative ordering (as defined by the
                appropriate ordering algorithm) places the supplied value before any
                value of the attribute.

LessOrEqual     TRUE if and only if the relative ordering (as defined by the
                appropriate ordering algorithm) places the supplied value after any
                value of the attribute.

Present         TRUE if and only if the named attribute is present in the entry.

ApproximateMatchTRUE if and only if there is a value of the attribute which matches
                that which is asserted by some locally-defined approximate matching
                algorithm (e.g. spelling variations, phonetic match, etc.). If
                approximate matching is not supported, this search item should be
                treated as a match for equality.

Aliases among the subordinates of the base entry shall be dereferenced during the search,
subject to the setting of the searchAliases . If the searchAliases is TRUE, aliases shall be
dereferenced; if the parameter is FALSE, aliases shall not be dereferenced. If the
searchAliases is TRUE, the search shall continue in the subtree of the aliased entry. The
request succeeds if the base entry is located, regardless of whether there are any subordinates
to return.Note: As a corollary to the above, the outcome of an (unfiltered) search applied to a single
entry may not be identical to a read which seeks to interrogate the same set of attributes of the
entry. This is because the latter will return an attribute error if none of the selected attributes
exist in the entry.

The following reply parameters are defined:

iterationHandle
numberOfNodesSearched
infotype
lengthOfSearchInfoEntries
entries[]
LengthOfSearchReferrals
referral[]

The iterationHandle in the response is set to 0xFFFFFFFF if this message contains the
remainder of the response. Otherwise, this ID can be used in subsequent Search requests to
obtain further portions of the response. The level of granularity for partial results is an
individual attribute value of a value set. If a value set is split across two or more response
messages, the entry information and attribute name of the current value set is repeated in each
response. Note that an individual value cannot be split across multiple responses. This means
that a value (plus message overhead) cannot be bigger than the minimum-maximum message
size supported by the NCP protocol (MAX_MESSAGE_LEN). The first implementation will
limit maximum attribute value length to MAX_MESSAGE_LEN.

The numberOfNodesSearched is the number of nodes searched.

The infoType indicates what information from the entry is requested. This parameter is
described in the request.

The lengthOfSearchInfoEntries is the length (in bytes) of the information returned for a
specific entry search.

The entries are the entries (objects) in the Directory that are being searched.

The lengthOfSearchReferrals is the length (in bytes) of the information returned for a referral
search.

The referral[] is a set of entry referrals (possibly empty). Each entry referral consists of the
name of an entry and a tag to indicate whether the entry is an alias. Note as explained earlier,
that the entryFlags field in the SearchInfo structure used in the response has an alias bit
(DS_ALIAS_ENTRY) that is set if an entry is an alias.The referral will have a non zero count if regions of the Directory were not explored by this
server. Information in referral allows the client agent to continue the processing of the Search
operation by contacting other servers if it so chooses.

The entry is subordinate to the base entry of the search, but possibly not stored in the current
server. The client agent is responsible for the possible overlap in the entrys which are
returned in the various entry referrals.

ARGS:

INCLUDE:

RETURN:

SERVER:

CLIENT:

SEE:

NCP:

CHANGES:
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/

N_GLOBAL_LIBRARY(NWRCODE)
NWNCPDS6v3Search
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   pnuint32 pluIterationHandle,
   nuint32  luBaseEntryID,
   nuint32  luScope,
   nuint32  luNodesToSearch,
   nuint32  luInfoType,
   nuint32  luInfoFlags,
   nuint32  blAllAttributes,
   nuint32  luAttrNamesLen,
   pnuint8  pAttrNames,
   nuint32  luFilterLen,
   pnuint8  pFilter,
   pnuint32 pluNumberOfNodesSearched,
   pnuint32 pluSearchInfoLen,
   pnuint8  pSearchInfo
)
{
   nuint8   buReq1[39]; /* 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 3 (padding) */
   nuint8   buReq2[7];  /* 4 + 3 (padding) */
   nuint8   buRep[11];  /* 4 + 4 + 3 (padding) */
   NWCFrag  reqFrag[3];
   NWCFrag  repFrag[2];
   NWRCODE  err;
   nuint    uActualReplySize;
   pnuint8  cur;

   cur = buReq1;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)3;
   NCopyLoHi32(&cur[4], &flFlags);
   NCopyLoHi32(&cur[8], pluIterationHandle);
   NCopyLoHi32(&cur[12], &luBaseEntryID);
   NCopyLoHi32(&cur[16], &luScope);
   NCopyLoHi32(&cur[20], &luNodesToSearch);
   NCopyLoHi32(&cur[24], &luInfoType);
   NCopyLoHi32(&cur[28], &luInfoFlags);
   NCopyLoHi32(&cur[32], &blAllAttributes);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 36;

   if (blAllAttributes != 0L && pAttrNames != NULL)
   {
      reqFrag[1].pAddr = pAttrNames;
      reqFrag[1].uLen = (nuint)luAttrNamesLen;
   }
   else
   {
      cur = buReq2;
      NAlign32(&cur);

      *(pnuint32)&cur[0] = (nuint32)0;

      reqFrag[1].pAddr = cur;
      reqFrag[1].uLen = 4;
   }

   reqFrag[2].pAddr = pFilter;
   reqFrag[2].uLen = (nuint)luFilterLen;

   cur = buRep;
   NAlign32(&cur);

   repFrag[0].pAddr = cur;
   repFrag[0].uLen = 8;

   repFrag[1].pAddr = pSearchInfo;
   repFrag[1].uLen = (nuint)*pluSearchInfoLen;

   err = NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)6, (nuint)3,
            reqFrag, (nuint)2, repFrag, &uActualReplySize);

   if (err < 0)
      return err;

   cur = repFrag[0].pAddr;
   NCopyLoHi32(pluIterationHandle, &cur[0]);

   if (pluNumberOfNodesSearched != NULL)
      NCopyLoHi32(pluNumberOfNodesSearched, &cur[4]);

   /* uActualReplySize is the number of bytes of data received.  We
      need to set the *pluSearchInfoLen to the number of bytes received
      minus the 8 bytes that comprise the iterationhandle and the
      count of objects searched (unused).
   */
   *pluSearchInfoLen = uActualReplySize - 8;

   return err;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d6v3.c,v 1.7 1994/09/26 17:41:22 rebekah Exp $
*/
