#!/bin/bash

WRITER_PROCS=32
READER_PROCS=(64 128) #64
METHOD=("non_blocking_on;" "NON_BLOCKING_ON=tree;" "NON_BLOCKING_ON=ring;" "NON_BLOCKING_ON=ring_of_rings;")
for proc in ${READER_PROCS[@]}
do
    for meth in ${METHOD[@]}
    do
	echo "On method: $meth"
	perl change_blocking_method.pl $meth test_config_flex.xml
	mpirun -n $WRITER_PROCS --hostfile falcon_writer_mpi_hosts ./writer_non_blocking -t flx &
	sleep 15
	mpirun -n $proc		--hostfile falcon_reader_mpi_hosts ./reader_non_blocking -t flx >reader_flexpath_${proc}.out 2>&1 &
	echo "Started all procs!"
	wait
    done
done
