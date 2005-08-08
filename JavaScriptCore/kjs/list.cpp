/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2003 Apple Computer, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#include "list.h"

#include "internal.h"

#define DUMP_STATISTICS 0

namespace KJS {

// tunable parameters
const int poolSize = 512;
const int inlineValuesSize = 4;


enum ListImpState { unusedInPool = 0, usedInPool, usedOnHeap, immortal };

struct ListImp : ListImpBase
{
    ListImpState state;
    ValueImp *values[inlineValuesSize];
    int capacity;
    ValueImp **overflow;

    ListImp *nextInFreeList;
    ListImp *nextInOutsideList;
    ListImp *prevInOutsideList;

#if DUMP_STATISTICS
    int sizeHighWaterMark;
#endif

    void markValues();
};

static ListImp pool[poolSize];
static ListImp *poolFreeList;
static ListImp *outsidePoolList;
static int poolUsed;

#if DUMP_STATISTICS

static int numLists;
static int numListsHighWaterMark;

static int listSizeHighWaterMark;

static int numListsDestroyed;
static int numListsBiggerThan[17];

struct ListStatisticsExitLogger { ~ListStatisticsExitLogger(); };

static ListStatisticsExitLogger logger;

ListStatisticsExitLogger::~ListStatisticsExitLogger()
{
    printf("\nKJS::List statistics:\n\n");
    printf("%d lists were allocated\n", numLists);
    printf("%d lists was the high water mark\n", numListsHighWaterMark);
    printf("largest list had %d elements\n", listSizeHighWaterMark);
    if (numListsDestroyed) {
        putc('\n', stdout);
        for (int i = 0; i < 17; i++) {
            printf("%.1f%% of the lists (%d) had more than %d element%s\n",
                100.0 * numListsBiggerThan[i] / numListsDestroyed,
                numListsBiggerThan[i],
                i, i == 1 ? "" : "s");
        }
        putc('\n', stdout);
    }
}

#endif


inline void ListImp::markValues()
{
    int inlineSize = MIN(size, inlineValuesSize);
    for (int i = 0; i != inlineSize; ++i) {
	if (!values[i]->marked()) {
	    values[i]->mark();
	}
    }

    int overflowSize = size - inlineSize;
    for (int i = 0; i != overflowSize; ++i) {
	if (!overflow[i]->marked()) {
	    overflow[i]->mark();
	}
    }
}

void List::markProtectedLists()
{
    int seen = 0;
    for (int i = 0; i < poolSize; i++) {
        if (seen >= poolUsed)
            break;

        if (pool[i].state == usedInPool) {
            seen++;
            if (pool[i].valueRefCount > 0) {
                pool[i].markValues();
            }
        }
    }

    for (ListImp *l = outsidePoolList; l; l = l->nextInOutsideList) {
        if (l->valueRefCount > 0) {
            l->markValues();
        }
    }
}


static inline ListImp *allocateListImp()
{
    // Find a free one in the pool.
    if (poolUsed < poolSize) {
	ListImp *imp = poolFreeList ? poolFreeList : &pool[0];
	poolFreeList = imp->nextInFreeList ? imp->nextInFreeList : imp + 1;
	imp->state = usedInPool;
	poolUsed++;
	return imp;
    }
    
    ListImp *imp = new ListImp;
    imp->state = usedOnHeap;
    // link into outside pool list
    if (outsidePoolList) {
        outsidePoolList->prevInOutsideList = imp;
    }
    imp->nextInOutsideList = outsidePoolList;
    imp->prevInOutsideList = NULL;
    outsidePoolList = imp;

    return imp;
}

static inline void deallocateListImp(ListImp *imp)
{
    if (imp->state == usedInPool) {
        imp->state = unusedInPool;
	imp->nextInFreeList = poolFreeList;
	poolFreeList = imp;
	poolUsed--;
    } else {
        // unlink from outside pool list
        if (!imp->prevInOutsideList) {
            outsidePoolList = imp->nextInOutsideList;
            if (outsidePoolList) {
                outsidePoolList->prevInOutsideList = NULL;
            }
        } else {
            imp->prevInOutsideList->nextInOutsideList = imp->nextInOutsideList;
            if (imp->nextInOutsideList) {
                imp->nextInOutsideList->prevInOutsideList = imp->prevInOutsideList;
            }
        }

        delete imp;
    }
}

List::List() : _impBase(allocateListImp()), _needsMarking(false)
{
    ListImp *imp = static_cast<ListImp *>(_impBase);
    imp->size = 0;
    imp->refCount = 1;
    imp->valueRefCount = 1;
    imp->capacity = 0;
    imp->overflow = 0;
#if DUMP_STATISTICS
    if (++numLists > numListsHighWaterMark)
        numListsHighWaterMark = numLists;
    imp->sizeHighWaterMark = 0;
#endif
}

List::List(bool needsMarking) : _impBase(allocateListImp()), _needsMarking(needsMarking)
{
    ListImp *imp = static_cast<ListImp *>(_impBase);
    imp->size = 0;
    imp->refCount = 1;
    imp->valueRefCount = !needsMarking;
    imp->capacity = 0;
    imp->overflow = 0;

#if DUMP_STATISTICS
    if (++numLists > numListsHighWaterMark)
        numListsHighWaterMark = numLists;
    imp->sizeHighWaterMark = 0;
#endif
}

void List::markValues()
{
    static_cast<ListImp *>(_impBase)->markValues();
}

void List::release()
{
    ListImp *imp = static_cast<ListImp *>(_impBase);
    
#if DUMP_STATISTICS
    --numLists;
    ++numListsDestroyed;
    for (int i = 0; i < 17; i++)
        if (imp->sizeHighWaterMark > i)
            ++numListsBiggerThan[i];
#endif

    delete [] imp->overflow;
    deallocateListImp(imp);
}

ValueImp *List::at(int i) const
{
    ListImp *imp = static_cast<ListImp *>(_impBase);
    if ((unsigned)i >= (unsigned)imp->size)
        return jsUndefined();
    if (i < inlineValuesSize)
        return imp->values[i];
    return imp->overflow[i - inlineValuesSize];
}

void List::clear()
{
    _impBase->size = 0;
}

void List::append(ValueImp *v)
{
    ListImp *imp = static_cast<ListImp *>(_impBase);

    int i = imp->size++;

#if DUMP_STATISTICS
    if (imp->size > listSizeHighWaterMark)
        listSizeHighWaterMark = imp->size;
#endif

    if (i < inlineValuesSize) {
        imp->values[i] = v;
        return;
    }
    
    if (i >= imp->capacity) {
        int newCapacity = i * 2;
        ValueImp **newOverflow = new ValueImp * [newCapacity - inlineValuesSize];
        ValueImp **oldOverflow = imp->overflow;
        int oldOverflowSize = i - inlineValuesSize;
        for (int j = 0; j != oldOverflowSize; j++)
            newOverflow[j] = oldOverflow[j];
        delete [] oldOverflow;
        imp->overflow = newOverflow;
        imp->capacity = newCapacity;
    }
    
    imp->overflow[i - inlineValuesSize] = v;
}

List List::copy() const
{
    List copy;

    ListImp *imp = static_cast<ListImp *>(_impBase);

    int size = imp->size;

    int inlineSize = MIN(size, inlineValuesSize);
    for (int i = 0; i != inlineSize; ++i)
        copy.append(imp->values[i]);

    ValueImp **overflow = imp->overflow;
    int overflowSize = size - inlineSize;
    for (int i = 0; i != overflowSize; ++i)
        copy.append(overflow[i]);

    return copy;
}


List List::copyTail() const
{
    List copy;

    ListImp *imp = static_cast<ListImp *>(_impBase);

    int size = imp->size;

    int inlineSize = MIN(size, inlineValuesSize);
    for (int i = 1; i < inlineSize; ++i)
        copy.append(imp->values[i]);

    ValueImp **overflow = imp->overflow;
    int overflowSize = size - inlineSize;
    for (int i = 0; i < overflowSize; ++i)
        copy.append(overflow[i]);

    return copy;
}

const List &List::empty()
{
    static List emptyList;
    return emptyList;
}

List &List::operator=(const List &b)
{
    ListImpBase *bImpBase = b._impBase;
    ++bImpBase->refCount;
    if (!_needsMarking)
        ++bImpBase->valueRefCount;
    deref();
    _impBase = bImpBase;
    return *this;
}

} // namespace KJS
