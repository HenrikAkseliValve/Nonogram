/*
* Mobule given basic structures and functions for handling nonograms.
*/
#ifndef _NONOGRAMS_H_
#define _NONOGRAMS_H_
#include<stdint.h>

/*
* Structure to store information to help
* writing tables in SVG output mode.
*/
typedef struct{
	uint8_t *nonotemplate;
	uint8_t *pixelindicatorstart;
	uint8_t *pixelindicatorend;
	uint32_t *blockrangesabscissacachelen;
	uint32_t *blockrangesordinatecachelen;
	uint32_t nonotemplatelen;
	int32_t pixelsize;
}NonoHTML;
/*
* Description stores block lengths for the line.
* 
* Members:
*   lengths is array of the block lengths.
*   length is number of hints.
*/
typedef struct{
	int32_t *lengths;
	int32_t length;
}Description;
/*
* General structure for storing nonogram.
* 
* Members:
*   colsdesc is array of desctiptions for columns.
*   rowsdesc is array of descriptions for rows.
*   width is number of columns.
*   height is number of rows.
*   flags is 16 bit bit flag. has_partial is flag for
*     indicating first table is partial solution. 
*/
typedef struct{
	Description *rowsdesc;
	Description *colsdesc;
	int32_t width;
	int32_t height;
	union{
		struct{
			uint16_t haspartial:1;
		};
		uint16_t shadow;
	}flags;
}Nonogram;
/*
* Enumerate for the pixel's of Nonograms.
*/
enum Pixel{
	UNKNOWN_PIXEL,
	WHITE_PIXEL,
	BLACK_PIXEL,
};
typedef union{
	enum Pixel e;
	uint8_t b;
}Pixel;
/*
* Store one lines block's ranges in this stucture.
* Number of these ranges is number of blocks stored
* at Nonogram structure most likely.
*/
typedef struct BlocksRanges{
	int32_t *blocksrangelefts;
	int32_t *blocksrangerights;
}BlocksRanges;
/*
* Linked list of solutions or partial solutions of nonogram structure. 
* Width and height of the table isn't store in the structure.
*
* Members:
*   next is link to next table.
*   pixels is array of pixels of the solution or partial solution.
*     Stored in row-major-order.
*/
typedef struct Table{
	struct Table *next;
	Pixel *pixels;
	BlocksRanges *row;
	BlocksRanges *col;
}Table;
/*
* Table head stores head of the tables.
* It stores the first table and link to last table.
*
* Members:
*   firsttable is the first table in the list.
*   last is the pointer to last table in the link.
*/
typedef struct{
	Table *last;
	Table firsttable;
}TableHead;

/*
* Colour pixel location in the table for given line.
* 
* Parameters:
*   line is starting pointer of the line.
*   nth is index in the line.
*   stride is amount of pixels between line's pixels on the table.
*   colour is the colour pixl will be painted.
* Returns:
*   Returns 1 on success and 0 if location is already painted.
*
* NOTE: ARGUMENT lines or colored IS NOT NULL CHECKED!
*/
int8_t colourLinesPixel(enum Pixel *lines,int32_t nth,uint32_t stride,enum Pixel colour,int32_t *colored);
/*
* Get pixel line's nth pixel.
* 
* Parameters:
*   line is starting pointer of the line.
*   nth is index in the line.
*   stride is amount of pixels between line's pixels on the table.
*
* NOTE: ARGUMENT lines IS NOT NULL CHECKED!
*/
enum Pixel *getLinePixel(enum Pixel *lines,int32_t nth,uint32_t stride);
/*
* Colour pixel of some row and col of given table.
*
* Parameters:
*   table is the table which pixel is coloured.
*   row is the row index pixel is from.
*   col is the col index pixel is from.
*   colour is the colour pixel will set to.
* Returns:
*   Returns 1 on success and 0 if location is already painted.
*/
void colourPixel(Nonogram *nono,Table *table,int32_t row,int32_t col,enum Pixel colour);
/*
* Get pixel colour at row and col location.
*
* Parameters:
*   row is the row index pixel is from.
*   col is the col index pixel is from.
* Returns:
*   Returns the colour.
*/
enum Pixel getPixel(Nonogram *nono,Table *table,int32_t row,int32_t col);
/*
* Get biggest number of blocks in array of descriptions.
* If no descriptions are given then length returned is 0.
*
* Parameters:
*   descs is array of descriptions.
*
* NOTE: ARGUMENT desc IS NOT NULL CHECKED!
*/
int32_t biggestDescriptionSize(const Description *desc,int32_t length);
/*
* Calcualte description's length
*/
int32_t descriptionsLength(const Description *desc);
/*
* Initialize block's range.
*
* Parameters:
*   ranges is pointer to block's ranges to be initialized.
*   blocknumber is number of blocks in the line's description.
*   length is the length of the line in pixels.
*/
void initBlocksRanges(BlocksRanges *ranges,Description *desc,int32_t length);
/*
* This reads saved nonogram from the file. Format of the file is 
* described at nonogramSave function in this file. Nonogram
* structure described in this file will be filled.
*
* Parameters:
*   nono is nonogram structure that will be filled.
*     Assumed NOT to be NULL!
*   file is description to file that is ALREADY OPENED!
*
* Returns:
*   Returns 0 on success.
*
* NOTE: ARGUMENT nono IS NOT NULL CHECKED!
*/
uint8_t nonogramLoad(int file,Nonogram *nono);
/*
* Load nonogram from cofiguration file.
* Configuration file is human writable input. 
* 
* Parameters:
*   nono is structure that will be filled.
*     Assumed NOT to be NULL!
*   file is description to file that is ALREADY OPENED!
*
* Returns:
*   Returns 0 on success.
*
* NOTE: ARGUMENT nono IS NOT NULL CHECKED!
*/
struct NonogramLoadConfResult{
	uint32_t id:3;
	uint32_t line:29;
};
struct NonogramLoadConfResult nonogramLoadConf(int file,Nonogram *nono);
/*
* Save nonogram to file. File includes descriptions and
* tables of solutions and partial solutions.
*
* File format is following: Start there is magic "NONO".
* Magic is used for detecting endianess.
* After this is a 2 byte flag space. First low bit tells is first
* table partial solution. Width and height is telled in 
* the file 4 bytes for each. Then row and column descriptions
* are written by 4 bytes telling the length of the block 
* followed by block lengths. After descriptions tables are
* written using one byte for each pixel.
*
* Parameters:
*   nono is nonogram to save.
*   file is file descriptor for the file. ALREADY OPENED!
*
* Return:
*   0 on success.
*
* NOTE: ARGUMENT nono IS NOT NULL CHECKED!
*/
uint8_t nonogramSave(Nonogram *nono,int file);
/*
* Generate SVG page's start for the nonogram.
* Initializes HTML buffer and SVG tamplate.
* SVG will be descriptions in left and up sides 
* of the rectangle. Pixel box is 10x10 pixels.
* Lower left corner is used to indicate the type of
* pixel. Other corners are used to indicate block's
* range.
*
* Parameters:
*   nono is nonogram SVG page will be generated from.
*   file is file descriptor to ALREADY OPEN file.
*
* Return:
*   0 on success.
*
* NOTE: NONE OF THE ARGUMENTS ARE NULL CHECKED!
*/
uint8_t nonogramGenSvgStart(const int file,const Nonogram *restrict nono,NonoHTML *restrict htmlpage);
/*
* Write SVG table.
*/
typedef enum NonogramWriteSvgFlags{
	NONOGRAM_WRITE_SVG_FLAG_EMPTY=0b0,
	NONOGRAM_WRITE_SVG_FLAG_WRITE_BLOCKS_RANGES=0b1
}NonogramWriteSvgFlags;
int nonogramWriteSvg(int file,NonoHTML *restrict htmlpage,const Nonogram *restrict nono,Table *restrict table,NonogramWriteSvgFlags flags);
/*
* Free SVG page structure.
*/
void nonogramFreeSvg(NonoHTML *htmlpage);
/*
* Empty the nonogram meaning that what has been allocated for it  
* is freed. This function doesn't free nonogram structure it self.
* NOTE: DOESN'T CHECK FOR NULL NONOGRAM!
*
* Parameters:
*   nono is nonogram that will be emptied.
*/
void nonogramEmpty(Nonogram *nono);
/*
* Initialize new table. Does not allocate a table.
*
* Return:
*   1 on success.
*   0 on failure.
* NOTE: NONE OF THE ARGUMENTS ARE NULL CHECKED!
*/
int nonogramInitTable(const Nonogram *restrict nono,TableHead *restrict tables,Table *restrict table);
/*
* Initilize Table head.
*
* Return:
*   1 on success.
*   0 on failure.
*/
static inline int nonogramInitTableHead(const Nonogram *restrict nono,TableHead *restrict tables){

	// This just tricks the InitTable to initialize the first table.
	// Since pointer to structure is same as pointer structure's first member.
	// No need for if sentence this way.
	{
		TableHead *p=tables;
		return nonogramInitTable(nono,(TableHead *)&p,&tables->firsttable);
	}
}
/*
* Get line linenumber from the table.
*
* Parameters:
*   nono is the nonogram.
*   roworcol is false line is a row and if true line is a column.
*   linenumber is index of the line wanted.
*   table is the table of pixels.
*
* Returns:
*   Pointer to first pixel in the line.
*/
static inline enum Pixel *getLine(Nonogram *restrict nono,uint8_t roworcol,int32_t linenumber,Table *restrict table){
	return (enum Pixel*)table->pixels+(!roworcol)*nono->width*linenumber+roworcol*linenumber;
}
/*
* Get description of the line.
*
* Parameters:
*   nono is the nonogram.
*   roworcol is false line is a row and if true line is a column.
*   linenumber is index of the line wanted.
*
* Returns:
*   Pointer to line's description.
*/
static inline Description *getDesc(Nonogram *restrict nono,uint8_t roworcol,int32_t linenumber){
	return (*((&nono->rowsdesc)+roworcol))+linenumber;
}
/*
* Get Block's ranges of the line.
*
* Parameters:
*   nono is the nonogram.
*   roworcol is false line is a row and if true line is a column.
*   linenumber is index of the line wanted.
*   table is the table of pixels.
*
* Returns:
*   Returns array of the block's ranges.
*/
static inline BlocksRanges *getBlockRanges(Nonogram *restrict nono,uint8_t roworcol,int32_t linenumber,Table *restrict table){
	return (*((&table->row)+roworcol))+linenumber;
}
/*
* Allocate and initialize new table. New table is given as return value.
* If return value is NULL error happened.
* NOTE: NONE OF THE ARGUMENTS ARE NULL CHECKED!
*/
Table *nonogramAllocTable(const Nonogram *restrict nono,TableHead *restrict tables);
/*
* Empties solution tables of the nonogram.
* NOTE: DOESN'T CHECK FOR NULL NONOGRAM!
*
* Parameters:
*     nono is nonogram that will be emptied.
*/
void nonogramEmptyTables(Nonogram *restrict nono,TableHead *restrict tables);

#endif /* _NONOGRAMS_H_ */
