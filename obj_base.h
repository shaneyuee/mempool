// obj_base.h
// Describing interfaces needed by ObjPool
//
// Shaneyu@tencent.com
//
// 2013-08-28	shaneyu 	Created
//

#ifndef __UC_OBJ_BASE__
#define __UC_OBJ_BASE__

template<class obj_type> class ObjPool;

// Base object class for ObjPool
// Objects can be linked in a list through next member
class ObjBase
{
public:
	ObjBase(): prev(NULL), next(NULL) { }
	virtual ~ObjBase() { }

	void *prev; // previous object in obj
	void *next; // Next object in ObjPool

	virtual void ReleaseObject() = 0;
};


#endif

