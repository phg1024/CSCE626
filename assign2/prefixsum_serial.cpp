/*
 *  sum_serial.cpp - Demonstrates parallelism via random fill and sum routines
 *                   This program uses OpenMP.
 */

/*---------------------------------------------------------
 *  Serial Prefix Sum
 *---------------------------------------------------------*/


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/resource.h>
#include <sys/time.h>
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

    vector<long> data;
    vector<long> prefix_sums;

    struct timeval start, end;   /* gettimeofday stuff */
    struct timezone tzp;

    if( argc < 3) {
        printf("Usage: %s [numints] [numiterations]\n\n", argv[0]);
        exit(1);
    }

    numints       = atoi(argv[1]);
    numiterations = atoi(argv[2]);

    printf("\nExecuting %s: numints=%d, numiterations=%d\n",
            argv[0], numints, numiterations);

    /* Allocate memory for the input sequence */
    data.resize(numints);

    /*****************************************************
     * Generate the random ints                          *
     *****************************************************/
    srand(tid + time(NULL));    /* Seed rand functions */
    std::for_each(data.begin(), data.end(), [](long &x){ x = rand(); });

    /* Make a copy of data as input for prefix sums */

    /*****************************************************
     * Generate the sum of the ints                      *
     * NOTE: Repeated for numiterations                  *
     *****************************************************/

    long totalTime = 0;
    for(int iteration=0; iteration < numiterations; ++iteration) {
        prefix_sums = data;

        gettimeofday(&start, &tzp);

        /* Compute the prefix sum */
        for(int i=1;i<numints;++i) prefix_sums[i] += prefix_sums[i-1];

        gettimeofday(&end,&tzp);
        totalTime += elapsed(&start, &end);
    }


    /*****************************************************
     * Output timing results                             *
     *****************************************************/

    std::cout << "Total elapsed time = " << totalTime / (double)numiterations << " (usec)" << std::endl;
    std::cout << std::endl;

    std::ostream_iterator<long> out_it (std::cout," ");
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