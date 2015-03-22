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

all: prefixsum_serial prefixsum_openmp prefixsum_mpi

#
# Serial prefix sum program
#
prefixsum_serial:prefixsum_serial.cpp
	$(CC) $(CFLAGS) $(DFLAGS) -o $@  $@.cpp

#
# OpenMP prefix sum program
#

prefixsum_openmp:prefixsum_openmp.cpp
	$(CC) $(CFLAGS) $(DFLAGS) -openmp  -o $@  $@.cpp

#
# MPI prefix sum program
#

prefixsum_mpi:prefixsum_mpi.cpp
	$(MPICC) $(CFLAGS) $(DFLAGS) -o $@ $@.cpp

#
# clean up
#
clean:
	rm prefixsum_serial prefixsum_openmp prefixsum_mpi > /dev/null 2>&1
