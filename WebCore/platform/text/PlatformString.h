/*
 * (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef PlatformString_h
#define PlatformString_h

// This file would be called String.h, but that conflicts with <string.h>
// on systems without case-sensitive file systems.

#include "StringImpl.h"

#if PLATFORM(CF)
typedef const struct __CFString * CFStringRef;
#endif

#if PLATFORM(QT)
class QString;
#endif

#if PLATFORM(WX)
class wxString;
#endif


namespace WebCore {

class CString;
class DeprecatedString;
struct StringHash;

class String {
public:
    String() { } // gives null string, distinguishable from an empty string
    String(const UChar*, unsigned length);
    String(const UChar*); // Specifically for null terminated UTF-16
    String(const KJS::Identifier&);
    String(const KJS::UString&);
    String(const char*);
    String(const char*, unsigned length);
    String(StringImpl* i) : m_impl(i) { }
    String(PassRefPtr<StringImpl> i) : m_impl(i) { }
    String(RefPtr<StringImpl> i) : m_impl(i) { }

    static String adopt(StringBuffer& buffer) { return StringImpl::adopt(buffer); }
    static String adopt(Vector<UChar>& vector) { return StringImpl::adopt(vector); }

    operator KJS::Identifier() const;
    operator KJS::UString() const;

    unsigned length() const;
    const UChar* characters() const;
    const UChar* charactersWithNullTermination();
    
    UChar operator[](unsigned i) const; // if i >= length(), returns 0    
    UChar32 characterStartingAt(unsigned) const; // Ditto.
    
    bool contains(UChar c) const { return find(c) != -1; }
    bool contains(const char* str, bool caseSensitive = true) const { return find(str, 0, caseSensitive) != -1; }
    bool contains(const String& str, bool caseSensitive = true) const { return find(str, 0, caseSensitive) != -1; }

    int find(UChar c, int start = 0) const
        { return m_impl ? m_impl->find(c, start) : -1; }
    int find(const char* str, int start = 0, bool caseSensitive = true) const
        { return m_impl ? m_impl->find(str, start, caseSensitive) : -1; }
    int find(const String& str, int start = 0, bool caseSensitive = true) const
        { return m_impl ? m_impl->find(str.impl(), start, caseSensitive) : -1; }

    int reverseFind(UChar c, int start = -1) const
        { return m_impl ? m_impl->reverseFind(c, start) : -1; }
    int reverseFind(const String& str, int start = -1, bool caseSensitive = true) const
        { return m_impl ? m_impl->reverseFind(str.impl(), start, caseSensitive) : -1; }
    
    bool startsWith(const String& s, bool caseSensitive = true) const
        { return m_impl ? m_impl->startsWith(s.impl(), caseSensitive) : s.isEmpty(); }
    bool endsWith(const String& s, bool caseSensitive = true) const
        { return m_impl ? m_impl->endsWith(s.impl(), caseSensitive) : s.isEmpty(); }

    void append(const String&);
    void append(char);
    void append(UChar);
    void append(const UChar*, unsigned length);
    void insert(const String&, unsigned pos);
    void insert(const UChar*, unsigned length, unsigned pos);

    String& replace(UChar a, UChar b) { if (m_impl) m_impl = m_impl->replace(a, b); return *this; }
    String& replace(UChar a, const String& b) { if (m_impl) m_impl = m_impl->replace(a, b.impl()); return *this; }
    String& replace(const String& a, const String& b) { if (m_impl) m_impl = m_impl->replace(a.impl(), b.impl()); return *this; }
    String& replace(unsigned index, unsigned len, const String& b) { if (m_impl) m_impl = m_impl->replace(index, len, b.impl()); return *this; }

    void truncate(unsigned len);
    void remove(unsigned pos, int len = 1);

    String substring(unsigned pos, unsigned len = UINT_MAX) const;
    String left(unsigned len) const { return substring(0, len); }
    String right(unsigned len) const { return substring(length() - len, len); }

    // Returns a lowercase/uppercase version of the string
    String lower() const;
    String upper() const;

    String stripWhiteSpace() const;
    String simplifyWhiteSpace() const;
    
    // Return the string with case folded for case insensitive comparison.
    String foldCase() const;

    static String number(int);
    static String number(unsigned);
    static String number(long);
    static String number(unsigned long);
    static String number(long long);
    static String number(unsigned long long);
    static String number(double);
    
    static String format(const char *, ...) WTF_ATTRIBUTE_PRINTF(1, 2);

    Vector<String> split(const String& separator, bool allowEmptyEntries = false) const;
    Vector<String> split(UChar separator, bool allowEmptyEntries = false) const;

    int toInt(bool* ok = 0) const;
    int64_t toInt64(bool* ok = 0) const;
    uint64_t toUInt64(bool* ok = 0) const;
    double toDouble(bool* ok = 0) const;
    float toFloat(bool* ok = 0) const;
    Length* toLengthArray(int& len) const;
    Length* toCoordsArray(int& len) const;
    bool percentage(int &_percentage) const;

    // Makes a deep copy. Helpful only if you need to use a String on another thread.
    // Since the underlying StringImpl objects are immutable, there's no other reason
    // to ever prefer copy() over plain old assignment.
    String copy() const;

    bool isNull() const { return !m_impl; }
    bool isEmpty() const;

    StringImpl* impl() const { return m_impl.get(); }

#if PLATFORM(CF)
    String(CFStringRef);
    CFStringRef createCFString() const;
#endif

#ifdef __OBJC__
    String(NSString*);
    
    // This conversion maps NULL to "", which loses the meaning of NULL, but we 
    // need this mapping because AppKit crashes when passed nil NSStrings.
    operator NSString*() const { if (!m_impl) return @""; return *m_impl; }
#endif

#if PLATFORM(QT)
    String(const QString&);
    String(const QStringRef&);
    operator QString() const;
#endif

#if PLATFORM(SYMBIAN)
    String(const TDesC&);
    operator TPtrC() const { return des(); }
    TPtrC des() const { if (!m_impl) return KNullDesC(); return m_impl->des(); }
#endif

#if PLATFORM(WX)
    String(const wxString&);
    operator wxString() const;
#endif

#ifndef NDEBUG
    Vector<char> ascii() const;
#endif

    CString latin1() const;
    CString utf8() const;

    static String fromUTF8(const char*, size_t);
    static String fromUTF8(const char*);

    // Determines the writing direction using the Unicode Bidi Algorithm rules P2 and P3.
    WTF::Unicode::Direction defaultWritingDirection() const { return m_impl ? m_impl->defaultWritingDirection() : WTF::Unicode::LeftToRight; }
    
    String(const DeprecatedString&);
    DeprecatedString deprecatedString() const;
    
private:
    RefPtr<StringImpl> m_impl;
};

String operator+(const String&, const String&);
String operator+(const String&, const char*);
String operator+(const char*, const String&);

inline String& operator+=(String& a, const String& b) { a.append(b); return a; }

inline bool operator==(const String& a, const String& b) { return equal(a.impl(), b.impl()); }
inline bool operator==(const String& a, const char* b) { return equal(a.impl(), b); }
inline bool operator==(const char* a, const String& b) { return equal(a, b.impl()); }

inline bool operator!=(const String& a, const String& b) { return !equal(a.impl(), b.impl()); }
inline bool operator!=(const String& a, const char* b) { return !equal(a.impl(), b); }
inline bool operator!=(const char* a, const String& b) { return !equal(a, b.impl()); }

inline bool equalIgnoringCase(const String& a, const String& b) { return equalIgnoringCase(a.impl(), b.impl()); }
inline bool equalIgnoringCase(const String& a, const char* b) { return equalIgnoringCase(a.impl(), b); }
inline bool equalIgnoringCase(const char* a, const String& b) { return equalIgnoringCase(a, b.impl()); }

bool operator==(const String& a, const DeprecatedString& b);
inline bool operator==(const DeprecatedString& b, const String& a) { return a == b; }
inline bool operator!=(const String& a, const DeprecatedString& b) { return !(a == b); }
inline bool operator!=(const DeprecatedString& b, const String& a ) { return !(a == b); }

inline bool operator!(const String& str) { return str.isNull(); }

#ifdef __OBJC__
// This is for situations in WebKit where the long standing behavior has been
// "nil if empty", so we try to maintain longstanding behavior for the sake of
// entrenched clients
inline NSString* nsStringNilIfEmpty(const String& str) {  return str.isEmpty() ? nil : (NSString*)str; }
#endif

}

namespace WTF {

    // StringHash is the default hash for String
    template<typename T> struct DefaultHash;
    template<> struct DefaultHash<WebCore::String> {
        typedef WebCore::StringHash Hash;
    };

}

#endif
