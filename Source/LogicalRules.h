/*
* Logical rules for solving nonograms. Based on article
* An Efficient Algorithm For Solving Nononograms from Appl Intell written by
* Chiung-Hsueh Yu, Hui-Lung Lee, Ling-Hwei Chen. Which divide rules to three categories
* and initialization. Initialzation is done in when Nonogram is created and other logical
* rules are implemented in this module.
*/
#ifndef _LOGICAL_RULES_H_
#include"Nonograms.h"

/*
* Rule zero. For some reson zero lines weren't
* written so rule zero will white all the lines
* that have zero description. Returns number of pixel colored.
* NOTE: NOT part of the An Efficient Algorithm For Solving Nononograms
*/
int32_t nonogramLogicalRule0(enum Pixel *line,int32_t length,uint32_t stride,Description *desc,BlocksRanges *ranges);
/*
* Area that blocks rightmost and leftmost position
* overlap should be black. Returns number of
* pixels colored.
*/
int32_t nonogramLogicalRule11(enum Pixel *line,int32_t length,uint32_t stride,Description *desc,BlocksRanges *ranges);
/*
* Pixel is white if pixel doesn't belong to any blocks' range.
*/
int32_t nonogramLogicalRule12(enum Pixel *line,int32_t length,uint32_t stride,Description *desc,BlocksRanges *ranges);
/*
* If blocks ends are also covared by block of length one
* then pixel next to the end will be white.
* 
* Since it is unlikely that block's ends are black
* pixel and that line would have blocks that are one so
* test for ends are done separetly.
*/
int32_t nonogramLogicalRule13(enum Pixel *line,int32_t length,uint32_t stride,Description *desc,BlocksRanges *ranges);
/*
* If there is pixel-1 to pixel+1 are black and
* pixel is unknow we can place white at pixel if
* lengths of all the possiable blocks cavering 
* pixel are larger then blocks covering the area
* of the pixel.
*/
int32_t nonogramLogicalRule14(enum Pixel *line,int32_t length,uint32_t stride,Description *desc,BlocksRanges *ranges);
/*
* If there is black pixel and ether side of that 
* pixel is white pixel close enough (minimun
* block length from the black pixel?) some pixel
* of the opposite side of the black pixel can be
* colored. Additionally if all the blocks covering pixel 
* are same length and equal to our black group we can 
* say pixel next to ends of the run are white.
*/
int32_t nonogramLogicalRule15(enum Pixel *line,int32_t length,uint32_t stride,Description *desc,BlocksRanges *ranges);
/*
* Check that no block's range "fully" inside other block's range.
* (For every block blocksrangelefts[block]<blocksrangelefts[block+1] must be true).
*/
int32_t nonogramLogicalRule21(enum Pixel *line,int32_t length,uint32_t stride,Description *desc,BlocksRanges *ranges);
/*
* Move the block's area so that there is white space
* between black pixels that aren't covered by the block.
* This can only happen near endings of the block.
*/
int32_t nonogramLogicalRule22(enum Pixel *line,int32_t length,uint32_t stride,Description *desc,BlocksRanges *ranges);
/*
* If in block's range has multiple black pixel groups
* then if left or right group is larger then block
* then range can be refined.
*/
int32_t nonogramLogicalRule23(enum Pixel *line,int32_t length,uint32_t stride,Description *desc,BlocksRanges *ranges);
/*
* Reduce the block's range if there is black group between two white pixel
* that can said to be certain block.
* NOTE: NOT part of the An Efficient Algorithm For Solving Nononograms
*/
int32_t nonogramLogicalRule24(enum Pixel *line,int32_t length,uint32_t stride,Description *desc,BlocksRanges *ranges);
/*
* Rule to deduce range by checking does block fit between white spaces.
* NOTE: NOT part of the An Efficient Algorithm For Solving Nononograms
*/
int32_t nonogramLogicalRule25(enum Pixel *line,int32_t length,uint32_t stride,Description *desc,BlocksRanges *ranges);
/*
* If pixel n and m are black and only covered 
* by one block then pixels between n and m 
* are black and block's range can be updated
* to take count that m-n+1 blocks are known.
*/
int32_t nonogramLogicalRule31(enum Pixel *line,int32_t length,uint32_t stride,Description *desc,BlocksRanges *ranges);
/*
* Forevery block check for group's ending with
* white both sides. Paper has rather specific
* algorithm. If such groups's length is less then
* blocks length and is only inside one block's
* range.
*/
int32_t nonogramLogicalRule32(enum Pixel *line,int32_t length,uint32_t stride,Description *desc,BlocksRanges *ranges);
/*
* If the block's ranges don't overlap there is three situation where we can deduce things. In the
*/
int32_t nonogramLogicalRule33(enum Pixel *line,int32_t length,uint32_t stride,Description *desc,BlocksRanges *ranges);

#endif /* _LOGICAL_RULES_H_ */
