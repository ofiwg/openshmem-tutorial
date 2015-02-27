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

#ifndef SHMACRO_SHMEM_INCLUDE
#define SHMACRO_SHMEM_INCLUDE

#ifdef __cplusplus
#define __STDC_LIMIT_MACROS
#endif

#include "shmem.h"
#include "pm/shmem_pm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>
#include <rdma/fabric.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_tagged.h>
#include <rdma/fi_rma.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_atomic.h>


#ifdef FORCE_INLINE_SHMEM
#define   start_pes         SHMEMI_Start_pes
#define  _my_pe             SHMEMI_My_pe
#define   shfree            SHMEMI_Shfree
#define   shmalloc          SHMEMI_Shmalloc
#define   shmem_barrier_all SHMEMI_Barrier_all
#define   shmem_long_put    SHMEMI_Long_put
#define   shmem_wait        SHMEMI_Wait
#endif

#define __SI__ __attribute__((always_inline)) static inline

#define EPNAMELEN  16

/* ************************************************************************** */
/* Type Definitions                                                           */
/* ************************************************************************** */
/* Global Object for state tracking */
typedef struct global_t
{
  struct fid_fabric *fabric;
  struct fid_domain *domain;
  struct fid_ep     *endpoint;
  struct fid_cntr   *putcntr;
  struct fid_cntr   *getcntr;
  struct fid_cntr   *targetcntr;
  struct fid_cq     *cq;
  struct fid_av     *av;
  struct fid_mr     *mr;
  char               epname[EPNAMELEN];
  int                rank;
  int                size;
  uint64_t           put_ctr_pending;
}global_t;
extern global_t _g;

struct request;

/* SFI Callback structure */
typedef int (*callback_fn)  (struct request*);

/* Request object for tagged send/recv */
typedef struct request
{
  struct fi_context context;   /* Nonblocking Handle */
  callback_fn       cb;        /* Callback function  */
  int               done;      /* Request state      */
}request_t;

/* General Completion Entry */
typedef union completion_entry
{
  struct fi_cq_tagged_entry tagged; /* Tagged completion entry */
}completion_entry_t;


/* ************************************************************************** */
/* Error checking macros                                                      */
/* ************************************************************************** */
#define ERRCHKSFI(ret)                                                  \
  if(ret != 0)                                                          \
    {                                                                   \
      fprintf(stderr, "SFI Error Line %d:%d \"%s\"\n", __LINE__,(int)ret,fi_strerror(-ret)); \
      exit(-1);                                                         \
    }


__SI__
void SHMEMI_Finalize(void)
{
  SHMEMI_PM_Finalize();
}

__SI__
void SHMEMI_Start_pes(int npes)
{
  int    i, jobid;
  size_t epnamelen = sizeof(_g.epname);

  struct fi_info        *p_info     = NULL;
  struct fi_info         hints;
  struct fi_cq_attr      cq_attr;
  struct fi_av_attr      av_attr;
  struct fi_cntr_attr    cntr_attr;
  struct fi_domain_attr  domain_attr;
  struct fi_tx_attr      tx_attr;
  struct fi_rx_attr      rx_attr;
  struct fi_ep_attr      ep_attr;
  memset(&hints,      0, sizeof(hints));
  memset(&cq_attr,    0, sizeof(cq_attr));
  memset(&av_attr,    0, sizeof(av_attr));
  memset(&cntr_attr,  0, sizeof(cntr_attr));
  memset(&domain_attr,0, sizeof(domain_attr));
  memset(&tx_attr,0, sizeof(tx_attr));
  memset(&rx_attr,0, sizeof(rx_attr));
  memset(&ep_attr,0, sizeof(ep_attr));

  /* ************************************* */
  /* Initialize job launch runtime         */
  /* ************************************* */
  SHMEMI_PM_Initialize(&jobid, &_g.rank, &_g.size);

  /* ************************************* */
  /*  Hints to filter providers            */
  /*  See man fi_getinfo for a list        */
  /*  of all filters                       */
  /* ************************************* */
  hints.mode          = FI_CONTEXT;
  hints.ep_attr       = &ep_attr;
  hints.ep_attr->type = FI_EP_RDM;      /* reliable connectionless */

  /* ************************************* */
  /* Endpoint capabilities to request      */
  /* ************************************* */
  hints.caps      = FI_RMA;             /* request rma capability  */
  hints.caps     |= FI_TAGGED;          /* Tagged operation        */
  hints.caps     |= FI_REMOTE_COMPLETE; /* Remote completion       */

  /* ************************************* */
  /* Default flags to set on any endpoints */
  /* ************************************* */

  /* ************************************* */
  /* Domain attributes                     */
  /* ************************************* */
  hints.caps                |= FI_DYNAMIC_MR;    /* Allow dynamic registration of any memory */

  domain_attr.data_progress  = FI_PROGRESS_AUTO; /* Provider makes progress without calls    */
  hints.domain_attr          = &domain_attr;

  tx_attr.op_flags = FI_REMOTE_COMPLETE; /* Disable completions, FI_COMPLETION is not set */
  rx_attr.op_flags = 0;                  /* Disable completions, FI_COMPLETION is not set */
  hints.tx_attr    = &tx_attr;
  hints.rx_attr    = &rx_attr;

  /* *************************************************** */
  /* fi_getinfo:  returns information about fabric       */
  /* services for reaching a remote node or service      */
  /* This does not necessarily allocate resources, but   */
  /* provides a list of services for the fabric          */
  /* *************************************************** */
  ERRCHKSFI(fi_getinfo(FI_VERSION(1,0), /* Interface version requested               */
                       NULL,            /* Optional name or fabric to resolve        */
                       "0",             /* Service name or port number to request    */
                       FI_SOURCE,       /* Flag:  node/service specify local address */
                       &hints,          /* In:  Hints to filter available providers  */
                       &p_info));       /* Out: List of providers that match hints   */

  /* **************************************************** */
  /* The provider info struct returns a fabric attribute  */
  /* struct that can be used to instantiate the virtual   */
  /* or physical network.  This opens a "fabric provider" */
  /* We choose the first available fabric, but getinfo    */
  /* returns a list.  see man fi_fabric for details       */
  /* **************************************************** */
  ERRCHKSFI(fi_fabric(p_info->fabric_attr,  /* In:   Fabric attributes */
                      &_g.fabric,           /* Out:  Fabric descriptor */
                      NULL));               /* Context: fabric events  */

  /* **************************************************** */
  /* Create the access domain, which is the physical or   */
  /* virtual network or hardware port or collection of    */
  /* ports.  Returns a domain object that can be used     */
  /* to create endpoints.  See man fi_domain for details  */
  /* **************************************************** */
  ERRCHKSFI(fi_domain(_g.fabric,  /* In:  Fabric object             */
                      p_info,     /* In:  default domain attributes */
                      &_g.domain, /* Out: domain object             */
                      NULL));     /* Context: Domain events         */

  /* **************************************************** */
  /* Create a transport level communication endpoint.     */
  /* To use the endpoint, it must be bound to completion  */
  /* counters or event queues and enabled, and the        */
  /* resources consumed by it, such as address vectors,   */
  /* counters, completion queues, etc                     */
  /* see man fi_endpoint for more details                 */
  /* **************************************************** */
  ERRCHKSFI(fi_endpoint(_g.domain,           /* In: Domain Object        */
                        p_info,              /* In: Configuration object */
                        &_g.endpoint,        /* Out: Endpoint Object     */
                        NULL));              /* Context: endpoint events */
  /* **************************************************** */
  /* First, create the objects that will be bound to the  */
  /* endpoint.  The objects include:                      */
  /*     * completion counters for put and get            */
  /*     * counters for incoming writes                   */
  /*     * completion queue for events                    */
  /*     * address vector of other endpoint addresses     */
  /*     * memory region for symmetric heap               */
  /* **************************************************** */
  cntr_attr.events   = FI_CNTR_EVENTS_COMP;
  ERRCHKSFI(fi_cntr_open(_g.domain,   /* In:  Domain Object        */
                         &cntr_attr,  /* In:  Configuration object */
                         &_g.putcntr, /* Out: Counter Object       */
                         NULL));      /* Context: counter events   */

  ERRCHKSFI(fi_cntr_open(_g.domain,   /* In:  Domain Object        */
                         &cntr_attr,  /* In:  Configuration object */
                         &_g.getcntr, /* Out: Counter Object       */
                         NULL));      /* Context: counter events   */

  ERRCHKSFI(fi_cntr_open(_g.domain,        /* In:  Domain Object        */
                         &cntr_attr,       /* In:  Configuration object */
                         &_g.targetcntr,   /* Out: Counter Object       */
                         NULL));           /* Context: counter events   */

  cq_attr.format    = FI_CQ_FORMAT_TAGGED;
  ERRCHKSFI(fi_cq_open(_g.domain,        /* In:  Domain Object        */
                       &cq_attr,         /* In:  Configuration object */
                       &_g.cq,           /* Out: CQ Object            */
                       NULL));            /* Context: CQ events       */

  av_attr.type   = FI_AV_TABLE;      /* Logical addressing mode    */
  ERRCHKSFI(fi_av_open(_g.domain,    /* In:  Domain Object         */
                       &av_attr,     /* In:  Configuration object  */
                       &_g.av,       /* Out: AV Object             */
                       NULL));       /* Context: AV events         */

  ERRCHKSFI(fi_mr_reg(_g.domain,         /* In:  Domain Object            */
                      0,                 /* In:  Lower address            */
                      UINT64_MAX,        /* In:  Upper address            */
                      FI_REMOTE_READ |   /* In:  Expose MR for read/write */
                      FI_REMOTE_WRITE,
                      0,                 /* In:  offset                    */
                      0ULL,              /* In:  requested key             */
                      FI_MR_KEY,         /* In:  flags                     */
                      &_g.mr,            /* Out: memregion object          */
                      NULL));            /* Context: memregion events      */

  ERRCHKSFI(fi_ep_bind(_g.endpoint,    /* Enable for remote write         */
                    &_g.putcntr->fid,
                    FI_WRITE));

  ERRCHKSFI(fi_ep_bind(_g.endpoint,    /* Enable endpoint for remote read   */
                    &_g.getcntr->fid,
                    FI_READ));

  ERRCHKSFI(fi_ep_bind(_g.endpoint,        /* Enable CQ for writing and disable */
                       &_g.cq->fid,        /* events by default                 */
                       FI_SEND|FI_RECV|FI_COMPLETION));

  ERRCHKSFI(fi_ep_bind(_g.endpoint,     /* Enable AV                         */
                    &_g.av->fid,
                    0));

  ERRCHKSFI(fi_mr_bind(_g.mr,           /* Bind target counter to memory region */
                    &_g.targetcntr->fid,
                    FI_REMOTE_READ|FI_REMOTE_WRITE));

  ERRCHKSFI(fi_ep_bind(_g.endpoint,     /* Bind memory region to endpoint */
                    &_g.mr->fid,
                    FI_REMOTE_READ|FI_REMOTE_WRITE));

  /* **************************************************** */
  /* Enable the endpoints for communication               */
  /* **************************************************** */
  ERRCHKSFI(fi_enable(_g.endpoint));

  /* **************************************************** */
  /* Exchange endpoint addresses using scalable database  */
  /* or job launcher, in this case, use interfaces        */
  /* **************************************************** */
  ERRCHKSFI(fi_getname((fid_t)_g.endpoint, _g.epname, &epnamelen));
  assert(epnamelen <= sizeof(_g.epname));

  char *addrNames = (char*)malloc(_g.size*epnamelen);
  assert(addrNames);

  SHMEMI_PM_Exchange(_g.epname, _g.rank, epnamelen, addrNames, _g.size);

  /* **************************************************** */
  /* Map the addresses into an address vector             */
  /* The addressing mode is logical so destinations can   */
  /* be addressed by index                                */
  /* **************************************************** */
  int num_inserted;
  num_inserted=fi_av_insert(_g.av,    /* Address vector                          */
                            addrNames,/* Array of address names, size of addrlen */
                            _g.size,  /* Count of addresses                      */
                            NULL,     /* Output addresses, unused in logical     */
                            0,        /* FLAGS:  none required                   */
                            NULL);    /* Contextual information (optional)        */

  free(addrNames);
  fi_freeinfo(p_info);

  SHMEMI_PM_Barrier();
}

__SI__
int SHMEMI_My_pe(void)
{
  return _g.rank;
}

__SI__
int SHMEMI_Num_pes(void)
{
  return _g.size;
}

__SI__
int SHMEMI_poll()
{
  ssize_t      ret;
  completion_entry_t  entry;
  request_t   *req;

  ret = fi_cq_read(_g.cq,(void*)&entry,1);
  assert(ret >= 0);
  if(ret > 0)
    {
      req = (request_t*)entry.tagged.op_context;
      req->cb(req);
    }
  return 0;
}

__SI__
int SHMEMI_Send_cb(request_t* req)
{
  req->done = 1;
  return 0;
}

__SI__
int SHMEMI_Recv_cb(request_t* req)
{
  req->done = 1;
  return 0;
}

__SI__
int SHMEMI_send(void *buf, size_t len, int dest, request_t *req)
{
  struct fi_msg_tagged msg;
  struct iovec         iov;
  uint64_t match_bits = _g.rank+1000;
  req->cb             = SHMEMI_Send_cb;
  req->done           = 0;
  iov.iov_base        = buf;
  iov.iov_len         = len;
  msg.msg_iov         = &iov;
  msg.desc            = NULL;
  msg.iov_count       = 1;
  msg.addr            = dest;
  msg.tag             = match_bits;
  msg.context         = (void*)&req->context;
  ERRCHKSFI(fi_tsendmsg(_g.endpoint,&msg,FI_COMPLETION));
  return 0;
}

__SI__
int SHMEMI_recv(void *buf, size_t len, int dest, request_t *req)
{
  struct fi_msg_tagged msg;
  struct iovec         iov;
  uint64_t match_bits  = dest+1000;
  uint64_t ignore_bits = 0;
  req->cb              = SHMEMI_Recv_cb;
  req->done            = 0;
  iov.iov_base         = buf;
  iov.iov_len          = len;
  msg.msg_iov          = &iov;
  msg.desc             = NULL;
  msg.iov_count        = 1;
  msg.addr             = dest;
  msg.tag              = match_bits;
  msg.ignore           = ignore_bits;
  msg.context          = (void*)&req->context;

  ERRCHKSFI(fi_trecvmsg(_g.endpoint,&msg,FI_COMPLETION));
  return 0;
}

__SI__
int SHMEMI_reduce(uint64_t *sbuf,
                  uint64_t *rbuf)
{
  /* Very simple root 0 ring.  Not suitable for scale */
  request_t req;
  int       sz = sizeof(uint64_t);
  if(_g.size == 1) return 0;
  if(_g.rank == 0)
    {
      SHMEMI_send(sbuf, sz, 1, &req);
      while(!req.done)
        SHMEMI_poll();
      SHMEMI_recv(rbuf, sz, _g.size-1, &req);
      while(!req.done)
        SHMEMI_poll();

    }
  else if(_g.rank == _g.size-1)
    {
      SHMEMI_recv(rbuf, sz, _g.rank-1, &req);
      while(!req.done)
        SHMEMI_poll();
      *rbuf = *rbuf & *sbuf;
      SHMEMI_send(rbuf, sz, 0, &req);
      while(!req.done)
        SHMEMI_poll();
    }
  else
    {
      SHMEMI_recv(rbuf, sz, _g.rank-1, &req);
      while(!req.done)
        SHMEMI_poll();
      *rbuf = *rbuf & *sbuf;
      SHMEMI_send(rbuf, sz, _g.rank+1, &req);
      while(!req.done)
        SHMEMI_poll();
    }
  return 0;
}

__SI__
int SHMEMI_bcast(void *buf, size_t len)
{
  /* Very simple root 0 ring.  Not suitable for scale */
  request_t req;
  if(_g.size == 1) return 0;
  if(_g.rank == 0)
    {
      SHMEMI_send(buf, len, 1, &req);
      while(!req.done)
        SHMEMI_poll();
    }
  else if(_g.rank == _g.size-1)
    {
      SHMEMI_recv(buf, len, _g.rank-1, &req);
      while(!req.done)
        SHMEMI_poll();
    }
  else
    {
      SHMEMI_recv(buf, len, _g.rank-1, &req);
      while(!req.done)
        SHMEMI_poll();
      SHMEMI_send(buf, len, _g.rank+1, &req);
      while(!req.done)
        SHMEMI_poll();
    }
  return 0;
}

__SI__
int SHMEMI_allreduce(uint64_t *sbuf,
                     uint64_t *rbuf)
{
  SHMEMI_reduce(sbuf,rbuf);
  SHMEMI_bcast(rbuf,1);
  return 0;
}

__SI__
int SHMEMI_barrier()
{
  int       i,n;
  int       nphases = 0;
  request_t req[2];
  for (n=_g.size-1;n>0;n>>=1)nphases++;
  for (i=0; i<nphases; i++)
    {
      int shift       = (1<<i);
      int to          = (_g.rank+shift)%_g.size;
      int from        = (_g.rank)-shift;
      if(from<0) from = _g.size+from;
      char val;
      SHMEMI_recv(&val, 1, from,&req[0]);
      SHMEMI_send(&val, 1, to, &req[1]);

      while(!req[0].done || !req[1].done)
        SHMEMI_poll();
    }
  return 0;
}

__SI__
void SHMEMI_quiet()
{
  ERRCHKSFI(fi_cntr_wait(_g.putcntr,
                         _g.put_ctr_pending,
                         -1));
}


__SI__
size_t SHMEMI_Get_mapsize(size_t size,size_t *psz)
{
  long    page_sz = sysconf(_SC_PAGESIZE);
  size_t  mapsize = (size + (page_sz-1))&(~(page_sz-1));
  *psz            = page_sz;
  return mapsize;
}

__SI__
int SHMEMI_Check_maprange(void   *start,size_t  size)
{
  int     rc          = 0;
  size_t  page_sz;
  size_t  mapsize     = SHMEMI_Get_mapsize(size,&page_sz);
  size_t  i,num_pages = mapsize/page_sz;
  char   *ptr         = (char*)start;
  for(i=0;i<num_pages;i++)
    {
      rc = msync(ptr,page_sz,0);
      if(rc == -1)
        {
          assert(errno==ENOMEM);
          ptr+=page_sz;
        }
      else
        return 0;
    }
  return 1;
}

__SI__
void * SHMEMI_random_addr(size_t size)
{
  /* starting position for pointer to map
   * This is not generic, probably only works properly on Linux
   * but it's not fatal since we bail after a fixed number of iterations
   */
#define MAP_POINTER ((random_int&((0x00006FFFFFFFFFFF&(~(page_sz-1)))|0x0000600000000000)))
  char            random_state[256];
  char           *oldstate;
  size_t          page_sz, random_int;
  size_t          mapsize     = SHMEMI_Get_mapsize(size,&page_sz);
  uintptr_t       map_pointer;
  struct timeval  ts;
  int             iter = 100;
  gettimeofday(&ts, NULL);
  initstate(ts.tv_usec,random_state,256);
  oldstate    = setstate(random_state);
  random_int  = random()<<32|random();
  map_pointer = MAP_POINTER;
  while(SHMEMI_Check_maprange((void*)map_pointer,mapsize) == 0)
    {
      random_int  = random()<<32|random();
      map_pointer = MAP_POINTER;
      iter--;
      if(iter == 0)
        return (void*)-1ULL;
    }
  setstate(oldstate);
  return (void*)map_pointer;
}

__SI__
void *SHMEMI_Symmetric_heap(size_t size)
{
  uint64_t  test, result;
  int       iter=100;
  void     *baseP;
  size_t    page_sz;
  size_t    mapsize;

  mapsize = SHMEMI_Get_mapsize(size, &page_sz);
  result = 0;
  while(!result && --iter!=0)
    {
      uintptr_t  map_pointer = 0ULL;
      baseP                  = (void*)-1ULL;
      if (_g.rank == 0)
        {
          map_pointer = (uintptr_t)SHMEMI_random_addr(mapsize);
          baseP       = mmap((void*)map_pointer,
                             mapsize,
                             PROT_READ|PROT_WRITE,
                             MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,
                             -1, 0 );
        }
      SHMEMI_bcast(&map_pointer,8);
      if ( _g.rank != 0)
        {
          int rc = SHMEMI_Check_maprange((void*)map_pointer,mapsize);
          if(rc)
            {
              baseP = mmap((void*)map_pointer,
                           mapsize,
                           PROT_READ|PROT_WRITE,
                           MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,
                           -1, 0);
            }
          else
            baseP = (void*)-1ULL;
        }
      if(mapsize == 0) baseP = (void*)map_pointer;

      test = ((uintptr_t)baseP != -1ULL)?1:0;
      SHMEMI_allreduce(&test,&result); // unsigned band
      if(result == 0 && baseP!=(void*)-1ULL)
        munmap(baseP, mapsize);
    }
  if(iter == 0)
    {
      fprintf(stderr, "WARNING: heap allocate:  Unable to allocate symmetric heap\n");
      baseP = malloc(size);
    }
  return baseP;
}

__SI__
void SHMEMI_Shfree(void *ptr)
{
  long  page_sz = sysconf(_SC_PAGESIZE);
  char *back    = (char*)ptr - page_sz;
  size_t *valptr= (size_t*)back;
  size_t val   = *valptr;

  SHMEMI_barrier();
  munmap(back, val);
}

__SI__
void *SHMEMI_Shmalloc (size_t size)
{
  SHMEMI_barrier();
  long    page_sz = sysconf(_SC_PAGESIZE);
  void   *m       = SHMEMI_Symmetric_heap(size+page_sz);
  char   *ptr     = (char*)m + page_sz;
  size_t *val     = (size_t*)m;
  *val            = size;
  return (void*)ptr;
}

__SI__
void SHMEMI_Barrier_all (void)
{
  SHMEMI_quiet();
  SHMEMI_barrier();
}

__SI__
void SHMEMI_Long_put (long       *dest,
                      const long *src,
                      size_t      nelems,
                      int         pe)
{
  ERRCHKSFI(fi_write(_g.endpoint,         /* Endpoint            */
                       src,                 /* Origin buffer       */
                       nelems*sizeof(long), /* buffer size         */
                       _g.mr,               /* Memory region       */
                       pe,                  /* Destination Address */
                       (uint64_t)dest,      /* Target buffer       */
                       0ULL,                /* Key                 */
                       NULL));

  _g.put_ctr_pending++;
  SHMEMI_quiet();
}

__SI__
void SHMEMI_Wait (long *ivar,long  cmp_value)
{
  uint64_t count;
  while (*(ivar) == cmp_value)
    {
      count = fi_cntr_read(_g.targetcntr);
      asm volatile("" ::: "memory");
      if (*(ivar) != cmp_value) return;
      ERRCHKSFI(fi_cntr_wait(_g.targetcntr,
                             (count + 1),
                             -1));

    }
}


#endif /* SHMACRO_SHMEM_INCLUDE */
