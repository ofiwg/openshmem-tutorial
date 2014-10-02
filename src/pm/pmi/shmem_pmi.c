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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "pm/shmem_pm.h"
#include "pm/pmi/pmi_loader.h"


#define KVSNAMELEN 1024
#define KVSKEYLEN  1024
#define KVSVALLEN  1024

PMIFunc global_pmi_func;


/* ************************************************************************** */
/* Error checking macros                                                      */
/* ************************************************************************** */
#define ERRCHKPMI(ret)                                          \
  if(ret != 0)                                                  \
    {                                                           \
      fprintf(stderr, "PMI Error Line %d:%d File=%s\n",__LINE__,ret,__FILE__);   \
      exit(-1);                                                 \
    }

/* ************************************************************************** */
/* Utility/helper functions                                                   */
/* ************************************************************************** */
void SHMEMI_Encode(const void *in, int inlen, char *out, int outlen)
{
  char        hex[]= "0123456789abcdef";
  const char *bin  = (char*)in;
  int         i;
  if((inlen*2+1) > outlen)
    {
      fprintf(stderr, "Fatal Encoding Error for PMI string\n");
      exit(1);
    }
  for (i = 0; i < inlen; i++)
    {
      out[i*2+0] = hex[(bin[i])&0xF];
      out[i*2+1] = hex[(bin[i]>>4)];
    }
  out[inlen * 2] = 0;
}

int SHMEMI_Decode(const char *in, void *out, int outlen)
{
  int i;
  if (outlen != (int)strlen(in) / 2) {
    return 1;
  }
  uint8_t *val = (uint8_t*)out;
  for (i = 0 ; i < outlen ; ++i) {
    char c[4] = {in[i*2], '\0', in[i*2+1], '\0'};
    val[i]    = atoi(&c[0]) | atoi(&c[2])<<4;
  }
  return 0;
}

void SHMEMI_PM_Barrier(void)
{
  PMI_Barrier();
}

void SHMEMI_PM_Finalize(void)
{
  PMI_Finalize();
}

void SHMEMI_PM_Initialize(int *jobid, int *rank, int *nranks)
{
  int spawned;

  pmi_load(&global_pmi_func);

  ERRCHKPMI(PMI_Init(&spawned));

  atexit(SHMEMI_PM_Finalize);
  ERRCHKPMI(PMI_Get_appnum(jobid));
  ERRCHKPMI(PMI_Get_rank(rank));
  ERRCHKPMI(PMI_Get_size(nranks));
}

/**
 * \brief foo
 *
 * \param[in] name
 * \param[in] rank
 */
void SHMEMI_PM_Exchange(char const *name, unsigned rank, unsigned maxlen,
                        char *names, unsigned nranks)
{
  unsigned i;
  char  pmi_key[KVSKEYLEN];
  char  addrStr[KVSVALLEN];
  char  kvsname[KVSNAMELEN];

  memset(addrStr, 0, sizeof(addrStr));

  SHMEMI_Encode(name,maxlen,addrStr,KVSVALLEN);

  sprintf(pmi_key, "shmem-pmi-%d",rank);

  ERRCHKPMI(PMI_KVS_Get_my_name(kvsname, KVSNAMELEN));
  ERRCHKPMI(PMI_KVS_Put(kvsname, pmi_key, addrStr));
  ERRCHKPMI(PMI_KVS_Commit(kvsname));
  ERRCHKPMI(PMI_Barrier());

  char *addrNames = (char*)malloc(nranks*maxlen);
  assert(addrNames);

  for(i=0;i<nranks;i++)
    {
      sprintf(pmi_key, "shmem-pmi-%d", i);
      memset(addrStr, 0, sizeof(addrStr));
      ERRCHKPMI(PMI_KVS_Get(kvsname, pmi_key, addrStr, KVSVALLEN));
      SHMEMI_Decode(addrStr,&names[i*maxlen],maxlen);
    }

  free(addrNames);
}


