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

#ifndef QSTRING_H_
#define QSTRING_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if (defined(__APPLE__) && defined(__OBJC__) && defined(__cplusplus))
// These macros are TEMPORARY hacks to convert between NSString and QString.
// They should be replaced with correct implementations.  They should only be
// used for immutable strings.
#define QSTRING_TO_NSSTRING(aString) \
    [NSString stringWithCString: aString.latin1()]
#define QSTRING_TO_NSSTRING_LENGTH(aString,l) \
    [NSString stringWithCString: aString.latin1() length: l]
#define NSSTRING_TO_QSTRING(aString) \
    QString([aString cString])
#endif

// USING_BORROWED_QSTRING ======================================================
#ifdef USING_BORROWED_QSTRING
#include <_qstring.h>

#else

#define Fixed MacFixed
#define Rect MacRect
#define Boolean MacBoolean
#include <CoreFoundation/CoreFoundation.h>
#undef Fixed
#undef Rect
#undef Boolean

#include "qcstring.h"

class QString;
class QRegExp;

// QChar class =================================================================

class QChar {
public:

    // typedefs ----------------------------------------------------------------

    // enums -------------------------------------------------------------------

    enum Direction {
        // NOTE: alphabetical order
        DirAL, DirAN, DirB, DirBN, DirCS, DirEN, DirES, DirET, DirL, DirLRE,
        DirLRO, DirNSM, DirON, DirPDF, DirR, DirRLE, DirRLO, DirS, DirWS
    };

    // constants ---------------------------------------------------------------

    static const QChar null;

    // static member functions -------------------------------------------------

    // constructors, copy constructors, and destructors ------------------------

    QChar();
    QChar(char);
    QChar(uchar);
    QChar(short);
    QChar(ushort);
    QChar(int);
    QChar(uint);

    QChar(const QChar &);

    ~QChar();

    // member functions --------------------------------------------------------

    ushort unicode() const;
    uchar cell() const;
    uchar row() const;
    char latin1() const;
    bool isNull() const;
    bool isSpace() const;
    bool isDigit() const;
    bool isLetter() const;
    bool isNumber() const;
    bool isLetterOrNumber() const;
    bool isPunct() const;
    QChar lower() const;
    QChar upper() const;
    Direction direction() const;
    bool mirrored() const;
    QChar mirroredChar() const;

    // operators ---------------------------------------------------------------

    operator char() const;
    friend int operator==(QChar, QChar);
    friend int operator==(QChar, char);
    friend int operator==(char, QChar);
    friend int operator!=(QChar, QChar);
    friend int operator!=(QChar, char);
    friend int operator!=(char, QChar);

// protected -------------------------------------------------------------------
// private ---------------------------------------------------------------------

private:

    // data members ------------------------------------------------------------

    UniChar c;

    // friends -----------------------------------------------------------------

    friend class QString;
    friend class QConstString;

}; // class QChar ==============================================================


// QString class ===============================================================

class QString {
public:

    // typedefs ----------------------------------------------------------------
    // enums -------------------------------------------------------------------
    // constants ---------------------------------------------------------------

    static const QString null;

    // static member functions -------------------------------------------------

    static QString number(int /* NOTE: base NOT used */ );
    static QString fromLatin1(const char * /* NOTE: len NOT used */ );
#ifdef USING_BORROWED_KURL
    static QString fromLocal8Bit(const char *, int len=-1);
#endif

    // constructors, copy constructors, and destructors ------------------------

    QString();
    QString(QChar);
    QString(const QByteArray &);
    QString(const QChar *, uint);
    QString(const char *);

    QString(const QString &);

    ~QString();

    // assignment operators ----------------------------------------------------

    QString &operator=(const QString &);
    QString &operator=(const QCString &);
    QString &operator=(const char *);
    QString &operator=(QChar);
    QString &operator=(char);

    // member functions --------------------------------------------------------

    uint length() const;

    const QChar *unicode() const;
    const char *latin1() const;
    const char *ascii() const;
    QCString utf8() const;
    QCString local8Bit() const;

    bool isNull() const;
    bool isEmpty() const;

#ifdef USING_BORROWED_KURL
    QChar at(uint) const;
#endif

    int compare(const QString &) const;

    bool startsWith(const QString &) const;

    int find(QChar, int) const;
    int find(char, int index=0) const;
    int find(const QString &, int index=0) const;
    int find(const char *, int index=0, bool cs=TRUE) const;
    int find(const QRegExp &, int index=0) const;

    int findRev(char, int index=0) const;
    int findRev(const char *, int index=0) const;

    int contains(char) const;
    int contains(const char *, bool cs=TRUE) const;

#ifdef USING_BORROWED_KURL
    ushort toUShort() const;
#endif
    int toInt(bool *ok=NULL, int base=10) const;
    uint toUInt(bool *ok=NULL /* NOTE: base NOT used */ ) const;
    long toLong(bool *ok=NULL, int base=10) const;
    float toFloat(bool *ok=NULL) const;

    QString arg(const QString &, int width=0) const;
    QString arg(int, int width=0 /* NOTE: base NOT used */ ) const;

    QString left(uint) const;
    QString right(uint) const;
    QString mid(int, int len=0xffffffff) const;

#ifdef USING_BORROWED_KURL
    QString copy() const;
#endif

    QString lower() const;
    QString stripWhiteSpace() const;
    QString simplifyWhiteSpace() const;

    QString &setUnicode(const QChar *, uint);
    QString &setNum(int /* NOTE: base NOT used */ );
    QString &sprintf(const char *, ...);

    QString &prepend(const QString &);
    QString &append(const QString &);
    QString &insert(uint, const QString &);
    QString &insert(uint, QChar);
    QString &insert(uint, char);
    QString &remove(uint, uint);
    QString &replace(const QRegExp &, const QString &);

    void truncate(uint);
    void fill(QChar, int len=-1);

    void compose();
    QString visual();

    // operators ---------------------------------------------------------------

    bool operator!() const;

    operator const char *() const;

    QChar operator[](int) const;

    QString &operator+=(const QString &);
    QString &operator+=(QChar);
    QString &operator+=(char);

// protected -------------------------------------------------------------------
// private ---------------------------------------------------------------------

private:

    // private enums -----------------------------------------------------------

    enum CacheType {
        CacheInvalid, CacheUnicode, CacheLatin1
    };

    // private member functions ------------------------------------------------

    void flushCache() const;
    QCString convertToQCString(CFStringEncoding) const;
    QString leftRight(uint width, bool left) const;

    // data members ------------------------------------------------------------

    CFMutableStringRef s;
    mutable void *cache;
    mutable CacheType cacheType;

    // friends -----------------------------------------------------------------

    friend bool operator==(const QString &, const QString &);
    friend bool operator==(const QString &, const char *);
    friend bool operator==(const char *, const QString &);

    friend bool operator!=(const QString &, const QString &);
    friend bool operator!=(const QString &, const char *);
    friend bool operator!=(const char *, const QString &);

    friend QString operator+(const QString &, const QString &);
    friend QString operator+(const QString &, const char *);
    friend QString operator+(const char *, const QString &);

    friend class QConstString;
    friend class QGDict;

}; // class QString ============================================================


// operators associated with QString ===========================================

bool operator==(const QString &, const QString &);
bool operator==(const QString &, const char *);
bool operator==(const char *, const QString &);

bool operator!=(const QString &, const QString &);
bool operator!=(const QString &, const char *);
bool operator!=(const char *, const QString &);

QString operator+(const QString &, const QString &);
QString operator+(const QString &, const char *);
QString operator+(const char *, const QString &);


// class QConstString ==========================================================

class QConstString : private QString {
public:

    // typedefs ----------------------------------------------------------------
    // enums -------------------------------------------------------------------
    // constants ---------------------------------------------------------------

    // static member functions -------------------------------------------------

    // constructors, copy constructors, and destructors ------------------------

    QConstString(QChar *, uint);
#ifdef _KWQ_PEDANTIC_
    // NOTE: copy constructor not needed
    // QConstString(const QConstString &);
#endif

    // NOTE: destructor not needed
    //~QConstString();

    // member functions --------------------------------------------------------

    const QString &string() const;

    // operators ---------------------------------------------------------------

// protected -------------------------------------------------------------------
// private ---------------------------------------------------------------------

// private:

    // assignment operators ----------------------------------------------------

    // private declaration prevents assignment
#ifdef _KWQ_PEDANTIC_
    // NOTE: assignment operator not needed
    // QConstString &operator=(const QConstString &);
#endif

}; // class QConstString =======================================================

#endif // USING_BORROWED_QSTRING

#endif
