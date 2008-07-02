// -*- mode: c++; c-basic-offset: 4 -*-
/*
 * This file is part of the KDE libraries
 * Copyright (C) 2005 Apple Computer, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef KJS_JSLock_h
#define KJS_JSLock_h

#include <wtf/Assertions.h>
#include <wtf/Noncopyable.h>

namespace KJS {

    // To make it safe to use JavaScript on multiple threads, it is
    // important to lock before doing anything that allocates a
    // JavaScript data structure or that interacts with shared state
    // such as the protect count hash table. The simplest way to lock
    // is to create a local JSLock object in the scope where the lock 
    // must be held. The lock is recursive so nesting is ok. The JSLock 
    // object also acts as a convenience short-hand for running important
    // initialization routines.

    // To avoid deadlock, sometimes it is necessary to temporarily
    // release the lock. Since it is recursive you actually have to
    // release all locks held by your thread. This is safe to do if
    // you are executing code that doesn't require the lock, and you
    // reacquire the right number of locks at the end. You can do this
    // by constructing a locally scoped JSLock::DropAllLocks object. The 
    // DropAllLocks object takes care to release the JSLock only if your
    // thread acquired it to begin with.

    // For per-thread contexts, JSLock doesn't do any locking, but we
    // still need to perform all the counting in order to keep debug
    // assertions working, so that clients that use a shared context don't break.

    class ExecState;

    class JSLock : Noncopyable {
    public:
        JSLock(ExecState* exec);

        JSLock(bool lockingForReal)
            : m_lockingForReal(lockingForReal)
        {
#ifdef NDEBUG
            // For per-thread contexts, locking is a debug-only feature.
            if (!lockingForReal)
                return;
#endif
            lock(lockingForReal);
            if (lockingForReal)
                registerThread();
        }

        ~JSLock()
        { 
#ifdef NDEBUG
            // For per-thread contexts, locking is a debug-only feature.
            if (!m_lockingForReal)
                return;
#endif
            unlock(m_lockingForReal); 
        }
        
        static void lock(bool);
        static void unlock(bool);
        static void lock(ExecState*);
        static void unlock(ExecState*);

        static int lockCount();
        static bool currentThreadIsHoldingLock();

        static void registerThread();

        bool m_lockingForReal;

        class DropAllLocks : Noncopyable {
        public:
            DropAllLocks(ExecState* exec);
            DropAllLocks(bool);
            ~DropAllLocks();
            
        private:
            int m_lockCount;
            bool m_lockingForReal;
        };
    };

} // namespace

#endif // KJS_JSLock_h
