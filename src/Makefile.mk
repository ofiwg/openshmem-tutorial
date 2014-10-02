

include_HEADERS += src/shmem.h src/shmem_impl.h

libshmem_la_SOURCES += src/shmem.c

include $(srcdir)/src/pm/Makefile.mk

