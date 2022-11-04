/*
* Tool to generate nonogram configuration files.
*/
#include<getopt.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<stdlib.h>
#include<setjmp.h>
#include<string.h>
#include<sys/uio.h>
#include<sys/random.h>
#include<limits.h>
#include"Nonograms.h"
#include"ValidityCheck.h"
#include"Etc.h"

/*
* Callback for generating Nonogram configuration.
*/
typedef void (*ConfGeneratorCallback)(Nonogram *nono,Table *solution,int32_t sizecoeffx,int32_t sizecoeffy,void* userinfo);

/*
* Error longjmp variable.
*/
jmp_buf ErrorJmpBuffer;


/*
* Function to check allocate random buffer.
* Static variables used here are:
*  - RBuff to store random data.
*  - RBuffLen to store length of the buffer
*  - RBuffIndex to store next unused part of the buffer.
*  - RollingBitmask to index individual bits.
*/
static uint8_t RBuff[256];
static ssize_t RBuffLen=0;
static ssize_t RBuffIndex=0;
static uint8_t RollingBitmask=0x01;
void AllocRBuff(int32_t sizeestimate){
	// Sizeestimate should not be more then RBuff.
	int32_t length=0;
	if(sizeestimate<=sizeof(RBuff)){
		length=sizeestimate;
	}
	// Get random data via getRandom.
	if((RBuffLen=getrandom(RBuff,length,0))==-1){
		longjmp(ErrorJmpBuffer,51);
	}
	RBuffIndex=0;
	RollingBitmask=0x01;
}
/*
* Throw a perfect coin (decision on two events with equal probability).
* Function parameter sizeestimate is estimate about amount of coin
* tosses is actually needed.
*
* Tosses are separated by zero and non-zero.
*/
uint8_t throwPerfectCoin(int32_t sizeestimate){
	
	// Check is there buffer left.
	if(RBuffIndex==RBuffLen){
		AllocRBuff(sizeestimate);
	}
	
	// Check the coin flip. Every bit is a coin toss.
	uint8_t cointoss=RBuff[RBuffIndex]&RollingBitmask;
	
	// Offset the rolling bitmask.
	RollingBitmask<<=1;
	// If rolling bitmask is zero it means index has to be updated.
	// (Bit shift is outside of the byte.)
	if(RollingBitmask==0x00){
		RBuffIndex++;
		RollingBitmask=0x01;
	}

	return cointoss;
}
/*
* Simulate uniform three event probability distribution.
*/
uint8_t sampleDiscreteUniformThree(int32_t sizeestimate){
	
	// Use byte (8 bits) sized number to check is number less than or equal
	// to 85 for even A; more than 85 and less or equal than 2*85=170 for
	// event B; more than 170 (and less than 256) is event C.
	
	// Because we need a byte of information check that there is a byte 
	// of information avaible at RBuff. To get this information calculate
	// amount bits rollingbitmask used already.
	uint8_t bitsused;
	switch(RollingBitmask){
		case 0x01:
			bitsused=0;
			break;
		case 0x02:
			bitsused=1;
			break;
		case 0x04:
			bitsused=2;
			break;
		case 0x08:
			bitsused=3;
			break;
		case 0x10:
			bitsused=4;
			break;
		case 0x20:
			bitsused=5;
			break;
		case 0x40:
			bitsused=6;
			break;
		case 0x80:
			bitsused=7;
			break;
	}
	if((8*RBuffLen-8)<8*RBuffIndex+bitsused){
		// There is not byte amount of information.
		// Just throw out unused bits.
		(void)write(STDERR_FILENO,"NOTE:Program threw out random bits!\n",36);
		AllocRBuff(sizeestimate);
	}
	
	// Get bits for one byte.
	uint8_t samplehighmask=0xFF<<bitsused;
	uint8_t thesample=(RBuff[RBuffIndex]&(samplehighmask))+(RBuff[RBuffIndex+1]&(~samplehighmask));
	
	// Move on by one byte meaning one one index.
	RBuffIndex++;
	
	// Return the event.
	// NOTE: since first even is zero it's product is commented out.
	return /*(thesample<=85)*0+*/(85<thesample && thesample<=170)*1+(170<thesample)*2;
}
/*
* Integer by Binomial distribution via n perfect coin flips.
* Expected value is n/2 and variance is n*0.5(1-0.5)=n*0.25=n/4
*/
int32_t sampleBinomialNCoinFlips(int32_t sizeestimate,int32_t n){
	int32_t value=0;
	for(int32_t i=n;i>0;i--){
		// Double negation operation ('!') maps non-zero integers
		// to integers with keeping zero to zero mapping.
		value+=(!(!throwPerfectCoin(sizeestimate)));
	}
	return value;
}
/*
* Simple nextto edge generator which creates a solution.
*/
void nexttoSSCGen(Nonogram *nono,Table *solution,int32_t sizecoeffx,int32_t sizecoeffy,void *null){

	// Result of nextto generator is a solution not a partial solution.
	// Hence marking is false.
	nono->flags.haspartial=0;

	// Set size of the nonogram
	nono->width=sizecoeffx;
	nono->height=sizecoeffx;

	// Allocate memory for the example solution.
	solution->pixels=malloc(sizeof(Pixel)*nono->width*nono->height);
	if(!solution->pixels){
		// Allocation error.
		longjmp(ErrorJmpBuffer,50);
	}

	// Colour the solution.
	for(int32_t col=nono->width-1;col>=0;col--){
		for(int32_t row=nono->height-1;row>=0;row--){
			if(col==row) colourTablePixel (nono,solution,row,col,BLACK_PIXEL);
			else colourTablePixel (nono,solution,row,col,WHITE_PIXEL);
		}
	}
}
/*
* Colour an edge on the line.
*/
static void colourEdge(Nonogram *nono,Table *solution,int32_t row,int32_t col,int32_t i,int32_t length,EdgeState type){
	// We know that nextto type is not need to be handled.
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wswitch"
	switch(type){
		case NONO_EDGE_STATE_FULL_BLACK:
			colourTablePixel (nono,solution,row,col,BLACK_PIXEL);
			break;
		case NONO_EDGE_STATE_FULL_WHITE:
			colourTablePixel (nono,solution,row,col,WHITE_PIXEL);
			break;
		case NONO_EDGE_STATE_MIX:{
				Pixel colour;
				if(i%2==0 && i<length) colour.e=BLACK_PIXEL;
				else colour.e=WHITE_PIXEL;
				colourTablePixel (nono,solution,row,col,colour.e);
			}
			break;
	}
	#pragma GCC diagnostic pop
}
/*
* It is easier to make One Colourable One Pixel SSC generator which 
* outputs partial solution. To have a solution generator is to just
* give unique colouring which is simple process. Hence function
* oneColourableOnePixelSSCPartialgen_internal is the internals of 
* partial solution generator which does not mark flags.haspartial or
* block's ranges.
*/
void oneColourableOnePixelSSCPartialgen_internal(Nonogram *nono,Table *solution,int32_t sizecoeffx,int32_t sizecoeffy){
	
	// Sizecoeffx is only used since SSC.
	// Sizecoeffx is the size of the generated switching component!
	// Nonograms size hence depends upon choices made for the pixels
	// in-between unknown pixels.
	//
	// Choose edge randomly from four?
	// No! Nextto edge needs no inbetween pixels hence rectangle
	// shape with filling cannot happen if side of the nextto edge
	// is other type of edge.
	//
	// We want this:
	// ?----??---?
	// ?----??---?
	// |    ||   |
	// ?----??---?
	// |    ||   |
	// ?----??---?
	//
	// Not this:
	//
	// ??---??
	// |??--??
	// ?---???
	// |    ||
	// | ?---?
	// |    ?
	// ?
	//
	// Full black is also special type reserved only to 2 by 2
	// one-Colourable One-Pixel SSCs.
	//
	// Mixed edge does have additional requirement of three
	// in-between pixels minimum.
	//
	// How to uniformly generate?
	//
	// Decision on one "line" of edges.
	// ├─ One "line" of edges are Nextto
	// └─ "line" of edges has in-between pixels
	//    ├─ In-between with more than or equal three pixels.
	//    │  └─ Mixture of mixed, fullblack (if sizecoeffx=2), fullwhite
	//    └─ In-between with less than three pixels.
	//       └─ Mixture of fullblack (if sizecoeffx=2), fullwhite
	//
	// Since mixed edge implies in-between must be more than or equal to three pixels
	// then it is uniformly close enough to say mixed, fullblack, fullwhite have equal
	// probability per edge.
	//
	// To have equal distribution of edges when fullblack is allowed for sizecoeffx=2:
	// 
	//  u is variable going throught the sides (u is always less than sizecoeffx).
	//
	//                         No decision on an edge
	//                      ╭─────────────┴─────────────╮
	//                  3/4 │                           │ 1/4
	//                 in-between                     u=0 nextto edge
	//             ╭────────┴──────┬───────────────╮
	//         1/3 │               │ 1/3           │ 1/3
	//           u=0 fullblack   u=0 fullwhite   u=0 mixed
	//
	// To have equal distribution of edges when sizecoeffx=3 (fullblack is not allowed):
	// 
	//                                No decision on an edge
	//                           ╭───────────────┴───────────────╮
	//                       4/5 │                               │ 1/5
	//                      in-between                         u=0 nextto edge
	//             ╭─────────────┴──────────────╮                │ 1
	//         1/2 │                            │ 1/2          u=1 nextto edge
	//           u=0 fullwhite                u=0 mixed
	//     ╭───────┴───────╮            ╭───────┴───────╮
	// 1/2 │               │ 1/2    1/2 │               │ 1/2
	//   u=1 fullwhite   u=1 mixed    u=1 fullwhite   u=1 mixed
	//
	// To have equal distribution of edges when sizecoeffx=3 (fullblack is not allowed):
	//
	//                                                                 No decision on an edge
	//                                                       ╭────────────────────┴───────────────────────────────╮
	//                                                   8/9 │                                                    │ 1/9
	//                                                  in-between                                              u=0 nextto edge
	//                           ╭───────────────────────────┴────────────────────────────╮                       │ 1
	//                       1/2 │                                                        │ 1/2                 u=1 nextto edge
	//                         u=0 fullwhite                                            u=1 mixed                 │ 1
	//             ╭─────────────┴──────────────╮                           ╭─────────────┴──────────────╮      u=2 nextto edge
	//         1/2 │                            │ 1/2                   1/2 │                            │ 1/2
	//           u=1 fullwhite                u=1 mixed                   u=1 fullwhite                u=1 mixed
	//     ╭───────┴───────╮            ╭───────┴───────╮           ╭───────┴───────╮            ╭───────┴───────╮
	// 1/2 │               │ 1/2    1/2 │               │ 1/2    1/2│               │ 1/2    1/2 │               │ 1/2
	//   u=2 fullwhite   u=2 mixed    u=2 fullwhite   u=2 mixed   u=2 fullwhite   u=2 mixed    u=2 fullwhite   u=2 mixed
	//
	// First choice is 1 over 2^(sizecoeffx-1)+1... bit problematic since 2^n+1 is exponentially growing
	// and cannot be easily broke up.
	// Best choice here is to estimate via 1/2^(n+1) which is got by throwing perfect coin n+1 times and if
	// no "heads" are got then nextto edge is chosen. This means nextto edges are bit rarer than other
	// edges but not significantly enough.
	//TODO: There could be a problem with the generator. If nextto edge and fullwhite mix in the way that
	//      part of the generated table the is enclosed by border of two white pixels result may not be partial
	//      solution anymore.
	
	uint8_t fullblackallowed=(sizecoeffx==2);
	int32_t numberofsides=sizecoeffx-1;
	int32_t RBuffestimate=2*numberofsides+1+(fullblackallowed*4)+numberofsides*sizecoeffx;
	
	// Store lengths of inbetween pixels and type of edge it is.
	// Note that perimeter has sizecoeffx-1 edges but internally every column and row
	// of edges has sizecoeffx amount of edges.
	// Indexing is first the column and then the rows.
	int32_t *inbetweenlengths=malloc(numberofsides*2*sizeof(int32_t));
	EdgeState *types=malloc(numberofsides*sizecoeffx*2*sizeof(EdgeState));
	if(!inbetweenlengths || !types){
		// Allocation failed
		longjmp(ErrorJmpBuffer,50);
	}
	
	// Loop to deciding type and length of each edge.
	for(int32_t i=0;i<numberofsides*2;i++){
		// Throw a coin sizecoeffx times to decide between
		// in-between edge and nextto edge.
		for(int32_t r=sizecoeffx;r>0;r--){
			if(throwPerfectCoin(RBuffestimate)){
				// Coin toss decided event in-between.
				#if defined(_DEBUG_) && defined(EDGE_MESSAGE)
					(void)write(STDERR_FILENO,"DEBUG:IN-BETWEEN\n",17);
				#endif
				inbetweenlengths[i]=sampleBinomialNCoinFlips(RBuffestimate,16)+3;
				if(fullblackallowed){
					for(int32_t a=0;a<sizecoeffx;a++){
						switch(sampleDiscreteUniformThree(RBuffestimate)){
							case 0:
								#if defined(_DEBUG_) && defined(EDGE_MESSAGE)
									(void)write(STDERR_FILENO,"DEBUG:FULLWHITE\n",16);
								#endif
								types[i*sizecoeffx+a]=NONO_EDGE_STATE_FULL_WHITE;
								break;
							case 1:
								#if defined(_BEBUG_) && defined(EDGE_MESSAGE)
									(void)write(STDERR_FILENO,"DEBUG:MIX\n",10);
								#endif
								types[i*sizecoeffx+a]=NONO_EDGE_STATE_MIX;
								break;
							case 2:
								#if defined(_DEBUG_) && defined(EDGE_MESSAGE)
									(void)write(STDERR_FILENO,"DEBUG:FULLBLACK\n",16);
								#endif
								types[i*sizecoeffx+a]=NONO_EDGE_STATE_FULL_BLACK;
								break;
						}
					}
				}
				else{
					for(int32_t a=0;a<sizecoeffx;a++){
						if(throwPerfectCoin(RBuffestimate)){
							#if defined(_DEBUG_) && defined(EDGE_MESSAGE)
								(void)write(STDERR_FILENO,"DEBUG:FULLWHITE\n",16);
							#endif
							types[i*sizecoeffx+a]=NONO_EDGE_STATE_FULL_WHITE;
						}
						else{
							#if defined(_DEBUG_) && defined(EDGE_MESSAGE)
								(void)write(STDERR_FILENO,"DEBUG:MIX\n",10);
							#endif
							types[i*sizecoeffx+a]=NONO_EDGE_STATE_MIX;
						}
					}
				}
				
				// Continue outher for loop.
				goto CONTINUE_EGDE_TYPE_AND_LENGTH_LOOP;
			}
		}
		// Coin decided nextto edges.
		inbetweenlengths[i]=0;
		for(int32_t a=0;a<sizecoeffx;a++){
			#if defined(_DEBUG_) && defined(EDGE_MESSAGE)
				(void)write(STDOUT_FILENO,"DEBUG:NEXTTO\n",13);
			#endif
			types[i*sizecoeffx+a]=NONO_EDGE_STATE_NEXTTO;
		}
		
		CONTINUE_EGDE_TYPE_AND_LENGTH_LOOP:;
	}

	// Calculate size of the nonogram.
	nono->width=sizecoeffx;
	nono->height=sizecoeffx;
	for(int32_t i=0;i<numberofsides;i++){
		nono->width+=inbetweenlengths[i];
		nono->height+=inbetweenlengths[i+numberofsides];
	}
	// Add two columns and rows to side of the SSC
	// as a "root". Root is static line used as
	// support to SSC shape. Lines are static to
	// have most little impact.
	nono->width+=2;
	nono->height+=2;

	// Allocate a partial solution.
	solution->pixels=malloc(sizeof(Pixel)*nono->width*nono->height);
	if(!solution->pixels){
		// Allocation error.
		longjmp(ErrorJmpBuffer,50);
	}

	// Make partial solution.
	{
		// Colour the column sides.
		int32_t row,col=1;
		for(int32_t edgei=0;edgei<numberofsides;edgei++){
			// Colour the first unknown on the row.
			row=1;
			colourTablePixel(nono,solution,row,col,UNKNOWN_PIXEL);
			for(int32_t urow=0;urow<numberofsides;urow++){
				for(int32_t i=1;i<=inbetweenlengths[edgei];i++){
					colourEdge(nono,solution,row,col+i,i,inbetweenlengths[edgei],types[edgei*sizecoeffx+urow]);
				}
				// Move to next unknown pixel along the column
				row+=inbetweenlengths[urow+numberofsides]+1;
				// Colour the unknown pixel.
				colourTablePixel(nono,solution,row,col,UNKNOWN_PIXEL);
			}
			// Last row is handled outside of loop.
			for(int32_t i=1;i<=inbetweenlengths[edgei];i++){
				colourEdge(nono,solution,row,col+i,i,inbetweenlengths[edgei],types[edgei*sizecoeffx+numberofsides]);
			}
			// Move to next column.
			col+=inbetweenlengths[edgei]+1;
		}
	}
	{
		// Colour the row sides except last one.
		int32_t row=1,col;
		for(int32_t edgei=0;edgei<numberofsides;edgei++){
			// Colour the first unknown on the row.
			col=1;
			for(int32_t ucol=0;ucol<numberofsides;ucol++){
				for(int32_t i=1;i<=inbetweenlengths[edgei+numberofsides];i++){
					colourEdge(nono,solution,row+i,col,i,inbetweenlengths[edgei+numberofsides],types[numberofsides*sizecoeffx+edgei*sizecoeffx+ucol]);
				}
				// Move to next unknown pixel along the column
				col+=inbetweenlengths[ucol]+1;
			}
			// Last row is handled outside of loop.
			for(int32_t i=1;i<=inbetweenlengths[edgei+numberofsides];i++){
				colourEdge(nono,solution,row+i,col,i,inbetweenlengths[edgei+numberofsides],types[numberofsides*sizecoeffx+edgei*sizecoeffx+numberofsides]);
			}
			// Move to next unknown pixel along the column
			row+=inbetweenlengths[edgei+numberofsides]+1;
		}
	}
	// Colour "root" sides of the nonogram. Rule for colouring
	// is white pixels when next to unknown pixel and black
	// otherwise.
	{
		// Colour corners to black
		colourTablePixel(nono,solution,0             ,0            ,BLACK_PIXEL);
		colourTablePixel(nono,solution,nono->height-1,0            ,BLACK_PIXEL);
		colourTablePixel(nono,solution,nono->height-1,nono->width-1,BLACK_PIXEL);
		colourTablePixel(nono,solution,0             ,nono->width-1,BLACK_PIXEL);
	}

	// Handle zeroth row and last row.
	for(int32_t col=1;col<nono->width-1;col++){
		if(getTablePixel(nono,solution,1,col)==UNKNOWN_PIXEL){
			colourTablePixel(nono,solution,0,col,WHITE_PIXEL);
		}
		else{
			colourTablePixel(nono,solution,0,col,BLACK_PIXEL);
		}
		if(getTablePixel(nono,solution,nono->height-2,col)==UNKNOWN_PIXEL){
			colourTablePixel(nono,solution,nono->height-1,col,WHITE_PIXEL);
		}
		else{
			colourTablePixel(nono,solution,nono->height-1,col,BLACK_PIXEL);
		}
	}

	// Handle zeroth row and last row.
	for(int32_t row=1;row<nono->height-1;row++){
		if(getTablePixel(nono,solution,row,1)==UNKNOWN_PIXEL){
			colourTablePixel(nono,solution,row,0,WHITE_PIXEL);
		}
		else{
			colourTablePixel(nono,solution,row,0,BLACK_PIXEL);
		}
		if(getTablePixel(nono,solution,row,nono->width-2)==UNKNOWN_PIXEL){
			colourTablePixel(nono,solution,row,nono->width-1,WHITE_PIXEL);
		}
		else{
			colourTablePixel(nono,solution,row,nono->width-1,BLACK_PIXEL);
		}
	}

	// Fill the middles to black.
	//TODO: Maybe better to do more situational heuristic
	//      since nextto edge combination with full white
	//      edges could enclose filling.
	int32_t rowoffset=2,coloffset;
	for(int32_t erow=0;erow<numberofsides;erow++){
		coloffset=2;
		for(int32_t ecol=0;ecol<numberofsides;ecol++){
			for(int32_t row=0;row<inbetweenlengths[erow+numberofsides];row++){
				for(int32_t col=0;col<inbetweenlengths[ecol];col++){
					colourTablePixel(nono,solution,row+rowoffset,col+coloffset,BLACK_PIXEL);
				}
			}
			coloffset+=inbetweenlengths[ecol]+1;
		}
		rowoffset+=inbetweenlengths[erow+numberofsides]+1;
	}

	free(types);
	free(inbetweenlengths);
}

void oneColourableOnePixelSSCgen(Nonogram *nono,Table *solution,int32_t sizecoeffx,int32_t sizecoeffy,void *null){
	
	// Result of this generator is a solution not a partial solution.
	// Hence flag is marked false.
	nono->flags.haspartial=0;

	oneColourableOnePixelSSCPartialgen_internal(nono,solution,sizecoeffx,sizecoeffy);

	// Colour the one-black Colourable One-Pixel SSC by colouring
	// diagonal to black.
	int32_t row=1,col=1;
	for(int32_t u=1;u<sizecoeffx;u++){
		// Colour diagonal pixel to black.
		colourTablePixel (nono,solution,row,col,BLACK_PIXEL);

		// Colour every other unknown pixels on the row and column to white.
		// First loops calculate next correct row, and col values in the diagonal
		// and fills to first unknown pixels in the line.
		// Second loops fills to other unknown pixels. The iu just counts
		// number of unknown pixels coloured. The ending comparison iu<sizecoeffx-u
		// is because sizecoeffx is number of unknown pixels. As we move on the diagonal
		// we have less unknown pixels to colour to white as previous loop already
		// coloured them.
		// ROWS
		int32_t nrow=row+1;
		while( getTablePixel (nono,solution,nrow,col)!=UNKNOWN_PIXEL) nrow++;
		colourTablePixel (nono,solution,nrow,col,WHITE_PIXEL);
		for(int32_t irow=row+1,iu=1;iu<sizecoeffx-u;iu++){
			// Find the next unknown pixel in the row.
			while( getTablePixel (nono,solution,irow,col)!=UNKNOWN_PIXEL) irow++;
			// Colour unknown pixel to white.
			colourTablePixel (nono,solution,irow,col,WHITE_PIXEL);
		}
		// COLUMNS
		int32_t ncol=col+1;
		while( getTablePixel (nono,solution,row,ncol)!=UNKNOWN_PIXEL) ncol++;
		colourTablePixel (nono,solution,row,ncol,WHITE_PIXEL);
		for(int32_t icol=col+1,iu=1;iu<sizecoeffx-u;iu++){
			// Find the next unknown pixel in the column.
			while( getTablePixel (nono,solution,row,icol)!=UNKNOWN_PIXEL) icol++;
			// Colour unknown pixel to white.
			colourTablePixel (nono,solution,row,icol,WHITE_PIXEL);
		}

		// Set to next unknown pixel.
		row=nrow;
		col=ncol;
	}
	// Colour the final diagonal pixel to black.
	// No need to colour anything white.
	colourTablePixel (nono,solution,row,col,BLACK_PIXEL);
}
/*
* Generate multiple nonograms as one line.
*/
void concatenateVLGen(Nonogram *nono,Table *solution,int32_t sizecoeffx,int32_t sizecoeffy,void *userinfo){
		// Allocate temporary Nonogram structures.
	Nonogram *nonotemps=malloc(sizecoeffy*sizeof(Nonogram));
	if(!nonotemps){
		// Allocation error.
		longjmp(ErrorJmpBuffer,50);
	}
	Table *solutiontemps=malloc(sizecoeffy*sizeof(Table));
	if(!solutiontemps){
		// Allocation error.
		longjmp(ErrorJmpBuffer,50);
	}

	// Estimate maximum random bit usage.
	const int32_t restimate=256;

	// Generate the solutions.
	for(int32_t i=sizecoeffy-1;i>=0;i--){

		// Have some variation on sizecoeffx.
		// For small values there has to check
		// for having high enough value.
		int32_t rsizecoeffx=sampleBinomialNCoinFlips(restimate,(2*sizecoeffx));
		if(rsizecoeffx<=1) rsizecoeffx=2;

		// Have 1/16 change to get nextto generator rather then
		// one-black Colourable One-Pixel SSC genrator.
		for(int32_t r=4;r>0;r--){
			if(throwPerfectCoin(restimate)){
				oneColourableOnePixelSSCgen(nonotemps+i,solutiontemps+i,rsizecoeffx,sizecoeffy,NULL);
				goto SSC_GEN_CHOICE_CONTINUE;
			}
		}
		nexttoSSCGen(nonotemps+i,solutiontemps+i,rsizecoeffx,sizecoeffy,NULL);
		SSC_GEN_CHOICE_CONTINUE:;
	}

	// Determent biggest width which is the width of the final nonogram.
	nono->width=nonotemps[sizecoeffy-1].width;
	for(int32_t i=sizecoeffy-2;i>=0;i--){
		if(nono->width<nonotemps[i].width) nono->width=nonotemps[i].width;
	}

	// Calculate height of final nonogram.
	// Add two rows of stability padding between the generated nonograms.
	nono->height=0;
	for(int32_t i=0;i<sizecoeffy-1;i++){
		nono->height+=nonotemps[i].height+2;
	}
	nono->height+=nonotemps[sizecoeffy-1].height;

	// Allocate a partial solution.
	solution->pixels=malloc(sizeof(Pixel)*nono->width*nono->height);
	if(!solution->pixels){
		// Allocation error.
		longjmp(ErrorJmpBuffer,50);
	}
	nono->flags.shadow=0;

	// Add the nonotemps by simple offsetting (0,sum of previous nonotemp[i].heights).
	// Copying the solutiontemps to solution has to be done per line because
	// nonotemps width maybe less than final nonogram.
	int32_t rowoffset=0;
	for(int32_t i=0;i<sizecoeffy;i++){
		for(int32_t row=0;row<nonotemps[i].height;row++){
			// Copy the tables.
			memcpy(solution->pixels+(row+rowoffset)*nono->width,solutiontemps[i].pixels+row*nonotemps[i].width,nonotemps[i].width);
			// If there is empty space left on the row fill it with black.
			for(int32_t col=nonotemps[i].width;col<nono->width;col++){
				colourTablePixel(nono,solution,row+rowoffset,col,BLACK_PIXEL);
			}
		}
		rowoffset+=nonotemps[i].height;

		// Colour the safe the padding if not last nonotemp.
		if(i<sizecoeffy-1){
			// Filling should be black so that no fullwhite edges are created.
			for(int32_t col=0;col<nono->width;col++){
				colourTablePixel(nono,solution,rowoffset,col,BLACK_PIXEL);
				colourTablePixel(nono,solution,rowoffset+1,col,BLACK_PIXEL);
			}
			rowoffset+=2;
		}
	}

	free(solutiontemps);
	free(nonotemps);
}
/*
* Generator which randomly concatenate result of other generators.
*/
void consecutiveGen(Nonogram *nono,Table *solution,int32_t sizecoeffx,int32_t sizecoeffy,void *userinfo){

	//TODO: use userinfo to transfer information on concatenated generators

	// Allocate temporary Nonogram structures.
	Nonogram *nonotemps=malloc(sizecoeffy*sizeof(Nonogram));
	if(!nonotemps){
		// Allocation error.
		longjmp(ErrorJmpBuffer,50);
	}
	Table *solutiontemps=malloc(sizecoeffy*sizeof(Table));
	if(!solutiontemps){
		// Allocation error.
		longjmp(ErrorJmpBuffer,50);
	}

	// Estimate maximum random bit usage.
	const int32_t restimate=256;

	// Generate the solutions.
	for(int32_t i=sizecoeffy-1;i>=0;i--){
		// Have 1/16 change to get nextto generator rather then
		// one-black Colourable One-Pixel SSC genrator.
		for(int32_t r=4;r>0;r--){
			if(throwPerfectCoin(restimate)){
				oneColourableOnePixelSSCgen(nonotemps+i,solutiontemps+i,sizecoeffx,sizecoeffy,NULL);
				goto SSC_GEN_CHOICE_CONTINUE;
			}
		}
		nexttoSSCGen(nonotemps+i,solutiontemps+i,sizecoeffx,sizecoeffy,NULL);
		SSC_GEN_CHOICE_CONTINUE:;
	}

	// Generate the offsets which generated nonograms exists in
	// the final nonogram?
	//
	// How to avoid overlaps?
	// Use data structure to divide regions?
	// I don't have canvas thought hence I'm not restricted to absolute coordinates.
	// Let's make this easier by not trying to make perfect uniform distribution but
	// just guaranteed no-overlap generation.
	//
	// Let's set coordinate relative to zeroth nonogram (nonotemps[0]).
	// Meaning it is always the rectangle region (0,0) to (nonotemps[0].width-1,nonotemps[0].height-1).
	// Adding nonotemps[1] can mainly be left, right, up, or down direction from nonotemps[0].
	// After selecting main direction nonotemps[1] distance and relative offset can be calculated.
	// Now that nonotemps[1] has been added it and nonotemps[0] form a region. This region can be
	// thought as node in a quadtree where two child nodes are not yet in use. After this one
	// can semi-uniformly place rest of the nonotemps by uniformly selecting between up, down,
	// left, or right of the root node or one of the empty nodes.
	//
	// What if nonograms size is too big for quadtrees remaining region?
	// Couple of options:
	//  - Make sure leafs are big enough. Negative is that it wastes space.
	//  - Reroll if region was not big enough. Negative is the potential to waste time.
	//  - Resize the parent region. Overlap can occur.
	//TODO: FINISH THIS!
	int32_t*offsetx=malloc(sizecoeffy*sizeof(int32_t));
	int32_t*offsety=malloc(sizecoeffy*sizeof(int32_t));
	for(int32_t i=sizecoeffy-1;i>=0;i--){
		;
	}
	// Calculate the size of the final nonogram.

	// Free resources used in this function only.
	free(offsety);
	free(offsetx);
	free(solutiontemps);
	free(nonotemps);
}

/*
* Start of the solver program.
* Option arguments:
*   -g select generator type (default is none).
*   -h display this information.
*/
int main(int argc,char *argv[]){
	
	// Generator selection (NULL means not selected)
	ConfGeneratorCallback confgen=NULL;
	// Coefficients to generator effecting size of outputted nonogram.
	int32_t sizecoeffx=2,sizecoeffy=2;
	
	// Handle the options.
	{
		int opt;
		while((opt=getopt(argc,argv,"hg:n:m:"))!=-1){
			switch(opt){
				case 'g':
					// Read the string argument after the '-g'.
					if(strcmp("Nextto-SSC-Gen",optarg)==0){
						confgen=nexttoSSCGen;
					}
					else if(strcmp("One-Black-Colourable-One-Pixel-SSC-Gen",optarg)==0){
						confgen=oneColourableOnePixelSSCgen;
					}
					else if(strcmp("VLConcatenate",optarg)==0){
						confgen=concatenateVLGen;
					}
					else if(strcmp("none",optarg)==0){
						confgen=NULL;
					}
					else{
						(void)write(STDERR_FILENO,"ERROR: generator given does not exist!\n",40);
						return 2;
					}
					break;
				case 'h':{
						const char usage[]="Usage: NonoConfGen.exe [options]\n"
							                 "Options:\n"
							                 "\t-g select generator type (default is \e[3mnone\e[0m). Possiable values:\n"
							                 "\t\t* \e[3mnone\e[0m No generator is used. Removes effects of any previous -g.\n"
							                 "\t\t* \e[3mNextto-SSC-Gen\e[0m Generates sizecoeffx x sizecoeffx nonogram with\n\t\t"" "" ""descriptions of one block of length one.\n"
							                 "\t\t* \e[3mOne-Black-Colourable-One-Pixel-SSC-Gen\e[0m Generate sizecoeffx x\n\t\t"" "" "" ""sizecoeffx sizes One Black Colourable One pixel SSC.\n"
							                 "\t\t* \e[3mVLConcatenate\e[0m Concatenate random nonograms to vertical line.\n"
							                 //"\t\t* \e[3mRConcatenate\e[0m Concatenate other generators to one Nonogram \n\t\t"" "" "" ""somewhat random manner. Use sizecoeffy to control number of\n\t\t"" "" "" ""generators are concatenated. (subject to change to be more\n\t\t"" "" "" ""controllable).\n"
							                 "\t-n -m options effect size of the outputted nonogram. Option -n effects\n\t"" "" "" "" "" "" "" ""sizecoeffx and option effects sizecoeffy. Not all generators\n\t"" "" "" "" "" "" "" ""use sizecoeffy. Please read generator's description know effect\n\t"" "" "" "" "" "" "" ""sizecoeffx and sizecoeffy have.\n"
							                 "\t-h display this information.\n";
						(void)write(STDOUT_FILENO,usage,sizeof(usage)-1);
					}
					return 0;
				case 'n':
					sizecoeffx=atoi(optarg);
					if(sizecoeffx==0){
						(void)write(STDERR_FILENO,"ERROR: sizecoeffx cannot be zero!\n",34);
						return 2;
					}
					break;
				case 'm':
					sizecoeffy=atoi(optarg);
					if(sizecoeffy==0){
						(void)write(STDERR_FILENO,"ERROR: sizecoeffy cannot be zero!\n",34);
						return 2;
					}
					break;
				case '?':
					(void)write(STDERR_FILENO,"ERROR: unrecognized option!\n",28);
					return 1;
			}
		}
	}

	// Make a solution table which description are generated from.
	Nonogram nono;
	// A solution or a partial solution which description are generated from.
	//TODO: use block's range pointers in generation from partial solution 
	//TODO: linked list next member is not used. Used it in additive generator to segment the solution?
	Table solution;
	
	// Make error handling by long jump.
	{
		int errorcode;
		if((errorcode=setjmp(ErrorJmpBuffer))>0){
			(void)write(STDERR_FILENO,"ERROR: Generator failed!\n",25);
			return errorcode;
		}
	}
	
	if(confgen) confgen(&nono,&solution,sizecoeffx,sizecoeffy,NULL);
	else{
		//TODO... Nonogram and solution has to be somehow allocated...
		(void)write(STDERR_FILENO,"ERROR: Without generator implementation not done!\nPlease use -g option.\n",72);
		return 200;
	}
	
	if(nono.width==0 || nono.height==0){
		(void)write(STDERR_FILENO,"Zero by zero nonogram is not interesting!\n",42);
		return 0;
	}
	
	// Program's return value after this point
	int returnval=0;
	
	nono.rowsdesc=malloc(sizeof(Description)*nono.height);
	if(nono.rowsdesc){
		nono.colsdesc=malloc(sizeof(Description)*nono.width);
		if(nono.colsdesc){

			// Generate descriptions from the solution or partial solution.
			if(!nono.flags.haspartial){
				// Generate from a solution

				// Read the solution to get lengts of column descriptions.
				// Also count the total number of blocks.
				uint32_t totalnumberofblocks=0;
				for(int32_t col=nono.width-1;col>=0;col--){
					// Flag for checking is previous pixel black or not.
					uint8_t flag=0;
					// Inialize count to zero.
					nono.colsdesc[col].length=0;
					for(int32_t row=nono.height-1;row>=0;row--){
						enum Pixel pixel=getTablePixel (&nono,&solution,row,col);
						if(pixel==BLACK_PIXEL){
							flag=1;
						}
						else if(flag==1){
							flag=0;
							nono.colsdesc[col].length++;
						}
					}
					if(flag==1) nono.colsdesc[col].length++;
					// Increase total block count.
					totalnumberofblocks+=nono.colsdesc[col].length;
				}
				
				// Read the solution to get column descriptions.
				for(int32_t col=0;col<nono.width;col++){
					
					// Allocate space for the description numbers.
					if(nono.colsdesc[col].length==0){
						nono.colsdesc[col].length=1;
						// Total number of blocks was not incremented yet.
						totalnumberofblocks+=1;
						nono.colsdesc[col].lengths=malloc(sizeof(int32_t));
						if(!nono.colsdesc[col].lengths){
							goto ERROR_COL_DESCRIPTION;
						}
						nono.colsdesc[col].lengths[0]=0;
					}
					else{
						nono.colsdesc[col].lengths=malloc(sizeof(int32_t)*nono.colsdesc[col].length);
						if(!nono.colsdesc[col].lengths){
							ERROR_COL_DESCRIPTION:
							// Allocation failed so free what was allocated.
							col++;
							while(col<nono.width){
								free(nono.colsdesc[col].lengths);
								col++;
								goto ERROR_DESCRIPTION_COLUMN_ALLOC;
							}
						}

						// Go throught pixels of the line.
						int32_t blockindex=0;
						int32_t blocklength=0;
						for(int32_t row=0;row<nono.height;row++){
							enum Pixel pixel=getTablePixel (&nono,&solution,row,col);
							if(pixel==BLACK_PIXEL) blocklength++;
							else{
								if(blocklength>0){
									nono.colsdesc[col].lengths[blockindex]=blocklength;
									blocklength=0;
									blockindex++;
								}
							}
						}
						// If we end on black pixel then this check sets up
						// the last block.
						if(blocklength>0) nono.colsdesc[col].lengths[blockindex]=blocklength;
					}
				}
				
				// Read the solution to get row descriptions.
				// Increment variable totalnumberofblocks.
				for(int32_t row=nono.height-1;row>=0;row--){
				
					// Flag for checking is previous pixel black or not.
					int8_t flag=0;
					// initialize the block count to zero.
					nono.rowsdesc[row].length=0;
				
					// Count the block length.
					for(int32_t col=0;col<nono.width;col++){
						enum Pixel pixel=getTablePixel (&nono,&solution,row,col);
						if(pixel==BLACK_PIXEL){
							flag=1;
						}
						else if(flag==1){
							flag=0;
							nono.rowsdesc[row].length++;
						}
					}
					if(flag==1) nono.rowsdesc[row].length++;
					
					// Increase total block count.
					totalnumberofblocks+=nono.rowsdesc[row].length;
				}
				
				// Read the solution to get column descptions.
				for(int32_t row=0;row<nono.height;row++){
					
					// Allocate space for the description numbers.
					// Check that there is a block.
					if(nono.rowsdesc[row].length==0){
						nono.rowsdesc[row].length=1;
						// Total number of blocks was not incremented yet.
						totalnumberofblocks+=1;
						nono.rowsdesc[row].lengths=malloc(sizeof(int32_t));
						if(!nono.rowsdesc[row].lengths){
							goto ERROR_ROW_DESCRIPTION;
						}
						nono.rowsdesc[row].lengths[0]=0;
					}
					else{
						nono.rowsdesc[row].lengths=malloc(sizeof(int32_t)*nono.rowsdesc[row].length);
						if(!nono.rowsdesc[row].lengths){
							ERROR_ROW_DESCRIPTION:
							// Allocation failed so free what was allocated.
							// Move to previous row as that is first one to be allocated.
							row++;
							while(row<nono.height){
								free(nono.rowsdesc[row].lengths);
								row++;
							}
							goto ERROR_DESCRIPTION_ROW_ALLOC;
						}
					
						// Go throught pixels of the line.
						int32_t blockindex=0;
						int32_t blocklength=0;
						for(int32_t col=0;col<nono.width;col++){
							enum Pixel pixel=getTablePixel (&nono,&solution,row,col);
							if(pixel==BLACK_PIXEL) blocklength++;
							else{
								if(blocklength>0){
									nono.rowsdesc[row].lengths[blockindex]=blocklength;
									blocklength=0;
									blockindex++;
								}
							}
						}
						if(blocklength>0) nono.rowsdesc[row].lengths[blockindex]=blocklength;
					}
				}
				
				
				// Write the nonogram configuration.
				// Start with a comment header.
				// There has to be iovec for:
				//  - starting comment +1
				//  - width and height of nonogram with newline between after two newlines after +4
				//  - every description's block with space between and newline ending +2*number of blocks
				//     - End of row descriptions has extra newline seperating rows from columns.
				//  - Every row and column description line in the file starts with number of blocks +nono.width+nono.height
				struct iovec *segments=malloc(sizeof(struct iovec)*(nono.height+nono.width+1+4+2*(totalnumberofblocks)+1));
				if(segments){
					// Constant.
					const char comment_header[]={'#','T','h','i','s',' ','n','o','n','o','g','r','a','m',' ','c','o','n','f','i','g','u','r','a','t','i','o','n',' ','f','i','l','e',' ','w','a','s',' ','c','r','e','a','t','e','d',' ','b','y',' ','N','o','n','o','C','o','n','f','G','e','n','.','e','x','e','\n'};
					const char newline_newline[]={'\n','\n'};
					const char space[]={' '};
					
					// For easier memory manegement (mostly freeing).
					//TODO: this isn't really needed. Update later only use iov_base.
					//      This update can be done by removing space and newline having there
					//      own iovec. This consumes less memory and same amount of CPU time.
					//      less memory comes from having smaller iovec array and CPU time
					//      comes from fact iovec to space or newline is not set.
					uint8_t **intstrings=malloc((nono.height+nono.width+totalnumberofblocks+2)*sizeof(uint8_t*));
					if(intstrings){
						int32_t intstringsindex=-1;

						// starting comment.
						segments[0].iov_base=(char*)comment_header;
						segments[0].iov_len=sizeof(comment_header);

						// Width and Height of nonogram.
						{
							intstringsindex++;
							segments[1].iov_len=i32toalen(nono.width);
							intstrings[intstringsindex]=malloc(segments[1].iov_len);
							if(!intstrings[intstringsindex]){
								intstringsindex--;
								(void)write(STDERR_FILENO,"ERROR: Allocation of integer string for width failed!\n",45);
								returnval=202;
								goto FREE_SEGMENTS;
							}
							i32toa(nono.width,intstrings[intstringsindex],segments[1].iov_len);
							segments[1].iov_base=intstrings[intstringsindex];
						}
						segments[2].iov_base=(char*)newline_newline;
						segments[2].iov_len=1;
						{
							intstringsindex++;
							segments[3].iov_len=i32toalen(nono.height);
							intstrings[intstringsindex]=malloc(segments[3].iov_len);
							if(!intstrings[intstringsindex]){
								intstringsindex--;
								(void)write(STDERR_FILENO,"ERROR: Allocation of integer string for height failed!\n",45);
								returnval=203;
								goto FREE_SEGMENTS;
							}
							i32toa(nono.height,intstrings[intstringsindex],segments[3].iov_len);
							segments[3].iov_base=intstrings[intstringsindex];
						}
						segments[4].iov_base=(char*)newline_newline;
						segments[4].iov_len=2;

						// Since following indexes in segments depend on number
						// of blocks for each row or column description, indexing
						// is done vie variable segmentsindex.
						// Starting index 4 is because indexes 0 to 4 were used above.
						int32_t segmentsindex=4;

						// Write column descriptions
						for(int32_t col=0;col<nono.width;col++){
							int32_t k;
							// Write number of blocks in the row.
							segmentsindex++;
							segments[segmentsindex].iov_len=i32toalen(nono.colsdesc[col].length);
							intstringsindex++;
							intstrings[intstringsindex]=malloc(segments[segmentsindex].iov_len+1);
							if(!intstrings[intstringsindex]){
								intstringsindex--;
								returnval=204;
								goto FREE_SEGMENTS;
							}
							// Setup the segment
							i32toa(nono.colsdesc[col].length,intstrings[intstringsindex],segments[segmentsindex].iov_len);
							segments[segmentsindex].iov_base=intstrings[intstringsindex];
							// Add the space.
							intstrings[intstringsindex][segments[segmentsindex].iov_len]=' ';
							segments[segmentsindex].iov_len+=1;

							for(k=0;k<nono.colsdesc[col].length;k++){

								// block's character length
								segmentsindex++;
								segments[segmentsindex].iov_len=i32toalen(nono.colsdesc[col].lengths[k]);

								// Allocate memory for the string.
								intstringsindex++;
								intstrings[intstringsindex]=malloc(segments[segmentsindex].iov_len);
								if(!intstrings[intstringsindex]){
									intstringsindex--;
									returnval=204;
									goto FREE_SEGMENTS;
								}

								// Setup the segment.
								i32toa(nono.colsdesc[col].lengths[k],intstrings[intstringsindex],segments[segmentsindex].iov_len);
								segments[segmentsindex].iov_base=intstrings[intstringsindex];

								// After a bloc's length is written ether space or newline is written.
								// Choice depends on is next block in same description or not.
								// Hence, check was the block written last block of description is made.
								// Under newline there is possibility that newline newline is written.
								// This depends on is the last row's description to be written or not.
								//TODO: move to outside of inner loop.
								segmentsindex++;
								if(k<nono.colsdesc[col].length-1){
									segments[segmentsindex].iov_base=(char*)space;
									segments[segmentsindex].iov_len=1;
								}
								else{
									segments[segmentsindex].iov_base=(char*)newline_newline;
									if(col<nono.width-1){
										segments[segmentsindex].iov_len=1;
									}
									else{
										segments[segmentsindex].iov_len=2;
									}
								}
							}
						}

						// Write Row descriptions
						for(int32_t row=0;row<nono.height;row++){

							// Write number of blocks in the col.
							segmentsindex++;
							segments[segmentsindex].iov_len=i32toalen(nono.rowsdesc[row].length);
							intstringsindex++;
							intstrings[intstringsindex]=malloc(segments[segmentsindex].iov_len+1);
							if(!intstrings[intstringsindex]){
								intstringsindex--;
								returnval=204;
								goto FREE_SEGMENTS;
							}
							// Setup the segment
							i32toa(nono.rowsdesc[row].length,intstrings[intstringsindex],segments[segmentsindex].iov_len);
							segments[segmentsindex].iov_base=intstrings[intstringsindex];
							// Add the space
							intstrings[intstringsindex][segments[segmentsindex].iov_len]=' ';
							segments[segmentsindex].iov_len+=1;

							int32_t k;
							for(k=0;k<nono.rowsdesc[row].length;k++){

								// Block's character length
								segmentsindex++;
								segments[segmentsindex].iov_len=i32toalen(nono.rowsdesc[row].lengths[k]);

								// Allocate memory for the string.
								intstringsindex++;
								intstrings[intstringsindex]=malloc(segments[segmentsindex].iov_len);
								if(!intstrings[intstringsindex]){
									intstringsindex--;
									returnval=204;
									goto FREE_SEGMENTS;
								}

								// Setup the segment
								i32toa(nono.rowsdesc[row].lengths[k],intstrings[intstringsindex],segments[segmentsindex].iov_len);
								segments[segmentsindex].iov_base=intstrings[intstringsindex];

								// After a bloc's length is written ether space or newline is written.
								// Choice depends on is next block in same description or not.
								// Hence, check was the block written last block of description is made.
								// File's ending needs a newline.
								segmentsindex++;
								if(k<nono.rowsdesc[row].length-1){
									segments[segmentsindex].iov_base=(char*)space;
									segments[segmentsindex].iov_len=1;
								}
								else{
									segments[segmentsindex].iov_base=(char*)newline_newline;
									segments[segmentsindex].iov_len=1;
								}
							}
						}

						segmentsindex+=1;
						{
							// Write iov_vecs to IO. There maybe is too many
							// iov_vecs for one go so loop throught them.
							//TODO __IOV_MAX maybe -1 to indicate no limit?
							int32_t i=0;
							while(i<=segmentsindex-__IOV_MAX){
								writev(STDOUT_FILENO,segments+i,__IOV_MAX);
								i+=__IOV_MAX;
							}
							writev(STDOUT_FILENO,segments+i,segmentsindex-i);
						}

						#if defined(_DEBUG_) && defined(_DEBUG_SVG_OUT_)
							int fd=open("/tmp/NonoConfDebug.html",O_CREAT | O_TRUNC | O_WRONLY,S_IRUSR | S_IWUSR);
							if(fd==-1){
								returnval=209;
							}
							NonoHTML nonoHTML;
							if(nonogramGenSvgStart(fd,&nono,&nonoHTML)){
								returnval=210;
							}
							if(nonogramWriteSvg(fd,&nonoHTML,&nono,&solution,NONOGRAM_WRITE_SVG_FLAG_EMPTY)){
								returnval=211;
							}
							nonogramFreeSvg(&nonoHTML);
						#endif

						FREE_SEGMENTS:
						for(int32_t i=intstringsindex;i>=0;i--) free(intstrings[i]);
						free(intstrings);
					}
					free(segments);
				}
				else{
					(void)write(STDERR_FILENO,"ERROR: Allocation of iov_vecs failed!\n",39);
					returnval=201;
				}
				
				// Free descriptions.
				for(int32_t row=nono.height-1;row>=0;row--) free(nono.rowsdesc[row].lengths);
				ERROR_DESCRIPTION_ROW_ALLOC:
				for(int32_t col=nono.width-1;col>=0;col--) free(nono.colsdesc[col].lengths);
				ERROR_DESCRIPTION_COLUMN_ALLOC:;
			}
			else{
				// Generate from a partial solution.
				(void)write(STDERR_FILENO,"ERROR: Nonogram generation from partial solution not implemented!\n",66);
				returnval=200;
			}
			free(nono.colsdesc);
		}
		else{
			(void)write(STDERR_FILENO,"ERROR: Column description allocation error!\n",44);
			returnval=11;
		}
		free(nono.rowsdesc);
	}
	else{
		(void)write(STDERR_FILENO,"ERROR: Row description allocation error!\n",41);
		returnval=10;
	}
	
	// Free pixel matrix allocation for the solution/partial solution
	free(solution.pixels);

	return returnval;
}
