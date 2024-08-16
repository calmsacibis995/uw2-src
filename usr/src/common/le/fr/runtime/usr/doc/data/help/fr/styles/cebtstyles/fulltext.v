<!-- Style sheet for the default browser view of IMI documents.
  This uses the ANSI AAP tag set, but has a few additions for
  supporting hypertext elements.

$Id: fulltext.v,v 1.14 1993/06/04 13:36:31 aet Exp aet $

  Copyright 1992, Electronic Book Technologies.  All rights reserved.
-->

<!ENTITY	art.color	CDATA	"#000000"	>
<!ENTITY	hottext.fontfam	CDATA	"courier"	>
<!ENTITY	hottext.foreground	CDATA	"#0101E2"	>
<!ENTITY	hottext.slant	CDATA	"Roman"	>
<!ENTITY	hottext.weight	CDATA	"Bold"	>
<!ENTITY	std.font	CDATA	"helvetica"	>
<!ENTITY	std.font-size	CDATA	"14"	>
<!ENTITY	std.leftindent	CDATA	"5"	>
<!ENTITY	std.line-spacing	CDATA	"20"	>
<!ENTITY	std.rightindent	CDATA	"6"	>
<!ENTITY	subscript	CDATA	"-3"	>
<!ENTITY	title.color	CDATA	"#FF0000"	>
<!ENTITY	title.font	CDATA	"courier"	>
<!ENTITY	verbatim.font	CDATA	"courier"	>
<!ENTITY	verbatim.font-size	CDATA	"10"	>
<!ENTITY	verbatim.line-spacing	CDATA	"16"	>

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
	<font-slant>	Italic	</>
</style>

<style name="FUNC" group="emphs">
	<font-slant>	Roman	</>
	<script>	ebt-link root=idmatch(id, attr(rid)) window="new"	</>
</style>

<style name="INLINE" group="emphs">
	<font-slant>	Roman	</>
</style>

<style name="PROP" group="emphs">
	<script>	ebt-link root=idmatch(id, attr(rid)) window="new"	</>
</style>

<style name="RESV" group="emphs">
	<font-slant>	Roman	</>
	<script>	ebt-link root=idmatch(id, attr(rid)) window="new"	</>
</style>

<style name="SCRIPT" group="emphs">
	<script>	ebt-link root=idmatch(id, attr(rid)) window="new"	</>
</style>



<?INSTED COMMENT: GROUP equations>

<group name="equations">
	<hide>	Children	</>
	<break-before>	None	</>
	<break-after>	None	</>
	<inline>	equation target=me()	</>
</group>

<style name="F" group="equations">
</style>

<style name="FD" group="equations">
	<font-size>	18	</>
	<space-before>	10	</>
	<space-after>	10	</>
	<break-before>	line	</>
	<break-after>	line	</>
</style>



<?INSTED COMMENT: GROUP title>

<group name="title">
	<font-family>	&title.font	</>
	<font-weight>	Bold	</>
	<foreground>	&title.color	</>
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
	<left-indent>	if(eq(cnum(ancestor()),1),+=0,+=22)	</>
	<first-indent>	if(eq(cnum(ancestor()),1),0,-22)	</>
	<line-spacing>	30	</>
	<space-before>	45	</>
	<text-before>if(eq(cnum(ancestor()),1),'',join(sub(cnum(parent()),1),'. '))</>
</style>

<style name="LABEL" group="title">
	<font-size>	14	</>
	<line-spacing>	21	</>
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
	<font-size>	12	</>
	<score>	Under	</>
</style>

<style name="ABLOCK">
	<hide>	All	</>
</style>

<style name="ABLOCK,TABLE">
	<hide>	All	</>
</style>

<style name="APPENDIX">
	<break-before>	Line	</>
</style>

<style name="ART">
	<space-before>	12	</>
	<break-before>	Line	</>
	<script>	ebt-raster filename="attr(FILE).tif" title="attr(TITLE)"	</>
	<inline>	raster filename="attr(FILE).tif"	</>
	<text-before>Figure if(gt(cnum(ancestor(CHAPTER)),1),join(sub(cnum(ancestor(CHAPTER)),1),-gcnum():),gcnum():) attr(TITLE)</>
</style>

<style name="ART,#TEXT-BEFORE">
	<font-family>	&title.font	</>
	<font-weight>	bold	</>
	<font-size>	12	</>
	<foreground>	&art.color	</>
	<line-spacing>	18	</>
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

<style name="CELLRULE">
	<hide>	All	</>
	<column>	True	</>
</style>

<style name="CHAPTER">
	<break-before>	Line	</>
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
	<foreground>	#0101E2	</>
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
	<font-weight>	&hottext.weight	</>
	<font-slant>	&hottext.slant	</>
	<foreground>	&hottext.foreground	</>
	<score>	Under	</>
	<script>	ebt-link root=idmatch(ID, ATTR(RID)) book=attr(book)    window=new stylesheet=popup.rev	</>
</style>

<style name="EXTREF,I">
	<font-slant>	Italics	</>
	<script>	ebt-link root=idmatch(ID, ATTR(RID,parent()))  book=attr(book,parent())   window=new stylesheet=popup.rev	</>
</style>

<style name="I">
	<font-slant>	Italics	</>
</style>

<style name="IMIDOC">
	<font-family>	&std.font	</>
	<font-size>	14	</>
	<foreground>	black	</>
	<left-indent>	5	</>
	<right-indent>	&std.rightindent	</>
	<line-spacing>	20	</>
</style>

<style name="ITEM">
	<left-indent>	+=10	</>
	<break-before>	False	</>
</style>

<style name="ITEM,P">
	<space-after>	2	</>
</style>

<style name="LIST">
	<left-indent>	+=5	</>
	<space-before>	4	</>
	<break-before>	line	</>
	<break-after>	line	</>
</style>

<style name="MARKER">
	<font-family>	if(isempty(attr(type)),symbol,)	</>
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

<style name="NAME">
	<width>	100	</>
	<column>	True	</>
</style>

<style name="NOTICE">
	<font-family>	&title.font	</>
	<font-size>	14	</>
	<line-spacing>	20	</>
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
	<space-before>	10	</>
	<break-before>	line	</>
</style>

<style name="PART">
	<space-before>	60	</>
</style>

<style name="PROPERTY">
	<break-before>	Line	</>
</style>

<style name="RM">
	<font-weight>	Medium	</>
	<font-slant>	Roman	</>
</style>

<style name="ROWRULE">
	<hrule>	Before	</>
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

<style name="SP.TEX">
	<break-before>	Line	</>
</style>

<style name="SP.TEXEQN">
	<break-before>	Line	</>
</style>

<style name="SPECIAL">
	<font-family>	&title.font	</>
	<select>	SP.toupper(attr(type))	</>
</style>

<style name="STYLE">
	<space-before>	6	</>
	<space-after>	6	</>
	<break-before>	Line	</>
	<text-before>if(attr(NAME), Property setting\(s\) for style attr(NAME):, Property setting:)</>
</style>

<style name="STYLE,#TEXT-BEFORE">
	<font-weight>	Bold	</>
</style>

<style name="SUBBLOCK,LABEL">
	<font-weight>	Bold	</>
	<foreground>	&title.color	</>
	<space-before>	8	</>
	<justification>	Left	</>
	<break-before>	line	</>
</style>

<style name="SUBBLOCK,P">
	<space-before>	if(gt(cnum(),1), 10,)	</>
	<break-before>	if(gt(cnum(),1), Line, None)	</>
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
	<!-- The select property in comments doesn't work; all tables are iconized 
	<select>	TABLE.gamut(attr(type,ancestor()),'proc2 other2','inline inline','HIDDEN')	</>  -->
	<select>	TABLE.switch(attr(type,ancestor()),'proc2','inline','other2','inline',default,'HIDDEN')	</>
</style>

<style name="TABLE.HIDDEN">
	<right-indent>	6	</>
	<space-after>	10	</>
	<icon-position>	Right	</>
	<hide>	Children	</>
	<break-before>	Line	</>
	<script>	ebt-reveal stylesheet=arbor.rev hscroll=yes width=550 height=350	</>
	<icon-type>	table	</>
</style>

<style name="TABLE.INLINE">
	<space-before>	20	</>
	<space-after>	20	</>
	<break-before>	Line	</>
</style>

<style name="TABLECELL">
	<left-indent>	+=int(div(tableinfo(arbor,left-indent,5),3))	</>
	<width>	int(sub(div(tableinfo(arbor,width,5),3),10))	</>
	<justification>	tableinfo(arbor,justification)	</>
	<column>	True	</>
</style>

<style name="TABLECELL,LIST">
	<space-before>	4	</>
	<break-before>	line	</>
</style>

<style name="TABLECELL,P">
	<left-indent>	if(eq(cnum(ancestor()),1), +=10, +=0)	</>
	<break-before>	if(eq(cnum(),1), None, Line)	</>
</style>

<style name="TABLEROW">
	<font-weight>	if(gt(cnum(),1),Medium,Bold)	</>
	<space-before>	4	</>
	<space-after>	4	</>
	<vrule>	Children	</>
	<break-before>	Line	</>
</style>

<style name="TBLOCK">
	<break-before>	Line	</>
</style>

<style name="TEX">
	<font-family>	&std.font	</>
	<font-size>	18	</>
	<space-before>	10	</>
	<space-after>	10	</>
	<hide>	Children	</>
	<break-before>	Line	</>
	<break-after>	Line	</>
	<inline>	equation target=me()	</>
</style>

<style name="TEXEQN">
	<icon-position>	Right	</>
	<hide>	Children	</>
	<break-after>	Line	</>
	<script>	ebt-reveal root=me() stylesheet=texeqn.rev	</>
	<icon-type>	default	</>
</style>

<style name="TOCPG">
	<hide>	All	</>
</style>

<style name="UL">
	<font-weight>	Bold	</>
	<score>	None	</>
</style>

<style name="UNIX">
	<select>	if(gt(index(env(PATH),'/'),0),ME.SHOW,ME.HIDE)	</>
</style>

<style name="VALUE">
	<left-indent>	+=55	</>
	<column>	True	</>
</style>

<style name="VERBATIM">
	<font-family>	&verbatim.font	</>
	<font-size>	&verbatim.font-size	</>
	<line-spacing>	&verbatim.line-spacing	</>
	<space-before>	if(eq(tag(lsibling()),'VERBATIM'),2,8)	</>
	<space-after>	if(eq(tag(rsibling()),'VERBATIM'),2,8)	</>
	<justification>	verbatim	</>
	<break-before>	Line	</>
	<break-after>	Line	</>
</style>

<style name="XREF">
	<font-family>	&hottext.fontfam	</>
	<font-weight>	&hottext.weight	</>
	<font-slant>	&hottext.slant	</>
	<foreground>	&hottext.foreground	</>
	<score>	Under	</>
	<script>	ebt-link root=idmatch(ID, ATTR(RID)) window=new stylesheet=popup.rev	</>
</style>



</sheet>
