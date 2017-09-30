#!/bin/bash

WRITER_PROCS=64
READER_PROCS=( 4 8 16 32 64)
QUEUE_SIZES=(2 4 8 16 32)
METHOD=("non_blocking_off;" "NON_BLOCKING_ON=tree;" "NON_BLOCKING_ON=ring;" "NON_BLOCKING_ON=ring_of_rings;")
COUNT=0
for proc in ${READER_PROCS[@]}
do
    for meth in ${METHOD[@]}
    do
	for queue in ${QUEUE_SIZES[@]}
	do
	    if [ $COUNT -lt 5 ]
	    then
		(( COUNT++ ))
		continue
	    fi
	    echo "On method: $meth for queue size: $queue for proc_size: $proc"
	    perl change_queue_size.pl $queue test_config_flex.xml
	    perl change_blocking_method.pl $meth test_config_flex.xml
	    #export FLEXPATH_VERBOSE=1
	    mpirun -n $WRITER_PROCS --hostfile falcon_writer_mpi_hosts ./writer_non_blocking_set -t flx &
	    sleep 15
	    mpirun -n $proc -x WEIR_VERBOSE --hostfile falcon_reader_mpi_hosts ./reader_non_blocking_set -t flx 2>&1 | tee reader_flexpath_${proc}.out &
	    echo "Started all procs!"
	    wait
	done
    done
done
echo done
