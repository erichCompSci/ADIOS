#!/bin/bash

#PBS -N test_weir
#PBS -A CSC143 
#PBS -l nodes=67,walltime=2:30:00 
#PBS -j oe
#PBS -o both.out


###
# -------------------- set the environment -------------------- #
###

# change to the directory where we were launched
cd $PBS_O_WORKDIR

# get the current working directory
export cwd=`pwd`
echo ${cwd}
# timestamp
date

# load modules
module swap PrgEnv-pgi PrgEnv-gnu
module load cudatoolkit
module load papi
module load cmake
module load perl

#export FLEXPATH_VERBOSE=1

WRITER_PROCS=32
READER_PROCS=(4 16 32 64 128 256 1040)
QUEUE_SIZES=(4 16 32 64)
METHOD=("NON_BLOCKING_ON=tree;")  #"non_blocking_off;" "NON_BLOCKING_ON=ring;" "NON_BLOCKING_ON=ring_of_rings;")
COUNT=0
N_COUNT=16
for meth in ${METHOD[@]}
do
    for proc in ${READER_PROCS[@]}
    do
	for queue in ${QUEUE_SIZES[@]}
	do
            if [ $proc -lt 16 ]
            then
                N_COUNT=4
            else
                N_COUNT=16
            fi
	    echo "On method: $meth for queue size: $queue for proc_size: $proc"
	    perl change_queue_size.pl $queue test_config_flex.xml
	    perl change_blocking_method.pl $meth test_config_flex.xml
	    aprun  -n $WRITER_PROCS -N $N_COUNT ./writer_non_blocking -t flx >writer_${meth%;}_${queue}_${READER_PROCS}.out &
	    sleep 15
	    aprun  -n $proc  -N $N_COUNT ./reader_non_blocking -t flx >reader_${meth%;}_${queue}_${READER_PROCS}.out &
	    echo "Started all procs!"
	    wait
	done
    done
done
echo done
