/*
* Module handles Switching component structures in side of partial solution
* of a nonogram. Switching component is set of unknown pixels which can be
* coloured multiple ways independent of colourings of other unknown pixels
* of partial solution.
*/
#ifndef _SWITCHING_COMPONENT_H_
#define _SWITCHING_COMPONENT_H_

#include<stdint.h>
#include"Nonograms.h"
/*
* Unknown pixel marker.
* Macro NO_COMPONENT_MARKING is used to
* mark when marker is not marked to be
* part of component.
*/
#define NO_COMPONENT_MARKING 0
typedef struct UnknownPixelMarker{
	union{/*Phantom union*/
		struct{/*Phantom structure*/
			struct UnknownPixelMarker *restrict chigher;
			struct UnknownPixelMarker *restrict rhigher;
			struct UnknownPixelMarker *restrict clower;
			struct UnknownPixelMarker *restrict rlower;
		};
		struct UnknownPixelMarker *restrict edges[4];
	};
	union{/*Phantom union*/
		struct{/*Phantom structure*/
			EdgeValue *chigherval;
			EdgeValue *rhigherval;
			EdgeValue *clowerval;
			EdgeValue *rlowerval;
		};
		EdgeValue *edgevals[4];
	};
	union{/*Phantom union*/
		struct UnknownPixelMarker *memchigher;
		struct UnknownPixelMarker *itelink;
	};
	const Pixel *restrict pixel;
	uint32_t componentid;
	int32_t vertexid;
	const int32_t row;
	const int32_t col;
	//TODO: information about block range?
}UnknownPixelMarker;
/*
* Structure to store a switching component.
*/
typedef struct SwitchingComponent{
	UnknownPixelMarker *unknownpixelgraph;
	uint32_t sizeofunknownpixelgraph;
}SwitchingComponent;

/*
* Detect switching components.
* Switching components are given as linked
* list which user has to free.
* Return value is number of switching components
* or:
* -1 indicating first table in tables couldn't
*    produce a solution.
* -2 problem with detecting unknown pixels
* -3 memory allocation problem
*/
int32_t detectSwitchingComponents(Nonogram *restrict nono,TableHead *restrict tables,SwitchingComponent **restrict scomp);
/*
* Detect is a Switching Component a one black colourable one pixel square Switching Component
*/
uint32_t NonoDetectOneBlackColourableOnePixelSquareSwitchingComponent(Nonogram *restrict nono,TableHead *restrict tables,SwitchingComponent *restrict scomp);
/*
* Callback iteration function for every vertex in the graph.
* Here order is depth first search.
* Callback can stop iteration by returning 1. 0 will continue the iteration.
*/
typedef int32_t depthFirstIterateVertexesCallback(UnknownPixelMarker*,void*);
void depthFirstIterateVertexes(SwitchingComponent *scomp,void *data,depthFirstIterateVertexesCallback* callback);
/*
* Free switching component structure array.
* Includes freeing the memory of the structures them self.
*/
void freeSwitchingComponentArray(SwitchingComponent *scomp,int32_t arrlen);

#endif /* _SWITCHING_COMPONENT_H_ */
