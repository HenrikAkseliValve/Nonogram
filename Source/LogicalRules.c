#include<stdint.h>
#include"LogicalRules.h"
#include"Nonograms.h"

int32_t nonogramLogicalRule0(enum Pixel *line,int32_t length,uint32_t stride,Description *desc,BlocksRanges *ranges){
	int32_t colored=0;
	if(desc->lengths[0]==0){
		for(int32_t nth=length-1;nth>=0;nth--) if(colourLinesPixel(line,nth,stride,WHITE_PIXEL,&colored)==0) return -1;
		return colored;
	}
	return colored;
}
int32_t nonogramLogicalRule11(enum Pixel *line,int32_t length,uint32_t stride,Description *desc,BlocksRanges *ranges){

	if(desc->lengths[0]==0) return 0;

	int32_t colored=0;
	
	// Go throught every block.
	for(int32_t block=0;block<desc->length;block++){
		// Alias for block's length.
		register int32_t blockslength=desc->lengths[block];
		// Go though every pixel which leftmost and rightmost solution overlap.
		for(int32_t pixel=ranges->blocksrangerights[block]+1-blockslength;pixel<=ranges->blocksrangelefts[block]-1+blockslength;pixel++){
			if(!colourLinesPixel(line,pixel,stride,BLACK_PIXEL,&colored)) return -1;
		}
	}
	
	return colored;
	
}
int32_t nonogramLogicalRule12(enum Pixel *line,int32_t length,uint32_t stride,Description *desc,BlocksRanges *ranges){

	if(desc->lengths[0]==0) return 0;

	int32_t colored=0;
	
	// Before first block is there outside of block's range unknown pixels.
	for(int32_t pixel=0;pixel<ranges->blocksrangelefts[0];pixel++){
		if(!colourLinesPixel(line,pixel,stride,WHITE_PIXEL,&colored)) return -1;
	}
	
	// After the first block's rightmost pixel and before last block's leftmost pixel.
	for(int32_t block=1;block<desc->length;block++){
		for(int32_t pixel=ranges->blocksrangerights[block-1]+1;pixel<ranges->blocksrangelefts[block];pixel++){
			if(!colourLinesPixel(line,pixel,stride,WHITE_PIXEL,&colored)) return -1;
		}
	}
	
	// After last block's range.
	for(int32_t pixel=ranges->blocksrangerights[desc->length-1]+1;pixel<length;pixel++){
		if(!colourLinesPixel(line,pixel,stride,WHITE_PIXEL,&colored)) return -1;
	}
	
	return colored;
}
int32_t nonogramLogicalRule13(enum Pixel *line,int32_t length,uint32_t stride,Description *desc,BlocksRanges *ranges){

	if(desc->lengths[0]==0) return 0;

	if(desc->length>1){
		int colored=0;
		
		for(int32_t block1=0;block1<desc->length;block1++){
			// Mark pixel start of the block under testing.
			int32_t pixel=ranges->blocksrangelefts[block1];
			
			// has to be black and pixel isn't zeroth at the line.
			if(*getLinePixel(line,pixel,stride)==BLACK_PIXEL && pixel>0){
				// Handling for left of block1
				for(int32_t block2=0;block2<block1;block2++){
					
					if(ranges->blocksrangelefts[block2]<=pixel && ranges->blocksrangerights[block2]>=pixel){
						// Check that length is one because otherwise rule doesn't apply.
						if(desc->lengths[block2]>1) goto START_PIXEL_LOOP_EXIT; // Jump down
					}
				}
			
				// Handling for right of block1
				for(int32_t block2=block1+1;block2<desc->length;block2++){
					if(ranges->blocksrangelefts[block2]<=pixel && pixel<=ranges->blocksrangerights[block2]){
						// Check that length is one because otherwise rule doesn't apply
						if(desc->lengths[block2]>1) goto START_PIXEL_LOOP_EXIT; // Jump down
					}
				}
				
				// Mark pixel next to zero.
				if(!colourLinesPixel(line,--pixel,stride,WHITE_PIXEL,&colored)) return -1;
			}
			START_PIXEL_LOOP_EXIT:
			
			pixel=ranges->blocksrangerights[block1];
			// Has to be black and pixel isn't at length-1!
			if(*getLinePixel(line,pixel,stride)==BLACK_PIXEL && pixel<length-1){
				// Handling for left of block1
				for(int32_t block2=0;block2<block1;block2++){
					if(ranges->blocksrangelefts[block2]<=pixel && pixel<=ranges->blocksrangerights[block2]){
						// Check that length is one because otherwise rule doesn't apply.
						if(desc->lengths[block2]>1) goto END_PIXEL_LOOP_EXIT;
					}
					
				}
				// Handling for right of block1
				for(int32_t block2=block1+1;block2<desc->length;block2++){
					if(ranges->blocksrangelefts[block2]<=pixel && pixel<=ranges->blocksrangerights[block2]){
						if(desc->lengths[block2]>1) goto END_PIXEL_LOOP_EXIT;
					}
				}
				
				// Mark pixel next to zero.
				if(!colourLinesPixel(line,++pixel,stride,WHITE_PIXEL,&colored)) return -1;
			}
			
			END_PIXEL_LOOP_EXIT:;
		}
		return colored;
	}
	return 0;
}
int32_t nonogramLogicalRule14(enum Pixel *line,int32_t length,uint32_t stride,Description *desc,BlocksRanges *ranges){

	if(desc->lengths[0]==0) return 0;

	int colored=0;
	// First we have to find pixel which has
	// black,unknow,block in it.
	for(int32_t pixel=1;pixel<length-1;pixel++){
		if(*getLinePixel(line,pixel-1,stride)==BLACK_PIXEL 
		   && *getLinePixel(line,pixel,stride)==UNKNOWN_PIXEL
		   && *getLinePixel(line,pixel+1,stride)==BLACK_PIXEL){
			// Found a pixel that has the attribute.
			// Now we have to find how large hypothetical block would be!
			// NOTE: We could get leftmost at outher loop by checking per pixel
			//       when does block change.
			//       However this isn't optimal for large line lengths
			//       and relative small block sizes because you would
			//       ask question (do we come from white to black or black to white)
			//       for every single pixel and updating "machine" state.
			register int32_t leftmost;
			for(leftmost=pixel-1;leftmost>0 && *getLinePixel(line,leftmost-1,stride)==BLACK_PIXEL;leftmost--);
			if(*getLinePixel(line,leftmost,stride)!=BLACK_PIXEL) leftmost++;

			register int32_t rightmost;
			for(rightmost=pixel+1;rightmost<length-1 && *getLinePixel(line,rightmost+1,stride)==BLACK_PIXEL;rightmost++);
			if(*getLinePixel(line,rightmost,stride)!=BLACK_PIXEL) rightmost--;


			// Check that if pixel is black leftmost and rightmost makes
			// group that is smaller then any block.
			for(int32_t block=0;block<desc->length;block++){
				if(!(ranges->blocksrangerights[block]<leftmost || rightmost<ranges->blocksrangelefts[block])){
					if(rightmost-leftmost+1<=desc->lengths[block]) goto FOUND_POSSIBILITY;
				}
			}
			if(!colourLinesPixel(line,pixel,stride,WHITE_PIXEL,&colored)) return -1;
			FOUND_POSSIBILITY:;
		}
	}
	
	return colored;
}
int32_t nonogramLogicalRule15(enum Pixel *line,int32_t length,uint32_t stride,Description *desc,BlocksRanges *ranges){

	if(desc->lengths[0]==0) return 0;

	int32_t colored=0;
	
	for(int32_t pixel=1;pixel<length-1;pixel++){
		// If pixel is black check that ether side is not.
		// This is to prevent using pixel that is middle
		// of the black pixel group.
		if(*getLinePixel(line,pixel,stride)==BLACK_PIXEL){
			if(*getLinePixel(line,pixel-1,stride)!=BLACK_PIXEL || *getLinePixel(line,pixel+1,stride)!=BLACK_PIXEL){
				// Get minimun length of the block covering the pixel
				int32_t minblocklength=INT32_MAX;
				
				// If zero then there is no block for black pixel (should not happen).
				// If one then blocks of same length.
				// If two or more then blocks covering pixel are defferent length.
				int32_t samelength=0;
				for(int32_t block=0;block<desc->length;block++){
					if(ranges->blocksrangelefts[block]<=pixel && pixel<=ranges->blocksrangerights[block]){
						if(minblocklength!=desc->lengths[block]) samelength++;
						if(minblocklength>desc->lengths[block]) minblocklength=desc->lengths[block];
					}
				}
				if(minblocklength==INT32_MAX) continue;
				
				// Memory for leftmost and rightmost new black pixel group rule is trying to make.
				int32_t leftmost=pixel;
				int32_t rightmost=pixel;
				
				// If pixel-1 is non-black then find nearest on left white pixel.
				// This check is done so that earlier if sentece was true because
				// of this truth sentence.
				if(*getLinePixel(line,pixel-1,stride)!=BLACK_PIXEL){
					// Find nearest white pixel from left which is not too far.
					int32_t emptypixel;
					for(emptypixel=pixel-1
					   ;*getLinePixel(line,emptypixel,stride)!=WHITE_PIXEL
					     && emptypixel>0
					     && emptypixel>pixel-minblocklength+1
					   ;emptypixel--);
					// Check that we really found white pixel.
					// If yes colour black from right side of pixel.
					if(*getLinePixel(line,emptypixel,stride)==WHITE_PIXEL){
						int32_t colour;
						for(colour=pixel+1;colour<=emptypixel+minblocklength && colour<length;colour++){
							if(!colourLinesPixel(line,colour,stride,BLACK_PIXEL,&colored)) return -1;
						}
						rightmost=colour-1;
					}
					else{
						// Since we didn't find right most ether find right most black pixel.
						for(;*getLinePixel(line,rightmost,stride)==BLACK_PIXEL;rightmost++);
						// Loop jump one over.
						rightmost--;
					}
				}
				
				// if pixel+1 is non-black then find right white pixel.
				// This check is done so that earlier if sentece was true because
				// of this truth sentence.
				if(*getLinePixel(line,pixel+1,stride)!=BLACK_PIXEL){
					int32_t emptypixel;
					for(emptypixel=pixel+1
					   ;*getLinePixel(line,emptypixel,stride)!=WHITE_PIXEL
					     && emptypixel<length-1
					     && emptypixel<pixel+minblocklength-1
					   ;emptypixel++);
					// Check that we really found white pixel.
					// If yes colour black from right side of pixel.
					if(*getLinePixel(line,emptypixel,stride)==WHITE_PIXEL){
						int32_t colour;
						for(colour=pixel-1;colour>=emptypixel-minblocklength && colour>=0;colour--){
							if(!colourLinesPixel(line,colour,stride,BLACK_PIXEL,&colored)) return -1;
						}
						leftmost=colour+1;
					}
					else{
						// Since we didn't find left most ether find left most black pixel.
						for(;*getLinePixel(line,leftmost,stride)!=BLACK_PIXEL;leftmost--);
						// Loop jump one over.
						leftmost++;
					}
				}
				
				// If every block that covering pixel is same length
				// then test that newly colored black pixel group is
				// same length so that endings can be coloured to white.
				if(samelength==1 && rightmost-leftmost+1==minblocklength){
					if(rightmost<length-1){
						if(!colourLinesPixel(line,rightmost+1,stride,WHITE_PIXEL,&colored)) return -1;
					}
					if(leftmost>0){
						if(!colourLinesPixel(line,leftmost-1,stride,WHITE_PIXEL,&colored)) return -1;
					}
				}
			}
		}
	}
	return colored;
}
int32_t nonogramLogicalRule21(enum Pixel *line,int32_t length,uint32_t stride,Description *desc,BlocksRanges *ranges){
	
	if(desc->lengths[0]==0) return 0;

	int32_t updates=0;
	
	// This rule is pointless if there is not more then one block.
	if(desc->length>1){
		
		int32_t block=0;
		
		// Check first block's right since left from it is no block.
		if(ranges->blocksrangerights[block]>=ranges->blocksrangerights[block+1]-desc->lengths[block+1]){
			ranges->blocksrangerights[block]=ranges->blocksrangerights[block+1]-desc->lengths[block+1]-1;
			updates++;
		}
		

		for(block++;block<desc->length-1;block++){
			if(ranges->blocksrangelefts[block]<=ranges->blocksrangelefts[block-1]+desc->lengths[block-1]){
				ranges->blocksrangelefts[block]=ranges->blocksrangelefts[block-1]+desc->lengths[block-1]+1;
				updates++;
			}

			if(ranges->blocksrangerights[block]>=ranges->blocksrangerights[block+1]-desc->lengths[block+1]){
				ranges->blocksrangerights[block]=ranges->blocksrangerights[block+1]-desc->lengths[block+1]-1;
				updates++;
			}
		}

		// Check that last block's left side only since from right we don't have anything.
		if(ranges->blocksrangelefts[block]<=ranges->blocksrangelefts[block-1]+desc->lengths[block-1]){
			ranges->blocksrangelefts[block]=ranges->blocksrangelefts[block-1]+desc->lengths[block-1]+1;
			updates++;
		}
	}
	
	return updates;
}
int32_t nonogramLogicalRule22(enum Pixel *line,int32_t length,uint32_t stride,Description *desc,BlocksRanges *ranges){
	
	if(desc->lengths[0]==0) return 0;

	int32_t update=0;
	
	for(int32_t block=0;block<desc->length;block++){
		if(ranges->blocksrangelefts[block]>0 && *getLinePixel(line,ranges->blocksrangelefts[block]-1,stride)==BLACK_PIXEL){
			ranges->blocksrangelefts[block]+=1;
			update++;
		}
		if(ranges->blocksrangerights[block]<length-1 && *getLinePixel(line,ranges->blocksrangerights[block]+1,stride)==BLACK_PIXEL){
			ranges->blocksrangerights[block]-=1;
			update++;
		}
	}
	
	return update;
}
int32_t nonogramLogicalRule23(enum Pixel *line,int32_t length,uint32_t stride,Description *desc,BlocksRanges *ranges){

	if(desc->lengths[0]==0) return 0;

	int32_t update=0;
	
	if(desc->length>1){
		int32_t grouplen=0;
		int32_t leftmostblock=0;
		
		for(int32_t pixel=0;pixel<length;pixel++){
			// Detect black pixel group and if group is found count
			// the length. When group ends check can blocks' ranges
			//  be updated.
			if(*getLinePixel(line,pixel,stride)==BLACK_PIXEL) grouplen++;
			else if(grouplen){
				
				int32_t groupstart=pixel-grouplen;
				int32_t groupend=pixel-1;
				
				// Increment block until block which covering black group under inspection is found.
				while(leftmostblock+1<desc->length && ranges->blocksrangerights[leftmostblock]<groupend){
					leftmostblock++;
				}
				
				// Go though blocks one by one and mark firstlongenough and lastlongenough.
				// If both exists we can't do anything.
				int32_t firstlongenough=-1;
				int32_t lastlongenough=-1;

				int32_t rightmostblock;
				for(rightmostblock=leftmostblock
				   ;rightmostblock<desc->length && ranges->blocksrangelefts[rightmostblock]<=groupstart
				   ;rightmostblock++)
				{
					if(desc->lengths[rightmostblock]>=grouplen){
						if(firstlongenough==-1) firstlongenough=rightmostblock;
						lastlongenough=rightmostblock;
					}
				}
				// After the loop rightmostblock is outside group we investigating.

				if(firstlongenough>-1){
					int32_t step=0;
					for(int32_t block=firstlongenough-1;block>=leftmostblock;block--){
						step+=2;
						ranges->blocksrangerights[block]=groupstart-step;
					}
				}
				if(lastlongenough>-1){
					int32_t step=0;
					for(int32_t block=lastlongenough+1;block<rightmostblock;block++){
						step+=2;
						ranges->blocksrangelefts[block]=groupend+step;
					}
				}

				// This group has been handled so
				// start new count.
				grouplen=0;

			}
		}
		
	}
	return update;
}
int32_t nonogramLogicalRule24(enum Pixel *line,int32_t length,uint32_t stride,Description *desc,BlocksRanges *ranges){

	if(desc->lengths[0]==0) return 0;

	int32_t update=0;
	// For every black pixel group between white pixels
	// try determined which block pixel belongs to.
	// Update block's ranges if it can be determined.

	// We won't be checking last pixel as it being white the following pixel cannot be a black pixel
	// as there is no pixel in first place.
	int32_t pixel=-1;
	while(pixel<length-1){
		if((pixel==-1 || *getLinePixel(line,pixel,stride)==WHITE_PIXEL) && *getLinePixel(line,pixel+1,stride)==BLACK_PIXEL){
			// Mark the first white pixel.
			int32_t firstwhite=pixel;
			// Count number of black pixels and check for white pixels to stop the count.
			// pixel+=2 in the start of the for loop is because we already know pixel+1 is black
			// from previos if statement.
			for(pixel+=2;pixel<length && *getLinePixel(line,pixel,stride)==BLACK_PIXEL;pixel++);
			// Pixel was not black or we hit border of the line.
			// Do notting it pixel was unknown pixel.
			if(pixel==length || *getLinePixel(line,pixel,stride)==WHITE_PIXEL){
				// Mark the second white pixel or the "next" pixel after last pixel in the line.
				int32_t secondwhite=pixel;

				// We now have two white pixels which between we had black pixels.
				// What block is it?
				// Length of the BLACK block is (secondwhite-1)-(firstwhite+1)+1
				// since we have to move over white pixels.
				int32_t blocklength=secondwhite-firstwhite-1;

				// Is there block of that length in the area?
				// If not report clear error.
				// Count is number of blocks that can explain the pixel pattern
				// (white pixel, black pixel group, and white pixel).
				int32_t count=0;
				int32_t firstblock;
				int32_t lastblock;
				for(int32_t block=0;block<desc->length;block++){
					// Block to explain the pattern it must covered it fully and be same length. 
					if(blocklength==desc->lengths[block] && ranges->blocksrangelefts[block]<=firstwhite+1 && ranges->blocksrangerights[block]>=secondwhite-1){
						if(count==0) firstblock=block;
						lastblock=block;
						count++;
					}
				}

				if(count==0) return -1;
				// For two blocks first block cannot explain anything
				// more then this block to right. For the last block it
				// cannot explain anything more left then this block.
				if(ranges->blocksrangerights[firstblock]>=secondwhite){
					ranges->blocksrangerights[firstblock]=secondwhite-1;
					update++;
				}
				if(ranges->blocksrangelefts[lastblock]<=firstwhite){
					ranges->blocksrangelefts[lastblock]=firstwhite+1;
					update++;
				}
#if 0
// More "safe" version in the sense that it has more checks.
				else if(count==1){
					// We know that there is one block so
					// just move range to as black pixel group.
					if(ranges->blocksrangelefts[lastblock]<=firstwhite){
						ranges->blocksrangelefts[lastblock]=firstwhite+1;
						update++;
					}
					if(ranges->blocksrangerights[lastblock]>=secondwhite){
						ranges->blocksrangerights[lastblock]=secondwhite-1;
						update++;
					}
				}
				else if(count==2){
					// For two blocks first block cannot explain anything
					// more then this block to right. For the last block it
					// cannot explain anything more left then this block.
					if(ranges->blocksrangerights[firstblock]>=secondwhite){
						ranges->blocksrangerights[firstblock]=secondwhite-1;
						update++;
					}
					if(ranges->blocksrangelefts[lastblock]<=firstwhite){
						ranges->blocksrangelefts[lastblock]=firstwhite+1;
						update++;
					}
				}
				else if(count>2){
					// For more blocks then check could be made but their
					// is good change ether left of right of it is only two blocks
					// case. Making it possible next update this would be in two
					// block case.
				}
#endif
			}

			// Since second white pixel can be next first pixel
			// jump back to the while loop start without incrementing
			// pixel.
			continue;
		}
		pixel++;
	}

	return update;
}
int32_t nonogramLogicalRule25(enum Pixel *line,int32_t length,uint32_t stride,Description *desc,BlocksRanges *ranges){

	if(desc->lengths[0]==0) return 0;

	int32_t update=0;

	// Handle from the left.
	int32_t pixel=ranges->blocksrangelefts[0];
	int32_t lastwhite=pixel-1;
	for(int32_t block=0;block<desc->length;block++){
		int8_t notfound=1;
		do{

			if(pixel==length || *getLinePixel(line,pixel,stride)==WHITE_PIXEL){
				if(pixel-lastwhite-1>=desc->lengths[block]){
					if(ranges->blocksrangelefts[block]<=lastwhite){
						ranges->blocksrangelefts[block]=lastwhite+1;
						update++;
					}
					notfound=0;
				}
				lastwhite=pixel;
			}
			else{
				// Here length is calculate so that we jump over white to possible black (lastwhite+1)
				// and pixel is black so +1 to adjust the length to be in pixels.
				if(pixel-(lastwhite+1)+1==desc->lengths[block]){

					if(ranges->blocksrangelefts[block]<=lastwhite){
						ranges->blocksrangelefts[block]=lastwhite+1;
						update++;
					}

					// There is enough space for a block.
					notfound=0;
					// Empty space for white pixel
					pixel++;
					// Next pixel is possible white but we can't say for sure.
					// For algorithm let's treated as such so that empty space
					// which can't take another block is ignored.
					lastwhite=pixel;
				}
			}

			// Increment pixel and safety check.
			pixel++;

		}while(notfound && pixel<=length);
		if(notfound && pixel>length) goto RULE25_ERROR_EXIT;
	}

	// Handle from the right.
	pixel=ranges->blocksrangerights[desc->length-1];
	lastwhite=pixel+1;
	for(int32_t block=desc->length-1;block>=0;block--){
		int8_t notfound=1;
		do{

			if(pixel==-1 || *getLinePixel(line,pixel,stride)==WHITE_PIXEL){
				if(lastwhite-pixel-1>=desc->lengths[block]){
					if(ranges->blocksrangerights[block]>=lastwhite){
						ranges->blocksrangerights[block]=lastwhite-1;
						update++;
					}
					notfound=0;
				}
				lastwhite=pixel;
			}
			else{
				// Here length is calculate by jumping over white (lastwhite-1)
				// and adjusting to number of pixels by +1.
				if((lastwhite-1)-pixel+1==desc->lengths[block]){

					if(ranges->blocksrangerights[block]>=lastwhite){
						ranges->blocksrangerights[block]=lastwhite-1;
						update++;
					}

					// There is enough space for a block.
					notfound=0;
					// Empty space for white pixel.
					pixel--;
					// Next pixel is possible white but we can't say for sure.
					// For algorithm let's treated as such so that empty space
					// which can't take another block is ignored.
					lastwhite=pixel;
				}
			}

			// Decrement pixel and safety check.
			pixel--;

		}while(notfound && pixel>=-1);
		if(notfound && pixel<-1) goto RULE25_ERROR_EXIT;
	}

	return update;

	// Handle error if there isn't space for every block.
	RULE25_ERROR_EXIT:
	return -1;

}
int32_t nonogramLogicalRule31(enum Pixel *line,int32_t length,uint32_t stride,Description *desc,BlocksRanges *ranges){

	if(desc->lengths[0]==0) return 0;

	int32_t update=0;
	int32_t colored=0;
	
	for(int32_t block=0;block<desc->length;block++){
		// Calculate last possible pixel that could be black.
		// This is ether block's range's right most pixel or
		// if blocks' ranges overlap then leftmost pixel of
		// next block's range minus 1 to get out of the
		// next block.
		int32_t lastblack;
		if(block<desc->length-1 && ranges->blocksrangelefts[block+1]<=ranges->blocksrangerights[block]){
			lastblack=ranges->blocksrangelefts[block+1]-1;
		}
		else lastblack=ranges->blocksrangerights[block];
		
		// Calculate starting position of possible first black pixel.
		// Mirror of previews lines basically.
		int32_t firstblack;
		if(block>0 && ranges->blocksrangelefts[block]<=ranges->blocksrangerights[block-1]){
			firstblack=ranges->blocksrangerights[block-1]+1;
		}
		else firstblack=ranges->blocksrangelefts[block];
		
		// Find first black pixel.
		for(;firstblack<=lastblack;firstblack++){
			if(*getLinePixel(line,firstblack,stride)==BLACK_PIXEL){
				// Find last black pixel.
				for(;lastblack>=firstblack;lastblack--){
					if(*getLinePixel(line,lastblack,stride)==BLACK_PIXEL){
						
						// Check in between is not larger then the selected block.
						if(lastblack-firstblack+1>desc->lengths[block]) return -2; 
						
						// Colour in between to black.
						for(int32_t pixel=firstblack+1;pixel<lastblack;pixel++){
							if(!colourLinesPixel(line,pixel,stride,BLACK_PIXEL,&colored)) return -1;
						}
						{
							// Make block's range update.
							// Variable candidate is candidate pixel for new range border.
							register int32_t candidate;
							// Left side
							candidate=lastblack+1-desc->lengths[block];
							if(ranges->blocksrangelefts[block]<candidate){
								ranges->blocksrangelefts[block]=candidate;
								update++;
							}
							// Right side
							candidate=firstblack-1+desc->lengths[block];
							if(ranges->blocksrangerights[block]>candidate){
								ranges->blocksrangerights[block]=candidate;
								update++;
							}
						}
					}
				}
			}
		}
		
	}
	
	return update+colored;
}
int32_t nonogramLogicalRule32(enum Pixel *line,int32_t length,uint32_t stride,Description *desc,BlocksRanges *ranges){

	if(desc->lengths[0]==0) return 0;

	int32_t update=0;
	int32_t coloured=0;
	
	for(int32_t block=0;block<desc->length;block++){
		
		// Since length of the pixel group between white pixels
		// has to be less then block's length there is no point
		// comparing blocks which are length one since groups
		// less than that don't exist.
		if(desc->lengths[block]>1){
			
			// Start of the non-white group
			int32_t startofgroup=ranges->blocksrangelefts[block];
			int32_t lastrangeupdategroup=0;
			// Iteration variable for indexing pixels in the bloc's range.
			int32_t iterator=ranges->blocksrangelefts[block];
			
			// Is true if last pixel tested was white.
			int8_t lastwaswhite=0;
			
			// Store information is previous or next block's range
			// overlapping with current blocks range. If not then
			// block's range is stored.
			// Used to check can group be colored white.
			int32_t leftborder;
			if(block>0 && ranges->blocksrangelefts[block]<ranges->blocksrangerights[block-1]){
				leftborder=ranges->blocksrangerights[block-1];
			}
			else leftborder=ranges->blocksrangelefts[block];

			int32_t rightborder;
			if(block<desc->length-1 && ranges->blocksrangerights[block]>ranges->blocksrangelefts[block+1]){
				rightborder=ranges->blocksrangelefts[block+1];
			}
			else rightborder=ranges->blocksrangerights[block];
			
			// Search for pixel group (exclusive) between white pixels.
			while(iterator<=ranges->blocksrangerights[block]){
				if(*getLinePixel(line,iterator,stride)!=WHITE_PIXEL){
					if(lastwaswhite) startofgroup=iterator;
					if(iterator==ranges->blocksrangerights[block] || *getLinePixel(line,iterator+1,stride)==WHITE_PIXEL){
						// If block is bigger or equal to group then mark as possible block's range update block.
						// If not check is group too small for only possible block covering it.
						if(iterator-startofgroup+1>=desc->lengths[block]){
							if(lastrangeupdategroup==0 && ranges->blocksrangelefts[block]<startofgroup){
								ranges->blocksrangelefts[block]=startofgroup;
								update++;
							}
							lastrangeupdategroup=iterator;
						}
						else if(leftborder<startofgroup && rightborder>iterator){
							// There is no overlapping group so we can color white.
							for(int32_t pixel=startofgroup;pixel<=iterator;pixel++){
								if(!colourLinesPixel(line,pixel,stride,WHITE_PIXEL,&coloured)) return -1;
							}
						}
					}
					lastwaswhite=0;
				}
				else lastwaswhite=1;
				iterator++;
			}
			// Here handle last range update run.
			if(ranges->blocksrangerights[block]>lastrangeupdategroup){
				ranges->blocksrangerights[block]=lastrangeupdategroup;
				update++;
			}
		}
	}
	
	return update+coloured;
}
int32_t nonogramLogicalRule33(enum Pixel *line,int32_t length,uint32_t stride,Description *desc,BlocksRanges *ranges){

	if(desc->lengths[0]==0) return 0;

	int32_t colour=0;
	int32_t update=0;
	
	// Boolean to store did overlap happen last iteration.
	// Start at false (zero) because first block doesn't
	// have prevoius block.
	uint8_t previousoverlap=0;
	uint8_t nextoverlap;
	
	for(int32_t block=0;block<desc->length;block++){

		// First check that next block doesn't overlap
		// and the boolean does previous overlap. 
		if(block<desc->length-1 && ranges->blocksrangerights[block]>=ranges->blocksrangelefts[block+1]) nextoverlap=1;
		else nextoverlap=0;
		
		// Only one of the rules apply at one time.
		// First check rule 3.3.1 since 3.3.2 and 3.3.3
		// need additional information form pixels between
		// leftmost and rightmost.
		// Also rules 3.3.2 and 3.3.3 won't be done if
		// previousoverlap xor nextoverlap is false since
		// one side has to overlap and other one doesn't.
		if(!previousoverlap && *getLinePixel(line,ranges->blocksrangelefts[block],stride)==BLACK_PIXEL){
			// 3.3.1 left to right
			// Mark pixels black.
			for(int32_t pixel=ranges->blocksrangelefts[block]+1;pixel<ranges->blocksrangelefts[block]+desc->lengths[block];pixel++){
				if(!colourLinesPixel(line,pixel,stride,BLACK_PIXEL,&colour)) return -1;
			}
			
			// update ranges
			int32_t test=ranges->blocksrangelefts[block]+desc->lengths[block]-1;
			if(test>=length) return -1;
			if(ranges->blocksrangerights[block]>test){
				ranges->blocksrangerights[block]=test;
				update++;
			}
			{
				int32_t test=ranges->blocksrangerights[block];
				if(nextoverlap && test>ranges->blocksrangelefts[block+1]){
					ranges->blocksrangelefts[block+1]=test;
					update++;
				}
			}
			
			// Mark pixels white
			if(ranges->blocksrangelefts[block]>0 && *getLinePixel(line,stride,ranges->blocksrangelefts[block]-1)==UNKNOWN_PIXEL){
				if(!colourLinesPixel(line,ranges->blocksrangelefts[block]-1,stride,WHITE_PIXEL,&colour)) return -1;
				if(block>0 && ranges->blocksrangerights[block-1]==ranges->blocksrangelefts[block]-1){
					if(ranges->blocksrangerights[block]==0) return -1;
					ranges->blocksrangerights[block-1]--;
					update++;
				}
			}
			if(ranges->blocksrangerights[block]<desc->length-1
				&& *getLinePixel(line,ranges->blocksrangerights[block]+1,stride)==UNKNOWN_PIXEL){
				if(!colourLinesPixel(line,ranges->blocksrangerights[block]+1,stride,WHITE_PIXEL,&colour)) return -1;
			}
		}
		else if(!nextoverlap && *getLinePixel(line,ranges->blocksrangerights[block],stride)==BLACK_PIXEL){
			// 3.3.1 right to left
			// mark pixels black
			for(int32_t pixel=ranges->blocksrangerights[block]-1;pixel>ranges->blocksrangerights[block]-desc->lengths[block];pixel--){
				if(!colourLinesPixel(line,pixel,stride,BLACK_PIXEL,&colour)) return -1;
			}
			
			// Update ranges
			int32_t test=ranges->blocksrangerights[block]-desc->lengths[block]+1;
			if(test<0) return -1;
			if(ranges->blocksrangelefts[block]<test){
				ranges->blocksrangelefts[block]=test;
				update++;
			}
			{
				int32_t test=ranges->blocksrangelefts[block]-2;
				if(previousoverlap && test<ranges->blocksrangerights[block-1]){
					ranges->blocksrangerights[block-1]=test;
					update++;
				}
			}
			
			// Mark pixels white
			if(ranges->blocksrangelefts[block]>0
			  && *getLinePixel(line,ranges->blocksrangelefts[block]-1,stride)==UNKNOWN_PIXEL){
				if(!colourLinesPixel(line,ranges->blocksrangelefts[block]-1,stride,WHITE_PIXEL,&colour)) return -1;
			}
			if(ranges->blocksrangerights[block]<desc->length-1
			  && *getLinePixel(line,ranges->blocksrangerights[block]+1,stride)==UNKNOWN_PIXEL){
				if(!colourLinesPixel(line,ranges->blocksrangerights[block]+1,stride,WHITE_PIXEL,&colour)) return -1;
				if(block<=length-1 && ranges->blocksrangelefts[block+1]==ranges->blocksrangerights[block]+1){
					if(ranges->blocksrangelefts[block]==length-1) return -1;
					ranges->blocksrangelefts[block+1]++;
					update++;
				}
			}
		}
		else if(previousoverlap!=nextoverlap){
			// For rules 3.3.2 and 3.3.3 we need to know some facts
			// about pixels between the leftmost and rightmost.
			// Oh and since we know that previosoverlap != nextoverlap
			// is true we can just use nextoverlap to know
			// what is the value of the other one
			if(nextoverlap){
				int32_t lastblack;
				for(int32_t pixel=ranges->blocksrangelefts[block];pixel<=ranges->blocksrangerights[block];pixel++){
					if(*getLinePixel(line,pixel,stride)==BLACK_PIXEL){
						lastblack=pixel;
						while(++pixel<=ranges->blocksrangerights[block]){
							if(*getLinePixel(line,pixel,stride)==BLACK_PIXEL){
								// 3.3.3 left
								// Find the last pixel in this black group that is in the block's range.
								int32_t startblack=pixel;
								while(pixel+1<=ranges->blocksrangerights[block] && *getLinePixel(line,pixel+1,stride)==BLACK_PIXEL){
									pixel++;
								}
								if(*getLinePixel(line,pixel,stride)==BLACK_PIXEL) goto CONTINUE_FOR_INNER_LOOPS;
								if((pixel-1)-lastblack+1>desc->lengths[block]){
									ranges->blocksrangerights[block]=startblack-2;
									update++;
									goto  CONTINUE_FOR_INNER_LOOPS;
								}
							}
							else if(*getLinePixel(line,pixel,stride)==WHITE_PIXEL){
								// 3.3.2 left
								if(pixel-1<ranges->blocksrangelefts[block]) return -1;
								ranges->blocksrangerights[block]=pixel-1;
								update++;
								goto  CONTINUE_FOR_INNER_LOOPS;
							}
						}
					}
				}
			}
			else{
				int32_t lastblack;
				for(int32_t pixel=ranges->blocksrangerights[block];pixel>=ranges->blocksrangelefts[block];pixel--){
					if(*getLinePixel(line,pixel,stride)==BLACK_PIXEL){
						lastblack=pixel;
						while(--pixel>=ranges->blocksrangelefts[block]){
							if(*getLinePixel(line,pixel,stride)==BLACK_PIXEL){
								// 3.3.3 right
								// Find the first pixel in this black group that is in the block's range.
								int32_t startblack=pixel;
								while(pixel-1>=ranges->blocksrangelefts[block] && *getLinePixel(line,pixel-1,stride)==BLACK_PIXEL){
									pixel--;
								}
								if(*getLinePixel(line,pixel,stride)==BLACK_PIXEL) goto CONTINUE_FOR_INNER_LOOPS;
								if(lastblack-(pixel+1)+1>desc->lengths[block]){
										ranges->blocksrangelefts[block]=startblack+2;
										update++;
										goto  CONTINUE_FOR_INNER_LOOPS;
									}
								if(*getLinePixel(line,pixel,stride)==WHITE_PIXEL){
									// 3.3.2 right
									if(pixel+1<ranges->blocksrangerights[block]) return -1;
									ranges->blocksrangelefts[block]=pixel+1;
									update++;
									goto  CONTINUE_FOR_INNER_LOOPS;
								}
							}
						}
					}
				}
			}
		}
		CONTINUE_FOR_INNER_LOOPS:
		// We are done with this loop
		// there for nextovelap is prevuisoverlap
		// because we move to next block to right.
		previousoverlap=nextoverlap;
	}
	
	return update+colour;
}
