/*
 * Copyright (c) 2014 Intel Corp., Inc.  All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "shmem_impl.h"

global_t _g;

void start_pes (int npes)
{
  SHMEMI_Start_pes(npes);
}

int _my_pe (void)
{
  return SHMEMI_My_pe();
}

int _num_pes (void)
{
  return SHMEMI_Num_pes();
}

void shfree (void *ptr)
{
  SHMEMI_Shfree(ptr);
}

void *shmalloc (size_t size)
{
  return SHMEMI_Shmalloc(size);
}

void shmem_barrier_all (void)
{
  SHMEMI_Barrier_all();
}

void shmem_long_put (long       *dest,
                     const long *src,
                     size_t      nelems,
                     int         pe)
{
  SHMEMI_Long_put(dest,src,nelems,pe);
}

void shmem_wait (long *ivar,
                 long  cmp_value)
{
  SHMEMI_Wait(ivar,cmp_value);
}
