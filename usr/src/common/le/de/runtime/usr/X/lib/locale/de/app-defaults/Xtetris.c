?.eraseStipple:			stripe4
?.baseTranslations:		#override <Unmap>:Pause()
*Background:			black
*BorderColor:			white
*Font:				-*-helvetica-bold-r-*-*-*-120-75-75-*-*-*-*
*Frame*sensitive:		true
*Frame.Buttons*width:		110
*NewGame.fromVert:		Start
*NewGame.label:			Zurücksetzen
*NewGame.baseTranslations:	#override <Btn1Down>,<Btn1Up>:NewGame() notify()
*Pause.mapWhenManaged:		False
*Pause.baseTranslations:	#override <Btn1Down>,<Btn1Up>:Pause()
*Quit.fromVert:			NewGame
*Quit.baseTranslations:		#override <Btn1Down>,<Btn1Up>:Quit() notify()
*About.fromVert:		Quit
*About.baseTranslations:	#override <Btn1Down>,<Btn1Up>: About()
*Scores.fromVert:		About
*Scores.baseTranslations:	#override <Btn1Down>,<Btn1Up>: Scores()
*Start.resizable:		True
*Start.baseTranslations:	#override <Btn1Down>,<Btn1Up>:Start()
*Buttons.borderWidth:		0
*Buttons.fromVert:		Status
*Buttons.hSpace:		0
*Buttons.vertDistance:		10
*Buttons.width:			120
*Canvas.fromHoriz:		TitleBar
*Canvas.fromVert:		TitleBar
*Canvas.sensitive:		True
*Canvas.baseTranslations:	#override 	<Expose>:Refresh()
*Canvas.accelerators: #override 	!Shift<Btn1Down>:RotateCCW() \n	!<Btn1Down>:MoveLeft() \n	!Shift<Btn3Down>:RotateCW() \n	!<Btn3Down>:MoveRight() \n	!Shift<Btn2Down>:Drop() \n	!<Btn2Down>:Drop() \n	<Key>space:Drop() \n	<Key>h:MoveLeft() \n	<Key>q:Quit() \n	<Key>p:Pause() \n	<Key>s:Start() \n	<Key>r:NewGame() \n	<Key>Left:MoveLeft() \n	<Key>j:RotateCW() \n	<Key>Down:RotateCW() \n	<Key>Begin:RotateCW() \n	<Key>k:RotateCCW() \n	<Key>Up:RotateCCW() \n	<Key>l:MoveRight() \n	<Key>Right:MoveRight()
*Canvas.vertDistance:		-22
*NextObject.borderWidth:	0
*NextObject.fromHoriz:		TitleBar
*NextObject.fromVert:		NextObjectLabel
*NextObject.height:		64
*NextObject.horizDistance:	-100
*NextObject.width:		64
*NextObject.baseTranslations:	#override 	<Expose>:Refresh()
*NextObjectLabel.Label:		Nächstes Objekt
*NextObjectLabel.borderWidth:	0
*NextObjectLabel.fromVert:	TitleBar
*NextObjectLabel.vertDistance:	20
*NextObjectLabel.width:		120
*Shadow.fromHoriz:		TitleBar
*Shadow.fromVert:		Canvas
*Shadow.height:			16
*Shadow.width:			160
*Shadow.baseTranslations:	#override 	<Expose>:Refresh()
*Status*borderWidth:		0
*Status*displayCaret:		False
*Status*resize:			True
*Status.Game.string:		"            "
*Status*width:			110
*Status.Level.string:		Stufe:  0
*Status.Rows.string:		Reihen:   0
*Status.Score.string:		Punkte:  0
*Status.fromVert:		TitleBar
*Status.left:			ChainLeft
*Status.right:			ChainRight
*Status.vertDistance:		120
*Status.width:			120
*TitleBar.Font:			-*-new century schoolbook-*-i-*-*-*-120-75-75-*-*-*-*
*TitleBar.Label:		XTETRIS 2.5
*TitleBar.foreground:		white
*TitleBar.height:		20
*TitleBar.width:		120
*Object.Background:		black
*ScoreText.font:		-*-fixed-medium-r-*-*-*-120-75-75-*-*-*-*
?.ScoreFrame*baseTranslations:	#override	<KeyPress>:Done()
?.AboutFrame*baseTranslations:	#override	<KeyPress>:Done()
*AboutText.baseTranslations:	#override	<KeyPress>:Done()
*customization:	    	    		.c
*Foreground:				wheat
*Frame.Buttons*borderColor:		darkslategrey
*Frame.Buttons.Pause.Background:	black
*Frame.Buttons.Pause.Foreground:	red
*Frame.Buttons.Start.Background:	black
*Frame.Buttons.Start.Foreground:	green
*Frame.Buttons.NewGame.Foreground:	yellow
*Frame.Buttons.About.Foreground:	yellow
*Frame.Buttons.Scores.Foreground:	yellow
*Frame.Buttons.Quit.Foreground:		yellow
*Frame.Canvas.background:		darkslategrey
*Frame.Shadow.background:		darkslategrey
*Frame.Status.Game.Foreground:		white
*Frame.TitleBar.background:		skyblue
*Frame.TitleBar.borderColor:		darkblue
*object0.foreground:	grey
*object0.background:	red
*object1.foreground:	yellow
*object1.background:	orange
*object2.foreground:	grey
*object2.background:	black
*object3.foreground:	lightgrey
*object3.background:	darkgreen
*object4.foreground:	skyblue
*object4.background:	darkblue
*object5.foreground:	cyan
*object5.background:	skyblue
*object6.foreground:	magenta
*object6.background:	blueviolet
