#
# makefile for summation demo programs
#
# prompt> make
#
CC     = icpc     # the c compiler to use
MPICC  = mpic++  # the MPI cc compiler
CFLAGS = -O3 -std=c++11  # optimize code
DFLAGS =         # common defines

default:all

all: sum_openmp sum_mpi sum_mpi2 test

#
# OpenMP summation demo program
#

sum_openmp:sum_openmp.cpp
	$(CC) $(CFLAGS) $(DFLAGS) -openmp  -o $@  $@.cpp

#
# MPI summation demo programs
#

sum_mpi:sum_mpi.cpp
	$(MPICC) $(CFLAGS) $(DFLAGS) -o $@ $@.cpp

sum_mpi2:sum_mpi2.cpp
	$(MPICC) $(CFLAGS) $(DFLAGS) -o $@ $@.cpp

test:test.cpp
	$(CC) -std=c++11 $(CLFAGS) $(DFLAGS) -o $@ $@.cpp
#
# clean up
#
clean:
	rm sum_openmp sum_mpi sum_mpi2 test > /dev/null 2>&1
