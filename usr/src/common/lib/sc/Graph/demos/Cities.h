/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:Graph/demos/Cities.h	3.1" */
/******************************************************************************
*
* C++ Standard Components, Release 3.0.
*
* Copyright (c) 1991, 1992 AT&T and Unix System Laboratories, Inc.
* Copyright (c) 1988, 1989, 1990 AT&T.  All Rights Reserved.
*
* THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T and Unix System
* Laboratories, Inc.  The copyright notice above does not evidence
* any actual or intended publication of such source code.
*
******************************************************************************/

#ifndef GRAPH_DEMO_DOT_H
#define GRAPH_DEMO_DOT_H

#include "Graph.h"
#include <String.h>
#include <Map.h>
#include <stream.h>

Graphdeclare1(Citygraph,Cityvertex,Cityedge)

class Citygraph: public Graph {
	public:
	Citygraph() : Graph() {}
	Citygraph(const Citygraph& g) : Graph(g) {}
	derivedGraph(Citygraph,Cityvertex,Cityedge)
	};

class Cityvertex: public Vertex {
	public:
	String id;
	Cityvertex() : Vertex() {}
	derivedVertex(Citygraph,Cityvertex,Cityedge)
	};

class Cityedge: public Edge {
	public:
	int id;
	Cityedge(Cityvertex* v1, Cityvertex* v2) : Edge(v1,v2) {}
	Cityedge(Cityvertex* v1, Cityvertex* v2, int i) :
		Edge(v1,v2) {id = i;}
	Cityedge(const Cityedge& e) : Edge(e) {}
	derivedEdge(Citygraph,Cityvertex,Cityedge)
	};

Graphdeclare2(Citygraph,Cityvertex,Cityedge)


#endif
