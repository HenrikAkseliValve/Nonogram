/*
* Module for collecting profiling data manually.
*
* Allocate memory for the data? What data is needed?
*
* Thesis needed:
* 1) NonoPartialSolver (just to compare with the citation) X=amount of pixels
* 2) NonoVerifier + NonoProducePixelGraphWithNonOverlapDivisionAndEdgeInfo X=amount of pixels
* 3) NonoDetectOneBlackColourableOnePixelSquareSwitchingComponent X=size of switching component
* 4) Total complexity X=amount of pixels.
*
* Just allocate four new sampling data? No there can be multiple calls to NonoDetect*
* per run. It is know at run time so initialize sample count later? There maybe more
* detectors later which all are complex in terms of size of switching component.
*/
#include<stdint.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/uio.h>
#include<sys/stat.h>
#include<stdlib.h>
#include<time.h>
#include"Etc.h"
#include"Profiling.h"

/*
* One sample structure.
*/
struct Sample{
	int32_t abscissa;
	struct timespec startstamps[3];
	struct timespec endstamps[3];
};

/*
* Open the file to append.
*/
static int32_t FileDescriptions[2];
/*
* Array of pointers for the samples.
*/
static struct Sample *Samples[2];
/*
* Sample offset.
*/
static int32_t SampleOffset[2];

void initProfiling(int32_t index,int32_t numnewsamples,char *path){
	// Open the data file.
	FileDescriptions[index]=open(path,O_WRONLY|O_APPEND|O_CREAT,S_IWUSR|S_IRUSR);
	if(FileDescriptions[index]<0){
		(void)write(STDERR_FILENO,"ERROR: Profiling failed to open data file!\n",43);
		exit(1000);
	}
	// Allocate new samples.
	Samples[index]=malloc(sizeof(struct Sample)*numnewsamples);
	SampleOffset[index]=-1;
}

void newSample(int32_t index, int32_t abscissa){
	SampleOffset[index]++;
	Samples[index][SampleOffset[index]].abscissa=abscissa;
}

void tickProfiling(int32_t index,int32_t column){
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID,Samples[index][SampleOffset[index]].startstamps+column);
}

void tockProfiling(int32_t index,int32_t column){
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID,Samples[index][SampleOffset[index]].endstamps+column);
}

// Function fo calculate the difference between timespecs.
static void diffTimespec(const struct timespec *restrict s,const struct timespec *restrict e,struct timespec *restrict result) {
	// Calculate the difference.
	result->tv_sec=e->tv_sec-s->tv_sec;
	result->tv_nsec=e->tv_nsec-s->tv_nsec;

	// Check that nsec of s was more than e.
  if (result->tv_nsec<0){
    result->tv_nsec+=1000000000;
    result->tv_sec--;
  }
}

void finalizePlot(int32_t index,int32_t numcols){
	// Variable diffresult is memory fo the
	struct timespec diffresult;
	// Since every timestamp is second and nanosecond
	// member of 64bits of size. Hence we have
	// 2*numcols of 18 buffers for one sample.
	uint8_t decbuffer[numcols][18*2];
	uint8_t abscissabuffer[11];
	const char seperator[1]={' '};
	const char newsample[1]={'\n'};
	// We output per sample which means 18
	struct iovec output[2*numcols+2];

	{
		// Mark the separator spots.
		int32_t col;
		for(col=0;col<numcols;col++){
			output[2*col+1].iov_base=(void*)seperator;
			output[2*col+1].iov_len=1;
		}
		// Mark the new sample
		output[2*col+1].iov_base=(void*)newsample;
		output[2*col+1].iov_len=1;
	}

	// Go through samples.
	//TODO: resetting iov_base is unnecessary.
	for(int32_t sample=0;sample<=SampleOffset[index];sample++){

		// Zeroth index of output is the abscissa of the sample.
		{
			uint32_t intlen=i32toalen(Samples[index][sample].abscissa);
			i32toa(Samples[index][sample].abscissa,abscissabuffer,intlen);
			output[0].iov_base=abscissabuffer;
			output[0].iov_len=intlen;
		}

		for(int32_t col=0;col<numcols;col++){
			// Calculate the difference of stamps.
			diffTimespec(&Samples[index][sample].startstamps[col],&Samples[index][sample].endstamps[col],&diffresult);

			// Write the difference result as number of nanoseconds.
			// This means tv_sec is written first then tv_nsec so
			// so that number is comes in right order. In-between
			// may need zero padding.
			//
			// DO NOT WRITE tv_sec IF IT IS ZERO. This would cause
			// additional zero front of the number.
			output[2*col+2].iov_base=decbuffer[col];
			if(diffresult.tv_sec!=0){
				// Write seconds first.
				uint32_t intlen=i64toalen(diffresult.tv_sec);
				i64toa(diffresult.tv_sec,decbuffer[col],intlen);
				output[2*col+2].iov_len=intlen;
				// Nanoseconds have to written so that zeros are added
				// if number is too low.
				// 9 here comes from fact that 10^9 nanoseconds is seconds
				// hence tv_nsec is number between 0 to 999'999'999.
				intlen=i64toalen(diffresult.tv_nsec);
				for(uint32_t i=0;i<(9-intlen);i++){
					decbuffer[col][output[2*col+2].iov_len+i]='0';
				}

				// Write the number after zeros have been added.
				i64toa(diffresult.tv_nsec,decbuffer[col]+output[2*col+2].iov_len+9-intlen,intlen);
				output[2*col+2].iov_len+=9;
			}
			else{
				// Just write nano seconds
				uint32_t intlen=i64toalen(diffresult.tv_nsec);
				i64toa(diffresult.tv_nsec,decbuffer[col],intlen);
				output[2*col+2].iov_len=intlen;
			}
		}
		// Write the result.
		(void)writev(FileDescriptions[index],output,2*numcols+2);
	}
}
