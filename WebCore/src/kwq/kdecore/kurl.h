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

#ifndef KURL_H_
#define KURL_H_

class QString;

class KURL {
public:
        KURL();
        KURL(const char *url, int encoding_hint = 0);
        KURL(const KURL& url, const QString &);
        KURL(const QString& url, int encoding_hint = 0);
        bool hasPath() const;
	unsigned short int port() const;
	QString path() const;
	QString query() const;
	QString ref() const;
	QString user() const;
	QString pass() const;
	QString url() const;
	QString host() const;
	QString protocol() const;
        void setQuery(const QString& _txt, int encoding_hint = 0);
        void setProtocol(const QString&);
        void setHost(const QString&);
        void setRef(const QString& _txt);
        void setPath(const QString& path);
        void setPort(unsigned short int);
	bool isEmpty() const;
	bool isMalformed() const;
        QString prettyURL(int _trailing = 0) const;
};

#endif
