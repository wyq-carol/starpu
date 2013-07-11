/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2011 - 2013  INRIA
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

#include "sc_hypervisor_lp.h"
#include "sc_hypervisor_policy.h"
#include <starpu_config.h>
#include <sys/time.h>

#ifdef STARPU_HAVE_GLPK_H
static void _try_resizing(int *sched_ctxs, int nsched_ctxs)
{
	/* for vite */
	starpu_trace_user_event(2);
	int ns = sched_ctxs == NULL ? sc_hypervisor_get_nsched_ctxs() : nsched_ctxs;
	int curr_sched_ctxs = sched_ctxs == NULL ? sc_hypervisor_get_sched_ctxs() : sched_ctxs;

	double nworkers_per_ctx[ns][2];
	int nw = 1;
#ifdef STARPU_USE_CUDA
	int ncuda = starpu_worker_get_count_by_type(STARPU_CUDA_WORKER);
	nw = ncuda != 0 ? 2 : 1;
#endif
	int total_nw[nw];
	sc_hypervisor_group_workers_by_type(NULL, -1, nw, total_nw);
	
	
	struct timeval start_time;
	struct timeval end_time;
	gettimeofday(&start_time, NULL);
	printf("###########################################try resize\n");
	double vmax = sc_hypervisor_lp_get_nworkers_per_ctx(ns, nw, nworkers_per_ctx, total_nw);
	gettimeofday(&end_time, NULL);
	
	long diff_s = end_time.tv_sec  - start_time.tv_sec;
	long diff_us = end_time.tv_usec  - start_time.tv_usec;
	
	float timing = (float)(diff_s*1000000 + diff_us)/1000;
	
	if(vmax != 0.0)
	{
		int nworkers_per_ctx_rounded[nsched_ctxs][nw];
		sc_hypervisor_lp_round_double_to_int(ns, nw, nworkers_per_ctx, nworkers_per_ctx_rounded);
		sc_hypervisor_lp_redistribute_resources_in_ctxs(ns, nw, nworkers_per_ctx_rounded, nworkers_per_ctx, curr_sched_ctxs);
	}
}

static void feft_lp_handle_poped_task(__attribute__((unused))unsigned sched_ctx, __attribute__((unused))int worker, 
				      __attribute__((unused))struct starpu_task *task, __attribute__((unused))uint32_t footprint)
{
	int ret = starpu_pthread_mutex_trylock(&act_hypervisor_mutex);
	if(ret != EBUSY)
	{
		unsigned criteria = sc_hypervisor_get_resize_criteria();
		if(criteria != SC_NOTHING && criteria == SC_SPEED)
		{
			if(sc_hypervisor_check_speed_gap_btw_ctxs())
			{
				_try_resizing(NULL, -1);
			}
		}
		starpu_pthread_mutex_unlock(&act_hypervisor_mutex);
	}

}
static void feft_lp_size_ctxs(int *sched_ctxs, int nsched_ctxs, int *workers, int nworkers)
{
	int ns = sched_ctxs == NULL ? sc_hypervisor_get_nsched_ctxs() : nsched_ctxs;
	int nw = 1;
#ifdef STARPU_USE_CUDA
	int ncuda = starpu_worker_get_count_by_type(STARPU_CUDA_WORKER);
	nw = ncuda != 0 ? 2 : 1;
#endif
	double nworkers_per_type[ns][nw];
	int total_nw[nw];
	sc_hypervisor_group_workers_by_type(workers, nworkers, nw, total_nw);

	starpu_pthread_mutex_lock(&act_hypervisor_mutex);
	double vmax = sc_hypervisor_lp_get_nworkers_per_ctx(ns, nw, nworkers_per_type, total_nw);
	if(vmax != 0.0)
	{
// 		printf("********size\n");
/* 		int i; */
/* 		for( i = 0; i < nsched_ctxs; i++) */
/* 		{ */
/* 			printf("ctx %d/worker type %d: n = %lf \n", i, 0, nworkers_per_type[i][0]); */
/* #ifdef STARPU_USE_CUDA */
/* 			int ncuda = starpu_worker_get_count_by_type(STARPU_CUDA_WORKER); */
/* 			if(ncuda != 0) */
/* 				printf("ctx %d/worker type %d: n = %lf \n", i, 1, nworkers_per_type[i][1]); */
/* #endif */
/* 		} */
		int nworkers_per_type_rounded[ns][nw];
		sc_hypervisor_lp_round_double_to_int(ns, nw, nworkers_per_type, nworkers_per_type_rounded);
/*       	for( i = 0; i < nsched_ctxs; i++) */
/* 		{ */
/* 			printf("ctx %d/worker type %d: n = %d \n", i, 0, nworkers_per_type_rounded[i][0]); */
/* #ifdef STARPU_USE_CUDA */
/* 			int ncuda = starpu_worker_get_count_by_type(STARPU_CUDA_WORKER); */
/* 			if(ncuda != 0) */
/* 				printf("ctx %d/worker type %d: n = %d \n", i, 1, nworkers_per_type_rounded[i][1]); */
/* #endif */
/* 		} */
		int *current_sched_ctxs = sched_ctxs == NULL ? sc_hypervisor_get_sched_ctxs() : sched_ctxs;

		unsigned has_workers = 0;
		int s;
		for(s = 0; s < ns; s++)
		{
			int nworkers_ctx = sc_hypervisor_get_nworkers_ctx(current_sched_ctxs[s], STARPU_ANY_WORKER);
			if(nworkers_ctx != 0)
			{
				has_workers = 1;
				break;
			}
		}
		if(has_workers)
			sc_hypervisor_lp_redistribute_resources_in_ctxs(ns, nw, nworkers_per_type_rounded, nworkers_per_type, current_sched_ctxs);
		else
			sc_hypervisor_lp_distribute_resources_in_ctxs(sched_ctxs, ns, nw, nworkers_per_type_rounded, nworkers_per_type, workers, nworkers);
	}
	starpu_pthread_mutex_unlock(&act_hypervisor_mutex);
}

static void feft_lp_handle_idle_cycle(unsigned sched_ctx, int worker)
{
	int ret = starpu_pthread_mutex_trylock(&act_hypervisor_mutex);
	if(ret != EBUSY)
	{
		unsigned criteria = sc_hypervisor_get_resize_criteria();
		if(criteria != SC_NOTHING && criteria == SC_IDLE)
		{
			
			if(sc_hypervisor_check_idle(sched_ctx, worker))
			{
				_try_resizing(NULL, -1);
//				sc_hypervisor_move_workers(sched_ctx, 3 - sched_ctx, &worker, 1, 1);
			}
		}
		starpu_pthread_mutex_unlock(&act_hypervisor_mutex);
	}
}

static void feft_lp_resize_ctxs(int *sched_ctxs, int nsched_ctxs , 
				__attribute__((unused))int *workers, __attribute__((unused))int nworkers)
{
	int ret = starpu_pthread_mutex_trylock(&act_hypervisor_mutex);
	if(ret != EBUSY)
	{
		struct sc_hypervisor_wrapper* sc_w  = NULL;
		int s = 0;
		for(s = 0; s < nsched_ctxs; s++)
		{
			 sc_w = sc_hypervisor_get_wrapper(sched_ctxs[s]);
			
			 if((sc_w->submitted_flops + (0.1*sc_w->total_flops)) < sc_w->total_flops)
			 {
				 starpu_pthread_mutex_unlock(&act_hypervisor_mutex);
				 return;
			 }
		}

		_try_resizing(sched_ctxs, nsched_ctxs);
		starpu_pthread_mutex_unlock(&act_hypervisor_mutex);
	}
}

struct sc_hypervisor_policy feft_lp_policy = {
	.size_ctxs = feft_lp_size_ctxs,
	.resize_ctxs = feft_lp_resize_ctxs,
	.handle_poped_task = feft_lp_handle_poped_task,
	.handle_pushed_task = NULL,
	.handle_idle_cycle = feft_lp_handle_idle_cycle, //NULL,
	.handle_idle_end = NULL,
	.handle_post_exec_hook = NULL,
	.handle_submitted_job = NULL,
	.end_ctx = NULL,
	.custom = 0,
	.name = "feft_lp"
};

#endif /* STARPU_HAVE_GLPK_H */
