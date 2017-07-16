#!/bin/bash

WRITER_PROCS=32
READER_PROCS=(128) #4 16 8 32 
QUEUE_SIZES=(2 4 8 16 32)
METHOD=("non_blocking_off;" "NON_BLOCKING_ON=tree;" "NON_BLOCKING_ON=ring;" "NON_BLOCKING_ON=ring_of_rings;")
COUNT=0
for proc in ${READER_PROCS[@]}
do
    for meth in ${METHOD[@]}
    do
	for queue in ${QUEUE_SIZES[@]}
	do
	    if [ $COUNT -lt 9 ]
	    then
	        (( COUNT++ ))
	        continue
	    fi
	    if [ $COUNT -lt 10 ]
	    then
	    echo "On method: $meth for queue size: $queue for proc_size: $proc"
	    perl change_queue_size.pl $queue test_config_flex.xml
	    perl change_blocking_method.pl $meth test_config_flex.xml
	    mpirun -n $WRITER_PROCS --hostfile falcon_writer_mpi_hosts ./writer_non_blocking -t flx 2>&1 &
	    sleep 15
	    mpirun -n $proc		--hostfile falcon_reader_mpi_hosts ./reader_non_blocking -t flx 2>&1 | tee reader_flexpath_${proc}.out &
	    echo "Started all procs!"
	    wait
	    (( COUNT++ ))
	    fi
	done
    done
done
echo done
