/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2010, 2016  Université de Bordeaux
 * Copyright (C) 2010, 2011, 2012, 2013  CNRS
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
#include "../helper.h"

/*
 * Try invalidating a variable which is pending a request
 */
#define SIZE (100<<20)

int main(int argc, char **argv)
{
	int ret;
	char *var = malloc(SIZE);
	starpu_data_handle_t handle;

	ret = starpu_init(NULL);
	if (ret == -ENODEV) return STARPU_TEST_SKIPPED;
	STARPU_CHECK_RETURN_VALUE(ret, "starpu_init");

	if (starpu_worker_get_count_by_type(STARPU_CUDA_WORKER) == 0 &&
		starpu_worker_get_count_by_type(STARPU_OPENCL_WORKER) == 0)
		goto enodev;

	starpu_variable_data_register(&handle, STARPU_MAIN_RAM, (uintptr_t)var, SIZE);

	/* Let a request fly */
	starpu_fxt_trace_user_event_string("requesting");
	starpu_data_fetch_on_node(handle, 1, 1);
	starpu_fxt_trace_user_event_string("requested");
	/* But suddenly invalidate the data while it's on the fly! */
	starpu_data_invalidate_submit(handle);
	starpu_fxt_trace_user_event_string("invalidated");

	starpu_data_unregister(handle);

	starpu_shutdown();

	return 0;

enodev:
	starpu_shutdown();
	return STARPU_TEST_SKIPPED;
}
