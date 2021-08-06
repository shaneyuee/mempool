# default Attr_API
ifeq ($(HAS_ATTR_API),)
  BASELIB=/usr/local/commlib/baselib
  ifneq ($(shell ls $(BASELIB) 2>/dev/null), )
    HAS_ATTR_API=1
    ATTR_API_INC=-I $(BASELIB)
    ATTR_API_LIB=$(BASELIB)/libcomm_oi.a
  endif
endif

OPTFLAG=-O3 -finline-functions -falign-loops
CFLAGS=$(OPTFLAG) -Wall -pthread -fno-strict-aliasing -g
DEPS=
EXTLIBS=

ifeq ($(HAS_ATTR_API),1)
	CFLAGS += $(ATTR_API_INC) -DHAS_ATTR_API
	EXTLIBS += $(ATTR_API_LIB)
else
	DEPS += Attr_API.h
endif

all: mempool_test

mempool_test: $(DEPS) mem_pool.cc mempool_test.cpp
	g++ $(CFLAGS) -o mempool_test mem_pool.cc mempool_test.cpp $(EXTLIBS)

clean:
	rm -f *.o mempool_test
