#include<stdint.h>
#include"ValidityCheck.h"
#include"Nonograms.h"

int8_t nonogramIsValid(Nonogram *nono){

	// Three checks performed here.
	// 1) column and row descriptions must have same total number of pixels.
	// 2) No description should be longer then line.
	// 3) Zero description have only one block length of zero.
	//
	// We could do these at the same time.

	int32_t rowpixelcount=0;
	for(int32_t row=nono->height-1;row>=0;row--){
		Description *desc=nono->rowsdesc+row;
		if(calcDescriptionsLength(desc)>nono->width) return 1;
		for(int32_t block=desc->length-1;block>0;block--){
			if(desc->lengths[block]==0) return 2;
			rowpixelcount+=desc->lengths[block];
		}
		rowpixelcount+=desc->lengths[0];
	}

	int32_t colpixelcount=0;
	for(int32_t col=nono->width-1;col>=0;col--){
		Description *desc=nono->colsdesc+col;
		if(calcDescriptionsLength(desc)>nono->height) return 3;
		for(int32_t block=desc->length-1;block>0;block--){
			if(desc->lengths[block]==0) return 4;
			colpixelcount+=desc->lengths[block];
		}
		colpixelcount+=desc->lengths[0];
	}

	if(rowpixelcount!=colpixelcount) return 5;

	return 0;
}
/*
* Helper function to place callback parameters.
*/
static inline uint8_t callCallback(enum Pixel *pixelval,const int32_t pixel,const uint32_t lineindex,const uint8_t roworcol,UnknownCallback *func,void *callbackdata,EdgeValue *edge){
	return func((Pixel*)pixelval,pixel*(roworcol)+lineindex*(!roworcol),pixel*(!roworcol)+lineindex*(roworcol),callbackdata,edge);
}
/*
* Helper function for solutionDimensionIsValid when unknown pixels detector function have to run for a line.
* Calculates edge state for callback.
*/
static inline uint8_t checkLineForUnknowns(enum Pixel *line,int32_t pixel,uint32_t stride,int32_t length,const uint8_t roworcol,const uint32_t lineindex,const Table *restrict table,UnknownCallback *func,void *callbackdata){
		// Additional edge information between unknown pixels given to callbacks.
	EdgeValue edge;
	// New line with one unknown already handled
	// so put state to NEXTTO.
	edge.state=NONO_EDGE_STATE_NEXTTO;
	edge.stateinfo.blackcount=0;
	edge.stateinfo.maxblackcount=0;
	edge.stateinfo.groupcount=0;

	for(pixel++;pixel<length;pixel++){
		enum Pixel *pixelval=getLinePixel(line,pixel,stride);
		if(*pixelval==UNKNOWN_PIXEL){
			uint8_t returned=callCallback(pixelval,pixel,lineindex,roworcol,func,callbackdata,&edge);
			if(returned) return returned;
			// Reset the state
			edge.state=NONO_EDGE_STATE_NEXTTO;
			edge.stateinfo.blackcount=0;
			edge.stateinfo.maxblackcount=0;
		}
		else if(*pixelval==BLACK_PIXEL){
			// If state is FULL_BLACK then count up.
			// If state is FULL WHITE then MIX.
			// If state is MIX then count up and compare max.
			// If state is NEXXTO then FULL_BLACK.
			switch(edge.state){
				case NONO_EDGE_STATE_FULL_BLACK:
					edge.stateinfo.blackcount++;
					break;
				case NONO_EDGE_STATE_FULL_WHITE:
					edge.state=NONO_EDGE_STATE_MIX;
					edge.stateinfo.blackcount=1;
					edge.stateinfo.maxblackcount=1;
					edge.stateinfo.groupcount=1;
					break;
				case NONO_EDGE_STATE_MIX:
					edge.stateinfo.blackcount++;
					if(edge.stateinfo.maxblackcount<edge.stateinfo.blackcount){
						edge.stateinfo.maxblackcount=edge.stateinfo.blackcount;
					}
					if(edge.stateinfo.blackcount>0){
						edge.stateinfo.groupcount+=1;
					}
					break;
				case NONO_EDGE_STATE_NEXTTO:
					edge.state=NONO_EDGE_STATE_FULL_BLACK;
					edge.stateinfo.blackcount=1;
					break;
			}
		}
		else{
			// If state is FULL_WHITE then noting
			// If state is FULL_BLACK then MIX.
			// If state is MIX then reset black pixel counting.
			// If state is NEXXTO then state is FULL_WHITE.
			if(edge.state==NONO_EDGE_STATE_FULL_BLACK){
				edge.state=NONO_EDGE_STATE_MIX;
				edge.stateinfo.maxblackcount=edge.stateinfo.blackcount;
				edge.stateinfo.blackcount=0;
			}
			else if(edge.state==NONO_EDGE_STATE_MIX){
				edge.stateinfo.blackcount=0;
			}
			else if(edge.state==NONO_EDGE_STATE_NEXTTO) edge.state=NONO_EDGE_STATE_FULL_WHITE;
		}
	}
	return 0;
}
NonogramsDimensionValidness solutionDimensionIsValid(const Nonogram *restrict nono,const Table *restrict table,const uint8_t roworcol,UnknownCallback *func,void *callbackdata){
	NonogramsDimensionValidness validness=NONO_SOLUTION_IS_VALID;

	// General variables based upon roworcol
	// (Note roworcol=true => columns and roworcol=false => rows).
	int32_t numberoflines=nono->width*roworcol+nono->height*(!roworcol);
	int32_t length=nono->width*(!roworcol)+nono->height*(roworcol);
	uint32_t stride=nono->width*(roworcol)+(!roworcol);

	// Check that nonogram agrees description of the dimensions.
	for(int32_t lineindex=0;lineindex<numberoflines;lineindex++){
		// Get the line and description of the line.
		enum Pixel *line=getLine((Nonogram*)nono,roworcol,lineindex,(Table*)table);
		Description *desc=getDesc((Nonogram*)nono,roworcol,lineindex);

		// Pixel index in the line.
		int32_t pixel=0;

		// Check every block.
		for(int32_t block=0;block<desc->length;block++){
			// White pixels left of current block value should be ignored.
			for(;pixel<length;pixel++){
				enum Pixel *pixelval=getLinePixel(line,pixel,stride);
				if(*pixelval==UNKNOWN_PIXEL){
					// Solution is partial
					validness=NONO_SOLUTION_IS_PARTIAL;

					// Call callback if it exists.
					if(func){
						uint8_t returned=callCallback(pixelval,pixel,lineindex,roworcol,func,callbackdata,0);
						if(returned) return NONO_SOLUTION_IS_CALLBACK_ERROR;
						returned=checkLineForUnknowns(line,pixel,stride,length,roworcol,lineindex,table,func,callbackdata);
						if(returned) return NONO_SOLUTION_IS_CALLBACK_ERROR;
					}
					// Can not be certain about this line since unknown
					// pixel maybe causing missing or shorter block.
					// Left the analysis up to the caller.
					goto SOLUTION_CHECK_CONTINUE;
				}
				else if(*pixelval==BLACK_PIXEL) break;
			}
			int32_t count=0;
			for(;pixel<length;pixel++){
				enum Pixel *pixelval=getLinePixel(line,pixel,stride);
				if(*pixelval==UNKNOWN_PIXEL){
					validness=NONO_SOLUTION_IS_PARTIAL;

					// Call callback if it exists.
					if(func){
						uint8_t returned=callCallback(pixelval,pixel,lineindex,roworcol,func,callbackdata,0);
						if(returned) return NONO_SOLUTION_IS_CALLBACK_ERROR;
						returned=checkLineForUnknowns(line,pixel,stride,length,roworcol,lineindex,table,func,callbackdata);
						if(returned) return NONO_SOLUTION_IS_CALLBACK_ERROR;
					}
					// Can not be certain about this line since unknown
					// pixel maybe causing missing or shorter block.
					// Left the analysis up to the caller.
					goto SOLUTION_CHECK_CONTINUE;
				}
				else if(*pixelval==WHITE_PIXEL) break;
				else count++;
			}
			// Check that count matches the block size.
			if(desc->lengths[block]!=count) return NONO_SOLUTION_IS_INVALID;
		}
		// Check that rest of the pixels are white.
		// Includes unknown pixels as well since they should be obvious white
		// pixels menaing something has gone wrong.
		for(;pixel<length;pixel++){
			enum Pixel *pixelval=getLinePixel(line,pixel,stride);
			if(*pixelval!=WHITE_PIXEL){
				return NONO_SOLUTION_IS_INVALID;
			}
		}
		SOLUTION_CHECK_CONTINUE:;
	}

	return validness;
}
