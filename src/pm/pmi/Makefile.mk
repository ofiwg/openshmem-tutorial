if BUILD_PM_PMI

libshmem_la_SOURCES  += src/pm/pmi/shmem_pmi.c
libshmem_la_CPPFLAGS += -I$(top_srcdir)/src
noinst_PROGRAMS      += pmitest
pmitest_SOURCES       = src/pm/pmi/pmitest.c
pmitest_CPPFLAGS      = -I$(top_srcdir)/src
pmitest_LDADD         = libshmem.la

endif
