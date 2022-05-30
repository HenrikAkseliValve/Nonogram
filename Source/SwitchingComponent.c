#include<stdint.h>
#include<stdlib.h>
#include"Nonograms.h"
#include"ValidityCheck.h"
#include"SwitchingComponent.h"
#include"Etc.h"

/*
* Line of interest marker.
*/
struct LineOfIntrest{
	UnknownPixelMarker* startoftheline;
};
/*
* Header for UnkownPixelMarker graph so that faster search
* information can be stored.
*/
struct UnknownPixelMarkerHeader{
	struct LineOfIntrest *rows;
};
/*
* Structure transferming information from callback functions.
*/
struct CallbackData{
	UnknownPixelMarker *lastunknown;
	struct UnknownPixelMarkerHeader* header;
	BlocksRanges *dimsblockranges;
	Description *blockdescs;
	uint32_t numberofunknownpixels;
};
/*
* Linked list of unknown pixel markers.
*/
struct LinkedListOfUnknownPixelMarker{
	struct LinkedListOfUnknownPixelMarker *next;
	UnknownPixelMarker* m;
};
/*
* Linked list of integers.
*/
struct IntLink{
	struct IntLink *next;
	uint32_t unknownpixelgraphsize;
};

static int32_t findLeftmostCover(int32_t pixel,BlocksRanges *ranges,int32_t length){
	int32_t blockindex;
	for(blockindex=0; blockindex<length; blockindex++){
		if(ranges->blocksrangelefts[blockindex]<=pixel && pixel<=ranges->blocksrangerights[blockindex]){
			break;
		}
	}
	return blockindex;
}
static int32_t findRightmostCover(int32_t pixel,BlocksRanges *ranges,int32_t length){
	// If block does not exists variable blockindex here equals length.
	int32_t blockindex=findLeftmostCover(pixel,ranges,length);
	while(blockindex<length-1 && ranges->blocksrangelefts[blockindex+1]<=pixel && pixel<=ranges->blocksrangerights[blockindex+1]){
		blockindex++;
	}
	return blockindex;
}
/*
* For unknown pixel graph which we use to store our switching components
* edge is NOT created if there is one block covering unknown pixel with
* no overlap "left" of the block and last unknown pixel was not covered
* by same block and pixels are not next to each other.
*
* Helper function because firstCallback and secondCallback uses the same
* function.
*
* Returns 1 if edge is allowed and 0 if not.
*/
static uint8_t allowUnknownPixelGraphEdge(BlocksRanges *ranges,int32_t pixel,int32_t numberofblocks,int32_t lastunknownpos,EdgeValue *edge){
	// If unknown pixels are next to each other allow edge creation.
	if(edge->state==NONO_EDGE_STATE_NEXTTO) return 1;
	
	// Find blocks of covering the new unknown pixel.
	int32_t blockindex;
	int32_t k;
	for(k=0;k<numberofblocks;k++){
		if(ranges->blocksrangelefts[k]<=pixel && pixel<=ranges->blocksrangerights[k]){

			blockindex=k;
			// If more then one block covers the pixel edge is allowed.
			// (This automaticly implies a overlap).
			// Since next block has to be that then just check that and move on based upon it.
			k++;
			// Guard against blockindex being the last block (hence k is not a block).
			if(k<numberofblocks){
				if(ranges->blocksrangelefts[k]<=pixel && pixel<=ranges->blocksrangerights[k]) return 1;
			}

			if(edge->state==NONO_EDGE_STATE_MIX){
				// If left overlap exist and last unknown pixel was covered by same block sequence then allow.
				int leftblock=findRightmostCover(lastunknownpos,ranges,numberofblocks);
				int32_t blkseq;
				for(int32_t blkseq=leftblock;blkseq<blockindex;blkseq++){
					if(ranges->blocksrangerights[blkseq]<ranges->blocksrangelefts[blkseq+1]) return 0;
				}
				return 1;
			}
			else{
				// If left overlap exist and last unknown pixel was covered by same block then allow.
				return (blockindex>0 && ranges->blocksrangerights[blockindex-1]>=ranges->blocksrangelefts[blockindex]) || lastunknownpos>=ranges->blocksrangelefts[blockindex];
			}
		}
	}
	// Umm... no blocks covers the unknown pixel... so it cannot be part of any
	// unknown pixel graph since only color it can be is white... this shouldn't happen
	// if partial solver has been used...
	return 0;

}
/*
* This callback creates unknown pixels and makes first links between unknown pixels.
*/
static uint8_t firstCallback(Pixel *pixel,int32_t row,int32_t col,void *callbackdata,EdgeValue *edge){
	// Cast structure for easy access.
	struct CallbackData *data=(struct CallbackData*)callbackdata;

	// Allocate new unknown and initialize.
	UnknownPixelMarker* newunknown=malloc(sizeof(UnknownPixelMarker));

	if(!newunknown) return 1;
	*((int32_t*)&(newunknown->row))=row;
	*((int32_t*)&(newunknown->col))=col;
	*((Pixel **)&(newunknown->pixel))=pixel;
	newunknown->componentid=NO_COMPONENT_MARKING;
	data->numberofunknownpixels++;

	// Link based upon last calls unknown location.
	if(!data->lastunknown || (data->lastunknown->row!=row && data->lastunknown->col!=col)){
		// First one in new row or column.
		newunknown->rhigher=0;
		newunknown->rlower=0;
		newunknown->chigher=0;
		newunknown->clower=0;
		// Assume row.
		data->header->rows[row].startoftheline=newunknown;
		#if defined(_DEBUG_) && defined(_DEBUG_UNKNOWN_PIXEL_GRAPH_CONNECTIONS)
			printf("Newline found row:%d col:%d\n",row,col);
		#endif
	}
	else if(data->lastunknown->row==row && data->lastunknown->col<col){
		// Initialize newunknown. Last unknown was column lower.
		newunknown->rhigher=0;
		newunknown->rlower=0;
		newunknown->chigher=0;
		// Connect just to make sure memory is accesible.
		// Some switching components are middle of the table
		// so no connection to left border in unknown pixel
		// graph.
		newunknown->memchigher=0;
		data->lastunknown->memchigher=newunknown;

		// Check should edge be created.
		if(allowUnknownPixelGraphEdge(data->dimsblockranges+row,col,data->blockdescs[row].length,data->lastunknown->col,edge)){
			newunknown->clower=data->lastunknown;
			// Update last unknown having chigher pixel.
			data->lastunknown->chigher=newunknown;
			#if defined(_DEBUG_) && defined(_DEBUG_UNKNOWN_PIXEL_GRAPH_CONNECTIONS)
				printf("Connection between row:%d col:%d and row:%d col:%d\n",data->lastunknown->row,data->lastunknown->col,row,col);
			#endif
			// Add edge value by copying what callback got.
			// Remember that other vertex also needs pointer
			// to the EdgeValue structure.
			newunknown->clowerval=malloc(sizeof(EdgeValue));
			*(newunknown->clowerval)=*edge;
			newunknown->clower->chigherval=newunknown->clowerval;
		}
		else{
			// No edge between the two in unknown pixel graph.
			newunknown->clower=0;
			data->lastunknown->chigher=0;
			#if defined(_DEBUG_) && defined(_DEBUG_UNKNOWN_PIXEL_GRAPH_CONNECTIONS)
				printf("NO connection between row:%d col:%d and row:%d col:%d\n",data->lastunknown->row,data->lastunknown->col,row,col);
			#endif
		}
	}

	// Mark last unkown for next call.
	data->lastunknown=newunknown;

	return 0;
}
/*
* This call just links already exists unknown pixels.
*/
static uint8_t secondCallback(Pixel *pixel,int32_t row,int32_t col,void *callbackdata,EdgeValue *edge){
	// Cast structure for easy access.
	struct CallbackData *data=(struct CallbackData*)callbackdata;

	// Allocations aren't needed since previous callback did do the allocations.
	// Now search have to be done find correct unknown pixel.
	// NOTE: memchigher used here to make sure if edge does not exist in
	//       unknown pixel graph.
	UnknownPixelMarker *selectedunknown=data->header->rows[row].startoftheline;
	do{
		if(selectedunknown->col==col) goto SECOND_CALLBACK_FOUND_UNKNOWN_PIXEL;
		selectedunknown=selectedunknown->memchigher;
	}while(selectedunknown);
	return 1;
	SECOND_CALLBACK_FOUND_UNKNOWN_PIXEL:

	// Which way links should be done.
	if(!data->lastunknown || (data->lastunknown->row!=row && data->lastunknown->col!=col)){
		// First in the line.
		#if defined(_DEBUG_) && defined(_DEBUG_UNKNOWN_PIXEL_GRAPH_CONNECTIONS)
			printf("First in the column row:%d col:%d\n",row,col);
		#endif
	}
	else if(data->lastunknown->row<row && data->lastunknown->col==col){
		if(allowUnknownPixelGraphEdge(data->dimsblockranges+col,row,data->blockdescs[col].length,data->lastunknown->row,edge)){
			// Last unknown was row smaller.
			// Update selectedunknown to have row lower.
			selectedunknown->rlower=data->lastunknown;
			// Update lastunknown to have row higher.
			data->lastunknown->rhigher=selectedunknown;

			// Add edge value by copying what callback got.
			// Remember that other vertex also needs pointer
			// to the EdgeValue structure.
			selectedunknown->rlowerval=malloc(sizeof(EdgeValue));
			*(selectedunknown->rlowerval)=*edge;
			selectedunknown->rlower->rhigherval=selectedunknown->rlowerval;

			#if defined(_DEBUG_) && defined(_DEBUG_UNKNOWN_PIXEL_GRAPH_CONNECTIONS)
				printf("Connection between row:%d col:%d and row:%d col:%d\n",data->lastunknown->row,data->lastunknown->col,row,col);
			#endif
		}
		else{
			// No edge between unknown pixels
			data->lastunknown->rhigher=0;
			selectedunknown->rlower=0;
			#if defined(_DEBUG_) && defined(_DEBUG_UNKNOWN_PIXEL_GRAPH_CONNECTIONS)
				printf("NO connection between row:%d col:%d and row:%d col:%d\n",data->lastunknown->row,data->lastunknown->col,row,col);
			#endif
		}
	}

	// Mark last unkown for next call.
	data->lastunknown=selectedunknown;
	return 0;
}
int32_t detectSwitchingComponents(Nonogram *restrict nono,TableHead *restrict tables,SwitchingComponent **restrict scomp){
	// Return value must be number of switching components.
	int32_t numberswitchingcomps;

	struct UnknownPixelMarkerHeader markerheader={.rows=calloc(nono->height,sizeof(struct LineOfIntrest))};
	struct CallbackData data={.lastunknown=0,.header=&markerheader,.dimsblockranges=tables->firsttable.row,.blockdescs=nono->rowsdesc,.numberofunknownpixels=0};

	NonogramsDimensionValidness rowvalidation=solutionDimensionIsValid(nono,&tables->firsttable,0,firstCallback,&data);
	// Had to be reset for next call.
	data.lastunknown=0;
	data.dimsblockranges=tables->firsttable.col;
	data.blockdescs=nono->colsdesc;
	NonogramsDimensionValidness columnvalidation=solutionDimensionIsValid(nono,&tables->firsttable,1,secondCallback,&data);

	// Check validity of row and columns.
	if(rowvalidation==NONO_SOLUTION_IS_VALID && columnvalidation==NONO_SOLUTION_IS_VALID){
		// No unknown pixels and solution is valid so return zero
		// and mark.
		numberswitchingcomps=0;
		goto DETECT_SWITCHING_COMPONENTS_ERROR_GOTO;
	}
	else if(rowvalidation==NONO_SOLUTION_IS_INVALID || columnvalidation==NONO_SOLUTION_IS_INVALID){
		// Nonogram is invalid hence just ignore it.
		numberswitchingcomps=-1;
		goto DETECT_SWITCHING_COMPONENTS_ERROR_GOTO;
	}
	else if(rowvalidation==NONO_SOLUTION_IS_CALLBACK_ERROR || columnvalidation==NONO_SOLUTION_IS_CALLBACK_ERROR){
		// Error on unknown pixels callback.
		numberswitchingcomps=-2;
		goto DETECT_SWITCHING_COMPONENTS_ERROR_GOTO;
	}

	// Divergienting switching components.
	// This is done by depth search iding
	// ones that are connected.
	// UnknownPixelMarker's member memclower can be used search
	// components in the middle of the nonogram which do not have
	// border node which would be in markerheader.
	numberswitchingcomps=0;
	// Allocate stack for qoing back and forth different paths of nodes.
	// At maximum conponent is all of the unknown pixels hence stack size
	// being number of unknown pixels makes sure stack is right size.
	// Variable iterotrstack is pointer arrays for the stack elements
	// and variable iteratostacktop is integer indexing top of the stack.
	UnknownPixelMarker **iteratorstack=malloc(sizeof(UnknownPixelMarker*)*data.numberofunknownpixels);
	uint32_t iteratorstacktop;
	// Safety check
	if(iteratorstack){

		// Since there maybe components middle of the graph so that only memory access to them is via other
		// other vertex's memchigher other stack is needed to keep track of the memchigers that are passed
		// by.
		//
		// What is max size of this stack? Well, some multiple of non-null elements of .rows in markerheader
		// is more then or equal then the max. This is because first unknown pixel in the row is in the
		// .rows so every other unknown pixels must be behind them. Hence best option is to make
		// linked list stack for processing. First one is used to initialize iterator so that memory
		// management isn't provoked.
		//
		// NOTE: one cannot optimize this by assuming that first pair of non-null element belongs to same
		//       switching component as switching components can overlap with each other.
		UnknownPixelMarker *iterator;
		struct LinkedListOfUnknownPixelMarker *memorystack;
		struct LinkedListOfUnknownPixelMarker *memorystacklast=(struct LinkedListOfUnknownPixelMarker*)&memorystack;
		{
			int32_t row;
			for(row=nono->height-1;row>=0;row--){
				if(markerheader.rows[row].startoftheline){
					iterator=markerheader.rows[row].startoftheline;
					break;
				}
			}
			for(--row;row>=0;row--){
				if(markerheader.rows[row].startoftheline){
					memorystacklast->next=malloc(sizeof(struct LinkedListOfUnknownPixelMarker));
					memorystacklast=memorystacklast->next;
					memorystacklast->m=markerheader.rows[row].startoftheline;
				}
			}
		}
		memorystacklast->next=0;

		// Linked list stack for switching component starts for faster array conversion processing.
		// Links of the memorystack reused here.
		struct LinkedListOfUnknownPixelMarker *scomppointerstack,*scomppointerstacklast;
		struct IntLink *sizestack,*sizestacklast=(struct IntLink*)&sizestack;

		// Start of new switching component.
		scomppointerstack=malloc(sizeof(struct LinkedListOfUnknownPixelMarker));
		scomppointerstacklast=scomppointerstack;
		scomppointerstacklast->m=iterator;
		scomppointerstacklast->next=0;

		// Store last iterator here for vertex linking.
		// Used to make iteration link between vertixes of a switching component.
		UnknownPixelMarker *lastiterator;

		_Static_assert(&(iterator->rlower)==iterator->edges+3,"clower in the UnknownPixelMarker structure is assumed to be same as iteratorstack->edges members.");

		SWITCHING_COMPONENT_DETECT_MORE_IN_MEMORY_STACK_LOOP:{
			// Initialize stack.
			iteratorstack[0]=iterator;
			iteratorstacktop=0;

			// Initialize lastiterator
			lastiterator=0;

			// We have found one more switching component.
			numberswitchingcomps++;
			uint32_t sizeofunknownpixelgraph=0;

			// Go through vertices in chigher, rhigher, clower, rlower order.
			do{
				// If you can follow an edge follow it!
				// NOTE: incrementing componentid here moves check to next edges!
				//       Order of chigher, rhigher, clower, rlower is gotten by order
				//       they are defined in the data structure.
				for(uint32_t edgeindex=iteratorstack[iteratorstacktop]->componentid;edgeindex<4;edgeindex++){
					if(iteratorstack[iteratorstacktop]->edges[edgeindex] && iteratorstack[iteratorstacktop]->edges[edgeindex]->componentid==NO_COMPONENT_MARKING){
						// No need to investigate this edge anymore or ones that
						// did not have anything.
						iteratorstack[iteratorstacktop]->componentid=edgeindex+1;

						// New stack top.
						iteratorstack[iteratorstacktop+1]=iteratorstack[iteratorstacktop]->edges[edgeindex];
						iteratorstacktop++;

						// Follow the edge by resetting edgeindex.
						edgeindex=iteratorstack[iteratorstacktop]->componentid-1;
					}
				}

				// Push memchigher to memory stack if it points to place which is not ID'd.
				if(iteratorstack[iteratorstacktop]->memchigher && iteratorstack[iteratorstacktop]->memchigher->componentid==NO_COMPONENT_MARKING){
					memorystacklast->next=malloc(sizeof(struct LinkedListOfUnknownPixelMarker));
					memorystacklast=memorystacklast->next;
					memorystacklast->m=iteratorstack[iteratorstacktop]->memchigher;
					memorystacklast->next=0;
				}

				// Mark that this one is part of a switching component
				// and pop stack (just decrement as value is overwritten
				// if needed anyway).
				iteratorstack[iteratorstacktop]->componentid=numberswitchingcomps;

				// Increment number of switching components for this graph;
				iteratorstack[iteratorstacktop]->vertexid=sizeofunknownpixelgraph;
				sizeofunknownpixelgraph++;

				// Use lastiterator to make iteration linking and update last iterator.
				iteratorstack[iteratorstacktop]->itelink=lastiterator;
				lastiterator=iteratorstack[iteratorstacktop];

				// NOTE: stack top is decrement here AFTER the condition check.
				//       Decrement happens regardless of evaluation of the condition.
			}while(iteratorstacktop-->0);

			// Lastiterator->itelink should be marked null so that
			// iteration can end.
			// lastiterator->itelink=0;

			// Now that size is known add to size stack.
			sizestacklast->next=malloc(sizeof(struct IntLink));
			sizestacklast=sizestacklast->next;
			sizestacklast->unknownpixelgraphsize=sizeofunknownpixelgraph;

			// Set new iterator.
			// First in memory stack maybe already marked so check has to be made.
			// No null members because they aren't pushed. Hence null means stack
			// is empty.
			while(memorystack && memorystack->m->componentid!=NO_COMPONENT_MARKING){
				struct LinkedListOfUnknownPixelMarker *temp=memorystack;
				memorystack=temp->next;
				free(temp);
			}
			// Is there other switching component to handle!
			if(memorystack){
				// Initialize next loop.
				iterator=memorystack->m;

				// Reuse memory
				{
					struct LinkedListOfUnknownPixelMarker *temp=memorystack->next;
					scomppointerstacklast->next=memorystack;
					scomppointerstacklast=scomppointerstacklast->next;
					scomppointerstacklast->next=0;
					memorystack=temp;
				}

				// THIS JUMPS UP!
				// Reason for goto rather then loop structure is that
				// for, while, and do while cannot be used on a loop
				// which first iteration has diferent initialization
				// then next iterations. For loop would decide after
				// initialization would it continue so it would
				// waste cycles. And while and do-while would need
				// extra if which is wasting cycles.
				goto SWITCHING_COMPONENT_DETECT_MORE_IN_MEMORY_STACK_LOOP;
			}
		}

		// Not needed anymore.
		free(iteratorstack);

		// Allocate switching component as one big array.
		SwitchingComponent *swc=malloc(sizeof(SwitchingComponent)*numberswitchingcomps);
		// Initialize switching components.
		for(uint32_t i=0;i<numberswitchingcomps;i++){

			// Initilize unknown pixel graph pointer.
			swc[i].unknownpixelgraph=scomppointerstack->m;
			swc[i].sizeofunknownpixelgraph=sizestack->unknownpixelgraphsize;

			// Free switchingcomponentstack elements.
			// Moving the next element in the iteration happens here.
			{
				struct LinkedListOfUnknownPixelMarker *temp=scomppointerstack->next;
				free(scomppointerstack);
				scomppointerstack=temp;
			}

			// Free sizestack element.
			{
				struct IntLink *temp=sizestack->next;
				free(sizestack);
				sizestack=temp;
			}

		}
		// Set return value.
		*scomp=swc;
	}
	else numberswitchingcomps=-3;

	// Error mark
	DETECT_SWITCHING_COMPONENTS_ERROR_GOTO:
	free(markerheader.rows);
	return numberswitchingcomps;
}
void freeSwitchingComponentArray(SwitchingComponent *scomp,int32_t arrlen){
	// Free the internals of every element of the array scomp.
	for(int32_t i=arrlen-1;i>=0;i--){
		// First the unknown pixel graph.
		// Go though by iteration link.
		UnknownPixelMarker *ite=scomp[i].unknownpixelgraph;
		do{
			// Temporary storage for next node.
			UnknownPixelMarker *temp=ite->itelink;

			// Free edge value if edge exists.
			// Because two markers share the same
			// structure free only clowerval and
			// rlowerval to avoid double free.
			if(ite->clower) free(ite->clowerval);
			if(ite->rlower) free(ite->rlowerval);

			// Free the marker structure.
			free(ite);

			// Move to next unknown pixel marker if it exists.
			ite=temp;
		}while(ite);
	}
	// Free the array it self.
	free(scomp);
}
void depthFirstIterateVertexes(SwitchingComponent *scomp,void *data,depthFirstIterateVertexesCallback* callback){
	// Stack for going back at the search.
	UnknownPixelMarker *vertexstack[scomp->sizeofunknownpixelgraph];
	uint32_t vertexstacktop=0;
	vertexstack[0]=scomp->unknownpixelgraph;

	// Table to mark visitation.
	uint8_t markingtable[scomp->sizeofunknownpixelgraph];
	for(uint32_t i=0;i<scomp->sizeofunknownpixelgraph;i++) markingtable[i]=0;
	do{
		uint32_t vertexid=vertexstack[vertexstacktop]->vertexid;
		for(uint32_t edgeindex=markingtable[vertexid];edgeindex<4;edgeindex++){
			if(vertexstack[vertexstacktop]->edges[edgeindex]){
				// New stack top.
				vertexstack[vertexstacktop+1]=vertexstack[vertexstacktop]->edges[edgeindex];
				vertexstacktop++;
			}
		}
		;
	}while(vertexstacktop-->0);
}
/*
* Internal structure to store bounds calculation.
*/
struct SwcBounds{
	UnknownPixelMarker *xminyminvertex;
	UnknownPixelMarker *xmaxyminvertex;
	UnknownPixelMarker *xminymaxvertex;
	UnknownPixelMarker *xmaxymaxvertex;
};
/*
* Calculate bounds of given switching component.
*/
static void calcSwitchingComponentBounds(struct SwcBounds *bounds,SwitchingComponent *restrict scomp){
	UnknownPixelMarker *ite=scomp->unknownpixelgraph->itelink;
	// Use first vertex on unknown pixel graph
	bounds->xminyminvertex=scomp->unknownpixelgraph;
	bounds->xmaxyminvertex=scomp->unknownpixelgraph;
	bounds->xminymaxvertex=scomp->unknownpixelgraph;
	bounds->xmaxymaxvertex=scomp->unknownpixelgraph;
	// Find the bounds by looping through the vertexes.
	while(ite){
		if(bounds->xminyminvertex->col>ite->col || bounds->xminyminvertex->row>ite->row){
			bounds->xminyminvertex=ite;
		}
		if(bounds->xminymaxvertex->col>ite->col || bounds->xminymaxvertex->row<ite->row){
			bounds->xminymaxvertex=ite;
		}
		if(bounds->xmaxyminvertex->col<ite->col || bounds->xmaxyminvertex->row>ite->row){
			bounds->xmaxyminvertex=ite;
		}
		if(bounds->xmaxymaxvertex->col<ite->col || bounds->xmaxymaxvertex->row<ite->row){
			bounds->xmaxymaxvertex=ite;
		}
		ite=ite->itelink;
	}
}

static int8_t CheckOneBlackColourableOnePixelCompliance(int32_t x1,int32_t x2,EdgeValue *restrict edge,Description *restrict desc, BlocksRanges *restrict ranges,int32_t *restrict pcoverblockx1,int8_t fullbackallowed){
	// Get the blocks which cover the pixels?
	// x1 leftmost cover block is a parameter.
	int32_t coverblockx2=findRightmostCover(x2,ranges,desc->length);

	// Check what edge
	switch(edge->state){
		case NONO_EDGE_STATE_NEXTTO:
		case NONO_EDGE_STATE_FULL_WHITE:
			if(desc->lengths[*pcoverblockx1]==1 && desc->lengths[coverblockx2]==1 && *pcoverblockx1==coverblockx2){
				return 1;
			}
			break;
		case NONO_EDGE_STATE_FULL_BLACK:
			// No need to check block exchange since block
			// should change if partial solver is used.
			if(fullbackallowed && desc->lengths[coverblockx2]-edge->stateinfo.blackcount==1 && *pcoverblockx1==coverblockx2){
				return 1;
			}
			break;
		case NONO_EDGE_STATE_MIX:
			if(desc->lengths[*pcoverblockx1]==1 && desc->lengths[coverblockx2]==1 && edge->stateinfo.maxblackcount==1 && *pcoverblockx1+edge->stateinfo.groupcount==coverblockx2){
				*pcoverblockx1=coverblockx2;
				return 1;
			}
			break;
	}
	return 0;
}

uint32_t NonoDetectOneBlackColourableOnePixelSquareSwitchingComponent(Nonogram *restrict nono,TableHead *restrict tables,SwitchingComponent *restrict scomp){
// One black colourable one pixel square switching component (one black colourable one pixel SSC).

	// Make the check that bounds as they are in a square (exists a min side and max side for both abscissa and ordinate).
	struct SwcBounds bounds;
	calcSwitchingComponentBounds(&bounds,scomp);
	if(bounds.xminyminvertex->col!=bounds.xminymaxvertex->col || bounds.xmaxyminvertex->col!=bounds.xmaxymaxvertex->col || bounds.xminyminvertex->row!=bounds.xmaxyminvertex->row || bounds.xminymaxvertex->row!=bounds.xmaxymaxvertex->row){
		return 0;
	}

	// Calculate the number vertexes between v_{x_min,y_min} and v_{x_min,y_max}.
	// v_{x_min,y_max} does not necessarily exist so be careful.
	UnknownPixelMarker *rowite=bounds.xminyminvertex;
	int32_t sidelengthofssc=1;
	int32_t clmblk=findLeftmostCover(bounds.xminyminvertex->row,tables->firsttable.col+bounds.xminyminvertex->col,nono->colsdesc[bounds.xminyminvertex->col].length);

	// Full black edge is allowed only in situation where SSC is ESC.
	// To detect this for next loop just check that rowite can be moved.
	// There should be least be one edge for both directions if partial solver
	// was used so null check is not needed.
	int8_t fullblackallowed=(!rowite->chigher->chigher || !rowite->rhigher->rhigher);

	while(rowite->rhigher){
		sidelengthofssc+=1;
		if(!CheckOneBlackColourableOnePixelCompliance(rowite->row,rowite->rhigher->row,rowite->rhigherval,nono->colsdesc+bounds.xminyminvertex->col,tables->firsttable.col+bounds.xminyminvertex->col,&clmblk,fullblackallowed)){
			return 0;
		}
		rowite=rowite->rhigher;
	}
	// If iterator's row and ymax's row aren't equal
	// proposed switching component is no SSC.
	if(rowite->row!=bounds.xminymaxvertex->row) return 0;

	int32_t rowblks[sidelengthofssc];
	int32_t rowref[sidelengthofssc];
	// Index for the rowblks and rowref.
	int32_t j=0;

	// Reset the iterator
	rowite=bounds.xminyminvertex;
	while(rowite){
		rowblks[j]=findLeftmostCover(bounds.xminyminvertex->col,tables->firsttable.row+rowite->row,nono->rowsdesc[rowite->row].length);
		rowref[j]=rowite->row;

		j+=1;
		rowite=rowite->rhigher;
	}

	// Reset iterator to next column.
	UnknownPixelMarker *columnite=bounds.xminyminvertex->chigher;
	int32_t prevclmindex=bounds.xminyminvertex->col;
	while(columnite){

		// New column so columns refind leftmost relevant block.
		clmblk=findLeftmostCover(bounds.xminyminvertex->row,tables->firsttable.col+columnite->col,nono->colsdesc[columnite->col].length);

		// Process rows.
		rowite=columnite;
		j=0;
		while(rowite->rhigher){

			// Check that current row is stored in rowref, there is edge to lower row index and that
			// found lower row unknown pixel is at previous column. If any of these fail
			// SWC cannot be a SSC.
			if(!(rowite->row==rowref[j] && rowite->clower && rowite->clower->col==prevclmindex)){
				return 0;
			}

			if(!CheckOneBlackColourableOnePixelCompliance(rowite->clower->col,rowite->col,rowite->clowerval,nono->rowsdesc+rowite->row,tables->firsttable.row+rowite->row,rowblks+j,fullblackallowed)){
				return 0;
			}

			if(!CheckOneBlackColourableOnePixelCompliance(rowite->row,rowite->rhigher->row,rowite->rhigherval,nono->colsdesc+rowite->col,tables->firsttable.col+rowite->col,&clmblk,fullblackallowed)){
				return 0;
			}

			j+=1;
			rowite=rowite->rhigher;
		}
		if(!(rowite->row==rowref[j] && rowite->clower && rowite->clower->col==prevclmindex)){
			return 0;
		}
		if(!CheckOneBlackColourableOnePixelCompliance(rowite->rlower->col,rowite->col,rowite->clowerval,nono->rowsdesc+rowite->row,tables->firsttable.row+rowite->row,rowblks+j,fullblackallowed)){
			return 0;
		}

		// Move to next column
		prevclmindex=columnite->col;
		columnite=columnite->chigher;
	}
	// We haven't handle xmax column so shape cannot be One-Black Colourable One-Pixel SSC.
	if(prevclmindex!=bounds.xmaxyminvertex->col){
		return 0;
	}

	// Return the factorial of length of the side of the SSC (sidelengthofssc)
	// as that is the number of the solutions.
	return ifactorial(sidelengthofssc);
}
