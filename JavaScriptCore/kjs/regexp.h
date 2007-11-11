// -*- c-basic-offset: 2 -*-
/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2007 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef KJS_REGEXP_H
#define KJS_REGEXP_H

#include "ustring.h"
#include <pcre.h>
#include <sys/types.h>
#include <wtf/OwnArrayPtr.h>

namespace KJS {

  class RegExp : Noncopyable {
  private:
    enum { 
        Global = 1, 
        IgnoreCase = 2, 
        Multiline = 4 
    };

  public:
    RegExp(const UString& pattern);
    RegExp(const UString& pattern, const UString& flags);
    ~RegExp();
    
    void ref() { ++m_refCount; }
    void deref() { if (--m_refCount == 0) delete this; }
    int refCount() { return m_refCount; }

    bool global() const { return m_flags & Global; }
    bool ignoreCase() const { return m_flags & IgnoreCase; }
    bool multiline() const { return m_flags & Multiline; }
    const UString& pattern() const { return m_pattern; }

    bool isValid() const { return !m_constructionError; }
    const char* errorMessage() const { return m_constructionError; }

    int match(const UString&, int offset, OwnArrayPtr<int>* ovector = 0);
    unsigned numSubpatterns() const { return m_numSubpatterns; }

  private:
    void compile();
    
    int m_refCount;
    
    // Data supplied by caller.
    UString m_pattern;
    int m_flags;

    // Data supplied by PCRE.
    JSRegExp* m_regExp;
    const char* m_constructionError;
    unsigned m_numSubpatterns;
  };

} // namespace

#endif
