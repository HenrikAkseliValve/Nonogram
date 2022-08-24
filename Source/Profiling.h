/*
* Module for collecting profiling data manually.
*/
#ifndef _PROFILING_H_
#define _PROFILING_H_

/*
* Initialize profiling. Location for profiling
* data output is given in the path variable.
*/
void initProfiling(int32_t index,int32_t numnewsamples,char *path);

/*
* Start new sample.
*/
void newSample(int32_t index,int32_t abscissa);

/*
* Start counting time at index.
*/
void tickProfiling(int32_t index,int32_t column);

/*
* End counting time at index.
*/
void tockProfiling(int32_t index,int32_t column);

/*
* Finalize profiling result as a plot file.
* By default append the file.
*/
void finalizePlot(int32_t index,int32_t numcols);

#endif /* _PROFILING_H_ */
