/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2010  Université de Bordeaux 1
 * Copyright (C) 2010, 2011  Centre National de la Recherche Scientifique
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

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <starpu.h>
#include <stdlib.h>
#include "../common/helper.h"

#define NBUFFERS	64
#define NITER		128
#define VECTORSIZE	1024

float *buffer[NBUFFERS];

starpu_data_handle v_handle[NBUFFERS];

static void dummy_codelet(void *descr[], __attribute__ ((unused)) void *_args)
{
}

static starpu_codelet cl = {
	.where = STARPU_CPU|STARPU_CUDA|STARPU_OPENCL,
	.cpu_func = dummy_codelet,
#ifdef STARPU_USE_CUDA
	.cuda_func = dummy_codelet,
#endif
#ifdef STARPU_USE_OPENCL
        .opencl_func = dummy_codelet,
#endif
	.nbuffers = 1
};

int use_handle(starpu_data_handle handle)
{
	int ret;
	struct starpu_task *task;

	task = starpu_task_create();
		task->cl = &cl;
		task->buffers[0].handle = handle;
		task->buffers[0].mode = STARPU_RW;
		task->detach = 0;

	ret = starpu_task_submit(task);
	return ret;
}

int main(int argc, char **argv)
{
	int ret;

	ret = starpu_init(NULL);
	STARPU_CHECK_RETURN_VALUE(ret, "starpu_init");

	/* Allocate all buffers and register them to StarPU */
	unsigned b;
	for (b = 0; b < NBUFFERS; b++)
	{
		ret = starpu_malloc((void **)&buffer[b], VECTORSIZE);
		STARPU_CHECK_RETURN_VALUE(ret, "starpu_malloc");
		starpu_vector_data_register(&v_handle[b], 0,
				(uintptr_t)buffer[b], VECTORSIZE, sizeof(char));
	}

	unsigned iter;
	for (iter = 0; iter < NITER; iter++)
	{
		/* Use the buffers on the different workers so that it may not
		 * be in main memory anymore */
		for (b = 0; b < NBUFFERS; b++) {
			ret = use_handle(v_handle[b]);
			if (ret == -ENODEV) goto enodev;
			STARPU_CHECK_RETURN_VALUE(ret, "starpu_task_submit");
		}

		ret = starpu_task_wait_for_all();
		STARPU_CHECK_RETURN_VALUE(ret, "starpu_task_wait_for_all");

		/* Grab the different pieces of data into main memory */
		for (b = 0; b < NBUFFERS; b++) {
			ret = starpu_data_acquire(v_handle[b], STARPU_RW);
			STARPU_CHECK_RETURN_VALUE(ret, "starpu_data_acquire");
		}

		/* Release them */
		for (b = 0; b < NBUFFERS; b++)
			starpu_data_release(v_handle[b]);
	}

	/* do some cleanup */
	for (b = 0; b < NBUFFERS; b++) {
		starpu_data_unregister(v_handle[b]);
		starpu_free(buffer[b]);
	}

	starpu_shutdown();

	return 0;

enodev:
	fprintf(stderr, "WARNING: No one can execute this task\n");
	/* yes, we do not perform the computation but we did detect that no one
 	 * could perform the kernel, so this is not an error from StarPU */
	starpu_shutdown();
	return 77;
}
