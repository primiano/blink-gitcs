/*
 * Copyright (C) 2001 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

// FIXME: obviously many functions here can be made inline

// #include <Foundation/Foundation.h>
#include <qstring.h>
#include <stdio.h>

#ifndef USING_BORROWED_QSTRING

// QString class ===============================================================

// constants -------------------------------------------------------------------

const QString QString::null;

// static member functions -----------------------------------------------------

QString QString::number(int n)
{
    QString qs;
    qs.setNum(n);
    return qs;
}

QString QString::fromLatin1(const char *chs)
{
    return QString(chs);
}

#ifdef USING_BORROWED_KURL
QString QString::fromLocal8Bit(const char *chs, int len)
{
    QString qs;
    if (chs && *chs) {
        qs.s = CFStringCreateMutable(kCFAllocatorDefault, 0);
        if (qs.s) {
            if (len < 0) {
                // FIXME: is MacRoman the correct encoding?
                CFStringAppendCString(qs.s, chs, kCFStringEncodingMacRoman);
            } else {
                const int capacity = 64;
                UniChar buf[capacity];
                int fill = 0;
                for (uint i = 0; (i < len) && chs[i]; i++) {
                    buf[fill] = chs[i];
                    fill++;
                    if (fill == capacity) {
                        CFStringAppendCharacters(qs.s, buf, fill);
                        fill = 0;
                    }
                }
                if (fill) {
                    CFStringAppendCharacters(qs.s, buf, fill);
                }
            }
        }
    }
    return qs;
}
#endif // USING_BORROWED_KURL

// constructors, copy constructors, and destructors ----------------------------

QString::QString()
{
    s = NULL;
    cache = NULL;
    cacheType = CacheInvalid;
}

QString::QString(QChar qc)
{
    s = CFStringCreateMutable(kCFAllocatorDefault, 0);
    if (s) {
        CFStringAppendCharacters(s, &qc.c, 1);
    }
    cache = NULL;
    cacheType = CacheInvalid;
}

QString::QString(const QByteArray &qba)
{
    if (qba.size() && *qba.data()) {
        s = CFStringCreateMutable(kCFAllocatorDefault, 0);
        if (s) {
            const int capacity = 64;
            UniChar buf[capacity];
            int fill = 0;
            for (uint len = 0; (len < qba.size()) && qba[len]; len++) {
                buf[fill] = qba[len];
                fill++;
                if (fill == capacity) {
                    CFStringAppendCharacters(s, buf, fill);
                    fill = 0;
                }
            }
            if (fill) {
                CFStringAppendCharacters(s, buf, fill);
            }
        }
    } else {
        s = NULL;
    }
    cache = NULL;
    cacheType = CacheInvalid;
}

QString::QString(const QChar *qcs, uint len)
{
    if (qcs || len) {
        s = CFStringCreateMutable(kCFAllocatorDefault, 0);
        if (s) {
            CFStringAppendCharacters(s, &qcs->c, len);
        }
    } else {
        s = NULL;
    }
    cache = NULL;
    cacheType = CacheInvalid;
}

QString::QString(const char *chs)
{
    if (chs && *chs) {
        s = CFStringCreateMutable(kCFAllocatorDefault, 0);
        if (s) {
            // FIXME: is ISO Latin-1 the correct encoding?
            CFStringAppendCString(s, chs, kCFStringEncodingISOLatin1);
        }
    } else {
        s = NULL;
    }
    cache = NULL;
    cacheType = CacheInvalid;
}

QString::QString(const QString &qs)
{
    // shared copy
    if (qs.s) {
        CFRetain(qs.s);
    }
    s = qs.s;
    cache = NULL;
    cacheType = CacheInvalid;
}

QString::~QString()
{
    if (s) {
        CFRelease(s);
    }
    if (cache) {
        CFAllocatorDeallocate(kCFAllocatorDefault, cache);
    }
}

// assignment operators --------------------------------------------------------

QString &QString::operator=(const QString &qs)
{
    // shared copy
    if (qs.s) {
        CFRetain(qs.s);
    }
    if (s) {
        CFRelease(s);
    }
    s = qs.s;
    cacheType = CacheInvalid;
    return *this;
}

QString &QString::operator=(const QCString &qcs)
{
    return *this = QString(qcs);
}

QString &QString::operator=(const char *chs)
{
    return *this = QString(chs);
}

QString &QString::operator=(QChar qc)
{
    return *this = QString(qc);
}

QString &QString::operator=(char ch)
{
    return *this = QString(QChar(ch));
}

// member functions ------------------------------------------------------------

uint QString::length() const
{
    return s ? CFStringGetLength(s) : 0;
}

const QChar *QString::unicode() const
{
    UniChar *ucs = NULL;
    uint len = length();
    if (len) {
        ucs = const_cast<UniChar *>(CFStringGetCharactersPtr(s));
        if (!ucs) {
            // NSLog(@"CFStringGetCharactersPtr returned NULL!!!");
            if (cacheType != CacheUnicode) {
                if (cache) {
                    CFAllocatorDeallocate(kCFAllocatorDefault, cache);
                    cache = NULL;
                    cacheType = CacheInvalid;
                }
                if (!cache) {
                    cache = CFAllocatorAllocate(kCFAllocatorDefault,
                            len * sizeof (UniChar), 0);
                }
                if (cache) {
                    CFStringGetCharacters(s, CFRangeMake(0, len), cache);
                    cacheType = CacheUnicode;
                }
            }
            ucs = cache;
        }
    }
    // NOTE: this only works since our QChar implementation contains a single
    // UniChar data member
    return reinterpret_cast<const QChar *>(ucs); 
}

const char *QString::latin1() const
{
    char *chs = NULL;
    uint len = length();
    if (len) {
        // FIXME: is ISO Latin-1 the correct encoding?
        chs = const_cast<char *>(CFStringGetCStringPtr(s,
                    kCFStringEncodingISOLatin1));
        if (!chs) {
            // NSLog(@"CFStringGetCStringPtr returned NULL!!!");
            if (cacheType != CacheLatin1) {
                if (cache) {
                    CFAllocatorDeallocate(kCFAllocatorDefault, cache);
                    cache = NULL;
                    cacheType = CacheInvalid;
                }
                if (!cache) {
                    cache = CFAllocatorAllocate(kCFAllocatorDefault, len + 1,
                            0);
                }
                if (cache) {
                    // FIXME: is ISO Latin-1 the correct encoding?
                    if (!CFStringGetCString(s, cache, len + 1,
                                kCFStringEncodingISOLatin1)) {
                        // NSLog(@"CFStringGetCString returned FALSE!!!");
                        *reinterpret_cast<char *>(cache) = '\0';
                    }
                    cacheType = CacheLatin1;
                }
            }
            chs = cache;
        }
    }
    if (!chs) {
        static char emptyString[] = "";
        chs = emptyString;
    }
    return chs; 
}

const char *QString::ascii() const
{
    return latin1(); 
}

QCString QString::utf8() const
{
    uint len = length();
    if (len) {
        char *chs = CFAllocatorAllocate(kCFAllocatorDefault, len + 1, 0);
        if (chs) {
            if (!CFStringGetCString(s, chs, len + 1, kCFStringEncodingUTF8)) {
                *reinterpret_cast<char *>(chs) = '\0';
            }
            QCString qcs = QCString(chs);
            CFAllocatorDeallocate(kCFAllocatorDefault, chs);
            return qcs;
        }
    }
    return QCString();
}

QCString QString::local8Bit() const
{
    uint len = length();
    if (len) {
        char *chs = CFAllocatorAllocate(kCFAllocatorDefault, len + 1, 0);
        if (chs) {
            // FIXME: is MacRoman the correct encoding?
            if (!CFStringGetCString(s, chs, len + 1,
                    kCFStringEncodingMacRoman)) {
                *reinterpret_cast<char *>(chs) = '\0';
            }
            QCString qcs = QCString(chs);
            CFAllocatorDeallocate(kCFAllocatorDefault, chs);
            return qcs;
        }
    }
    return QCString();
}

bool QString::isNull() const
{
    // NOTE: do NOT use "unicode() == NULL"
    return s == NULL;
}

bool QString::isEmpty() const
{
    return length() == 0;
}

#ifdef USING_BORROWED_KURL
QChar QString::at(uint i) const
{
    uint len = length();
    if (len && (i < len)) {
        CFStringInlineBuffer buf;
        CFStringInitInlineBuffer(s, &buf, CFRangeMake(0, i));
        return QChar(CFStringGetCharacterFromInlineBuffer(&buf, i));
    }
    return QChar(0);
}
#endif // USING_BORROWED_KURL

bool QString::startsWith(const QString &qs) const
{
    if (s && qs.s) {
        return CFStringHasPrefix(s, qs.s);
    }
    return FALSE;
}

int QString::compare(const QString &qs) const
{
    if (s == qs.s) {
        return kCFCompareEqualTo;
    }
    if (!s) {
        return kCFCompareLessThan;
    }
    if (!qs.s) {
        return kCFCompareGreaterThan;
    }
    return CFStringCompare(s, qs.s, 0);
}

int QString::contains(const char *, bool) const
{
    // FIXME: not yet implemented
    return 0;
}

int QString::contains(char) const
{
    // FIXME: not yet implemented
    return 0;
}

int QString::find(char, int, bool) const
{
    // FIXME: not yet implemented
    return 0;
}

int QString::find(QChar, int, bool) const
{
    // FIXME: not yet implemented
    return 0;
}

int QString::find(const QString &, int, bool) const
{
    // FIXME: not yet implemented
    return 0;
}

int QString::find(const QRegExp &, int, bool) const
{
    // FIXME: not yet implemented
    return 0;
}

int QString::findRev(char, int, bool) const
{
    // FIXME: not yet implemented
    return 0;
}

int QString::findRev(const char *, int, bool) const
{
    // FIXME: not yet implemented
    return 0;
}

#ifdef USING_BORROWED_KURL
ushort QString::toUShort() const
{
    return toUInt();
}
#endif // USING_BORROWED_KURL

int QString::toInt(bool *ok, int base) const
{
    return toLong(ok, base);
}

uint QString::toUInt(bool *ok) const
{
    uint n = 0;
    bool valid = FALSE;
    if (s) {
        CFIndex len = CFStringGetLength(s);
        if (len) {
            CFStringInlineBuffer buf;
            UniChar uc;
            CFCharacterSetRef wscs =
                CFCharacterSetGetPredefined(
                        kCFCharacterSetWhitespaceAndNewline);
            CFStringInitInlineBuffer(s, &buf, CFRangeMake(0, len));
            CFIndex i;
            for (i = 0; i < len; i++) {
                uc = CFStringGetCharacterFromInlineBuffer(&buf, i);
                if (!CFCharacterSetIsCharacterMember(wscs, uc)) {
                    break;
                }
            }
            while (i < len) {
                uc = CFStringGetCharacterFromInlineBuffer(&buf, i);
                if ((uc >= '0') && (uc <= '9')) {
                    n += uc - '0';
                } else {
                    break;
                }
                valid = TRUE;
                i++;
            }
            while (i < len) {
                uc = CFStringGetCharacterFromInlineBuffer(&buf, i);
                if (!CFCharacterSetIsCharacterMember(wscs, uc)) {
                    valid = FALSE;
                    break;
                }
                i++;
            }
        }
    }
    if (ok) {
        *ok = valid;
    }
    return valid ? n : 0;
}

long QString::toLong(bool *ok, int base) const
{
    long n = 0;
    bool valid = FALSE;
    if (s) {
        CFIndex len = CFStringGetLength(s);
        if (len) {
            CFStringInlineBuffer buf;
            UniChar uc;
            CFCharacterSetRef wscs =
                CFCharacterSetGetPredefined(
                        kCFCharacterSetWhitespaceAndNewline);
            CFStringInitInlineBuffer(s, &buf, CFRangeMake(0, len));
            CFIndex i;
            for (i = 0; i < len; i++) {
                uc = CFStringGetCharacterFromInlineBuffer(&buf, i);
                if (!CFCharacterSetIsCharacterMember(wscs, uc)) {
                    break;
                }
            }
            bool neg = FALSE;
            if (i < len) {
                uc = CFStringGetCharacterFromInlineBuffer(&buf, i);
                if (uc == '-') {
                    i++;
                    neg = TRUE;
                } else if (uc == '+') {
                    i++;
                }
            }
            while (i < len) {
                uc = CFStringGetCharacterFromInlineBuffer(&buf, i);
                // NOTE: ignore anything other than base 10 and base 16
                if ((uc >= '0') && (uc <= '9')) {
                    n += uc - '0';
                } else if (base == 16) {
                    if ((uc >= 'A') && (uc <= 'F')) {
                        n += 10 + (uc - 'A');
                    } else if ((uc >= 'a') && (uc <= 'f')) {
                        n += 10 + (uc - 'a');
                    } else {
                        break;
                    }
                } else {
                    break;
                }
                valid = TRUE;
                i++;
            }
            if (neg) {
                n = -n;
            }
            while (i < len) {
                uc = CFStringGetCharacterFromInlineBuffer(&buf, i);
                if (!CFCharacterSetIsCharacterMember(wscs, uc)) {
                    valid = FALSE;
                    break;
                }
                i++;
            }
        }
    }
    if (ok) {
        *ok = valid;
    }
    return valid ? n : 0;
}

float QString::toFloat(bool *ok) const
{
    float n;
    if (s) {
        n = CFStringGetDoubleValue(s);
    } else {
        n = 0.0;
    }
    if (ok) {
        // NOTE: since CFStringGetDoubleValue returns 0.0 on error there is no
        // way to know if "n" is valid in that case
        if (n == 0.0) {
            *ok = FALSE;
        } else {
            *ok = TRUE;
        }
    }
    return n;
}

QString QString::arg(const QString &replacement, int padding) const
{
    QString modified(*this);
    if (!modified.s) {
        modified.s = CFStringCreateMutable(kCFAllocatorDefault, 0);
    }
    if (modified.s) {
        CFIndex pos = 0;
        CFIndex len = CFStringGetLength(modified.s);
        if (len) {
            UniChar found = 0;
            CFStringInlineBuffer buf;
            CFStringInitInlineBuffer(modified.s, &buf, CFRangeMake(0, len));
            for (CFIndex i = 0; i < len; i++) {
                UniChar uc = CFStringGetCharacterFromInlineBuffer(&buf, i);
                if ((uc == '%') && ((i + 1) < len)) {
                    UniChar uc2 = CFStringGetCharacterFromInlineBuffer(&buf,
                            i + 1);
                    if ((uc2 >= '0') && (uc2 <= '9')) {
                        if (!found || (uc2 < found)) {
                            found = uc2;
                            pos = i;
                        }
                    }
                }
            }
        }
        CFIndex rlen;
        if (pos) {
            rlen = 2;
        } else {
            CFStringAppend(modified.s, CFSTR(" "));
            pos = len + 1;
            rlen = 0;
        }
        if (replacement.s) {
            CFStringReplace(modified.s, CFRangeMake(pos, rlen), replacement.s);
            if (padding) {
                CFMutableStringRef p =
                    CFStringCreateMutable(kCFAllocatorDefault, 0);
                if (p) {
                    CFIndex plen;
                    if (padding < 0) {
                        plen = -padding;
                        pos += CFStringGetLength(replacement.s);
                    } else {
                        plen = padding;
                    }
                    CFStringPad(p, CFSTR(" "), plen, 0);
                    CFStringInsert(modified.s, pos, p);
                    CFRelease(p);
                }
            }
        }
    }
    return modified;
}

QString QString::arg(int replacement, int padding) const
{
    return arg(number(replacement), padding);
}

QString QString::left(uint) const
{
    // FIXME: not yet implemented
    return QString(*this);
}

QString QString::right(uint) const
{
    // FIXME: not yet implemented
    return QString(*this);
}

QString QString::mid(int, int) const
{
    // FIXME: not yet implemented
    return QString(*this);
}

#ifdef USING_BORROWED_KURL
QString QString::copy() const
{
    // FIXME: not yet implemented
    return QString(*this);
}
#endif // USING_BORROWED_KURL

QString QString::lower() const
{
    // FIXME: not yet implemented
    return QString(*this);
}

QString QString::stripWhiteSpace() const
{
    // FIXME: not yet implemented
    return QString(*this);
}

QString QString::simplifyWhiteSpace() const
{
    // FIXME: not yet implemented
    return QString(*this);
}

QString &QString::setUnicode(const QChar *, uint)
{
    // FIXME: not yet implemented
    return *this;
}

QString &QString::setNum(int n)
{
    cacheType = CacheInvalid;
    if (!s) {
        s = CFStringCreateMutable(kCFAllocatorDefault, 0);
    }
    if (s) {
        const int capacity = 64;
        char buf[capacity];
        buf[snprintf(buf, capacity - 1, "%d", n)] = '\0';
        // NOTE: using private __CFStringMakeConstantString function instead of
        // creating a temporary string with CFStringCreateWithCString
        CFStringReplace(s, CFRangeMake(0, CFStringGetLength(s)),
                __CFStringMakeConstantString(buf));
    }
    return *this;
}

QString &QString::sprintf(const char *, ...)
{
    // FIXME: not yet implemented
    return *this;
}

QString &QString::prepend(const QString &)
{
    // FIXME: not yet implemented
    return *this;
}

QString &QString::append(const char *)
{
    // FIXME: not yet implemented
    return *this;
}

QString &QString::append(const QString &)
{
    // FIXME: not yet implemented
    return *this;
}

QString &QString::remove(uint, uint)
{
    // FIXME: not yet implemented
    return *this;
}

QString &QString::replace(const QRegExp &, const QString &)
{
    // FIXME: not yet implemented
    return *this;
}

QString &QString::insert(uint, char)
{
    // FIXME: not yet implemented
    return *this;
}

QString &QString::insert(uint, QChar)
{
    // FIXME: not yet implemented
    return *this;
}

QString &QString::insert(uint, const QString &)
{
    // FIXME: not yet implemented
    return *this;
}

void QString::truncate(uint)
{
    // FIXME: not yet implemented
}

void QString::fill(QChar, int)
{
    // FIXME: not yet implemented
}

void QString::compose()
{
    // FIXME: not yet implemented
}

QString QString::visual(int, int)
{
    // FIXME: not yet implemented
    return *this;
}

// operators -------------------------------------------------------------------

bool QString::operator!() const
{ 
    return isNull(); 
}

QString::operator QChar() const
{
    // FIXME: not yet implemented
    return QChar();
}

QString::operator const char *() const
{
    return latin1();
}

QChar QString::operator[](int) const
{
    // FIXME: not yet implemented
    return 0;
}

QString &QString::operator+(char)
{
    // FIXME: not yet implemented
    return *this;
}

QString &QString::operator+(QChar)
{
    // FIXME: not yet implemented
    return *this;
}

QString &QString::operator+(const QString &)
{
    // FIXME: not yet implemented
    return *this;
}

QString &QString::operator+=(char)
{
    // FIXME: not yet implemented
    return *this;
}

QString &QString::operator+=(QChar)
{
    // FIXME: not yet implemented
    return *this;
}

QString &QString::operator+=(const QString &)
{
    // FIXME: not yet implemented
    return *this;
}


// operators associated with QChar and QString =================================

bool operator==(const QString &, QChar)
{
    // FIXME: not yet implemented
    return FALSE;
}

bool operator==(const QString &qs1, const QString &qs2)
{
    if (qs1.s && qs2.s) {
        return CFStringCompare(qs1.s, qs2.s, 0) == kCFCompareEqualTo;
    }
    return FALSE;
}

bool operator==(const QString &, const char *)
{
    // FIXME: not yet implemented
    return FALSE;
}

bool operator==(const char *, const QString &)
{
    // FIXME: not yet implemented
    return FALSE;
}

bool operator!=(const QString &s, QChar c)
{
    // FIXME: not yet implemented
    return FALSE;
}

bool operator!=(const QString &qs1, const QString &qs2)
{
    if (qs1.s && qs2.s) {
        return CFStringCompare(qs1.s, qs2.s, 0) != kCFCompareEqualTo;
    }
    return TRUE;
}

bool operator!=(const QString &, const char *)
{
    // FIXME: not yet implemented
    return FALSE;
}

bool operator!=(const char *, const QString &)
{
    // FIXME: not yet implemented
    return FALSE;
}

QString operator+(char, const QString &)
{
    // FIXME: not yet implemented
    return QString();
}

QString operator+(const char *, const QString &)
{
    // FIXME: not yet implemented
    return QString();
}

QString operator+(QChar, const QString &)
{
    // FIXME: not yet implemented
    return QString();
}


// class QConstString ==========================================================

// constructors, copy constructors, and destructors ----------------------------

QConstString::QConstString(QChar *qcs, uint len)
{
    if (qcs || len) {
        // NOTE: use instead of CFStringCreateWithCharactersNoCopy function to
        // guarantee backing store is not copied even though string is mutable
        s = CFStringCreateMutableWithExternalCharactersNoCopy(
                kCFAllocatorDefault, &qcs->c, len, len, kCFAllocatorNull);
    } else {
        s = NULL;
    }
}

// member functions ------------------------------------------------------------

const QString &QConstString::string() const
{
    return *this;
}

#else // USING_BORROWED_QSTRING
// This will help to keep the linker from complaining about empty archives
void KWQString_Dummy() {}
#endif // USING_BORROWED_QSTRING
