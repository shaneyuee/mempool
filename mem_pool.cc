// mem_pool.cc
// A memory buffer pool based on ObjPool
//
// Shaneyu@tencent.com
//
// 2013-1-15	shaneyu 	Created
//
#include <iostream>
#include "mem_pool.h"


UcMem *UcMemPool::Alloc(bool &exceed_limit)
{
	exceed_limit = false;
	UcMem *m = pool.New();
	if(m==NULL)
	{
		Attr_API(380762, 1); // pool new failed
		return NULL;
	}

	m->pool = this;
	if(m->mem==NULL)
	{
		// allocate memory here
		Attr_API(380763, 1); // allocate new memory

		if(sz_total + sz_each > sz_max) // out of memory
		{
			Attr_API(380764, 1); // limit reached
			pool.Delete(m);
			exceed_limit = true;
			return NULL;
		}

		m->mem = malloc(sz_each);
		if(m->mem==NULL)
		{
			Attr_API(380765, 1); // malloc failed
			pool.Delete(m);
			return NULL;
		}
		sz_total += sz_each;
	}
	else // pick up old one
	{
		if(sz_free<sz_each)
		{
			Attr_API(380766, 1); // Bug
		}
		else
		{
			sz_free -= sz_each;
		}
		sz_total += sz_each;
	}
	return m;
}

void UcMemPool::Free(UcMem *m)
{
	pool.Delete(m);
	sz_free += sz_each;
	if(sz_total < sz_each)
	{
		Attr_API(380767, 1); // Bug
	}
	else
	{
		sz_total -= sz_each;
	}
}


uint64_t UcMemPool::Shrink()
{
	UcMem *m;
	uint64_t shk, sz_allocated, max_free;

	sz_allocated = sz_total + sz_free;
	if(sz_max > sz_allocated*2) // un-allocated > allocated
	{
		// shrink 1/2 un-allocated sz, allign to sz_each
		shk = ((sz_max - sz_allocated) / 2) & (~(sz_each - 1));
		if(shk > sz_each)
		{
			Attr_API(380768, 1); // shrink un-allocated space
			sz_max -= shk;
			return shk;
		}
	}

	shk = 0;
	max_free = sz_max/4;
	if(max_free > sz_total)
		max_free = sz_total;

	while(sz_free > max_free)
	{
		m = pool.New();
		if(m)
		{
			if(m->mem==NULL)
			{
				Attr_API(380769, 1); // Bug: has sz_free but no free element
				pool.Delete(m);
				break;
			}
			// free from free space
			free(m->mem);
			m->mem = NULL;
			pool.Delete(m);
			if(sz_free < sz_each)
			{
				Attr_API(380770, 1); // Bug
			}
			else
			{
				sz_free -= sz_each;
			}
			shk += sz_each;
		}
		else
		{
			Attr_API(380771, 1); // Bug: has sz_free but no free element
			break;
		}
	}
	if(sz_max<shk)
	{
		Attr_API(380772, 1); // Bug
	}
	else
	{
		sz_max -= shk;
	}
	return shk;
}

// each thread shall have its own set of pools
static __thread struct
{
	uint64_t sz;
	UcMemPool *pool;
} magics[] = {
	{ 64, NULL },
	{ 256, NULL },
	{ 1024, NULL },
	{ 4096, NULL },
	{ 16384, NULL },
	{ 65536, NULL },
	{ 262144, NULL },
	{ 1048576, NULL },
	{ 4194304, NULL },
	{ 16777216, NULL }
};

#define biggest_magic 16777216
#define nr_magics (sizeof(magics)/sizeof(magics[0]))

static int get_magic(uint64_t i)
{
	if(i<=biggest_magic)
	{
		//bsearch
		int low = 0, high = nr_magics-1, mid;
		while(low < high)
		{
			mid = (low + high) / 2;
			if(i==magics[mid].sz)
				return mid;

			if(i>magics[mid].sz)
			{
				if(low<mid)
					low = mid;
				else
					low ++;
			}
			else
			{
				if(high>mid)
					high = mid;
				else
					high --;
			}
		}
		return low;
	}
	else
	{
		Attr_API(380773, 1); // allocated size larger than supported
		return -1;
	}
}

inline UcMemPool *UcMemManager::GetPool(int magic)
{
	if(magics[magic].pool==NULL)
	{
		try {
			magics[magic].pool = new UcMemPool;
		} catch(...){
			return NULL;
		}
		magics[magic].pool->sz_each = magics[magic].sz;
		magics[magic].pool->sz_max = sz_max/nr_magics;
	}
	return magics[magic].pool;
}

inline uint64_t UcMemManager::Shrink(int magic)
{
	int i;
	uint64_t shk = 0, sz_needed = magics[magic].sz;
	// shrink pool larger than sz_needed
	for(i=nr_magics-1; i>magic; i--)
	{
		UcMemPool *p = GetPool(i);
		if(p==NULL) continue;
		shk += p->Shrink();
		if(shk>=sz_needed)
		{
			Attr_API(380774, 1);
			return shk;
		}
	}

	// cout << "shrink "<<shk<<" bytes from larger pool" << endl;

	// shrink pool smaller than sz_needed
	Attr_API(380775, 1);
	for(i--; i>=0; i--)
	{
		UcMemPool *p = GetPool(i);
		if(p)
			shk += p->Shrink();
	}

	// cout << "shrink "<<shk<<" bytes in total" << endl;

	return shk;
}


UcMem *UcMemManager::Alloc(uint64_t sz)
{
	UcMem *m = NULL;
	bool exceed_limit = false;
	int magic = get_magic(sz);
	uint64_t alloc_sz = magic>=0? magics[magic].sz : sz;
	UcMemPool *pool = NULL;
	UcMemManager *mng = GetInstance();

	try
	{
		if(alloc_sz > biggest_magic)
		{
			Attr_API(380776, 1); // allocate directly
			return new UcMem(sz);
		}

		pool = mng->GetPool(magic);
		if(pool==NULL) return NULL;
		m = pool->Alloc(exceed_limit);
	}
	catch(...)
	{
		m = NULL;
	}

	if(m==NULL && exceed_limit)
	{
		uint64_t shk = mng->Shrink(magic);
		if(shk)
		{
			pool->sz_max += shk;

			Attr_API(380777, 1); // try to re-allocated ater shrinking

			m = pool->Alloc(exceed_limit);
			if(m==NULL && exceed_limit)
			{
				Attr_API(380778, 1); // Bug
			}
			if(m)
			{
				Attr_API(380779, 1);
			}
		}
		else
		{
			Attr_API(380780, 1); // still out of limit
		}
	}
	return m;
}

void UcMemManager::Free(UcMem *m)
{
	if(m)
	{
		if(m->pool)
			m->pool->Free(m);
		else // allocated directly
			delete m;
	}
}

UcMemManager *UcMemManager::GetInstance()
{
	static UcMemManager g_mp_manager(1024*1024*1024UL);
	return &g_mp_manager;
}


