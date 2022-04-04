/*
* Functions to check nonogram solution is valid.
* Nonogram is said to be nonvalid if it has no solution
* because obviuos description problem.
*/
#ifndef _VALIDITY_CHECK_H_
#define _VALIDITY_CHECK_H_

#include<stdint.h>
#include"Nonograms.h"

/*
* Nonogram's solution is valid if solution
* agrees to pixels. Partial means pixel
* agree with descriptions but has unknown
* pixels. Invalid means there is black pixel
* at the wrong place making solution disagree
* with at least one of the nonogram's
* descriptions.
*/
typedef enum NonogramsDimensionValidness{
	NONO_SOLUTION_IS_VALID,
	NONO_SOLUTION_IS_PARTIAL,
	NONO_SOLUTION_IS_INVALID,
	NONO_SOLUTION_IS_CALLBACK_ERROR
}NonogramsDimensionValidness;

/*
* Type of edges solutionDimensionIsValid detects.
*/
typedef enum{
	NONO_EDGE_STATE_NEXTTO,
	NONO_EDGE_STATE_FULL_BLACK,
	NONO_EDGE_STATE_FULL_WHITE,
	NONO_EDGE_STATE_MIX,
}EdgeState;
/*
* Structure for storing edge state.
*/
typedef struct{
	struct{
		int32_t blackcount;
		int32_t maxblackcount;
		int32_t groupcount;
	}stateinfo;
	EdgeState state;
}EdgeValue;

/*
* Callback funtion when validation algorithm hits unknown pixels.
*/
typedef uint8_t UnknownCallback(Pixel *pixel,int32_t row,int32_t col,void *callbackdata,EdgeValue *edge);

/*
* Check that there is no obviuos description
* problem making nonogram have no solution.
*/
int8_t nonogramIsValid(Nonogram *nono);
/*
* Check that solution is valid. Tell also if
* solution is partial and user can give optional
* callback function how to handle unknown pixels.
*
* Parameters:
* nono is the nonogram which we are using.
* table is the [partial] solution to be validated,
* roworcol is false if lines are rows and if true lines are columns.
* func is callback function for handle unknown pixels. Can be NULL
* for disable the feature.
* callbackdata is user defined parameter to every time func is called.
*/
NonogramsDimensionValidness solutionDimensionIsValid(const Nonogram *restrict nono,const Table *restrict table,const uint8_t roworcol,UnknownCallback *func,void *callbackdata);

#endif
