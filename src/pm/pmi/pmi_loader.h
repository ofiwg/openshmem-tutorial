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

#if !defined(PMI_LOADER_H_INCLUDED)
#define PMI_LOADER_H_INCLUDED
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

#define ADD_SUFFIX(x) x##_fn
#define DECL_API(rc,fcnname,...)                    \
  typedef rc (*ADD_SUFFIX(fcnname))(__VA_ARGS__);   \
  rc PMI_##fcnname(__VA_ARGS__)

typedef struct PMI_keyval_t
{
  const char * key;
  char * val;
} PMI_keyval_t;

#define PMI_SUCCESS 0


DECL_API(int,Init,int *);
DECL_API(int,Initialized,int *);
DECL_API(int,Finalize,void);
DECL_API(int,Get_size,int*);
DECL_API(int,Get_rank,int*);
DECL_API(int,Get_universe_size,int*);
DECL_API(int,Get_appnum,int*);
DECL_API(int,Publish_name,const char[], const char[]);
DECL_API(int,Unpublish_name,const char[]);
DECL_API(int,Lookup_name,const char[], char[]);
DECL_API(int,Barrier,void);
DECL_API(int,Abort,int, const char[]);
DECL_API(int,KVS_Get_my_name,char[],int);
DECL_API(int,KVS_Get_name_length_max,int*);
DECL_API(int,KVS_Get_key_length_max,int*);
DECL_API(int,KVS_Get_value_length_max,int*);
DECL_API(int,KVS_Put,const char[],const char[],const char[]);
DECL_API(int,KVS_Commit,const char[]);
DECL_API(int,KVS_Get, const char[], const char[], char[],int);
DECL_API(int,Spawn_multiple,int,const char*[],const char**[],
         const int[],const int[],const PMI_keyval_t*[],
         int,const PMI_keyval_t[],int[]);

#define DECL_PMI(x) x##_fn x
typedef struct PMIFunc
{
  DECL_PMI(Init);
  DECL_PMI(Initialized);
  DECL_PMI(Finalize);
  DECL_PMI(Get_size);
  DECL_PMI(Get_rank);
  DECL_PMI(Get_universe_size);
  DECL_PMI(Get_appnum);
  DECL_PMI(Publish_name);
  DECL_PMI(Unpublish_name);
  DECL_PMI(Lookup_name);
  DECL_PMI(Barrier);
  DECL_PMI(Abort);
  DECL_PMI(KVS_Get_my_name);
  DECL_PMI(KVS_Get_name_length_max);
  DECL_PMI(KVS_Get_key_length_max);
  DECL_PMI(KVS_Get_value_length_max);
  DECL_PMI(KVS_Put);
  DECL_PMI(KVS_Commit);
  DECL_PMI(KVS_Get);
  DECL_PMI(Spawn_multiple);
}PMIFunc;

static const char *PMI_cmd_list[] =
{
  "PMI_Init",
  "PMI_Initialized",
  "PMI_Finalize",
  "PMI_Get_size",
  "PMI_Get_rank",
  "PMI_Get_universe_size",
  "PMI_Get_appnum",
  "PMI_Publish_name",
  "PMI_Unpublish_name",
  "PMI_Lookup_name",
  "PMI_Barrier",
  "PMI_Abort",
  "PMI_KVS_Get_my_name",
  "PMI_KVS_Get_name_length_max",
  "PMI_KVS_Get_key_length_max",
  "PMI_KVS_Get_value_length_max",
  "PMI_KVS_Put",
  "PMI_KVS_Commit",
  "PMI_KVS_Get",
  "PMI_Spawn_multiple",
};

enum
{
  I_PMI_Init = 0,
  I_PMI_Initialized,
  I_PMI_Finalize,
  I_PMI_Get_size,
  I_PMI_Get_rank,
  I_PMI_Get_universe_size,
  I_PMI_Get_appnum,
  I_PMI_Publish_name,
  I_PMI_Unpublish_name,
  I_PMI_Lookup_name,
  I_PMI_Barrier,
  I_PMI_Abort,
  I_PMI_KVS_Get_my_name,
  I_PMI_KVS_Get_name_length_max,
  I_PMI_KVS_Get_key_length_max,
  I_PMI_KVS_Get_value_length_max,
  I_PMI_KVS_Put,
  I_PMI_KVS_Commit,
  I_PMI_KVS_Get,
  I_PMI_Spawn_multiple
};

static inline void * pmi_import(void       *dlopen_file,
                                const char *funcname)
{
  dlerror();    /* Clear any existing error */
  void *handle = NULL;
  char *error  = NULL;
  handle       = dlsym(dlopen_file, funcname);
  if ((error = dlerror()) != NULL)
    {
      fprintf(stderr, "Error when loading PMI function %s: %s\n",
              funcname, error);
      handle = NULL;
    }
  return handle;
}

#define PMI_LDTABLE(x) table->x = (x##_fn)pmi_import(dlopen_obj, PMI_cmd_list[I_PMI_##x])
static inline int pmi_load(PMIFunc *table)
{
  void            *dlopen_obj  = NULL;
  char            *filename    = (char*)"libmpi.so";
  char            *c           = getenv("PMI_MODULE");
  if(!c) c = filename;

  dlopen_obj = dlopen(c, RTLD_NOW | RTLD_GLOBAL);
  if (NULL == dlopen_obj)
    {
      fprintf(stderr, "PMI_LOAD:  Error opening %s: %s\n", filename, dlerror());
      return -1;
    }
  PMI_LDTABLE(Init);
  PMI_LDTABLE(Initialized);
  PMI_LDTABLE(Finalize);
  PMI_LDTABLE(Get_size);
  PMI_LDTABLE(Get_rank);
  PMI_LDTABLE(Get_universe_size);
  PMI_LDTABLE(Get_appnum);
  PMI_LDTABLE(Publish_name);
  PMI_LDTABLE(Unpublish_name);
  PMI_LDTABLE(Lookup_name);
  PMI_LDTABLE(Barrier);
  PMI_LDTABLE(Abort);
  PMI_LDTABLE(KVS_Get_my_name);
  PMI_LDTABLE(KVS_Get_name_length_max);
  PMI_LDTABLE(KVS_Get_key_length_max);
  PMI_LDTABLE(KVS_Get_value_length_max);
  PMI_LDTABLE(KVS_Put);
  PMI_LDTABLE(KVS_Commit);
  PMI_LDTABLE(KVS_Get);
  PMI_LDTABLE(Spawn_multiple);
  return 0;
}

#define FUNC_OBJ global_pmi_func
#define CALL_FUNC(obj,x) obj.x
#define PMI_Init                     CALL_FUNC(FUNC_OBJ,Init)
#define PMI_Initialized              CALL_FUNC(FUNC_OBJ,Initialized)
#define PMI_Finalize                 CALL_FUNC(FUNC_OBJ,Finalize)
#define PMI_Get_size                 CALL_FUNC(FUNC_OBJ,Get_size)
#define PMI_Get_rank                 CALL_FUNC(FUNC_OBJ,Get_rank)
#define PMI_Get_universe_size        CALL_FUNC(FUNC_OBJ,Get_universe_size)
#define PMI_Get_appnum               CALL_FUNC(FUNC_OBJ,Get_appnum)
#define PMI_Publish_name             CALL_FUNC(FUNC_OBJ,Publish_name)
#define PMI_Unpublish_name           CALL_FUNC(FUNC_OBJ,Unpublish_name)
#define PMI_Lookup_name              CALL_FUNC(FUNC_OBJ,Lookup_name)
#define PMI_Barrier                  CALL_FUNC(FUNC_OBJ,Barrier)
#define PMI_Abort                    CALL_FUNC(FUNC_OBJ,Abort)
#define PMI_KVS_Get_my_name          CALL_FUNC(FUNC_OBJ,KVS_Get_my_name)
#define PMI_KVS_Get_name_length_max  CALL_FUNC(FUNC_OBJ,KVS_Get_name_length_max)
#define PMI_KVS_Get_key_length_max   CALL_FUNC(FUNC_OBJ,KVS_Get_key_length_max)
#define PMI_KVS_Get_value_length_max CALL_FUNC(FUNC_OBJ,KVS_Get_value_length_max)
#define PMI_KVS_Put                  CALL_FUNC(FUNC_OBJ,KVS_Put)
#define PMI_KVS_Commit               CALL_FUNC(FUNC_OBJ,KVS_Commit)
#define PMI_KVS_Get                  CALL_FUNC(FUNC_OBJ,KVS_Get)
#define PMI_Spawn_multiple           CALL_FUNC(FUNC_OBJ,Spawn_multiple)

extern PMIFunc global_pmi_func;


#endif /* PMI_H_INCLUDED */
