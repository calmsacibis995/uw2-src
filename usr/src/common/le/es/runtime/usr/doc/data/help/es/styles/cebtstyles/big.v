<!-- Style sheet for the default browser view of IMI documents.
  This uses the ANSI AAP tag set, but has a few additions for
  supporting hypertext elements.

$Id: big.v,v 1.4 1993/06/04 19:42:55 aet Exp aet $

  Copyright 1992, Electronic Book Technologies.  All rights reserved.
-->

<!ENTITY	hottext.fontfam	CDATA	"helvetica"	>
<!ENTITY	hottext.foreground	CDATA	"#0101E2"	>
<!ENTITY	hottext.slant	CDATA	"Italics"	>
<!ENTITY	std.font	CDATA	"times"	>
<!ENTITY	std.rightindent	CDATA	"6"	>
<!ENTITY	subscript	CDATA	"-3"	>
<!ENTITY	title.font	CDATA	"helvetica"	>

<sheet >



<?INSTED COMMENT: GROUP emphs>

<group name="emphs">
	<font-slant>	Italic	</>
</group>

<style name="CMD" group="emphs">
	<font-weight>	Bold	</>
	<font-slant>	Roman	</>
</style>

<style name="FILE" group="emphs">
</style>

<style name="FUNC" group="emphs">
</style>

<style name="INLINE" group="emphs">
</style>

<style name="PROP" group="emphs">
</style>

<style name="RESV" group="emphs">
</style>

<style name="SCRIPT" group="emphs">
</style>



<?INSTED COMMENT: GROUP equations>

<group name="equations">
	<hide>	Children	</>
	<break-before>	False	</>
	<break-after>	False	</>
	<inline>	equation target = me()	</>
</group>

<style name="F" group="equations">
</style>

<style name="FD" group="equations">
	<font-size>	18	</>
	<line-spacing>	24	</>
	<space-before>	10	</>
	<space-after>	10	</>
	<break-before>	line	</>
	<break-after>	line	</>
</style>



<?INSTED COMMENT: GROUP title>

<group name="title">
	<font-family>	&title.font	</>
	<font-weight>	Bold	</>
	<foreground>	blue	</>
	<justification>	Left	</>
	<break-before>	line	</>
</group>

<style name="APPENDIX,TITLE" group="title">
	<font-size>	24	</>
	<line-spacing>	30	</>
	<space-before>	45	</>
	<text-before>Appendix format(cnum(parent()),LETTER):  </>
</style>

<style name="CHAPTER,TITLE" group="title">
	<font-size>	24	</>
	<left-indent>	+=22	</>
	<first-indent>	-22	</>
	<line-spacing>	30	</>
	<space-before>	45	</>
	<text-before>if(gt(cnum(parent()),1),join(sub(cnum(parent()),1),'. '),'')</>
</style>

<style name="LABEL" group="title">
	<font-size>	18	</>
	<line-spacing>	24	</>
	<space-before>	12	</>
</style>

<style name="MAP,TITLE" group="title">
	<font-size>	18	</>
	<line-spacing>	24	</>
	<space-before>	25	</>
</style>

<style name="TITLEPG,TITLE" group="title">
	<font-size>	24	</>
	<line-spacing>	32	</>
</style>



<?INSTED COMMENT: UNGROUPED STYLES FOLLOW>

<style name="#SDATA">
	<font-family>	attr(font)	</>
	<font-weight>	attr(weight)	</>
	<character-set>	attr(charset)	</>
	<text-before>char(attr(code))</>
</style>

<style name="#TAGS">
	<font-family>	courier	</>
	<font-weight>	Medium	</>
	<font-size>	*	</>
	<foreground>	#8fbc8f	</>
	<score>	Under	</>
</style>

<style name="ABLOCK">
	<hide>	All	</>
</style>

<style name="ABLOCK,TABLE">
	<hide>	All	</>
</style>

<style name="ART">
	<space-before>	12	</>
	<break-before>	line	</>
	<script>	ebt-raster filename="attr(FILE).tif" title="attr(TITLE)"	</>
	<inline>	raster filename="attr(FILE).tif"	</>
	<text-before>Figure if(gt(cnum(ancestor(CHAPTER)),1),join(sub(cnum(ancestor(CHAPTER)),1),'-'),'')-gcnum(): attr(TITLE)</>
</style>

<style name="ART,#TEXT-BEFORE">
	<font-family>	&title.font	</>
	<font-weight>	bold	</>
	<font-size>	18	</>
	<line-spacing>	24	</>
	<space-before>	4	</>
	<space-after>	12	</>
	<justification>	left	</>
</style>

<style name="AUTHOR">
	<font-family>	&title.font	</>
	<font-size>	18	</>
	<line-spacing>	24	</>
	<space-before>	18	</>
	<justification>	Left	</>
	<break-before>	line	</>
</style>

<style name="B">
	<font-weight>	Bold	</>
</style>

<style name="BI">
	<font-weight>	Bold	</>
	<font-slant>	Italic	</>
</style>

<style name="COPYRTPG">
	<icon-position>	Right	</>
	<hide>	Children	</>
	<script>	ebt-reveal title="Copyright Notice" stylesheet="fulltext.v"	</>
	<icon-type>	copyrt	</>
</style>

<style name="DATE">
	<font-family>	&title.font	</>
	<font-size>	18	</>
	<line-spacing>	24	</>
	<space-before>	18	</>
	<justification>	Left	</>
	<break-before>	line	</>
</style>

<style name="DOCLINK">
	<font-slant>	Italics	</>
	<foreground>	red	</>
	<icon-position>	Right	</>
	<break-before>	line	</>
	<script>	ebt-link book="attr(DOC)" tname=ID tvalue="attr(RID)"	</>
	<icon-type>	exlink	</>
</style>

<style name="DOS">
	<break-before>	False	</>
	<select>	if(eq(index(env(PATH),'/'),0),ME.SHOW,ME.HIDE)	</>
</style>

<style name="EMPH">
	<select>	EMPH.attr(type)	</>
</style>

<style name="EMPH.ATTR">
	<font-slant>	Roman	</>
</style>

<style name="EMPH.BUTTON">
	<font-slant>	Italics	</>
</style>

<style name="EMPH.CMD">
	<font-weight>	Bold	</>
	<font-slant>	Roman	</>
</style>

<style name="EMPH.FILE">
	<font-slant>	Italic	</>
</style>

<style name="EMPH.FUNC">
	<font-slant>	Roman	</>
</style>

<style name="EMPH.INPUT">
	<font-family>	courier	</>
</style>

<style name="EMPH.MENU">
	<font-slant>	Roman	</>
</style>

<style name="EMPH.OUTPUT">
	<font-family>	courier	</>
</style>

<style name="EMPH.PROP">
	<font-slant>	Italic	</>
</style>

<style name="EMPH.PVAL">
	<font-slant>	Italic	</>
</style>

<style name="EMPH.RESV">
	<font-slant>	Roman	</>
</style>

<style name="EXTREF">
	<font-family>	&hottext.fontfam	</>
	<font-slant>	&hottext.slant	</>
	<foreground>	&hottext.foreground	</>
	<script>	ebt-link root=idmatch(ID, ATTR(RID)) book=attr(book)    window=new stylesheet=popup.rev	</>
</style>

<style name="I">
	<font-slant>	Italics	</>
	<script>	ebt-link root=idmatch(ID, ATTR(RID,parent()))  book=attr(book,parent())    window=new stylesheet=popup.rev	</>
</style>

<style name="IMIDOC">
	<font-family>	&std.font	</>
	<font-size>	18	</>
	<foreground>	black	</>
	<left-indent>	5	</>
	<right-indent>	&std.rightindent	</>
	<line-spacing>	24	</>
</style>

<style name="ITEM">
	<left-indent>	+=10	</>
	<break-before>	False	</>
</style>

<style name="ITEM,P">
	<space-after>	2	</>
</style>

<style name="LIST">
	<left-indent>	+=10	</>
	<space-before>	4	</>
	<break-before>	line	</>
	<break-after>	line	</>
</style>

<style name="MARKER">
	<font-weight>	Medium	</>
	<hide>	text	</>
	<break-before>	line	</>
	<text-before>switch(attr(type,parent(LIST)),'triple', format(cnum(),word('LETTER decimal roman',countword(qtag(), x,'eq(var(x),'LIST')',',')))., 'num', cnum()., 'default', content())</>
</style>

<style name="ME.HIDE">
	<hide>	All	</>
</style>

<style name="ME.SHOW">
	<hide>	Off	</>
</style>

<style name="NOTICE">
	<font-family>	&title.font	</>
	<font-size>	18	</>
	<line-spacing>	24	</>
	<space-before>	20	</>
	<break-before>	line	</>
</style>

<style name="OWNER">
	<font-family>	&title.font	</>
	<font-size>	18	</>
	<line-spacing>	24	</>
	<space-before>	18	</>
	<justification>	Left	</>
	<break-before>	line	</>
</style>

<style name="P">
	<space-before>	12	</>
	<break-before>	line	</>
</style>

<style name="PART">
	<space-before>	60	</>
</style>

<style name="RM">
	<font-weight>	Medium	</>
	<font-slant>	Roman	</>
</style>

<style name="SECTION">
	<space-before>	40	</>
</style>

<style name="SHOW">
	<space-before>	12	</>
	<icon-position>	Left	</>
	<break-before>	line	</>
	<script>	ebt-raster filename="attr(TYPE).tif" title="attr(Type) Icon"	</>
	<icon-type>	attr(TYPE)	</>
</style>

<style name="SIDEBAR">
	<icon-position>	Right	</>
	<hide>	Children	</>
	<script>	ebt-reveal title=" " stylesheet="fulltext.v"	</>
	<icon-type>	footnote	</>
</style>

<style name="SIDEBAR,LABEL">
	<hide>	Children	</>
</style>

<style name="SP.FN">
	<icon-position>	Right	</>
	<hide>	Children	</>
	<script>	ebt-reveal title="Footnote" stylesheet="fulltext.v"	</>
	<icon-type>	footnote	</>
</style>

<style name="SP.ICONSHOW">
	<left-indent>	17	</>
	<space-before>	12	</>
	<icon-position>	Inline	</>
	<break-before>	line	</>
	<icon-type>	attr(TITLE)	</>
</style>

<style name="SP.SUB">
	<vertical-offset>	&subscript	</>
</style>

<style name="SPECIAL">
	<font-family>	&title.font	</>
	<select>	SP.toupper(attr(type))	</>
</style>

<style name="SUBBLOCK,LABEL">
	<font-slant>	Italics	</>
	<foreground>	blue	</>
	<space-before>	12	</>
	<justification>	Left	</>
	<break-before>	line	</>
</style>

<style name="SUBBLOCK,P">
	<space-before>	if(gt(cnum(),1),12,0)	</>
	<break-before>	if(gt(cnum(),1),line,none)	</>
</style>

<style name="SUBTITLE">
	<font-family>	&title.font	</>
	<font-size>	18	</>
	<line-spacing>	24	</>
	<space-before>	18	</>
	<justification>	Left	</>
	<break-before>	line	</>
</style>

<style name="SYMBOL">
	<font-family>	symbol	</>
	<font-weight>	Medium	</>
</style>

<style name="TABLE">
	<foreground>	black	</>
	<left-indent>	+=13	</>
	<break-before>	line	</>
	<select>	TABLE.gamut(attr(type,parent()),'proc2 other2','inline inline','HIDDEN')	</>
</style>

<style name="TABLE.HIDDEN">
	<icon-position>	Right	</>
	<hide>	Children	</>
	<script>	ebt-reveal stylesheet=arbor.rev hscroll=yes	</>
	<icon-type>	table	</>
</style>

<style name="TABLECELL">
	<left-indent>	+=div(tableinfo(arbor,left-indent),3)	</>
	<width>	sub(div(tableinfo(arbor,width),3),10)	</>
	<justification>	tableinfo(arbor,justification)	</>
	<column>	True	</>
</style>

<style name="TABLECELL,P">
	<space-before>	5	</>
	<break-before>	line	</>
</style>

<style name="TABLEROW">
	<space-before>	8	</>
	<break-before>	line	</>
	<select>	TABLEROW.attr(hdr),TABLEROW	</>
</style>

<style name="TABLEROW.1">
	<font-weight>	Bold	</>
	<font-slant>	Italics	</>
</style>

<style name="TOCPG">
	<hide>	All	</>
</style>

<style name="UL">
	<score>	Under	</>
</style>

<style name="UNIX">
	<select>	if(gt(index(env(PATH),'/'),0),ME.SHOW,ME.HIDE)	</>
</style>

<style name="VERBATIM">
	<font-family>	courier	</>
	<font-size>	14	</>
	<line-spacing>	20	</>
	<space-before>	14	</>
	<space-after>	8	</>
	<justification>	verbatim	</>
	<break-before>	line	</>
	<break-after>	line	</>
</style>

<style name="XREF">
	<font-family>	&hottext.fontfam	</>
	<font-slant>	&hottext.slant	</>
	<foreground>	&hottext.foreground	</>
	<script>	ebt-link root=idmatch(ID, ATTR(RID))    window=new stylesheet=popup.rev	</>
</style>



</sheet>
