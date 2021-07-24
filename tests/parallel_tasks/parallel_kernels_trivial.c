/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2010-2021  Université de Bordeaux, CNRS (LaBRI UMR 5800), Inria
 * Copyright (C) 2013       Thibaut Lambert
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

#include <starpu.h>
#include <limits.h>
#include <unistd.h>
#include "../helper.h"

/*
 * Submit a simple testcase for parallel tasks.
 */

#define VECTORSIZE	1024

void codelet_null(void *descr[], void *_args)
{
	(void)descr;
	(void)_args;

	STARPU_SKIP_IF_VALGRIND;

	int worker_size = starpu_combined_worker_get_size();
	STARPU_ASSERT(worker_size > 0);
	usleep(1000/worker_size);
#if 0
	int id = starpu_worker_get_id();
	int combined_id = starpu_combined_worker_get_id();
	FPRINTF(stderr, "worker id %d - combined id %d - worker size %d\n", id, combined_id, worker_size);
#endif
}

struct starpu_perfmodel model =
{
	.type = STARPU_HISTORY_BASED,
	.symbol = "parallel_kernel_test"
};

static struct starpu_codelet cl =
{
	.type = STARPU_FORKJOIN,
	.max_parallelism = INT_MAX,
	.cpu_funcs = {codelet_null},
	.cuda_funcs = {codelet_null},
	.cpu_funcs_name = {"codelet_null"},
        .opencl_funcs = {codelet_null},
	.model = &model,
	.nbuffers = 1,
	.modes = {STARPU_R}
};

static struct starpu_codelet cl_seq =
{
	.cpu_funcs = {codelet_null},
	.cuda_funcs = {codelet_null},
	.cpu_funcs_name = {"codelet_null"},
        .opencl_funcs = {codelet_null},
	.model = &model,
	.nbuffers = 1,
	.modes = {STARPU_R}
};

int main(void)
{
	int ret;
	starpu_data_handle_t v_handle;
	unsigned *v;

        struct starpu_conf conf;
	starpu_conf_init(&conf);
	conf.ncpus = 2;
	conf.sched_policy_name = "pheft";
	conf.calibrate = 1;

	ret = starpu_init(&conf);
	if (ret == -ENODEV) return STARPU_TEST_SKIPPED;
	STARPU_CHECK_RETURN_VALUE(ret, "starpu_init");

	starpu_malloc((void **)&v, VECTORSIZE*sizeof(unsigned));
	starpu_vector_data_register(&v_handle, STARPU_MAIN_RAM, (uintptr_t)v, VECTORSIZE, sizeof(unsigned));

	/* First submit a sequential task */
	ret = starpu_task_insert(&cl_seq, STARPU_R, v_handle, 0);
	if (ret == -ENODEV) goto enodev;
	STARPU_CHECK_RETURN_VALUE(ret, "starpu_task_submit");

	/* Then a parallel task, which is not interesting to run in parallel when we have only two cpus */
	ret = starpu_task_insert(&cl, STARPU_R, v_handle, 0);
	if (ret == -ENODEV) goto enodev;
	STARPU_CHECK_RETURN_VALUE(ret, "starpu_task_submit");

        /* Then another parallel task, which is interesting to run in parallel
        since the two cpus are now finishing at the same time. */
	ret = starpu_task_insert(&cl, STARPU_R, v_handle, 0);
	if (ret == -ENODEV) goto enodev;
	STARPU_CHECK_RETURN_VALUE(ret, "starpu_task_submit");

	ret = starpu_task_wait_for_all();
	STARPU_CHECK_RETURN_VALUE(ret, "starpu_task_wait_for_all");

	starpu_data_unregister(v_handle);
	starpu_free_noflag(v, VECTORSIZE*sizeof(unsigned));
	starpu_shutdown();

	STARPU_RETURN(EXIT_SUCCESS);

enodev:
	starpu_data_unregister(v_handle);
	starpu_free_noflag(v, VECTORSIZE*sizeof(unsigned));
	fprintf(stderr, "WARNING: No one can execute this task\n");
	/* yes, we do not perform the computation but we did detect that no one
 	 * could perform the kernel, so this is not an error from StarPU */
	starpu_shutdown();
	STARPU_RETURN(STARPU_TEST_SKIPPED);
}
