/* GCC-StarPU
   Copyright (C) 2011 Institut National de Recherche en Informatique et Automatique

   GCC-StarPU is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   GCC-StarPU is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC-StarPU.  If not, see <http://www.gnu.org/licenses/>.  */

#undef NDEBUG

#include <mocks.h>

#define __output __attribute__ ((output))

/* The tasks under test.  */

static void my_pointer_task (size_t size, __output int *x)
  __attribute__ ((task));

static void my_pointer_task_cpu (size_t size, __output int *x)
  __attribute__ ((task_implementation ("cpu", my_pointer_task)));
static void my_pointer_task_opencl (size_t size, __output int *x)
  __attribute__ ((task_implementation ("opencl", my_pointer_task)));

static void
my_pointer_task_cpu (size_t size, __output int *x)
{
  printf ("%s: x = %p, size = %zi\n", __func__, x, size);
}

static void
my_pointer_task_opencl (size_t size, int *x)
{
  printf ("%s: x = %p, size = %zi\n", __func__, x, size);
}



static void my_array_task (size_t size, __output int x[size])
  __attribute__ ((task));

static void my_array_task_cpu (size_t size, __output int x[size])
  __attribute__ ((task_implementation ("cpu", my_array_task)));
static void my_array_task_opencl (size_t size, __output int x[size])
  __attribute__ ((task_implementation ("opencl", my_array_task)));

static void
my_array_task_cpu (size_t size, __output int x[size])
{
  printf ("%s: x = %p, size = %zi\n", __func__, x, size);
}

static void
my_array_task_opencl (size_t size, __output int x[size])
{
  printf ("%s: x = %p, size = %zi\n", __func__, x, size);
}



int
main (int argc, char *argv[])
{
#pragma starpu initialize

  size_t size = 42;
  int x[size];

  struct insert_task_argument expected[] =
    {
      { STARPU_VALUE, &size, sizeof size },
      { STARPU_W, x },
      { 0, 0, 0 }
    };

  expected_insert_task_arguments = expected;

  /* Invoke the task, which makes sure it gets called with EXPECTED.  */
  my_pointer_task (size, x);

  assert (tasks_submitted == 1);

  /* Again.  */
  my_array_task (size, x);

  assert (tasks_submitted == 2);

  return EXIT_SUCCESS;
}
