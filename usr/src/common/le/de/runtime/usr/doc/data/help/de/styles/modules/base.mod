<!-- /lfs/doc/DMG/Projects/sgml/sccs/styles/modules/s.base.mod 1.13 94/10/15 -->
<!-- ***************** CONTAINERS AND TITLE ELEMENTS **************** -->

<group name="bodytype">
   <font-family>        &bodfam; </>
   <font-size>          &bodsize; </>
   <line-spacing>       &bodspac; </>
   <font-weight>        medium </>
   <font-slant>         roman </>
</group>

<group name="boldbody">
   <font-family>        &bodfam; </>
   <font-size>          &bodsize; </>
   <line-spacing>       &bodspac; </>
   <vertical-offset>    &boldoff; </>
   <font-weight>        bold </>
</group>

<group name="computertype">
   <font-family>        &comfam; </>
   <font-size>		&rombump; </>
</group>

<group name="italcomputertype">
   <font-family>        &comfam; </>
   <font-slant>		&comslan; </>
   <font-size>		&itbump; </>
</group>

<group name="codetype">
   <font-family>        &codfam; </>
   <font-size>		&rombump; </>
</group>

<group name="container">
   <break-before>       line </>
</group>

<group name="hidden">
   <hide>               all </>
   <break-before>	none </>
   <break-after>	none </>
</group>

<!-- Hierarchical elements below the chapter level -->

<group name="t1">
   <font-family>        &ttlfam; </>
   <font-size>          &s1size; </>
   <line-spacing>       &s1spac; </>
   <space-before>       &s1fore; </>
   <space-after>        &s1aft; </>
   <left-indent>        &ctin; </>
   <font-weight>        bold </>
</group>

<group name="t2">
   <font-family>        &ttlfam; </>
   <font-size>          &s2size; </>
   <line-spacing>       &s2spac; </>
   <space-before>       &s2fore; </>
   <space-after>        &s2aft; </>
   <left-indent>        &ctin; </>
   <font-weight>        bold </>
   <font-slant>         &ttlslan; </>
</group>

<group name="t3">
   <font-family>        &ttlfam; </>
   <font-size>          &s3size; </>
   <line-spacing>       &s3spac;</>
   <space-before>       &s3fore; </>
   <space-after>        &s3aft; </>
   <font-weight>        bold </>
</group>

<group name="t4">
   <font-family>        &ttlfam; </>
   <font-size>          &s4size; </>
   <line-spacing>       &s4spac; </>
   <space-before>       &s4fore; </>
   <space-after>        &s4aft; </>
   <font-weight>        bold </>
   <font-slant>         &ttlslan; </>
</group>

<group name="t5">
   <font-family>        &ttlfam; </>
   <font-size>          &s5size; </>
   <line-spacing>       &s5spac; </>
   <space-before>       &s5fore; </>
   <space-after>        &s5aft; </>
   <font-weight>        bold </>
</group>

<style name="SECT1" group="container"></style>
<style name="SECT1,TITLE">
   <select>	sect1_as_attr(RENDERAS,ancestor(SECT1)) </>
</style>
<style name="sect1_as_" group="t1"> </>
<style name="sect1_as_sect1" group="t1"> </>
<style name="sect1_as_sect2" group="t2"> </>

<style name="SECT2" group="container"></style>
<style name="SECT2,TITLE">
   <select>	sect2_as_attr(RENDERAS,ancestor(SECT2)) </>
</style>
<style name="sect2_as_" group="t2"> </>
<style name="sect2_as_sect2" group="t2"> </>
<style name="sect2_as_sect3" group="t3"> </>

<style name="SECT3" group="container"></style>
<style name="SECT3,TITLE">
   <select>	sect3_as_attr(RENDERAS,ancestor(SECT3)) </>
</style>
<style name="sect3_as_" group="t3"> </>
<style name="sect3_as_sect3" group="t3"> </>
<style name="sect3_as_sect4" group="t4"> </>

<style name="SECT4" group="container"></style>
<style name="SECT4,TITLE">
   <select>	sect4_as_attr(RENDERAS,ancestor(SECT4)) </>
</style>
<style name="sect4_as_" group="t4"> </>
<style name="sect4_as_sect4" group="t4"> </>
<style name="sect4_as_sect5" group="t5"> </>

<style name="SECT5" group="container"></style>
<style name="SECT5,TITLE" group="t5"></style>

<style name="PARTINTRO" group="container"></style>
<style name="PARTINTRO,TITLE" group="t1"></style>

<!-- ******************** REFERENCE PAGE ELEMENTS ******************* -->

<style name="REFENTRY" group="container"> 
    <break-before>	&bbreak; </>
</style>

<style name="REFNAMEDIV">
   <break-after>	line </>
</style>

<style name="REFSECT1" group="container"></style>
<style name="REFSECT2" group="container"></style>

<style name="REFNAME">
   <break-before>	line </>
   <text-before>&namewd;  </>
</style>

<style name="REFNAME,#text-before">
   <break-after>	line </>
   <font-family>        &ttlfam; </>
   <font-size>          &s3size; </>
   <line-spacing>       &s3spac;</>
   <space-before>       &s3fore; </>
   <space-after>        &s3aft; </>
   <font-weight>        bold </>
</style>

<style name="REFPURPOSE">
   <text-before> - </>
   <break-after>	line </>
   <select>		REFPURPOSE,purp1_eq(cnum(),1) </>
</style>

<style name="purp1_True">
   <break-before>	none </>
</style>

<style name="purp1_False">
   <break-before>	line </>
</style>

<!-- The default REFMETA style is for man pages at the chapter level -->
<style name="REFMETA">
   <font-family>        &ttlfam; </>
   <font-size>          &ctsize; </>
   <line-spacing>       &ctspac; </>
   <space-before>       &ctfore; </>
   <space-after>        &ctaft; </>
   <left-indent>        &ctin; </>
   <font-weight>        bold </>
</style>
<style name="REFERENCE,REFENTRY,REFMETA" group="t1"></style>
<style name="CHAPTER,REFENTRY,REFMETA" group="t1"></style>
<style name="SECT1,REFENTRY,REFMETA" group="t2"></style>

<style name="MANVOLNUM">
   <text-before>\(</>
   <text-after>)</>
</style>

<style name="REFCLASS" group="container">
   <select>		REFCLASS,refclass_norole_isnull(attr(ROLE, me())) </>
</style>

<style name="refclass_norole_False">
   <text-before>attr(ROLE, me()): </>
</style>

<style name="refclass_norole_True"></style>

<style name="REFCLASS,#text-before">
   <break-after>	line </>
   <font-weight>	bold </>
</style>

<style name="REFSYNOPSISDIV" group="container"></style>
<style name="REFSYNOPSISDIV,TITLE" group="t3">
   <space-before>	&lead1; </>
</style>

<style name="SYNOPSIS" group="cverbatim"></style>
<style name="SYNOPSIS,FUNCTION">
   <font-weight>	bold </>
</style>

<style name="REFSECT1,TITLE" group="t3"></style>
<style name="REFSECT1,PARA" group="igraf"></style>
<style name="REFSECT2,TITLE" group="t4">
   <left-indent>        +=&in2; </>
</style>
<style name="REFSECT2,PARA" group="igraf"></style>

<style name="REFMISCINFO" group="hidden"></style>

<!-- ********************** PARAGRAPH ELEMENTS ********************** -->

<group name="graf">
   <break-before>       line </>
   <space-before>       &lead1; </>
   <space-after>        &lead1; </>
</group>

<group name="igraf">
   <break-before>       line </>
   <space-before>       &lead1; </>
   <space-after>        &lead1; </>
   <left-indent>        +=&in2; </>
</group>

<group name="runinhead">
   <font-weight>        bold </>
   <break-after>        none </>   
</group>

<group name="lowtitle">
   <font-family>        &bodfam; </>
   <font-size>          &ltsize; </>
   <line-spacing>       &ltspac; </>
   <space-before>       &ltfore; </>
   <space-after>        &ltaft; </>
   <font-weight>        bold </>
</group>

<style name="p1_True">
   <break-before>       none </>
   <space-after>        &lead1; </>
</style>

<style name="p1_False" group="graf"></style>
<style name="PARA" group="graf"></style>
<style name="FORMALPARA" group="graf"></style>
<style name="FORMALPARA,TITLE" group="runinhead"></style>
<style name="FORMALPARA,PARA">
   <break-before>       none </>
   <space-after>        &lead1; </>
</style>

<style name="BRIDGEHEAD" group="lowtitle"></style>

<style name="ABSTRACT" group="graf"></style>
<style name="ABSTRACT,TITLE" group="lowtitle"></style>

<style name="AUTHORBLURB" group="graf"></style>
<style name="AUTHORBLURB,TITLE" group="lowtitle"></style>

<style name="BLOCKQUOTE">
   <left-indent>	+=&in3; </>
   <right-indent>	+=&in3; </>
</style>

<!-- ********************* MISC. PARA CONTAINERS ******************** -->

<style name="EPIGRAPH" group="container"></style>

<!-- ************************** ADMONITIONS ************************* -->

<group name="admonish">
   <left-indent>        +=&admonin; </>
   <break-before>       line </>
   <space-before>       &lead1; </>
   <break-after>        line </>
</group>

<group name="admoslug">
   <font-family>        &slgfam; </>
   <font-size>          &slgsize; </>
   <vertical-offset>    &slgoff; </>
   <font-weight>        bold </>
   <break-after>        none </>
   <text-after>  </>
</group>

<style name="IMPORTANT" group="admonish">
   <text-before>&impwd; </>
</style>
<style name="IMPORTANT,#TEXT-BEFORE" group="admoslug"></style>
<style name="IMPORTANT,PARA">
   <select>             p1_eq(cnum(),1) </>
</style>

<style name="NOTE" group="admonish">
   <text-before>&notewd; </>
</style>
<style name="NOTE,#TEXT-BEFORE" group="admoslug"></style>
<style name="NOTE,PARA">
   <select>             p1_eq(cnum(),1) </>
</style>

<style name="TIP" group="admonish">
   <text-before>&tipwd; </>
</style>
<style name="TIP,#TEXT-BEFORE" group="admoslug"></style>
<style name="TIP,PARA">
   <select>             p1_eq(cnum(),1) </>
</style>

<style name="WARNING" group="admonish">
   <text-before>&warnwd; </>
   <font-weight>	bold </>
</style>
<style name="WARNING,#TEXT-BEFORE" group="admoslug"></style>
<style name="WARNING,PARA">
   <select>             p1_eq(cnum(),1) </>
</style>

<style name="CAUTION" group="admonish">
   <text-before>&ctnwd; </>
   <font-weight>	bold </>
</style>
<style name="CAUTION,#TEXT-BEFORE" group="admoslug"></style>
<style name="CAUTION,PARA">
   <select>             p1_eq(cnum(),1) </>
</style>

<!-- ***************************** LISTS **************************** -->

<style name="SIMPLELIST" group="container">
   <font-weight>	medium </>
   <space-after>	&lead1; </>
</style>

<style name="SIMPLELIST,MEMBER">
   <break-before>	line </>
</style>

<style name="LISTITEM,PARA">
   <select>             p1_eq(cnum(),1) </>
</style>

<style name="LISTITEM,FORMALPARA">
   <select>             p1_eq(cnum(),1) </>
</style>

<!-- Itemized lists -->

<style name="ITEMIZEDLIST" group="container">
   <font-weight>	medium </>
</style>

<style name="REFSECT1,ITEMIZEDLIST">
   <left-indent>        +=&in2; </>
</style>

<style name="REFSECT2,ITEMIZEDLIST">
   <left-indent>        +=&in2; </>
</style>

<group name="ilitem">
   <break-before>       line </>
   <space-before>       &lead1; </>
   <left-indent>        +=&in2; </>
   <first-indent>       -&in2; </>
</group>

<style name="ITEMIZEDLIST,LISTITEM" group="ilitem">
   <text-before>switch(attr(MARK,ancestor(ITEMIZEDLIST)),BULLET,&bullet1;,BOX,&ckbox;,DASH,&dash;,'DEFAULT',&bullet1;)</>
</style>

<style name="ITEMIZEDLIST,LISTITEM,#text-before">
   <font-family>        switch(attr(MARK,ancestor(ITEMIZEDLIST)),BULLET,&b1fam;,BOX,&ckfam;,'DEFAULT',&b1fam;)</>
   <font-size>          switch(attr(MARK,ancestor(ITEMIZEDLIST)),BULLET,&b1size;,BOX,&cksize;,'DEFAULT',&b1size;)</>
   <vertical-offset>    switch(attr(MARK,ancestor(ITEMIZEDLIST)),BULLET,&b1off;,BOX,&ckoff;,'DEFAULT',&b1off;)</>
   <font-weight>        bold </>
   <break-after>        none </>
</style>

<style name="ITEMIZEDLIST,LISTITEM,ITEMIZEDLIST,LISTITEM" group="ilitem">
   <text-before>switch(attr(MARK,ancestor(ITEMIZEDLIST)),BULLET,&bullet2;,BOX,&ckbox;,DASH,&dash;,'DEFAULT',&bullet2;)</>
</style>

<style name="ITEMIZEDLIST,LISTITEM,ITEMIZEDLIST,LISTITEM,#text-before">
   <font-family>        switch(attr(MARK,ancestor(ITEMIZEDLIST)),BULLET,&b2fam;,BOX,&ckfam;,'DEFAULT',&b2fam;)</>
   <font-size>          switch(attr(MARK,ancestor(ITEMIZEDLIST)),BULLET,&b2size;,BOX,&cksize;,'DEFAULT',&b2size;)</>
   <vertical-offset>    switch(attr(MARK,ancestor(ITEMIZEDLIST)),BULLET,&b2off;,BOX,&ckoff;,'DEFAULT',&b2off;)</>
   <font-weight>        bold </>
   <break-after>        none </>
</style>

<!-- Ordered lists -->

<group name="olitem">
   <break-before>       line </>

   <space-before>       &lead1; </>
   <left-indent>        +=&ol1in; </>
   <first-indent>       -&ol1in; </>
</group>

<style name="ORDEREDLIST" group="container">
   <font-weight>	medium </>
</style>

<style name="ORDEREDLIST,LISTITEM">
   <select>     olistitem_attr(NUMERATION,ancestor(ORDEREDLIST)) </>
</style>

<style name="olistitem_arabic" group="olitem">
   <text-before>cnum().</>
</style>

<style name="olistitem_upperalpha" group="olitem">
   <text-before>format(cnum(),LETTER).</>
</style>

<style name="olistitem_loweralpha" group="olitem">
   <text-before>format(cnum(),letter).</>
</style>

<style name="olistitem_upperroman" group="olitem">
   <text-before>format(cnum(),ROMAN).</>
</style>

<style name="olistitem_lowerroman" group="olitem">
   <text-before>format(cnum(),roman).</>
</style>

<style name="ORDEREDLIST,LISTITEM,#text-before">
   <break-after>        none </>
</style>

<!-- Procedure lists -->

<style name="PROCEDURE" group="container"></style>

<style name="PROCEDURE,TITLE" group="lowtitle">
   <break-before>       line </>
   <font-weight>        bold </>
   <left-indent>        0 </>
</style>

<style name="PROCEDURE,STEP" group="olitem">
   <text-before>cnum().</>
</style>

<style name="STEP,#text-before">
   <break-after>        none </>
   <font-weight>	bold </>
</style>

<style name="PROCEDURE,STEP,PARA">
   <select>             proc_p1_eq(cnum(),1) </>
</style>

<style name="SUBSTEPS" group="container"></style>

<style name="PROCEDURE,STEP,SUBSTEPS,STEP" group="olitem">
   <text-before>cnum(ancestor(STEP))format(cnum(),letter).</>
</style>

<style name="PROCEDURE,STEP,SUBSTEPS,STEP,PARA">
   <select>             proc_p1_eq(cnum(),1) </>
</style>

<style name="proc_p1_True">
   <break-before>       none </>
   <space-after>        &lead1; </>
   <font-weight>        bold </>
</style>

<style name="proc_p1_False" group="graf"></style>

<!-- ************************ LITERAL LAYOUTS *********************** -->

<style name="EXAMPLE" group="container"></style>
<style name="EXAMPLE,TITLE" group="lowtitle"></style>
<style name="REFSECT1,EXAMPLE,TITLE" group="t3"></style>
   
<style name="PROGRAMLISTING,FUNCTION">
   <font-weight>	bold </>
</style>

<!-- ************ SYSTEM MESSAGE STYLES ************* -->

<style name="MSGENTRY" group="container">
   <break-before>	on </>
   <space-before>	&lead2; </>
</style>

<style name="MSG">
   <font-family>	&bodfam; </>
   <font-size>		&bodsize; </>
   <line-spacing>	&bodspac; </>
   <foreground>		&msgclr; </>
   <font-weight>	bold </>
   <break-before>	on </>
   <break-after>	off </>
</style>

<style name="MSGSUB" group="container">
   <left-indent>	+=&in2; </>
</style>

<style name="MSGSUB,MSGTEXT" group="graf">
   <left-indent>	+=&in2; </>
</style>

<group name="explanlabel">
   <font-weight>	bold </>
   <break-after>	none </>
</group>

<style name="MSGORIG">
   <break-before>	off </>
   <space-after>	&lead1; </>
   <text-before>&origwd;  </>
</style>
<style name="MSGORIG,#text-before" group="explanlabel"></style>

<style name="MSGLEVEL">
   <break-before>	off </>
   <space-after>	&lead1; </>
   <text-before>&levlwd;  </>
</style>
<style name="MSGLEVEL,#text-before" group="explanlabel"></style>

<style name="MSGEXPLAN">
   <break-before>	off </>
   <space-after>	&lead1; </>
</style>
<style name="MSGEXPLAN,TITLE" group="explanlabel">
   <text-after>  </>
</style>
<style name="MSGEXPLAN,PARA">
   <select>             p1_eq(cnum(),1) </>
</style>

<!-- *********************** GLOSSARY ELEMENTS ********************** -->

<style name="GLOSSENTRY" group="container"></style>

<style name="GLOSSENTRY,GLOSSTERM">
   <font-family>        &ttlfam; </>
   <font-size>          &s3size; </>
   <line-spacing>       &s3spac; </>
   <foreground>         &msgclr; </>
   <font-weight>        bold </>
   <break-before>       line </>
   <space-before>       &lead2; </>
   <space-after>        &lead1; </>
   <left-indent>        &ctin; </>
</style>

<style name="GLOSSDEF" group="graf"></style>

<!-- ************************ VARIABLE LISTS ************************ -->

<style name="VARIABLELIST" group="graf"></style>

<style name="VARLISTENTRY,TERM">
   <break-before>       line </>
   <break-after>        line </>
   <left-indent>        +=&in2; </>
</style>

<style name="VARLISTENTRY,LISTITEM">
   <break-before>       line </>
   <space-before>       2 </>
   <break-after>        line </>
   <left-indent>        +=&in4; </>
</style>

<!-- *********************** GRAPHIC ELEMENTS *********************** -->

<style name="FIGURE">
   <break-before>       line </>
</style>

<!-- ********************* NOVELL INDEX ELEMENTS ******************** -->

<style name="IDXTERM" group="container"></style>

<style name="ILEVEL1" group="container"></style>
<style name="ILEVEL1,ITERM">
   <font-family>        &ttlfam; </>
   <font-size>          &s4size; </>
   <line-spacing>       &s4spac; </>
   <space-before>       &s4fore; </>
   <font-weight>        bold </>
   <break-before>       line </>
</style>

<style name="ILEVEL2" group="container">
   <left-indent>        +=&in2; </>
</style>
<style name="ILEVEL2,ITERM">
   <break-before>       line </>
</style>

<style name="ILEVEL3" group="container">
   <left-indent>        +=&in4; </>
</style>
<style name="ILEVEL3,ITERM">
   <break-before>       line </>
</style>

<style name="IDXSRC">
   <break-before>       line </>
   <break-after>        none </>
   <font-slant>         &bodslan; </>
   <left-indent>        +=&in4; </>
</style>

<style name="ISEE" group="container">
   <text-before>&seewd; </>
   <left-indent>        +=&in10; </>
   <first-indent>       -&in2; </>
</style>

<style name="ISEEALSO" group="container">
   <text-before>&alsowd; </>
   <left-indent>        +=&in10; </>
   <first-indent>       -&in2; </>
</style>

<!-- *********************** IN-LINE ELEMENTS *********************** -->

<style name="ANCHOR" group="hidden"></style>
<style name="COMMENT" group="hidden"></style>

<style name="LOTENTRY" group="hidden"></style>
<style name="PARTNUM" group="hidden"></style>
<style name="TITLEABBREV" group="hidden"></style>
<style name="TOCENTRY1" group="hidden"></style>
<style name="TOCENTRY2" group="hidden"></style>
<style name="TOCENTRY3" group="hidden"></style>
<style name="TOCENTRY4" group="hidden"></style>
<style name="TOCENTRY5" group="hidden"></style>

<style name="CITETITLE"> <font-slant> &bodslan; </></style>
<style name="COMMAND"> <font-weight> bold </></style>
<style name="MEDIALABEL"> <font-slant> &bodslan; </></style>
<style name="EMPHASIS"> <font-weight> bold </></style>
<style name="FIRSTTERM"> <font-weight> bold </></style>
<style name="FUNCTION"> <font-weight> bold </></style>
<style name="GLOSSTERM"> <font-weight> bold </></style>
<style name="KEYCAP">
   <font-weight>	bold </>
   <foreground>		&keyclr; </>
   <text-before>if(eq(attr(ROLE),HasNoBrackets),<,)</>
   <text-after>if(eq(attr(ROLE),HasNoBrackets),>,)</>
</style>
<style name="SHORTFORM"> <font-weight> bold </></style>
<style name="SYSTEMITEM"> <font-slant> &bodslan; </></style>

<style name="INTERFACE">
   <font-weight>	bold </>
   <font-slant>		&bodslan; </>
   <select>		INTERFACE,iface_attr(CLASS)_single_and(gt(sub(me(),lsibling(INTERFACE)),2),gt(abs(sub(rsibling(INTERFACE),me())),2)) </>
</style>
<style name="iface__single_True"></style>
<style name="iface__single_False"></style>
<style name="iface_menuitem_single_True"></style>
<style name="iface_menuitem_single_False">
   <break-before>	line </>
   <left-indent>	+=mult(&in2;,cnum()); </>
</style>

<style name="LITERAL" group="computertype"></style>
<style name="REPLACEABLE" group="italcomputertype"></style>
<style name="STRUCTFIELD" group="italcomputertype"></style>
<style name="PARAMETER" group="italcomputertype"></style>
<style name="TERM,PARAMETER" group="italcomputertype">
   <font-size>		+=4 </>
</style>

<style name="COMPUTEROUTPUT" group="codetype"></style>
<style name="PROMPT" group="codetype"></style>
<style name="USERINPUT" group="codetype"> <font-weight> bold </></style>

<style name="SUBSCRIPT">
   <font-size>		-=4 </>
   <vertical-offset>	-4 </>
</style>

<style name="SUPERSCRIPT">
   <font-size>		-=4 </>
   <vertical-offset>	4 </>
</style>

<!-- ******************* NOVELL-SPECIFIC ELEMENTS ******************* -->

<style name="LOCALBLOCKING">
   <text-before>&locwd;   </>
   <break-after>	line </>
   <left-indent>	+=&in2;
</style>
<style name="LOCALBLOCKING,#text-before">
   <font-weight>        bold </>
</style>

<style name="REMOTEBLOCKING">
   <text-before>&remwd;   </>
   <break-after>	line </>
   <left-indent>	+=&in2;
</style>
<style name="REMOTEBLOCKING,#text-before">
   <font-weight>        bold </>
</style>

<style name="APPLICATION">
   <select>             APPLICATION,class1_eq(cnum(),1) </>
</style>
<style name="APPLICATION,#text-before">
   <font-weight>        bold </>
</style>

<style name="class1_True">
   <left-indent>	+=&in2;
   <text-before>&classwd;   </>
</style>

<style name="class1_False">
   <break-before>       none </>
   <text-before>  </>
</style>

<style name="SMALLCAPS">
   <font-size>		-=2 </>
</style>

<style name="DIAMOND">
   <break-before>	off </>
   <text-before>	&diamond; </>
   <break-after>	off </>
</style>

<style name="DIAMOND,#text-before">
   <font-family>	&diamfam; </>
   <font-slant>		roman </>
</style>

<style name="HR">
   <break-before>       line </>
</style>

<!-- Target (in an indexed book) of an IDXREF (from an index) -->
<style name="IDXMARK">
   <hide>               all </>
</style>

<style name="DOCINFO" group="hidden"></style>

<style name="#SDATA">
   <font-family>   attr(font) </>
   <character-set> attr(charset) </>
   <text-before>char(attr(code))</>
</style>

