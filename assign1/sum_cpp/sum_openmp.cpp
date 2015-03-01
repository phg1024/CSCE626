/*
 *  sum_openmp.cpp - Demonstrates parallelism via random fill and sum routines
 *                   This program uses OpenMP.
 */

/*---------------------------------------------------------
 *  Parallel Prefix Sum
 *
 *  1. Each thread generates roughly numints_per_proc random integers (in parallel OpenMP region)
 *  2. Each thread computes the prefix sum of his numints_per_proc random integers (in parallel OpenMP region)
 *  3  One thread computes the prefix sum of the partial results.
 *  4. Each thread add back the partial prefix sum to his numints_per_proc prefix sums.
 *
 *  NOTE: steps 2-3 are repeated as many times as requested (numiterations)
 *---------------------------------------------------------*/


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <omp.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <iterator>
#include <numeric>
#include <cmath>
using namespace std;

/*==============================================================
 * compute elapsed time between start and end
 *==============================================================*/
long elapsed(struct timeval *start, struct timeval *end) {
  struct timeval elapsed;
  /* calculate elapsed time */
  if(start->tv_usec > end->tv_usec) {
    end->tv_usec += 1000000;
    end->tv_sec--;
  }
  elapsed.tv_usec = end->tv_usec - start->tv_usec;
  elapsed.tv_sec  = end->tv_sec  - start->tv_sec;
  return elapsed.tv_sec*1000000 + elapsed.tv_usec;
}

/*==============================================================
 * print_elapsed (prints timing statistics)
 *==============================================================*/
void print_elapsed(const char* desc, struct timeval* start, struct timeval* end, int niters) {
  printf("\n %s total elapsed time = %ld (usec)",
    desc, elapsed(start, end) / niters);
}


/*==============================================================
 *  Main Program (Parallel Summation)
 *==============================================================*/
int main(int argc, char *argv[]) {

  int numints = 0;
  int numiterations = 0;
  int numprocs = 0;
  int numints_per_proc = 0;

  vector<long> data;
  vector<long> partial_sums;
  vector<long> prefix_sums;

  struct timeval start, end;   /* gettimeofday stuff */
  struct timezone tzp;

  if( argc < 3) {
    printf("Usage: %s [numprocs] [numints] [numiterations]\n\n", argv[0]);
    exit(1);
  }

  numprocs      = atoi(argv[1]);
  numints       = atoi(argv[2]);
  numiterations = atoi(argv[3]);
  numints_per_proc = ceil(numints / (float)numprocs);

  printf("\nExecuting %s: nthreads=%d, numints=%d, numints_per_proc=%d, numiterations=%d\n",
            argv[0], numprocs, numints, numints_per_proc, numiterations);

  /* Allocate shared memory, enough for each thread to have numints*/
  data.resize(numints);

  /* Allocate shared memory for partial_sums */
  partial_sums.resize(numprocs+1, 0);

  /* Set number of threads */
  omp_set_num_threads(numprocs);

  /*****************************************************
   * Generate the random ints in parallel              *
   *****************************************************/
  #pragma omp parallel shared(numints_per_proc, data)
  {
    int tid;

    /* get the current thread ID in the parallel region */
    tid = omp_get_thread_num();

    srand(tid + time(NULL));    /* Seed rand functions */

    int pos0 = tid*numints_per_proc;
    int pos1 = std::min(pos0+numints_per_proc, numints);

    vector<long>::iterator it_cur = data.begin() + pos0;
    vector<long>::iterator it_end = data.begin() + pos1;

    for(; it_cur != it_end ; ++it_cur) {
      *it_cur = rand();
    }
  }

  /* Make a copy of data as input for prefix sums */

  /*****************************************************
   * Generate the sum of the ints in parallel          *
   * NOTE: Repeated for numiterations                  *
   *****************************************************/

  long totalTime = 0;
  for(int iteration=0; iteration < numiterations; ++iteration) {
    prefix_sums = data;

    gettimeofday(&start, &tzp);
    #pragma omp parallel shared(numints_per_proc,prefix_sums,partial_sums)
    {
      int tid;

      /* get the current thread ID in the parallel region */
      tid = omp_get_thread_num();

      /* Compute the local prefix sums */
      int pos0 = tid*numints_per_proc;
      int pos1 = std::min(pos0+numints_per_proc, numints);

      for(int pos = pos0+1;pos<pos1;++pos) 
          prefix_sums[pos] += prefix_sums[pos-1];
      partial_sums[tid+1] = prefix_sums[pos1-1];
    }

    /* Compute the prefix sum of the partial sums */
    for(int i=1;i<partial_sums.size();++i) partial_sums[i] += partial_sums[i-1];

    #pragma omp parallel shared(numints_per_proc,prefix_sums,partial_sums)
    {
      int tid;

      /* get the current thread ID in the parallel region */
      tid = omp_get_thread_num();

      /* get the prefix sum of the partial sums */
      long ps = partial_sums[tid];

      /* Compute the local prefix sums */
      int pos0 = tid*numints_per_proc;
      int pos1 = std::min(pos0+numints_per_proc, numints);

      /* add it back to the prefix sums */
      for(int pos=pos0;pos<pos1;++pos) prefix_sums[pos] += ps;
    }

    gettimeofday(&end,&tzp);
    totalTime += elapsed(&start, &end);
  }


  /*****************************************************
   * Output timing results                             *
   *****************************************************/

  std::cout << "Total elapsed time = " << totalTime / (double)numiterations << " (usec)" << std::endl; 
  std::cout << std::endl;

  std::ostream_iterator<int> out_it (std::cout," ");
  std::cout << "Input sequence: ";
  std::copy(data.begin(), data.end(), out_it);
  std::cout << std::endl;

  std::cout << "Prefix sum: ";
  std::copy(prefix_sums.begin(), prefix_sums.end(), out_it);
  std::cout << std::endl;

  /* Verify the result */
  vector<long> result_gold(data.size());
  std::partial_sum(data.begin(), data.end(), result_gold.begin());
  if( std::equal(result_gold.begin(), result_gold.end(), prefix_sums.begin()) ) {
    std::cout << "PASSED." << std::endl;
  }
  else {
    std::cout << "Reference prefix sum: ";
    std::copy(result_gold.begin(), result_gold.end(), out_it);
    std::cout << std::endl;
    std::cout << "FAILED." << std::endl;
    for(int i=0;i<result_gold.size();++i) {
      if( result_gold[i] != prefix_sums[i] ) {
        std::cout << i << "\t" << prefix_sums[i] << "\t" << result_gold[i] << std::endl;
      }
    }
  }

  return(0);
}
