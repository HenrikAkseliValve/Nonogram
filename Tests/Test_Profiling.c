/*
* Test file profiling tool.
*/
#include<stdint.h>
#include<stdlib.h>
#include<unistd.h>
#include"../Source/Profiling.h"
#define FIRST_INDEX_SAMPLE_NUM 4

int main(int argn,char *argv[]){

	if(argn<2) return 0;

	int32_t abscissa0=atoi(argv[1]);
	int32_t abscissa1=abscissa0;
	int32_t finalcount=0;

	// Create the profiles
	initProfiling(0,1,"/tmp/test.0.txt");
	initProfiling(1,FIRST_INDEX_SAMPLE_NUM,"/tmp/test.1.txt");

	// Start new sample for zeroth index.
	newSample(0,abscissa0);
	// Zeroth sample's zeroth column is overall time
	tickProfiling(0,0);
	{
		// Simple count measuring.
		tickProfiling(0,1);
		int32_t count1=0;
		for(int32_t i=abscissa0;i>0;i--) count1++;
		tockProfiling(0,1);

		// Measuring two times three times counting.
		tickProfiling(0,2);
		int32_t count2=0;
		for(int32_t i=2*abscissa0;i>0;i--){
			for(int32_t j=3;j>0;j--){
				count2++;
			}
		}
		tockProfiling(0,2);

		// Multisample count sampling.
		tickProfiling(0,3);
		for(int k=FIRST_INDEX_SAMPLE_NUM;k>0;k--){
			newSample(1,abscissa1);
			tickProfiling(1,0);
			for(int32_t i=abscissa1;i>0;i--){
				finalcount++;
			}
			tockProfiling(1,0);
			abscissa1+=abscissa0*count1+count2-k;
		}
		tockProfiling(0,3);
	}
	tockProfiling(0,0);

	// Finalize the plots.
	finalizePlot(0,3);
	finalizePlot(1,1);

	if(finalcount>9000){
		write(STDOUT_FILENO,"Final count is over 9000!\n",26);
	}
	return 0;
}
