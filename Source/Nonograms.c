#include<stdint.h>
#include<unistd.h>
#include<sys/uio.h>
#include<sys/stat.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<stdio.h>
#include"Etc.h"
#include"Nonograms.h"

#define MAX(A,B)\
 ({ __typeof__ (A) _a = (A); \
    __typeof__ (B) _b = (B); \
    _a > _b ? _a : _b; })

enum Pixel *getLinePixel(enum Pixel *lines,int32_t nth,uint32_t stride){
	return lines+nth*stride;
}
int8_t colourLinesPixel(enum Pixel *lines,int32_t nth,uint32_t stride,enum Pixel colour,int32_t *colored){
	enum Pixel *loc=getLinePixel(lines,nth,stride);
	if(*loc!=UNKNOWN_PIXEL && *loc!=colour) return 0;
	if(*loc==UNKNOWN_PIXEL){
		*loc=colour;
		(*colored)++;
	}
	return 1;
}
enum Pixel getPixel(Nonogram *nono,Table *table,int32_t row,int32_t col){
	return table->pixels[row*nono->width+col].e;
}
void colourPixel(Nonogram *nono,Table *table,int32_t row,int32_t col,enum Pixel colour){
	table->pixels[row*nono->width+col].e=colour;
}
int32_t biggestDescriptionSize(const Description *desc,int32_t length){
	int32_t max=desc[length-1].length;
	for(int32_t i=length-2;i>=0;i--){
		if(max<desc[i].length) max=desc[i].length;
	}
	return max;
}
int32_t descriptionsLength(const Description *desc){
	// NOTE: This work for zero description since there is
	//       one block of size zero hence answer is zero.
	int32_t length=desc->length-1;
	for(int32_t block=desc->length-1;block>=0;block--) length+=desc->lengths[block];
	return length;
}
void initBlocksRanges(BlocksRanges *ranges,Description *desc,int32_t length){

	if(desc->lengths[0]==0){
		ranges->blocksrangelefts[0]=-1;
		ranges->blocksrangerights[0]=-1;
		return;
	}

	// Initialize left side
	ranges->blocksrangelefts[0]=0;
	int32_t tempsum=desc->lengths[0]+1;
	for(int32_t block=1;block<desc->length;block++){
		ranges->blocksrangelefts[block]=tempsum;
		tempsum+=desc->lengths[block]+1;
	}

	// Initialize right side
	ranges->blocksrangerights[desc->length-1]=length-1;
	tempsum=length-1-(desc->lengths[desc->length-1]+1);
	for(int32_t block=desc->length-2;block>=0;block--){
		ranges->blocksrangerights[block]=tempsum;
		tempsum-=desc->lengths[block]+1;
	}

}
static inline uint32_t ascii2u32(ssize_t *bufferpoint,uint8_t *buffer){
	uint32_t value=0;
	while(buffer[*bufferpoint]!='\0' && isdigit(buffer[*bufferpoint])){
		value*=10;
		value+=buffer[*bufferpoint]-'0';
		(*bufferpoint)++;
	}
	return value;
}
static inline void jumpOverWhitespace(ssize_t *bufferpoint,uint8_t *buffer){
	while(buffer[*bufferpoint]!='\0' && isblank(buffer[*bufferpoint])) (*bufferpoint)++;
}
static inline void jumpOverWhitespceAndZeros(ssize_t *bufferpoint,uint8_t *buffer){
	while(buffer[*bufferpoint]!='\0' && (isblank(buffer[*bufferpoint]) || buffer[*bufferpoint]=='0')) (*bufferpoint)++;
}
static inline uint8_t checkNewline(ssize_t *bufferpoint,uint8_t *buffer){
	jumpOverWhitespace(bufferpoint,buffer);
	return (buffer[*bufferpoint]=='\n');
}
static inline void readAscii32Int(ssize_t *bufferpoint,uint8_t *buffer,uint32_t *var){
	jumpOverWhitespceAndZeros(bufferpoint,buffer);
	*var=ascii2u32(bufferpoint,buffer);
	checkNewline(bufferpoint,buffer);
	(*bufferpoint)++;
}
static inline int32_t readAscii2s32LengthGivenArray(ssize_t *bufferpoint,uint8_t *buffer,int32_t **arr){
	// Read first integer which is length of array.
	register int32_t lengthofarr=(int32_t)ascii2u32(bufferpoint,buffer);
	if(lengthofarr>0){
		*arr=malloc(lengthofarr*sizeof(int32_t));
		if(!*arr) return -1;

		// Store the array values.
		{
			int32_t i=0;
			while(!checkNewline(bufferpoint,buffer)){
				// Overflow protection.
				// Checked since user input is parsed by this function.
				if(i==lengthofarr){
					free(*arr);
					return -1;
				}
				(*arr)[i]=(int32_t)ascii2u32(bufferpoint,buffer);
				i++;
			}

			// Sanity check that every point was used.
			if(i<lengthofarr){
				free(*arr);
				return -1;
			}
		}

	}
	else{
		// If there is just newline this one catches it and make array of 1 set to zero.
		*arr=malloc(sizeof(int32_t));
		if(!*arr) return -1;

		(*arr)[0]=0;
		lengthofarr=1;
	}
	// Move over the newline.
	(*bufferpoint)++;
	return lengthofarr;
}
struct NonogramLoadConfResult nonogramLoadConf(int file,Nonogram *nono){
	//TODO: DOES NOT NOTICE IF NOT ENOUGH ROWS HAVE BEEN GIVEN!
	struct NonogramLoadConfResult result={0,1};

	// Read configuration file as whole.
	struct stat fileinfo;
	if(fstat(file,&fileinfo)==0){
		// Check file is regular.
		// TODO: Does this prevent over internet?
		if(S_ISREG(fileinfo.st_mode)){
			uint8_t *buffer=malloc(fileinfo.st_size+1);
			buffer[fileinfo.st_size]='\0';
			if(read(file,buffer,fileinfo.st_size)>0){

				ssize_t bufferpoint=0;

				// There maybe is starting comment. Skip these lines.
				// Comment start with #.
				while(buffer[bufferpoint]=='#'){
					while(buffer[bufferpoint]!='\n') bufferpoint++;
					result.line++;
					bufferpoint++;
				}

				// Configuration file has first two lines as width and height of nonogram.
				readAscii32Int(&bufferpoint,buffer,(uint32_t*)&nono->width);
				result.line++;
				readAscii32Int(&bufferpoint,buffer,(uint32_t*)&nono->height);
				result.line++;

				// Empty line for indication.
				if(checkNewline(&bufferpoint,buffer) && nono->width>0 && nono->height>0){
					result.line++;
					bufferpoint++;

					// Allocate descriptions
					nono->rowsdesc=malloc(sizeof(Description)*nono->height);
					nono->colsdesc=malloc(sizeof(Description)*nono->width);
					if(nono->rowsdesc && nono->colsdesc){

						// Column descriptions.
						for(uint32_t i=0;i<nono->width;i++){
							nono->colsdesc[i].length=readAscii2s32LengthGivenArray(&bufferpoint,buffer,&(nono->colsdesc[i].lengths));

							// TODO: If this error occures how to free allocated memory?
							if(nono->colsdesc[i].length==-1){
								result.id=6;
								return result;
							}

							// Everything is fine move to next line.
							result.line++;
						}

						// Empty line for indication.
						if(checkNewline(&bufferpoint,buffer)){
							result.line++;
							bufferpoint++;

							// Row descriptions.
							for(uint32_t i=0;i<nono->height;i++){
								nono->rowsdesc[i].length=readAscii2s32LengthGivenArray(&bufferpoint,buffer,&(nono->rowsdesc[i].lengths));

								if(nono->rowsdesc[i].length==-1){
									result.id=7;
									return result;
								}

								// Everything is fine move to next line.
								result.line++;
							}

							// Empty line for stop indication.
							checkNewline(&bufferpoint,buffer);
							result.line++;
							bufferpoint++;

							// Rest of the initialization.
							nono->flags.shadow=0;
						}else{
							result.id=5;
							return result;
						}
					}else{
						result.id=4;
						return result;
					}
				}else{
					result.id=3;
					return result;
				}
			}else{
				result.id=2;
				return result;
			}
			// Free the buffer
			free(buffer);
		}else{
			result.id=1;
			return result;
		}
	}else{
		result.id=1;
		return result;
	}
	return result;
}
static int32_t getMarkerLocationPerpendicular(int32_t pixelindex,int32_t pixelside){
	return pixelindex*pixelside+pixelside/4;
}
static int32_t getMarkerLocationParallel(int32_t pixelindex,int32_t pixelside){
	return pixelindex*pixelside+pixelside/2;
}
static int32_t getMarkerLocationPerpendicular2(int32_t pixelindex,int32_t pixelside){
	return pixelindex*pixelside+pixelside*3/4;
}
uint8_t nonogramGenSvgStart(const int file,const Nonogram * restrict nono,NonoHTML * restrict htmlpage){

	// Check that file is to actual file (ISREG), Pipe or FIFO special file (ISFIFO)
	// or terminal (isatty) so that we are outputting something senseable.
	// TODO: Add socket?
	// TODO: Reverse the question disinclude folder or symbolic link?
	struct stat fileinfo;
	if(!fstat(file,&fileinfo)){
		if(S_ISREG(fileinfo.st_mode) || S_ISFIFO(fileinfo.st_mode) || isatty(file)){

			// HTML template ------
			// For row descriptions one character is ≈9px and &nbsp;≈4.5px.
			// Hence by having &nbsp;block&nbsp; means ≈18px per block if
			// number is one digit long.
			// TODO: What if every description is one block but one of the blocks is two digits long?
			//       How about three digits?
			//       At somepoint overfloaw would happen hence longest number is digits in a block
			//       should effect the length calculatation. Matter is shelved for now for lack of
			//       need vs complexity.
			// For approximate round to highest 5px*n.
			// Variable is read as "description abscissa offset".
			int32_t descabscissaoffset=biggestDescriptionSize(nono->rowsdesc,nono->height)*18;
			descabscissaoffset=descabscissaoffset+(5-(descabscissaoffset%5));
			int32_t descordinateoffset=biggestDescriptionSize(nono->colsdesc,nono->width)*17;
			// Divade by 5 then add 1 if remaider was bigger then 2 and times five back.
			// Should be same as rounding to nearest 5;
			descordinateoffset=(descordinateoffset/5+(descordinateoffset%5>2))*5;
			// Calculate length of the side by finding longest number (in digits) in column descriptions.
			// Smallest length is 20 which used if longest number is one digit long.
			int32_t pixelside;
			int32_t colnumberofdigits;
			{
				int32_t max=0;
				for(int32_t i=nono->width-1;i>=0;i--){
					for(int32_t j=nono->colsdesc[i].length-1;j>=0;j--){
						if(max<nono->colsdesc[i].lengths[j]) max=nono->colsdesc[i].lengths[j];
					}
				}
				colnumberofdigits=max/10+1;
				pixelside=20+(colnumberofdigits-1)*8;
			}
			// Length of the whole table in pixels.
			int32_t pixelwidth=nono->width*pixelside+descabscissaoffset;
			int32_t pixelheight=nono->height*pixelside+descordinateoffset;
			// Plus so is to move indicator to lower corner. Check function description.
			int32_t descordinateoffsetextra=descordinateoffset+0;
			// Plus to move row descriptions down enought.
			int32_t descordinateoffsetextra2=descordinateoffset+15+(colnumberofdigits-1)*4;


			// Write HTML header.
			// This needs to be written only once.
			uint8_t head1[]={'<','!','D','O','C','T','Y','P','E',' ','h','t','m','l','>','\n'
			                ,'<','m','e','t','a',' ','c','h','a','r','s','e','t','=','U','T','F','-','8','>','\n'
			                ,'<','t','i','t','l','e','>','N','o','n','o','g','r','a','m','<','/','t','i','t','l','e','>','\n'
			                ,'<','s','v','g',' ','w','i','d','t','h','=','"','0','"',' ','h','e','i','g','h','t','=','"','0','"','>','\n'
			                ,'<','d','e','f','s','>','\n'
			                ,'<','g',' ','i','d','=','\"','U','\"',' ','t','r','a','n','s','f','o','r','m','=','\"','t','r','a','n','s','l','a','t','e','(','0',' '
			                };
			// pixelside/2
			uint8_t head1_2[]={')','"','>','\n'
			                     ,'<','r','e','c','t',' ','x','=','\"','0','\"',' ','y','=','\"','0','\"',' ','w','i','d','t','h','=','\"'
			                     };
			// pixelside/2;
			uint8_t head2[]={'\"',' ','h','e','i','g','h','t','=','\"'};
			// pixelside/2;
			uint8_t head3[]={'\"',' ','s','t','y','l','e','=','\"','f','i','l','l',':','#','e','c','e','c','e','c',';','\"','/','>','\n'
			                 ,'<','t','e','x','t',' ','x','=','\"','0','\"',' ','y','=','\"','0','\"',' ','w','i','d','t','h','=','\"'
			                 };
			// pixelside/2;
			uint8_t head3_2[]={'\"',' ','h','e','i','g','h','t','=','\"'};
			// pixelside/2;
			uint8_t head3_3[]={'\"',' ','f','i','l','l','=','"','b','l','a','c','k','"',' ','s','t','y','l','e','=','"','f','o','n','t','-','s','t','y','l','e',':','n','o','r','m','a','l',';','f','o','n','t','-','w','e','i','g','h','t',':','n','o','r','m','a','l',';','f','o','n','t','-','s','i','z','e',':'};
			// colnumberofdigits*2;
			uint8_t head4[]={'p','x',';','l','i','n','e','-','h','e','i','g','h','t',':','1','.','5',';','f','o','n','t','-','f','a','m','i','l','y',':','s','a','n','s','-','s','e','r','i','f',';','"',' ','t','e','x','t','-','a','n','c','h','o','r','=','"','m','i','d','d','l','e','"',' ','t','r','a','n','s','f','o','r','m','=','\"','t','r','a','n','s','l','a','t','e','('};
			// pixelside/4;
			uint8_t head4_1[]={' '};
			// pixelside/4+3;
			uint8_t head4_2[]={')','\"','>','?','<','/','t','e','x','t','>','\n'
			                  ,'<','/','g','>','\n'
			                  ,'<','r','e','c','t',' ','y','=','"'
			                  };
			// pixelside/2
			uint8_t head4_3[]={'"',' ','i','d','=','"','W','"',' ','w','i','d','t','h','=','"'};
			// pixelside/2;
			uint8_t head5[]={'"',' ','h','e','i','g','h','t','=','"'};
			// pixelside/2;
			uint8_t head6[]={'"',' ','s','t','y','l','e','=','"','f','i','l','l',':','n','o','n','e',';','s','t','r','o','k','e',':','#','0','0','0','0','0','0',';','s','t','r','o','k','e','-','w','i','d','t','h',':','0','.','7','5',';','"','/','>','\n'
			                     ,'<','r','e','c','t',' ','i','d','=','"','1','"',' ','w','i','d','t','h','=','\"'
			                     };
			// pixelside/2;
			uint8_t head7[]={'\"',' ','h','e','i','g','h','t','=','\"'};
			// pixelside/2;
			uint8_t head8[]={'"',' ','s','t','y','l','e','=','"','f','i','l','l',':','#','0','0','0','0','0','0',';','"',' ','y','=','\"'};
			// pixelside/2;
			uint8_t head8_1[]={'\"','/','>','\n'
			                  ,'<','m','a','r','k','e','r',' ','i','d','=','"','H','o','o','k','E','n','d','"',' ','s','t','y','l','e','=','"','o','v','e','r','f','l','o','w',':','v','i','s','i','b','l','e','"',' ','o','r','i','e','n','t','=','"','a','u','t','o','-','s','t','a','r','t','-','r','e','v','e','r','s','e','"','>','\n'
			                  ,'<','p','a','t','h',' ','s','t','y','l','e','=','"','f','i','l','l',':','n','o','n','e',';','f','i','l','l','-','r','u','l','e',':','e','v','e','n','o','d','d',';','s','t','r','o','k','e',':','#','0','0','0','0','0','0',';','s','t','r','o','k','e','-','w','i','d','t','h',':','0','.','5','p','t',';','s','t','r','o','k','e','-','o','p','a','c','i','t','y',':','1','"',' ','d','=','"','m',' ','-','2','.','5',',','2','.','5',' ','c',' ','1','.','5',',','0',' ','2','.','5',',','-','1',' ','2','.','5',',','-','2','.','5',' ','0',',','-','1','.','5',' ','-','1',',','-','2','.','5',' ','-','2','.','5',',','-','2','.','5','"','/','>','\n'
			                  ,'<','/','m','a','r','k','e','r','>','\n'
			                  ,'<','/','d','e','f','s','>','\n'
			                  ,'<','/','s','v','g','>','\n'
			                  };

			struct iovec ioparts[]={{head1,sizeof(head1)}
			                       ,{0,0} // 1
			                       ,{head1_2,sizeof(head1_2)}
			                       ,{0,0} // 3
			                       ,{head2,sizeof(head2)}
			                       ,{0,0} // 5
			                       ,{head3,sizeof(head3)}
			                       ,{0,0} // 7
			                       ,{head3_2,sizeof(head3_2)}
			                       ,{0,0} // 9
			                       ,{head3_3,sizeof(head3_3)}
			                       ,{0,0} // 11 FONT
			                       ,{head4,sizeof(head4)}
			                       ,{0,0} // 13 Quater
			                       ,{head4_1,sizeof(head4_1)}
			                       ,{0,0} // 15 Quater+
			                       ,{head4_2,sizeof(head4_2)}
			                       ,{0,0} // 17
			                       ,{head4_3,sizeof(head4_3)}
			                       ,{0,0} // 19
			                       ,{head5,sizeof(head5)}
			                       ,{0,0} // 21
			                       ,{head6,sizeof(head6)}
			                       ,{0,0} // 23
			                       ,{head7,sizeof(head7)}
			                       ,{0,0} // 25
			                       ,{head8,sizeof(head8)}
			                       ,{0,0} // 27
			                       ,{head8_1,sizeof(head8_1)}
			};

			int32_t pixelsidehalf=pixelside/2;
			int32_t pixelsidehalflen=i32toalen(pixelsidehalf);
			uint8_t *pixelsidebuff=malloc(pixelsidehalflen);
			i32toa(pixelsidehalf,pixelsidebuff,pixelsidehalflen);
			int32_t font=7+(colnumberofdigits-1)*2;
			int32_t fontlen=i32toalen(font);
			uint8_t *fontbuff=malloc(fontlen);
			i32toa(font,fontbuff,fontlen);
			int32_t pixelsidequater=pixelside/4;
			int32_t pixelsidequaterlen=i32toalen(pixelsidequater);
			uint8_t *pixelsidequaterbuff=malloc(pixelsidequaterlen);
			i32toa(pixelsidequater,pixelsidequaterbuff,pixelsidequaterlen);
			int32_t pixelsidequaterplus=pixelsidequater+3;
			int32_t pixelsidequaterpluslen=i32toalen(pixelsidequaterplus);
			uint8_t *pixelsidequaterplusbuff=malloc(pixelsidequaterpluslen);
			i32toa(pixelsidequaterplus,pixelsidequaterplusbuff,pixelsidequaterpluslen);
			ioparts[1].iov_base=pixelsidebuff;
			ioparts[1].iov_len=pixelsidehalflen;
			ioparts[3].iov_base=pixelsidebuff;
			ioparts[3].iov_len=pixelsidehalflen;
			ioparts[5].iov_base=pixelsidebuff;
			ioparts[5].iov_len=pixelsidehalflen;
			ioparts[7].iov_base=pixelsidebuff;
			ioparts[7].iov_len=pixelsidehalflen;
			ioparts[9].iov_base=pixelsidebuff;
			ioparts[9].iov_len=pixelsidehalflen;
			ioparts[11].iov_base=fontbuff;
			ioparts[11].iov_len=fontlen;
			ioparts[13].iov_base=pixelsidequaterbuff;
			ioparts[13].iov_len=pixelsidequaterlen;
			ioparts[15].iov_base=pixelsidequaterplusbuff;
			ioparts[15].iov_len=pixelsidequaterpluslen;
			ioparts[17].iov_base=pixelsidebuff;
			ioparts[17].iov_len=pixelsidehalflen;
			ioparts[19].iov_base=pixelsidebuff;
			ioparts[19].iov_len=pixelsidehalflen;
			ioparts[21].iov_base=pixelsidebuff;
			ioparts[21].iov_len=pixelsidehalflen;
			ioparts[23].iov_base=pixelsidebuff;
			ioparts[23].iov_len=pixelsidehalflen;
			ioparts[25].iov_base=pixelsidebuff;
			ioparts[25].iov_len=pixelsidehalflen;
			ioparts[27].iov_base=pixelsidebuff;
			ioparts[27].iov_len=pixelsidehalflen;

			const ssize_t result=writev(file,ioparts,sizeof(ioparts)/sizeof(struct iovec));
			if(result>(int32_t)sizeof(head1)+sizeof(head2)+sizeof(head3)+sizeof(head3_2)+sizeof(head3_3)+sizeof(head4)+sizeof(head5)+sizeof(head6)+sizeof(head7)+sizeof(head8)){

				// Free just allocated buffers.
				free(fontbuff);
				free(pixelsidebuff);
				free(pixelsidequaterbuff);
				free(pixelsidequaterplusbuff);

				// SVG head
				// First give constant characters in templates.
				// Order here is writing order! That is why i32toalen
				// functions are middle indicating the variable write.
				const uint8_t svg_template1[]={'<','s','v','g',' ','w','i','d','t','h','=','"'};
				const uint32_t pixelwidth10len=i32toalen(pixelwidth+10);
				const uint8_t svg_template2[]={'"',' ','h','e','i','g','h','t','=','"'};
				const uint32_t pixelheight10len=i32toalen(pixelheight+10);
				const uint8_t svg_template3[]={'"','>','\n'
				              ,'<','g',' ','t','r','a','n','s','f','o','r','m','=','"','t','r','a','n','s','l','a','t','e','(','5',' ','5',')','"','>','\n'
				              ,'<','r','e','c','t',' ','x','=','"','0','"',' ','y','=','"','0','"',' ','w','i','d','t','h','=','"'
				              };
				const uint32_t pixelwidthlen=i32toalen(pixelwidth);
				const uint8_t svg_template4[]={'"',' ','h','e','i','g','h','t','=','"'};
				const uint32_t pixelheightlen=i32toalen(pixelheight);
				const uint8_t svg_template5[]={'"',' ','s','t','y','l','e','=','"','f','i','l','l',':','#','d','f','d','f','e','b',';','"','/','>','\n'
				                              ,'<','r','e','c','t',' ','x','=','"'};
				const uint32_t descabscissaoffsetlen=i32toalen(descabscissaoffset);
				const uint8_t svg_template6[]={'"',' ','y','=','"'};
				const uint32_t descordinateoffsetlen=i32toalen(descordinateoffset);
				const uint8_t svg_template7[]={'"',' ','w','i','d','t','h','=','"'};
				const uint32_t pixelwidth_descabscissaoffsetlen=i32toalen(pixelwidth-descabscissaoffset);
				const uint8_t svg_template8[]={'"',' ','h','e','i','g','h','t','=','"'};
				const uint32_t pixelheight_descordinateoffsetlen=i32toalen(pixelheight-descordinateoffset);
				const uint8_t svg_template9[]={'"',' ','s','t','y','l','e','=','"','f','i','l','l',':','#','f','f','f','f','f','f',';','"','/','>','\n'
				                              ,'<','g',' ','t','r','a','n','s','f','o','r','m','=','"','t','r','a','n','s','l','a','t','e','(',};
				//descabscissaoffsetlen; Again!
				const uint8_t svg_template10[]={' '};
				const uint32_t descordinateoffsetextralen=i32toalen(descordinateoffsetextra);
				const uint8_t svg_template11[]={')','"','>','\n'};

				// For each pixel add these.
				const uint8_t svg_template12[]={'<','u','s','e',' ','x','=','"'};
				uint32_t pixelcoordlens[(nono->width>nono->height?nono->width:nono->height)];
				{
					uint32_t maxside=nono->width-1>nono->height-1?nono->width-1:nono->height-1;
					// TODO: This could be done procedurally.
					for(int32_t p=maxside;p>=0;p--){
						pixelcoordlens[p]=i32toalen(p*pixelside);
					}
				}
				const uint8_t svg_template13[]={'"',' ','y','=','"'};
				// pixellens; Again!
				const uint8_t svg_template14[]={'"',' ','h','r','e','f','=','"','#','U','"','/','>'};
				const uint8_t svg_template15[]={'\n','<','/','g','>','\n'
				                               ,'<','g',' ','s','t','y','l','e','=','"','s','t','r','o','k','e',':','#','6','6','6','6','6','6',';','s','t','r','o','k','e','-','w','i','d','t','h',':','1',';','"','>','\n'
				                               ,'<','r','e','c','t',' ','x','=','"','0','"',' ','y','=','"','0','"',' ','w','i','d','t','h','=','"'};
				// pixelwidthlen; Again!
				const uint8_t svg_template16[]={'"',' ','h','e','i','g','h','t','=','"'};
				// pixelheightlen; Again!
				const uint8_t svg_template17[]={'"',' ','s','t','y','l','e','=','"','f','i','l','l',':','n','o','n','e','"','/','>','\n'
				                               ,'<','g',' ','t','r','a','n','s','f','o','r','m','=','"','t','r','a','n','s','l','a','t','e','('
				                               };
				// descabscissaoffsetlen; Again!
				const uint8_t svg_template18[]={' ','0',')','"','>','\n'};

				// Make column lines.
				const uint8_t svg_template19[]={'<','l','i','n','e',' ','x','1','=','"'};
				// pixelcoordlens[col]; Again!
				const uint8_t svg_template20[]={'"',' ','y','1','=','"','0','"',' ','x','2','=','"'};
				const uint8_t svg_template21[]={'"',' ','y','2','=','"'};
				// pixelheightlen; Again!
				const uint8_t svg_template22[]={'"','/','>','\n'};

				const uint8_t svg_template23[]={'<','/','g','>','\n'
				                               ,'<','g',' ','t','r','a','n','s','f','o','r','m','=','"','t','r','a','n','s','l','a','t','e','(','0',' '
				                               };
				// descabscissaoffsetlen; Again!
				const uint8_t svg_template24[]={')','"','>','\n'};

				// Make row lines.
				const uint8_t svg_template25[]={'<','l','i','n','e',' ','x','1','=','"','0','"',' ','y','1','=','"'};
				// pixelcoordlens; Again!
				const uint8_t svg_template26[]={'"',' ','x','2','=','"'};
				// pixelwidthlen! Again!
				const uint8_t svg_template27[]={'"',' ','y','2','=','"'};
				// pixelcoordlens; Again!
				const uint8_t svg_template28[]={'"','/','>','\n'};

				const uint8_t svg_template29[]={'<','/','g','>','\n'
				                               ,'<','/','g','>','\n'
				                               ,'<','g',' ','s','t','y','l','e','=','"','f','o','n','t','-','s','t','y','l','e',':','n','o','r','m','a','l',';','f','o','n','t','-','w','e','i','g','h','t',':','n','o','r','m','a','l',';','f','o','n','t','-','s','i','z','e',':','1','4','p','x',';','l','i','n','e','-','h','e','i','g','h','t',':','1','.','5',';','f','o','n','t','-','f','a','m','i','l','y',':','s','a','n','s','-','s','e','r','i','f',';','"',' ','f','i','l','l','=','"','b','l','a','c','k','"','>'
				                               ,'<','g',' ','t','r','a','n','s','f','o','r','m','=','"','t','r','a','n','s','l','a','t','e','(',
				                               };
				const uint32_t descabscissaoffset5len=i32toalen(descabscissaoffset+5);
				const uint8_t svg_template30[]={' ','0',')','"','>','\n'};

				// Make column descriptions
				const uint8_t svg_template31[]={'<','g',' ','t','r','a','n','s','f','o','r','m','=','"','t','r','a','n','s','l','a','t','e','('};
				// pixelcoordlens; Again
				const uint8_t svg_template32[]={' ','0',')','"','>','<','t','e','x','t',' ','x','=','"','0','"',' ','y','=','"','1','3','"','>','\n'};
				// Write for each block.
				uint32_t *nonocolsdesclen[nono->width];
				{
					for(int32_t col=nono->width-1;col>=0;col--){
						nonocolsdesclen[col]=malloc(sizeof(uint32_t)*nono->colsdesc[col].length);
						if(!nonocolsdesclen[col]){
							// Something went wrong so free allocated memory and exit with error.
							for(int32_t t=nono->width-1;t>col;t--) free(nonocolsdesclen[t]);
							return 5;
						}
						for(int32_t block=0;block<nono->colsdesc[col].length;block++){
							nonocolsdesclen[col][block]=i32toalen(nono->colsdesc[col].lengths[block]);
						}
					}
				}
				const uint8_t svg_template33[]={'<','t','s','p','a','n',' ','x','=','"','0','"',' ','d','y','=','"','1','7','"','>'};
				// nonocolsdesclen; Again!
				const uint8_t svg_template34[]={'<','/','t','s','p','a','n','>'};
				const uint8_t svg_template35[]={'\n'
				                               ,'<','/','t','e','x','t','>','<','/','g','>','\n'
				                               };
				const uint8_t svg_template36[]={'<','/','g','>','\n'
			                                 ,'<','g',' ','t','r','a','n','s','f','o','r','m','=','"','t','r','a','n','s','l','a','t','e','(','0',' '
				                               };
				int32_t descordinateoffsetextra2len=i32toalen(descordinateoffsetextra2);
				const uint8_t svg_template37[]={')','"','>','\n'};

				// Row descriptions
				const uint8_t svg_template38[]={'<','t','e','x','t',' ','x','=','"','2','"',' ','y','=','"'};
				// pixelcoordlens; Again!
				const uint8_t svg_template39[]={'"','>','\n'};
				// Write for each block loop
				const uint8_t svg_template40[]={'<','t','s','p','a','n',' ','d','x','=','"','9','"','>'};
				// svg_template34 again!
				uint32_t *nonorowsdesclen[nono->height];
				{
					for(int32_t row=nono->height-1;row>=0;row--){
						nonorowsdesclen[row]=malloc(sizeof(uint32_t)*nono->rowsdesc[row].length);
						if(!nonorowsdesclen[row]){
							// Something went wrong so free allocated memory and exit with error.
							for(int32_t t=nono->height-1;t>row;t--) free(nonorowsdesclen[t]);
							for(int32_t col=nono->width-1;col>=0;col--) free(nonocolsdesclen[col]);
							return 6;
						}
						for(int32_t block=0;block<nono->rowsdesc[row].length;block++){
							nonorowsdesclen[row][block]=i32toalen(nono->rowsdesc[row].lengths[block]);
						}
					}
				}
				const uint8_t svg_template41[]={'<','/','t','e','x','t','>'};

				const uint8_t svg_template42[]={'\n','<','/','g','>','\n'
				                               ,'<','/','g','>','\n'
				                               ,'<','g',' ','t','r','a','n','s','f','o','r','m','=','"','t','r','a','n','s','l','a','t','e','(',
				                               };
				//descabscissaoffsetlen; Again!
				const uint8_t svg_template43[]={' '};
				//descordinateoffsetlen; Again!
				const uint8_t svg_template44[]={')','"',' ','m','a','r','k','e','r','-','s','t','a','r','t','=','"','u','r','l','(','#','H','o','o','k','E','n','d',')','"',' ','m','a','r','k','e','r','-','e','n','d','=','"','u','r','l','(','#','H','o','o','k','E','n','d',')','"',' ','s','t','y','l','e','=','"','s','t','r','o','k','e',':','#','0','0','0','0','0','0',';','s','t','r','o','k','e','-','w','i','d','t','h',':','1',';','"','>'};


				htmlpage->nonotemplatelen=sizeof(svg_template1)
				                         +pixelwidth10len
				                         +sizeof(svg_template2)
				                         +pixelheight10len
				                         +sizeof(svg_template3)
				                         +pixelwidthlen
				                         +sizeof(svg_template4)
				                         +pixelheightlen
				                         +sizeof(svg_template5)
				                         +descabscissaoffsetlen
				                         +sizeof(svg_template6)
				                         +descordinateoffsetlen
				                         +sizeof(svg_template7)
				                         +pixelwidth_descabscissaoffsetlen
				                         +sizeof(svg_template8)
				                         +pixelheight_descordinateoffsetlen
				                         +sizeof(svg_template9)
				                         +descabscissaoffsetlen
				                         +sizeof(svg_template10)
				                         +descordinateoffsetextralen
				                         +sizeof(svg_template11);
				for(int32_t px=nono->width-1;px>=0;px--){
					for(int32_t py=nono->height-1;py>=0;py--){
						htmlpage->nonotemplatelen+=sizeof(svg_template12);
						htmlpage->nonotemplatelen+=pixelcoordlens[px];
						htmlpage->nonotemplatelen+=sizeof(svg_template13);
						htmlpage->nonotemplatelen+=pixelcoordlens[py];
						htmlpage->nonotemplatelen+=sizeof(svg_template14);
					}
				}
				htmlpage->nonotemplatelen+=sizeof(svg_template15)
				                          +pixelwidthlen
				                          +sizeof(svg_template16)
				                          +pixelheightlen
				                          +sizeof(svg_template17)
				                          +descabscissaoffsetlen
				                          +sizeof(svg_template18);
				// Column line length addition.
				for(int32_t col=nono->width-1;col>=0;col--){
					htmlpage->nonotemplatelen+=sizeof(svg_template19)
					                          +pixelcoordlens[col]
					                          +sizeof(svg_template20)
					                          +pixelcoordlens[col]
					                          +sizeof(svg_template21)
					                          +pixelheightlen
					                          +sizeof(svg_template22);
				}
				htmlpage->nonotemplatelen+=sizeof(svg_template23)
				                          +descordinateoffsetlen
				                          +sizeof(svg_template24);
				// Row line length addition.
				for(int32_t row=nono->height-1;row>=0;row--){
					htmlpage->nonotemplatelen+=sizeof(svg_template25)
					                          +pixelcoordlens[row]
					                          +sizeof(svg_template26)
					                          +pixelwidthlen
					                          +sizeof(svg_template27)
					                          +pixelcoordlens[row]
					                          +sizeof(svg_template28);
				}
				htmlpage->nonotemplatelen+=sizeof(svg_template29)
				                          +descabscissaoffset5len
				                          +sizeof(svg_template30);

				// Column descriptions length addition.
				for(int32_t col=nono->width-1;col>=0;col--){
					htmlpage->nonotemplatelen+=sizeof(svg_template31)
					                          +pixelcoordlens[col]
					                          +sizeof(svg_template32)
					                          +nonocolsdesclen[col][0];
					for(int32_t block=1;block<nono->colsdesc[col].length;block++){
						htmlpage->nonotemplatelen+=sizeof(svg_template33)
						                          +nonocolsdesclen[col][block]
						                          +sizeof(svg_template34);
					}
					htmlpage->nonotemplatelen+=sizeof(svg_template35);
				}
				htmlpage->nonotemplatelen+=sizeof(svg_template36)
				                          +descordinateoffsetextra2len
				                          +sizeof(svg_template37);

				for(int32_t row=nono->height-1;row>=0;row--){
					htmlpage->nonotemplatelen+=sizeof(svg_template38)
					                          +pixelcoordlens[row]
					                          +sizeof(svg_template39);
					htmlpage->nonotemplatelen+=nonorowsdesclen[row][0];
					for(int32_t block=1;block<nono->rowsdesc[row].length;block++){
						htmlpage->nonotemplatelen+=sizeof(svg_template40)
						                          +nonorowsdesclen[row][block]
						                          +sizeof(svg_template34);
					}
					htmlpage->nonotemplatelen+=sizeof(svg_template41);
				}
				htmlpage->nonotemplatelen+=sizeof(svg_template42);
				htmlpage->nonotemplatelen+=descabscissaoffsetlen;
				htmlpage->nonotemplatelen+=sizeof(svg_template43);
				htmlpage->nonotemplatelen+=descordinateoffsetlen;
				htmlpage->nonotemplatelen+=sizeof(svg_template44);

				// Allocation.
				if((htmlpage->nonotemplate=malloc(htmlpage->nonotemplatelen))){

					// Move the data.
					memcpy(htmlpage->nonotemplate,svg_template1,sizeof(svg_template1));
					uint32_t bufferpoint=sizeof(svg_template1);
					i32toa(pixelwidth+10,htmlpage->nonotemplate+bufferpoint,pixelwidth10len);

					bufferpoint+=pixelwidth10len;
					memcpy(htmlpage->nonotemplate+bufferpoint,svg_template2,bufferpoint);
					bufferpoint+=sizeof(svg_template2);
					i32toa(pixelheight+10,htmlpage->nonotemplate+bufferpoint,pixelheight10len);

					bufferpoint+=pixelheight10len;
					memcpy(htmlpage->nonotemplate+bufferpoint,svg_template3,sizeof(svg_template3));
					bufferpoint+=sizeof(svg_template3);
					i32toa(pixelwidth,htmlpage->nonotemplate+bufferpoint,pixelwidthlen);

					bufferpoint+=pixelwidthlen;
					memcpy(htmlpage->nonotemplate+bufferpoint,svg_template4,sizeof(svg_template4));
					bufferpoint+=sizeof(svg_template4);
					i32toa(pixelheight,htmlpage->nonotemplate+bufferpoint,pixelheightlen);

					bufferpoint+=pixelheightlen;
					memcpy(htmlpage->nonotemplate+bufferpoint,svg_template5,sizeof(svg_template5));
					bufferpoint+=sizeof(svg_template5);
					i32toa(descabscissaoffset,htmlpage->nonotemplate+bufferpoint,descabscissaoffsetlen);

					bufferpoint+=descabscissaoffsetlen;
					memcpy(htmlpage->nonotemplate+bufferpoint,svg_template6,sizeof(svg_template6));
					bufferpoint+=sizeof(svg_template6);
					i32toa(descordinateoffset,htmlpage->nonotemplate+bufferpoint,descordinateoffsetlen);

					bufferpoint+=descordinateoffsetlen;
					memcpy(htmlpage->nonotemplate+bufferpoint,svg_template7,sizeof(svg_template7));
					bufferpoint+=sizeof(svg_template7);
					i32toa(pixelwidth-descabscissaoffset,htmlpage->nonotemplate+bufferpoint,pixelwidth_descabscissaoffsetlen);

					bufferpoint+=pixelwidth_descabscissaoffsetlen;
					memcpy(htmlpage->nonotemplate+bufferpoint,svg_template8,sizeof(svg_template8));
					bufferpoint+=sizeof(svg_template8);
					i32toa(pixelheight-descordinateoffset,htmlpage->nonotemplate+bufferpoint,pixelheight_descordinateoffsetlen);

					bufferpoint+=pixelheight_descordinateoffsetlen;
					memcpy(htmlpage->nonotemplate+bufferpoint,svg_template9,sizeof(svg_template9));
					bufferpoint+=sizeof(svg_template9);
					i32toa(descabscissaoffset,htmlpage->nonotemplate+bufferpoint,descabscissaoffsetlen);

					bufferpoint+=descabscissaoffsetlen;
					memcpy(htmlpage->nonotemplate+bufferpoint,svg_template10,sizeof(svg_template10));
					bufferpoint+=sizeof(svg_template10);
					i32toa(descordinateoffsetextra,htmlpage->nonotemplate+bufferpoint,descordinateoffsetextralen);

					bufferpoint+=descordinateoffsetextralen;
					memcpy(htmlpage->nonotemplate+bufferpoint,svg_template11,sizeof(svg_template11));
					bufferpoint+=sizeof(svg_template11);

					// Starting indicator for pixels list.
					// sizeofs are used to make sure start
					// is closer to last id attribute.
					// See variable svg_template14 above.
					htmlpage->pixelindicatorstart=htmlpage->nonotemplate+bufferpoint+sizeof(svg_template12)+sizeof(svg_template13);

					// Make pixel type indicators.
					// Ordinate (y) direction iteration has to be outer loop so that order is same as
					// the table.
					for(int32_t py=nono->height-1;py>=0;py--){
						for(int32_t px=nono->width-1;px>=0;px--){
							memcpy(htmlpage->nonotemplate+bufferpoint,svg_template12,sizeof(svg_template12));
							bufferpoint+=sizeof(svg_template12);
							i32toa(px*pixelside,htmlpage->nonotemplate+bufferpoint,pixelcoordlens[px]);
							bufferpoint+=pixelcoordlens[px];

							memcpy(htmlpage->nonotemplate+bufferpoint,svg_template13,sizeof(svg_template13));
							bufferpoint+=sizeof(svg_template13);
							i32toa(py*pixelside,htmlpage->nonotemplate+bufferpoint,pixelcoordlens[py]);
							bufferpoint+=pixelcoordlens[py];

							memcpy(htmlpage->nonotemplate+bufferpoint,svg_template14,sizeof(svg_template14));
							bufferpoint+=sizeof(svg_template14);
						}
					}
					// Capture last indicator's address
					// so that it can be change indicators
					// later. The -4 comes from us moving
					// back to 'U' in svg_template14 above.
					// After this by changing the later
					// pixel's indicator changes.
					htmlpage->pixelindicatorend=htmlpage->nonotemplate+bufferpoint-4;

					// Borderline
					memcpy(htmlpage->nonotemplate+bufferpoint,svg_template15,sizeof(svg_template15));
					bufferpoint+=sizeof(svg_template15);
					i32toa(pixelwidth,htmlpage->nonotemplate+bufferpoint,pixelwidthlen);
					bufferpoint+=pixelwidthlen;

					memcpy(htmlpage->nonotemplate+bufferpoint,svg_template16,sizeof(svg_template16));
					bufferpoint+=sizeof(svg_template16);
					i32toa(pixelheight,htmlpage->nonotemplate+bufferpoint,pixelheightlen);
					bufferpoint+=pixelheightlen;

					memcpy(htmlpage->nonotemplate+bufferpoint,svg_template17,sizeof(svg_template17));
					bufferpoint+=sizeof(svg_template17);
					i32toa(descabscissaoffset,htmlpage->nonotemplate+bufferpoint,descabscissaoffsetlen);
					bufferpoint+=descabscissaoffsetlen;

					memcpy(htmlpage->nonotemplate+bufferpoint,svg_template18,sizeof(svg_template18));
					bufferpoint+=sizeof(svg_template18);

					// Column lines
					for(int32_t col=nono->width-1;col>=0;col--){
						memcpy(htmlpage->nonotemplate+bufferpoint,svg_template19,sizeof(svg_template19));
						bufferpoint+=sizeof(svg_template19);
						i32toa(col*pixelside,htmlpage->nonotemplate+bufferpoint,pixelcoordlens[col]);
						bufferpoint+=pixelcoordlens[col];

						memcpy(htmlpage->nonotemplate+bufferpoint,svg_template20,sizeof(svg_template20));
						bufferpoint+=sizeof(svg_template20);
						i32toa(col*pixelside,htmlpage->nonotemplate+bufferpoint,pixelcoordlens[col]);
						bufferpoint+=pixelcoordlens[col];

						memcpy(htmlpage->nonotemplate+bufferpoint,svg_template21,sizeof(svg_template21));
						bufferpoint+=sizeof(svg_template21);
						i32toa(pixelheight,htmlpage->nonotemplate+bufferpoint,pixelheightlen);
						bufferpoint+=pixelheightlen;

						memcpy(htmlpage->nonotemplate+bufferpoint,svg_template22,sizeof(svg_template22));
						bufferpoint+=sizeof(svg_template22);
					}

					memcpy(htmlpage->nonotemplate+bufferpoint,svg_template23,sizeof(svg_template23));
					bufferpoint+=sizeof(svg_template23);
					i32toa(descordinateoffset,htmlpage->nonotemplate+bufferpoint,descordinateoffsetlen);
					bufferpoint+=descordinateoffsetlen;

					memcpy(htmlpage->nonotemplate+bufferpoint,svg_template24,sizeof(svg_template24));
					bufferpoint+=sizeof(svg_template24);

					// Row lines
					for(int32_t row=nono->height-1;row>=0;row--){
						memcpy(htmlpage->nonotemplate+bufferpoint,svg_template25,sizeof(svg_template25));
						bufferpoint+=sizeof(svg_template25);
						i32toa(row*pixelside,htmlpage->nonotemplate+bufferpoint,pixelcoordlens[row]);
						bufferpoint+=pixelcoordlens[row];

						memcpy(htmlpage->nonotemplate+bufferpoint,svg_template26,sizeof(svg_template26));
						bufferpoint+=sizeof(svg_template26);
						i32toa(pixelwidth,htmlpage->nonotemplate+bufferpoint,pixelwidthlen);
						bufferpoint+=pixelwidthlen;

						memcpy(htmlpage->nonotemplate+bufferpoint,svg_template27,sizeof(svg_template27));
						bufferpoint+=sizeof(svg_template27);
						i32toa(row*pixelside,htmlpage->nonotemplate+bufferpoint,pixelcoordlens[row]);
						bufferpoint+=pixelcoordlens[row];

						memcpy(htmlpage->nonotemplate+bufferpoint,svg_template28,sizeof(svg_template28));
						bufferpoint+=sizeof(svg_template28);
					}

					memcpy(htmlpage->nonotemplate+bufferpoint,svg_template29,sizeof(svg_template29));
					bufferpoint+=sizeof(svg_template29);
					i32toa(descabscissaoffset+5,htmlpage->nonotemplate+bufferpoint,descabscissaoffset5len);
					bufferpoint+=descabscissaoffset5len;

					memcpy(htmlpage->nonotemplate+bufferpoint,svg_template30,sizeof(svg_template30));
					bufferpoint+=sizeof(svg_template30);

					// Column descriptions
					for(int32_t col=nono->width-1;col>=0;col--){
						memcpy(htmlpage->nonotemplate+bufferpoint,svg_template31,sizeof(svg_template31));
						bufferpoint+=sizeof(svg_template31);
						i32toa(col*pixelside,htmlpage->nonotemplate+bufferpoint,pixelcoordlens[col]);
						bufferpoint+=pixelcoordlens[col];

						memcpy(htmlpage->nonotemplate+bufferpoint,svg_template32,sizeof(svg_template32));
						bufferpoint+=sizeof(svg_template32);
						i32toa(nono->colsdesc[col].lengths[0],htmlpage->nonotemplate+bufferpoint,nonocolsdesclen[col][0]);
						bufferpoint+=nonocolsdesclen[col][0];

						for(int32_t block=1;block<nono->colsdesc[col].length;block++){
							memcpy(htmlpage->nonotemplate+bufferpoint,svg_template33,sizeof(svg_template33));
							bufferpoint+=sizeof(svg_template33);
							i32toa(nono->colsdesc[col].lengths[block],htmlpage->nonotemplate+bufferpoint,nonocolsdesclen[col][block]);
							bufferpoint+=nonocolsdesclen[col][block];

							memcpy(htmlpage->nonotemplate+bufferpoint,svg_template34,sizeof(svg_template34));
							bufferpoint+=sizeof(svg_template34);
						}

						memcpy(htmlpage->nonotemplate+bufferpoint,svg_template35,sizeof(svg_template35));
						bufferpoint+=sizeof(svg_template35);
					}

					memcpy(htmlpage->nonotemplate+bufferpoint,svg_template36,sizeof(svg_template36));
					bufferpoint+=sizeof(svg_template36);
					i32toa(descordinateoffsetextra2,htmlpage->nonotemplate+bufferpoint,descordinateoffsetextra2len);
					bufferpoint+=descordinateoffsetextra2len;

					memcpy(htmlpage->nonotemplate+bufferpoint,svg_template37,sizeof(svg_template37));
					bufferpoint+=sizeof(svg_template37);

					// Row descriptions
					for(int32_t row=nono->height-1;row>=0;row--){
						memcpy(htmlpage->nonotemplate+bufferpoint,svg_template38,sizeof(svg_template38));
						bufferpoint+=sizeof(svg_template38);
						i32toa(row*pixelside,htmlpage->nonotemplate+bufferpoint,pixelcoordlens[row]);
						bufferpoint+=pixelcoordlens[row];

						memcpy(htmlpage->nonotemplate+bufferpoint,svg_template39,sizeof(svg_template39));
						bufferpoint+=sizeof(svg_template39);

						// Write the blocks
						i32toa(nono->rowsdesc[row].lengths[0],htmlpage->nonotemplate+bufferpoint,nonorowsdesclen[row][0]);
						bufferpoint+=nonorowsdesclen[row][0];
						for(int32_t block=1;block<nono->rowsdesc[row].length;block++){
							memcpy(htmlpage->nonotemplate+bufferpoint,svg_template40,sizeof(svg_template40));
							bufferpoint+=sizeof(svg_template40);
							i32toa(nono->rowsdesc[row].lengths[block],htmlpage->nonotemplate+bufferpoint,nonorowsdesclen[row][block]);
							bufferpoint+=nonorowsdesclen[row][block];

							memcpy(htmlpage->nonotemplate+bufferpoint,svg_template34,sizeof(svg_template34));
							bufferpoint+=sizeof(svg_template34);
						}
						memcpy(htmlpage->nonotemplate+bufferpoint,svg_template41,sizeof(svg_template41));
						bufferpoint+=sizeof(svg_template41);
					}
					// Template block's range lines
					memcpy(htmlpage->nonotemplate+bufferpoint,svg_template42,sizeof(svg_template42));
					bufferpoint+=sizeof(svg_template42);
					i32toa(descabscissaoffset,htmlpage->nonotemplate+bufferpoint,descabscissaoffsetlen);
					bufferpoint+=descabscissaoffsetlen;

					memcpy(htmlpage->nonotemplate+bufferpoint,svg_template43,sizeof(svg_template43));
					bufferpoint+=sizeof(svg_template43);
					i32toa(descordinateoffset,htmlpage->nonotemplate+bufferpoint,descordinateoffsetlen);
					bufferpoint+=descordinateoffsetlen;

					memcpy(htmlpage->nonotemplate+bufferpoint,svg_template44,sizeof(svg_template44));
					bufferpoint+=sizeof(svg_template44);

					// NOTE: End and range markers are defined at nonogramWriteSvg.
					htmlpage->pixelsize=pixelside;
					htmlpage->blockrangesabscissacachelen=malloc(sizeof(int32_t)*nono->height);
					for(int32_t row=nono->height-1;row>=0;row--){
						htmlpage->blockrangesabscissacachelen[row]=i32toalen(getMarkerLocationPerpendicular(row,pixelside));
					}
					htmlpage->blockrangesordinatecachelen=malloc(sizeof(int32_t)*nono->width);
					for(int32_t col=nono->width-1;col>=0;col--){
						htmlpage->blockrangesordinatecachelen[col]=i32toalen(getMarkerLocationPerpendicular2(col,pixelside));
					}

					// Free temporary allocated resources.
					for(int32_t col=nono->width-1;col>=0;col--){
						free(nonocolsdesclen[col]);
					}
					for(int32_t row=nono->height-1;row>=0;row--){
						free(nonorowsdesclen[row]);
					}
				}
				else return 4;
			}
			else return 3;
		}
		else return 2;
	}
	else return 1;
	// Success!
	return 0;

}
void nonogramFreeSvg(NonoHTML *htmlpage){
	free(htmlpage->blockrangesabscissacachelen);
	free(htmlpage->blockrangesordinatecachelen);
	free(htmlpage->nonotemplate);
}
/*
* Support function to NonogramWriteSvg to decide should block's range be added to SVG image.
*/
static int nonogramWriteSvg_support_Add_decision(int32_t left,int32_t right,int32_t blocklength,NonogramWriteSvgFlags flags){
	// Don't draw if the block is done.
	int decision=(left+blocklength-1!=right);
	// Don't draw if blocklength is zero.
	decision&=(blocklength>0);
	return decision;
}
int nonogramWriteSvg(int file,NonoHTML *restrict htmlpage,const Nonogram *restrict nono,Table *restrict table,NonogramWriteSvgFlags flags){
	uint8_t svg_ending[]={'\n','<','/','g','>','\n'
	                     ,'<','/','g','>','\n'
	                     ,'<','/','s','v','g','>','\n'
	                     };
	struct iovec svgwrites[]={{htmlpage->nonotemplate,htmlpage->nonotemplatelen},{0,0},{svg_ending,sizeof(svg_ending)}};
	// Set the pixel indicators based upon given table.
	// Technically uint32_t so that width and height product fits without overflow.
	uint32_t pixelcounter=nono->width*nono->height;
	for(uint8_t *pixel=htmlpage->pixelindicatorstart;pixel<=htmlpage->pixelindicatorend;pixel++){
		if(*pixel=='#'){
			pixel++;
			// Since pixelcounter is decreaed before used as index
			// counter should be alright.
			switch(table->pixels[--pixelcounter].e){
				case UNKNOWN_PIXEL:*pixel='U'; break;
				case WHITE_PIXEL:*pixel='W'; break;
				case BLACK_PIXEL:*pixel='1'; break;
			}
		}
	}

	// Write block range indicators.
	if(flags & NONOGRAM_WRITE_SVG_FLAG_WRITE_BLOCKS_RANGES){

		// Constants to be written to buffer between numbers for one block's range.
		const uint8_t blockrangesvg1[]={'\n','<','l','i','n','e',' ','x','1','=','\"'};
		const uint8_t blockrangesvg2[]={'\"',' ','y','1','=','\"'};
		const uint8_t blockrangesvg3[]={'\"',' ','x','2','=','\"'};
		const uint8_t blockrangesvg4[]={'\"',' ','y','2','=','\"'};
		const uint8_t blockrangesvg5[]={'\"','/','>'};

		// Allocate buffer for caching i32toalen results.
		// NOTE: Allocated buffer could be bigger then needed.
		// For the rows.
		uint32_t rowcachelen=0;
		for(int32_t row=nono->height-1;row>=0;row--){
			rowcachelen+=nono->rowsdesc[row].length;
		}
		uint32_t rowcacheleft[rowcachelen];
		uint32_t rowcacheright[rowcachelen];

		// For the columns.
		uint32_t colcachelen=0;
		for(int32_t col=nono->width-1;col>=0;col--){
			colcachelen+=nono->colsdesc[col].length;
		}
		uint32_t colcacheleft[colcachelen];
		uint32_t colcacheright[colcachelen];

		// Calculate length of the buffer.
		// Rows
		uint32_t blocksrangeslength=0;
		{
			uint32_t rowrangeindex=0;
			for(int32_t row=nono->height-1;row>=0;row--){
				for(int32_t block=nono->rowsdesc[row].length-1;block>=0;block--){
					// Get the range.
					int32_t left=table->row[row].blocksrangelefts[block];
					int32_t right=table->row[row].blocksrangerights[block];
					int32_t blocklength=nono->rowsdesc[row].lengths[block];

					// Check should the block's range be added to length.
					if(nonogramWriteSvg_support_Add_decision(left,right,blocklength,flags)){

						// Add new range to buffer's length
						blocksrangeslength+=sizeof(blockrangesvg1);

						rowcacheleft[rowrangeindex]=i32toalen(getMarkerLocationParallel(left,htmlpage->pixelsize));
						blocksrangeslength+=rowcacheleft[rowrangeindex];

						blocksrangeslength+=sizeof(blockrangesvg2);

						blocksrangeslength+=htmlpage->blockrangesabscissacachelen[row];

						blocksrangeslength+=sizeof(blockrangesvg3);

						rowcacheright[rowrangeindex]=i32toalen(getMarkerLocationParallel(right,htmlpage->pixelsize));
						blocksrangeslength+=rowcacheright[rowrangeindex];

						blocksrangeslength+=sizeof(blockrangesvg4);

						blocksrangeslength+=htmlpage->blockrangesabscissacachelen[row];

						blocksrangeslength+=sizeof(blockrangesvg5);
						rowrangeindex++;
					}
				}
			}
		}
		// Columns
		{
			uint32_t colrangeindex=0;
			for(int32_t col=nono->width-1;col>=0;col--){
				for(int32_t block=nono->colsdesc[col].length-1;block>=0;block--){
					int32_t left=table->col[col].blocksrangelefts[block];
					int32_t right=table->col[col].blocksrangerights[block];
					int32_t blocklength=nono->colsdesc[col].lengths[block];

					// Check should the block's range be added to length.
					if(nonogramWriteSvg_support_Add_decision(left,right,blocklength,flags)){

						// Add new range to buffer's length
						blocksrangeslength+=sizeof(blockrangesvg1);

						blocksrangeslength+=htmlpage->blockrangesordinatecachelen[col];

						blocksrangeslength+=sizeof(blockrangesvg2);

						colcacheleft[colrangeindex]=i32toalen(getMarkerLocationPerpendicular(left,htmlpage->pixelsize));
						blocksrangeslength+=colcacheleft[colrangeindex];

						blocksrangeslength+=sizeof(blockrangesvg3);

						blocksrangeslength+=htmlpage->blockrangesordinatecachelen[col];

						blocksrangeslength+=sizeof(blockrangesvg4);

						colcacheright[colrangeindex]=i32toalen(getMarkerLocationPerpendicular(right,htmlpage->pixelsize));
						blocksrangeslength+=colcacheright[colrangeindex];

						blocksrangeslength+=sizeof(blockrangesvg5);

						colrangeindex++;
					}
				}
			}
		}

		// Allocate the buffer.
		uint8_t buffer[blocksrangeslength];

		// Copy to buffer.
		{
			// Go throught row blocks.
			uint32_t rowrangeindex=0;
			int32_t bufferpoint=0;
			for(int32_t row=nono->height-1;row>=0;row--){
				for(int32_t block=nono->rowsdesc[row].length-1;block>=0;block--){
					// Get the range.
					int32_t left=table->row[row].blocksrangelefts[block];
					int32_t right=table->row[row].blocksrangerights[block];
					int32_t blocklength=nono->rowsdesc[row].lengths[block];

					// Check should the block's range be added to length.
					if(nonogramWriteSvg_support_Add_decision(left,right,blocklength,flags)){
						// Add block's range to buffer.
						memcpy(buffer+bufferpoint,blockrangesvg1,sizeof(blockrangesvg1));
						bufferpoint+=sizeof(blockrangesvg1);

						i32toa(getMarkerLocationParallel(left,htmlpage->pixelsize),buffer+bufferpoint,rowcacheleft[rowrangeindex]);
						bufferpoint+=rowcacheleft[rowrangeindex];

						memcpy(buffer+bufferpoint,blockrangesvg2,sizeof(blockrangesvg2));
						bufferpoint+=sizeof(blockrangesvg2);

						i32toa(getMarkerLocationPerpendicular(row,htmlpage->pixelsize),buffer+bufferpoint,htmlpage->blockrangesabscissacachelen[row]);
						bufferpoint+=htmlpage->blockrangesabscissacachelen[row];

						memcpy(buffer+bufferpoint,blockrangesvg3,sizeof(blockrangesvg3));
						bufferpoint+=sizeof(blockrangesvg3);

						i32toa(getMarkerLocationParallel(right,htmlpage->pixelsize),buffer+bufferpoint,rowcacheright[rowrangeindex]);
						bufferpoint+=rowcacheright[rowrangeindex];

						memcpy(buffer+bufferpoint,blockrangesvg4,sizeof(blockrangesvg4));
						bufferpoint+=sizeof(blockrangesvg4);

						i32toa(getMarkerLocationPerpendicular(row,htmlpage->pixelsize),buffer+bufferpoint,htmlpage->blockrangesabscissacachelen[row]);
						bufferpoint+=htmlpage->blockrangesabscissacachelen[row];

						memcpy(buffer+bufferpoint,blockrangesvg5,sizeof(blockrangesvg5));
						bufferpoint+=sizeof(blockrangesvg5);

						rowrangeindex++;
					}
				}
			}

			// throught column blocks.
			uint32_t colrangeindex=0;
			for(int32_t col=nono->width-1;col>=0;col--){
				for(int32_t block=nono->colsdesc[col].length-1;block>=0;block--){
					// Get the range.
					int32_t left=table->col[col].blocksrangelefts[block];
					int32_t right=table->col[col].blocksrangerights[block];
					int32_t blocklength=nono->colsdesc[col].lengths[block];

					// Check should the block's range be added to length.
					if(nonogramWriteSvg_support_Add_decision(left,right,blocklength,flags)){

						// Add block's range to buffer.
						memcpy(buffer+bufferpoint,blockrangesvg1,sizeof(blockrangesvg1));
						bufferpoint+=sizeof(blockrangesvg1);

						i32toa(getMarkerLocationPerpendicular2(col,htmlpage->pixelsize),buffer+bufferpoint,htmlpage->blockrangesordinatecachelen[col]);
						bufferpoint+=htmlpage->blockrangesordinatecachelen[col];

						memcpy(buffer+bufferpoint,blockrangesvg2,sizeof(blockrangesvg2));
						bufferpoint+=sizeof(blockrangesvg2);

						i32toa(getMarkerLocationPerpendicular(left,htmlpage->pixelsize),buffer+bufferpoint,colcacheleft[colrangeindex]);
						bufferpoint+=colcacheleft[colrangeindex];

						memcpy(buffer+bufferpoint,blockrangesvg3,sizeof(blockrangesvg3));
						bufferpoint+=sizeof(blockrangesvg3);

						i32toa(getMarkerLocationPerpendicular2(col,htmlpage->pixelsize),buffer+bufferpoint,htmlpage->blockrangesordinatecachelen[col]);
						bufferpoint+=htmlpage->blockrangesordinatecachelen[col];

						memcpy(buffer+bufferpoint,blockrangesvg4,sizeof(blockrangesvg4));
						bufferpoint+=sizeof(blockrangesvg4);

						i32toa(getMarkerLocationPerpendicular(right,htmlpage->pixelsize),buffer+bufferpoint,colcacheright[colrangeindex]);
						bufferpoint+=colcacheright[colrangeindex];

						memcpy(buffer+bufferpoint,blockrangesvg5,sizeof(blockrangesvg5));
						bufferpoint+=sizeof(blockrangesvg5);

						colrangeindex++;
					}
				}
			}
		}

		svgwrites[1].iov_base=buffer;
		svgwrites[1].iov_len=blocksrangeslength;

	}

	// Write the table.
	if(writev(file,svgwrites,sizeof(svgwrites)/sizeof(struct iovec))>0){
		return 1;
	}
	else return 0;
}
void nonogramEmpty(Nonogram *nono){
	for(int32_t i=0;i<nono->width;i++){
		free(nono->colsdesc[i].lengths);
	}
	for(int32_t i=0;i<nono->height;i++){
		free(nono->rowsdesc[i].lengths);
	}
	free(nono->colsdesc);
	free(nono->rowsdesc);
}
int nonogramInitTable(const Nonogram *restrict nono,TableHead *restrict tables,Table *restrict table){
	// Set new last table.
	tables->last->next=table;
	tables->last=table;
	// Initialize the table
	table->next=NULL;
	table->pixels=calloc(nono->width*nono->height,sizeof(Pixel));
	if(!table->pixels) return 0;

	// Allocate block's ranges.
	// First the arrays of the ranges for each line.
	table->col=malloc(sizeof(BlocksRanges)*nono->width);
	if(!table->col) goto ERROR_nonogramInitTable_1;
	table->row=malloc(sizeof(BlocksRanges)*nono->height);
	if(!table->row) goto ERROR_nonogramInitTable_2;

	// Second allocate range for individual blocks.
	for(int32_t col=nono->width-1;col>=0;col--){
		table->col[col].blocksrangelefts=malloc(sizeof(int32_t)*nono->colsdesc[col].length);
		table->col[col].blocksrangerights=malloc(sizeof(int32_t)*nono->colsdesc[col].length);

		// Error check.
		if(!table->col[col].blocksrangelefts || !table->col[col].blocksrangerights){
			// We don't know which allocation failed to be causes.
			// NOTE: Malloc is quarenteed to return zero so initialization happened!
			if(table->col[col].blocksrangelefts) free(table->col[col].blocksrangelefts);
			if(table->col[col].blocksrangerights) free(table->col[col].blocksrangerights);
			// Free ones that are succesfully allocated have to freed.
			for(int32_t c=col+1;c<nono->width-1;c++){
				free(table->col[c].blocksrangelefts);
				free(table->col[c].blocksrangerights);
			}
			// Free the rest.
			goto ERROR_nonogramInitTable_3;
		}
	}
	for(int32_t row=nono->height-1;row>=0;row--){
		table->row[row].blocksrangelefts=malloc(sizeof(int32_t)*nono->rowsdesc[row].length);
		table->row[row].blocksrangerights=malloc(sizeof(int32_t)*nono->rowsdesc[row].length);

		// Error check.
		if(!table->row[row].blocksrangelefts || !table->row[row].blocksrangerights){
			// We don't know which allocation failed to be causes.
			// NOTE: Malloc is quarenteed to return zero so initialization happened!
			if(table->row[row].blocksrangelefts) free(table->row[row].blocksrangelefts);
			if(table->row[row].blocksrangerights) free(table->row[row].blocksrangerights);
			// Free ones that are succesfully allocated have to freed.
			for(int32_t r=row+1;r<nono->height-1;r++){
				free(table->row[row].blocksrangelefts);
				free(table->row[row].blocksrangerights);
			}
			// Free the rest.
			goto ERROR_nonogramInitTable_4;
		}

	}

	return 1;

	ERROR_nonogramInitTable_4:
	for(int32_t col=nono->width-1;col>=0;col--){
		free(table->col[col].blocksrangelefts);
		free(table->col[col].blocksrangerights);
	}
	ERROR_nonogramInitTable_3:
	free(table->row);
	ERROR_nonogramInitTable_2:
	free(table->col);
	ERROR_nonogramInitTable_1:
	free(table->pixels);
	return 0;
}
Table *nonogramAllocTable(const Nonogram *restrict nono,TableHead *restrict tables){
	Table *table=malloc(sizeof(Table));
	if(table){
		if(nonogramInitTable(nono,tables,table)){
			return table;
		}
		free(table);
	}
	return NULL;
}
static void nonogramEmptyTablesInternal(Nonogram *nono,Table *ite1){
	free(ite1->pixels);
	for(int32_t col=nono->width-1;col>=0;col--){
		free(ite1->col[col].blocksrangelefts);
		free(ite1->col[col].blocksrangerights);
	}
	free(ite1->col);
	for(int32_t row=nono->height-1;row>=0;row--){
		free(ite1->row[row].blocksrangelefts);
		free(ite1->row[row].blocksrangerights);
	}
	free(ite1->row);
}
void nonogramEmptyTables(Nonogram *restrict nono,TableHead *restrict tables){
	Table *ite1=tables->firsttable.next;
	// Free the first table
	nonogramEmptyTablesInternal(nono,&tables->firsttable);
	// Free the rest.
	Table *ite2;
	while(ite1){
		ite2=ite1->next;
		nonogramEmptyTablesInternal(nono,ite1);
		free(ite1);
		ite1=ite2;
	}
}
