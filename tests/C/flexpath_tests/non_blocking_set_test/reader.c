/*
 * ADIOS is freely available under the terms of the BSD license described
 * in the COPYING file in the top level directory of this source distribution.
 *
 * Copyright (c) 2008 - 2009.  UT-BATTELLE, LLC. All rights reserved.
 */

/*************************************************************/
/*          Example of reading arrays in ADIOS               */
/*    which were written from the same number of processors  */
/*                                                           */
/*        Similar example is manual/2_adios_read.c           */
/*************************************************************/
#include "adios.h"
#include "mpi.h"
#include "public/adios_read.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "misc.h"
#include "test_common.h"
#include "utils.h"

int main(int argc, char **argv)
{
    int rank, j;
    int size;
    int NX, NY;
    int writer_size;
    double *t;
    double start_time, end_time;
    double start_time_in, end_time_in;
    double individual_timesteps[100];
    MPI_Comm comm = MPI_COMM_WORLD;
    diag_t return_val;
    
    struct err_counts err = {0, 0};
    struct adios_tsprt_opts adios_opts;
    struct test_info test_result = {TEST_PASSED, "non_blocking"};

    GET_ENTRY_OPTIONS(
        adios_opts,
        "Runs readers.");
    MPI_Init(&argc, &argv);
    start_time = MPI_Wtime();
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    SET_ERROR_IF_NOT_ZERO(adios_read_init_method(adios_opts.method, comm,
                                                 adios_opts.adios_options),
                          err.adios);
    RET_IF_ERROR(err.adios, rank);

    /* schedule_read of a scalar. */
    /*if(rank == 1)
    {
	int wait = 1;
	while(wait)
	{
	    //Do nothing
	}
    }
    */
    
    int test_scalar = -1;
    ADIOS_FILE *afile = adios_read_open(FILE_NAME, adios_opts.method, comm,
                                        ADIOS_LOCKMODE_NONE, 0.0);
    //Repeateable random sequence
    int max_sleep;
    srand(rank);

    int ii = 0;
    while (adios_errno != err_end_of_stream) {

	start_time_in = MPI_Wtime();
        /* get a bounding box - rank 0 for now*/
        ADIOS_VARINFO *nx_info = adios_inq_var(afile, "NX");
        ADIOS_VARINFO *ny_info = adios_inq_var(afile, "NY");
        ADIOS_VARINFO *writer_size_info = adios_inq_var(afile, "size");

        if (nx_info->value) {
            NX = *((int *)nx_info->value);
        } else {
            test_failed(test_result.name, rank);
            return_val = DIAG_ERR;
        }

        if (ny_info->value) {
            NY = *((int *)ny_info->value);
        } else {
            test_failed(test_result.name, rank);
            return_val = DIAG_ERR;
        }

        if (writer_size_info->value) {
            writer_size = *((int *)writer_size_info->value);
        } else {
            test_failed(test_result.name, rank);
            return_val = DIAG_ERR;
        }

	if(writer_size < size)
	{
	    fprintf(stderr, "Error: we don't support more readers than writers in this test currently!\n");
	    exit(1);
	}

        int sel_index = rank % writer_size;
        int k;

        /* Allocate space for the arrays */
        ADIOS_SELECTION *global_range_select;
        ADIOS_SELECTION *scalar_block_select;

        int nelem = writer_size * NX * NY;
        size_t arr_size = sizeof(double) * nelem;
        t = (double *)malloc(arr_size);
        memset(t, 0, arr_size);

        uint64_t *start_array = malloc(sizeof(uint64_t) * (NY + 1));
        uint64_t *count_array = malloc(sizeof(uint64_t) * (NY + 1));
	//This directs which writer ranks to read from
	
	uint64_t offset = writer_size / size;
	uint64_t rank_count = ((writer_size - rank * offset) < offset) ? writer_size - rank * offset : offset;
	//printf("Rank: %d --- Writer_size: %d\n",rank, writer_size);
	//printf("Rank: %d --- Reader_size: %d\n",rank,  size);
	//printf("Rank: %d --- Rank_count: %d\n", rank,  rank_count); 
	
	//MPI_Barrier(comm);
	//exit(1);
        start_array[0] = offset * rank;
        count_array[0] = rank_count;
        for (k = 1; k < (NY + 1); ++k) {
            start_array[k] = 0;
            count_array[k] = NX;
        }

        global_range_select =
            adios_selection_boundingbox((NY + 1), start_array, count_array);
        scalar_block_select = adios_selection_writeblock(sel_index);

        /* Read the arrays */
        adios_schedule_read(afile, global_range_select, "var_2d_array", 0, 1,
                            t);
        adios_schedule_read(afile, scalar_block_select, "test_scalar", 0, 1,
                            &test_scalar);

        adios_perform_reads(afile, 1);

	//Pause for some time randomly
	max_sleep = 2;
	double random_number = rand();
	int sleep_time = (int) ( random_number / RAND_MAX * max_sleep) + 1; //Just to avoid the zero sleep_time
	sleep(sleep_time);	
        adios_release_step(afile);
        adios_advance_step(afile, 0, 30);
	end_time_in = MPI_Wtime();
	individual_timesteps[ii] = end_time_in - start_time_in;
        ii++;
    }

just_clean:
    // clean everything
    free(t);
    t = NULL;

    adios_read_close(afile);
    adios_read_finalize_method(adios_opts.method);

    end_time = MPI_Wtime();
    double total_time = end_time - start_time;
    if(rank == 0)
    {
	double * final_times = calloc(size, sizeof(double));
	double * independent_timestep_times = calloc(size * 100, sizeof(double));
	MPI_Gather(&total_time, 1, MPI_DOUBLE, final_times, 1, MPI_DOUBLE, 0, comm);
	MPI_Gather(individual_timesteps, 100, MPI_DOUBLE, independent_timestep_times, 100, MPI_DOUBLE, 0, comm);

	char read_filename[100] = {FILE_NAME "_temp_out.txt"};
	FILE * read_file = fopen(read_filename, "r");
	int queue_size;
	char graph_type[25];
	fscanf(read_file, "%d", &queue_size);
	fscanf(read_file, "%s", graph_type);
	fclose(read_file);
	unlink(read_filename);
	time_t t = time(NULL);
	struct tm * calendar = localtime(&t);
	char filename[200];
	sprintf(filename, "%s_%d-%d-%d_%d:%d:%d_%d-readers_%d-writers_%d-max_sleep_%d-queue_size_.txt", graph_type, calendar->tm_mon, 
			    calendar->tm_mday, calendar->tm_year, calendar->tm_hour, calendar->tm_min, calendar->tm_sec, 
			    size, writer_size, max_sleep, queue_size);
	FILE * open_file = fopen(filename, "w");
	int i, k;
	for(i = 0; i < size; i++)
	    fprintf(open_file, "Rank: %d\t\t\t", i);
	fprintf(open_file, "\nFINAL_TIMES\n");
	
	for(i = 0; i < size; i++)
	    fprintf(open_file, "%f\t\t", final_times[i]);
	fprintf(open_file, "\nINDIVIDUAL_TIMES\n");

	for(k = 0; k < 100; k++)
	{
	    for(i = 0; i < size; i++)
	    {
		fprintf(open_file, "%f\t\t", independent_timestep_times[k + i * 100]);
	    }
	    fprintf(open_file, "\n");
	}

	fclose(open_file);
	
	
	
    }
    else
    {
	MPI_Gather(&total_time, 1, MPI_DOUBLE, NULL, 1, MPI_DOUBLE, 0, comm);
	MPI_Gather(individual_timesteps, 100, MPI_DOUBLE, NULL, 100, MPI_DOUBLE, 0, comm);

    }



close_adios:
    MPI_Finalize();
    return 0;
}
