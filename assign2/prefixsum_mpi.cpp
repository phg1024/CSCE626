/*
 *  prefixsum_mpi.cpp - Demonstrates parallelism via random fill and prefix sum
 *  routines.
 *  This program uses MPI.
 */

/*---------------------------------------------------------
 *  Parallel Prefix Sum
 *
 *  1. Processor 0 generates numints random integers
 *  2. Processor 0 distributes the integers to all processors
 *  3. Prefix sums:
 *  3.1 Each processor computes prefix sums of his numints random integers (in parallel)
 *  3.2 All the processes send the last element of their prefix sums to Processor 0
 *  3.3 Processor 0 receives the partial sums from all the other processes.
 *  3.3 Processor 0 computes prefix sums of the partial sums (sequentially)
 *  3.4 Processor 0 sends the result to all other processors
 *  3.5 All processors add the received number to local prefix sums.
 *
 *  NOTE: steps 3 are repeated as many times as requested (numiterations)
 *---------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <mpi.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <iostream>
#include <iterator>
#include <numeric>

using namespace std;

/*==============================================================
 * p_generate_random_ints (processor-wise generation of random ints)
 *==============================================================*/
void p_generate_random_ints(vector<long>& memory, int n) {
  /* generate & write this processor's random integers */
  for (int i = 0; i < n; ++i) {
    memory.push_back(rand());
  }
}

void p_prefix_sum(vector<long>& memory) {
  for(int i=1;i<memory.size();++i) {
    memory[i]+=memory[i-1];
  }
}

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
int main(int argc, char **argv) {

  int nprocs, numints, numints_per_proc, numiterations; /* command line args */
  bool write_outputs = false;

  int my_id, iteration;

  vector<long> gmemory; /* vector to store the input sequence */
  vector<long> results;  /* vector to store the results */
  vector<long> mymemory; /* Vector to store processes numbers */
  vector<long> partial_sums;
  long* buffer;         /* Buffer for inter-processor communication */

  struct timeval gen_start, gen_end; /* gettimeofday stuff */
  struct timeval start, end;         /* gettimeofday stuff */
  struct timezone tzp;

  MPI_Status status;              /* Status variable for MPI operations */

  /*---------------------------------------------------------
   * Initializing the MPI environment
   * "nprocs" copies of this program will be initiated by MPI.
   * All the variables will be private, only the program owner could see its own variables
   * If there must be a inter-procesor communication, the communication must be
   * done explicitly using MPI communication functions.
   *---------------------------------------------------------*/

  MPI::Init();
  //MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_id); /* Getting the ID for this process */

  /*---------------------------------------------------------
   *  Read Command Line
   *  - check usage and parse args
   *---------------------------------------------------------*/

  if(argc < 3) {

    if(my_id == 0)
      printf("Usage: %s [numints] [numiterations]\n\n", argv[0]);

    MPI_Finalize();
    exit(1);
  }

  numints       = atoi(argv[1]);
  numiterations = atoi(argv[2]);

  if(argc < 4) {
    write_outputs = false;
  }
  else {
    write_outputs = (string(argv[3]) == "-o");
  }

  MPI_Comm_size(MPI_COMM_WORLD, &nprocs); /* Get number of processors */

  numints_per_proc = ceil(numints / (float)nprocs);
  if( my_id == 0 ) {
    partial_sums.resize(nprocs+1, 0);
  }

  if(my_id == 0)
    printf("\nExecuting %s: nprocs=%d, numints=%d, numints_per_proc=%d, numiterations=%d\n",
           argv[0], nprocs, numints, numints_per_proc, numiterations);

  /*---------------------------------------------------------
   *  Initialization
   *  - allocate memory for work area structures and work area
   *---------------------------------------------------------*/
  if( my_id == 0 ) {
    gmemory.reserve(numints);
    results.resize(numints);
    /* get starting time */
    gettimeofday(&gen_start, &tzp);
    srand(my_id + time(NULL));                  /* Seed rand functions */
    p_generate_random_ints(gmemory, numints);  /* random parallel fill */
    gettimeofday(&gen_end, &tzp);
    print_elapsed("Input generated", &gen_start, &gen_end, 1);
  }

  int myint_first = my_id * numints_per_proc;
  int myint_last = std::min(myint_first+numints_per_proc, numints);

  if( myint_first < myint_last )
    mymemory.resize(myint_last - myint_first);
  else
    mymemory.resize(1); // hold a dummy value
  buffer = (long *) malloc(sizeof(long));

  if(buffer == NULL) {
    printf("Processor %d - unable to malloc()\n", my_id);
    MPI_Finalize();
    exit(1);
  }

  long totalTime = 0;

  MPI_Barrier(MPI_COMM_WORLD); /* Global barrier */

  /* repeat for numiterations times */
  for (iteration = 0; iteration < numiterations; iteration++) {
    /* Pass the input sequence to all processors */
    if( my_id == 0 ) {
      // send out the integers
      for(int i=1;i<nprocs;++i) {
        int pos0 = i * numints_per_proc;
        int pos1 = std::min(pos0+numints_per_proc, numints);
        if( pos0 < pos1 )
          MPI_Send(&gmemory[pos0], pos1-pos0, MPI_LONG, i, 0, MPI_COMM_WORLD);
      }
      std::copy(gmemory.begin(), gmemory.begin()+numints_per_proc, mymemory.begin());
    }
    else {
      int pos0 = my_id * numints_per_proc;
      int pos1 = std::min(pos0+numints_per_proc, numints);
      if( pos0 < pos1 )
        MPI_Recv(&mymemory[0], pos1-pos0, MPI_LONG, 0, 0, MPI_COMM_WORLD, &status);
    }

    /* Make sure everybody gets the data */
    MPI_Barrier(MPI_COMM_WORLD);

    gettimeofday(&start, &tzp);

    p_prefix_sum(mymemory); /* Compute the local prefix sum */

    /*---------------------------------------------------------------------
     * Procesor-wise sums are sent by all the other processors to the master procesor
     * Master Procesor receives the local sums form all the other processors.
     *-------------------------------------------------------------------*/

    if (my_id == 0) {
      /*this is the master processor*/
      /*get the partial sum value from every body*/
      for(int i = 1; i < nprocs; ++i) {

        /* Receive the message from the ANY processor */
        /* The message is stored into "buffer" variable */
        MPI_Recv(buffer, 1, MPI_LONG, i, 0, MPI_COMM_WORLD, &status);

        /* Add the processor-wise sum to the total sum */
        partial_sums[i+1] = *buffer;
      }
    }
    else {
      /* this is not the master processor */
      /* Send the local sum to the master process, which has ID = 0 */
      long local_prefix = mymemory.back();
      MPI_Send(&local_prefix, 1, MPI_LONG, 0, 0, MPI_COMM_WORLD);
    }

    /* Make sure all partial sums are sent to master */
    MPI_Barrier(MPI_COMM_WORLD);

    /* Computer prefix sum of the partial sums */
    if( my_id == 0 ) {
      partial_sums[1] = mymemory.back();
      for(int i=1;i<nprocs+1;++i) {
        partial_sums[i] += partial_sums[i-1];
      }
    }

    /* Master send back the prefix sum of partial sums */
    if( my_id == 0 ) {
      for(int i=1;i<nprocs;++i) {
        MPI_Send(&partial_sums[i], 1, MPI_LONG, i, 0, MPI_COMM_WORLD);
      }
    }
    else {
      MPI_Recv(buffer, 1, MPI_LONG, 0, 0, MPI_COMM_WORLD, &status);
    }

    /* Every node except master add back partial prefix sum */
    if( my_id > 0 ) {
      for(int i=0;i<mymemory.size();++i) {
        mymemory[i] += *buffer;
      }
    }

    /* Make sure every node finishes the computation */
    MPI_Barrier(MPI_COMM_WORLD);

    if(my_id == 0) {
      gettimeofday(&end,&tzp);

      totalTime += elapsed(&start, &end);
    }
  }

  /* Pass the results back to master */
  if( my_id == 0 ) {
    // send out the integers
    for(int i=1;i<nprocs;++i) {
      int pos0 = i * numints_per_proc;
      int pos1 = std::min(pos0+numints_per_proc, numints);
      if( pos0 < pos1 )
        MPI_Recv(&results[pos0], pos1-pos0, MPI_LONG, i, 0, MPI_COMM_WORLD, &status);
    }

    std::copy(mymemory.begin(), mymemory.end(), results.begin());
  }
  else {
    int pos0 = my_id * numints_per_proc;
    int pos1 = std::min(pos0+numints_per_proc, numints);
    if( pos0 < pos1 )
      MPI_Send(&mymemory[0], pos1-pos0, MPI_LONG, 0, 0, MPI_COMM_WORLD);
  }
  /* Make sure master gets all partial results */
  MPI_Barrier(MPI_COMM_WORLD);


  if( my_id == 0 ) {
    std::cout << std::endl;
    std::cout << "Total elapsed time = " << totalTime / (double)numiterations << " (usec)" << std::endl;
    std::cout << std::endl;

    if( write_outputs ) {
      /* Write results */
      std::cout << "Input sequence: " << std::endl;
      std::ostream_iterator<long> out_it(std::cout, " ");
      std::copy(gmemory.begin(), gmemory.end(), out_it);
      std::cout << std::endl;

      std::cout << "Prefix sums: " << std::endl;
      std::copy(results.begin(), results.end(), out_it);
      std::cout << std::endl;
    }

    /* Verify the result */
    vector<long> result_gold(gmemory.size());
    std::partial_sum(gmemory.begin(), gmemory.end(), result_gold.begin());
    if (std::equal(result_gold.begin(), result_gold.end(), results.begin())) {
      std::cout << "PASSED." << std::endl;
    }
    else {
      std::cout << "FAILED." << std::endl;
      if( write_outputs ) {
        std::cout << "Reference prefix sum: ";
        std::ostream_iterator<long> out_it(std::cout, " ");
        std::copy(result_gold.begin(), result_gold.end(), out_it);
        std::cout << std::endl;
        for (int i = 0; i < result_gold.size(); ++i) {
          if (result_gold[i] != results[i]) {
            std::cout << i << "\t" << results[i] << "\t" << result_gold[i] << std::endl;
          }
        }
      }
    }
  }

  /*---------------------------------------------------------
   *  Cleanup
   *  - free memory
   *---------------------------------------------------------*/

  /* free memory */
  free(buffer);

  MPI_Finalize();

  return 0;
} /* main() */
