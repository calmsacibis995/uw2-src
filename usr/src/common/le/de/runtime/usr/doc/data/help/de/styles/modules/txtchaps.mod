<!-- /lfs/doc/DMG/Projects/sgml/sccs/styles/modules/s.txtchaps.mod 1.2 94/10/16 -->
<!-- bookcontent.gp -->

<group name="chap">
   <break-before>       page </>
   <space-before>       &chfore; </>
</group>

<group name="chtitle">
   <font-family>        &ttlfam; </>
   <font-size>          &ctsize; </>
   <line-spacing>       &ctspac; </>
   <space-before>       &ctfore; </>
   <space-after>        &ctaft; </>
   <left-indent>	&ctin; </>
   <foreground>		&bigtclr; </>
   <font-weight>        bold </>
   <break-before>	line </>
</group>

<style name="BIBLIOGRAPHY" group="chap"> </style>
<style name="BIBLIOGRAPHY,TITLE" group="chtitle"> </style>
<style name="GLOSSARY" group="chap"> </style>
<style name="GLOSSARY,TITLE" group="chtitle"> </style>
<style name="PREFACE" group="chap"> </style>
<style name="PREFACE,TITLE" group="chtitle"> </style>
<style name="REFERENCE" group="chap"> </style>
<style name="REFERENCE,TITLE" group="chtitle"> </style>

<group name="kicker">
   <foreground>         &ktclr; </>
   <break-after>	line </>
</group>

<style name="PART" group="chap"></style>
<style name="PART,TITLE" group="chtitle">
   <text-before>&partnum;</>
</style>
<style name="PART,TITLE,#text-before" group="kicker">
   <font-size>		&ptksize; </>
</style>

<style name="CHAPTER" group="chap"></style>
<style name="CHAPTER,TITLE" group="chtitle">
   <text-before>&chapnum;</>
</style>
<style name="CHAPTER,TITLE,#text-before" group="kicker">
   <font-size>		&ctksize; </>
</style>

<style name="APPENDIX" group="chap"></style>
<style name="APPENDIX,TITLE" group="chtitle">
   <text-before>&appxnum;</>
</style>
<style name="APPENDIX,TITLE,#text-before" group="kicker">
   <font-size>		&ctksize; </>
</style>

<style name="BOOK,REFENTRY" group="chap"> </style>
<style name="BOOK,REFERENCE" group="chap"> </style>

