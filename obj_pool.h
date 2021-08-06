// obj_pool.h
// A general-purpose object pool
//
// Shaneyu@tencent.com
//
// 2013-08-28	shaneyu 	Created
// 2013-10-12	shaneyu		Use pointer in set<> instead of object
// 2014-02-04   shaneyu		Use pointer to replace iterator
// 2014-03-15   shaneyu		Remove the use of vector
//
#include <stdint.h>
#include <string.h>

#include "obj_base.h"
extern "C"
{
#include "Attr_API.h"
}

#ifndef __UC_OBJ_POOL__
#define __UC_OBJ_POOL__

using namespace std;

//
// ��̬���������󼯺ϵĶ����
//
// ͨ��ָ��ʵ��
// �� obj_type ��Լ����
// 1, obj_type�������int���͵�curr��next��Ա����
// 2, obj_type����ʵ��ReleaseObject()��Ա���������ձ��������Դ
//
template<class obj_type> class ObjPool
{
public:
	ObjPool() : obj_freelist(NULL) { }
	~ObjPool() { while(obj_freelist) { obj_type *next=(obj_type *)obj_freelist->next; delete obj_freelist; obj_freelist = next; } }

	obj_type *New(); // ������������
	int Delete(obj_type *obj); // ɾ����������obj

	obj_type *New(obj_type *&first); // �������󲢲嵽firstָ�������β��
	obj_type *NewFront(obj_type *&first); // �������󲢲嵽firstָ�������ͷ��
	int Delete(obj_type *&first, obj_type *obj); // ��firstָ��Ķ���������ɾ������obj
	int Detach(obj_type *&first, obj_type *obj); // ��firstָ��Ķ����������Ƴ�����obj���û���Ҫ����Delete(obj)�����ͷ�
	int DeleteAll(obj_type *&first); // ɾ��firstָ�����������
	int AddToFreeList(obj_type *first);

private:
	obj_type *obj_freelist; // �ͷź��obj������
};

//
// ʵ�ֲ���
//

template<class obj_type> inline obj_type *ObjPool<obj_type>::New()
{
	obj_type *o;

	if(obj_freelist)
	{
		o = obj_freelist;
		obj_freelist = (obj_type *)obj_freelist->next;
	}
	else
	{
		Attr_API(390992, 1);
		try {
			o = new obj_type();
		}catch(...) {
			Attr_API(391020, 1);
			return NULL;
		}
	}

	// ���뵽obj����
	o->next = NULL;
	o->prev = o;
	return o;
}

template<class obj_type> inline int ObjPool<obj_type>::Delete(obj_type *obj)
{
	obj->ReleaseObject(); // NOTE: ReleaseObject() should not change prev/next pointers

	// ��obj�ŵ�free����
	obj->next = obj_freelist;
	obj_freelist = obj;
	return 0;
}

template<class obj_type> inline obj_type *ObjPool<obj_type>::New(obj_type *&first)
{
	obj_type *o;

	if(obj_freelist)
	{
		o = obj_freelist;
		obj_freelist = (obj_type *)obj_freelist->next;
	}
	else
	{
		Attr_API(390992, 1);
		try {
			o = new obj_type();
		}catch(...) {
			Attr_API(391020, 1);
			return NULL;
		}
	}

	// ���뵽obj����
	o->next = NULL;
	if(first)
	{
		obj_type *lst = (obj_type *)first->prev; // must not be null
		lst->next = o;
		o->prev = lst;
		first->prev = o;
	}
	else
	{
		first = o;
		o->prev = o;
	}
	return o;
}

template<class obj_type> inline obj_type *ObjPool<obj_type>::NewFront(obj_type *&first)
{
	obj_type *o;

	if(obj_freelist)
	{
		o = obj_freelist;
		obj_freelist = (obj_type *)obj_freelist->next;
	}
	else
	{
		Attr_API(390992, 1);
		try {
			o = new obj_type();
		}catch(...) {
			Attr_API(391020, 1);
			return NULL;
		}
	}

	// ���뵽obj����ͷ��
	o->next = first;
	if(first)
	{
		o->prev = first->prev;
		first->prev = o;
	}
	else
	{
		o->prev = o;
	}
	first = o;
	return o;
}



template<class obj_type> inline int ObjPool<obj_type>::Delete(obj_type *&first, obj_type *obj)
{
	if(obj==first)
	{
		if(obj->next) // new first
			((obj_type *)obj->next)->prev = first->prev;
		first = (obj_type *)obj->next;
	}
	else
	{
		obj_type *p = (obj_type *)obj->prev; // must not null
		obj_type *n = (obj_type *)obj->next;
		p->next = n;
		if(n)
			n->prev = p;
		else // obj is the last
			first->prev = p;
	}

	obj->ReleaseObject();

	obj->next = obj_freelist;
	obj_freelist = obj;

	return 0;
}

template<class obj_type> inline int ObjPool<obj_type>::Detach(obj_type *&first, obj_type *obj)
{
	if(obj==first)
	{
		if(obj->next) // new first
			((obj_type *)obj->next)->prev = first->prev;
		first = (obj_type *)obj->next;
	}
	else
	{
		obj_type *p = (obj_type *)obj->prev; // must not null
		obj_type *n = (obj_type *)obj->next;
		p->next = n;
		if(n)
			n->prev = p;
		else // obj is the last
			first->prev = p;
	}

	obj->next = obj->prev = NULL;
	return 0;
}

template<class obj_type> inline int ObjPool<obj_type>::AddToFreeList(obj_type *obj)
{
	((obj_type *)obj->prev)->next = obj_freelist;
	obj_freelist = obj;
	return 0;
}

template<class obj_type> inline int ObjPool<obj_type>::DeleteAll(obj_type *&first)
{
	obj_type *o = first;
	while(o)
	{
		o->ReleaseObject(); // NOTE: ReleaseObject() should not change prev/next pointers
		o = (obj_type *)o->next;
	}

	// ��obj�ŵ�free����
	AddToFreeList(first);
	first = NULL;
	return 0;
}

#endif
