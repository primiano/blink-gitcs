/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2003, 2008 Apple Computer, Inc.
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
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#ifndef ScopeChain_h
#define ScopeChain_h

#include <wtf/Assertions.h>

namespace KJS {

    class JSGlobalObject;
    class JSObject;
    class ScopeChainIterator;
    
    class ScopeChainNode {
    public:
        ScopeChainNode(ScopeChainNode* n, JSObject* o, JSObject* gt)
            : next(n)
            , object(o)
            , globalThis(gt)
            , refCount(1)
        {
        }

        ScopeChainNode* next;
        JSObject* object;
        JSObject* globalThis;
        int refCount;

        void deref() { if (--refCount == 0) release(); }
        void ref() { ++refCount; }
        void release();

        // Before calling "push" on a bare ScopeChainNode, a client should
        // logically "copy" the node. Later, the client can "deref" the head
        // of its chain of ScopeChainNodes to reclaim all the nodes it added
        // after the logical copy, leaving nodes added before the logical copy
        // (nodes shared with other clients) untouched.
        ScopeChainNode* copy()
        {
            ref();
            return this;
        }

        JSObject* bottom() const;

        ScopeChainNode* push(JSObject*);
        ScopeChainNode* pop();

        ScopeChainIterator begin() const;
        ScopeChainIterator end() const;
        
        JSGlobalObject* globalObject() const; // defined in JSGlobalObject.h
        JSObject* globalThisObject() const { return globalThis; }
        
#ifndef NDEBUG        
        void print() const;
#endif
    };

    inline ScopeChainNode* ScopeChainNode::push(JSObject* o)
    {
        ASSERT(o);
        return new ScopeChainNode(this, o, globalThis);
    }

    inline ScopeChainNode* ScopeChainNode::pop()
    {
        ASSERT(next);
        ScopeChainNode* result = next;
        
        if (--refCount != 0)
            ++result->refCount;
        else
            delete this;
        
        return result;
    }

    inline JSObject* ScopeChainNode::bottom() const
    {
        const ScopeChainNode* n = this;
        while (n->next)
            n = n->next;
        return n->object;
    }

    inline void ScopeChainNode::release()
    {
        // This function is only called by deref(),
        // Deref ensures these conditions are true.
        ASSERT(refCount == 0);
        ScopeChainNode* n = this;
        do {
            ScopeChainNode* next = n->next;
            delete n;
            n = next;
        } while (n && --n->refCount == 0);
    }

    class ScopeChainIterator {
    public:
        ScopeChainIterator(const ScopeChainNode* node)
            : m_node(node)
        {
        }

        JSObject* const & operator*() const { return m_node->object; }
        JSObject* const * operator->() const { return &(operator*()); }
    
        ScopeChainIterator& operator++() { m_node = m_node->next; return *this; }

        // postfix ++ intentionally omitted

        bool operator==(const ScopeChainIterator& other) const { return m_node == other.m_node; }
        bool operator!=(const ScopeChainIterator& other) const { return m_node != other.m_node; }

    private:
        const ScopeChainNode* m_node;
    };

    inline ScopeChainIterator ScopeChainNode::begin() const
    {
        return ScopeChainIterator(this); 
    }

    inline ScopeChainIterator ScopeChainNode::end() const
    { 
        return ScopeChainIterator(0); 
    }

    class ScopeChain {
    public:
    ScopeChain(JSObject* o, JSObject* globalThis)
            : _node(new ScopeChainNode(0, o, globalThis))
        {
        }

        ScopeChain(const ScopeChain& c)
            : _node(c._node->copy())
        {
        }

        ScopeChain& operator=(const ScopeChain& c);

        explicit ScopeChain(ScopeChainNode* node)
            : _node(node->copy())
        {
        }
    
        ~ScopeChain() { _node->deref(); }

        void swap(ScopeChain&);

        ScopeChainNode* node() const { return _node; }

        JSObject* top() const { return _node->object; }
        JSObject* bottom() const { return _node->bottom(); }

        ScopeChainIterator begin() const { return _node->begin(); }
        ScopeChainIterator end() const { return _node->end(); }

        void push(JSObject* o) { _node = _node->push(o); }

        void pop() { _node = _node->pop(); }
        void clear() { _node->deref(); _node = 0; }
        
        JSGlobalObject* globalObject() const { return _node->globalObject(); }

        void mark() const;

#ifndef NDEBUG        
        void print() const { _node->print(); }
#endif

    private:
        ScopeChainNode* _node;
    };

    inline void ScopeChain::swap(ScopeChain& o)
    {
        ScopeChainNode* tmp = _node;
        _node = o._node;
        o._node = tmp;
    }

    inline ScopeChain& ScopeChain::operator=(const ScopeChain& c)
    {
        ScopeChain tmp(c);
        swap(tmp);
        return *this;
    }

} // namespace KJS

#endif // ScopeChain_h
