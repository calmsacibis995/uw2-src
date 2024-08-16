<!-- ArborText style table -->

<!-- It is assumed that %tblcon is defined when this file is included,
     to specify content for tablecell, e.g.,
	<!ENTITY % tblcon "#PCDATA|emphasis|%eqn|graphic" >
-->

<!-- [JFS] The table, tablerow and tablecell elements have a new
           attribute defined called "label". This attribute is
	   intended to be used by command language processing.
	   <tablecell> also has another attribute called "action". It
	   is anticipated that this attribute will also contribute to
	   command language processing of tables.
-->

<!ELEMENT table		- - (rowrule,(tablerow,rowrule)+)>
<!ATTLIST table		acl		CDATA	#IMPLIED
			chj		CDATA	#IMPLIED
			csl		CDATA	#IMPLIED
			cst		CDATA	#IMPLIED
			ctl		CDATA	#IMPLIED
			cwl		CDATA	#REQUIRED
			hff		CDATA	#IMPLIED
			hfs		CDATA	#IMPLIED
			htm		CDATA	#IMPLIED
			hts		CDATA	#IMPLIED
			jst		CDATA	#IMPLIED
			ncols		CDATA	#IMPLIED
			off		CDATA	#IMPLIED
			rth		CDATA	#IMPLIED
			rst		CDATA	#IMPLIED
			rvj		CDATA	#IMPLIED
			tff		CDATA	#IMPLIED
			tfs		CDATA	#IMPLIED
			tts		CDATA	#IMPLIED
			unt		CDATA	#IMPLIED
			wdm		CDATA	#REQUIRED
			ctmarg		CDATA	#IMPLIED
			cbmarg		CDATA	#IMPLIED
			clmarg		CDATA	#IMPLIED
			crmarg		CDATA	#IMPLIED
			dispwid		CDATA	#IMPLIED
			label		CDATA	#IMPLIED
			>

<!ELEMENT tablerow	- O (cellrule,(tablecell,cellrule)+)>
<!ATTLIST tablerow	hdr		CDATA	#IMPLIED
			rht		CDATA	#IMPLIED
			rvj		CDATA	#IMPLIED
			label           CDATA   #IMPLIED
			>

<!ELEMENT tablecell	- - (%tblcon)*>

<!ATTLIST tablecell	cff		CDATA	#IMPLIED
			cfs		CDATA	#IMPLIED
			chj		CDATA	#IMPLIED
			cts		CDATA	#IMPLIED
			cvj		CDATA	#IMPLIED
			shd		CDATA	#IMPLIED
			spn		CDATA	#IMPLIED
			vspn		CDATA	#IMPLIED
			label           CDATA   #IMPLIED
			action		CDATA	#IMPLIED>

<!ELEMENT rowrule	- O EMPTY>
<!ATTLIST rowrule	rty		CDATA	#IMPLIED
			rtl		CDATA	#IMPLIED>

<!ELEMENT cellrule	- O EMPTY>
<!ATTLIST cellrule	rty		CDATA	#IMPLIED>
