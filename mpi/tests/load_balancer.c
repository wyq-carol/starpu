/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2017  CNRS
 *
 * StarPU is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * StarPU is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU Lesser General Public License in COPYING.LGPL for more details.
 */

#include <starpu_mpi.h>
#include <starpu_mpi_lb.h>
#include "helper.h"

#if !defined(STARPU_HAVE_UNSETENV)

#warning unsetenv is not defined. Skipping test
int main(int argc, char **argv)
{
	return STARPU_TEST_SKIPPED;
}
#else

void get_neighbors(int **neighbor_ids, int *nneighbors)
{
	int ret, rank, size;
	starpu_mpi_comm_rank(MPI_COMM_WORLD, &rank);
	starpu_mpi_comm_size(MPI_COMM_WORLD, &size);
	*nneighbors = 1;
	*neighbor_ids = malloc(sizeof(int));
	*neighbor_ids[0] = rank==size-1?0:rank+1;
}

void get_data_unit_to_migrate(starpu_data_handle_t **handle_unit, int *nhandles, int dst_node)
{
	*nhandles = 0;
}

int main(int argc, char **argv)
{
	int ret;
	struct starpu_mpi_lb_conf itf;

	itf.get_neighbors = get_neighbors;
	itf.get_data_unit_to_migrate = get_data_unit_to_migrate;

	MPI_Init(&argc, &argv);
	ret = starpu_init(NULL);
	STARPU_CHECK_RETURN_VALUE(ret, "starpu_init");
	ret = starpu_mpi_init(NULL, NULL, 0);
	STARPU_CHECK_RETURN_VALUE(ret, "starpu_mpi_init");

	unsetenv("STARPU_MPI_LB");
	starpu_mpi_lb_init(NULL, NULL);
	starpu_mpi_lb_shutdown();

	starpu_mpi_lb_init("heat", &itf);
	starpu_mpi_lb_shutdown();

	starpu_mpi_shutdown();
	starpu_shutdown();
	MPI_Finalize();

	return 0;
}

#endif
