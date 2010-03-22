/*
    Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef qscriptsyntaxcheckresult_h
#define qscriptsyntaxcheckresult_h

#include "qtscriptglobal.h"
#include <QtCore/qshareddata.h>

class QScriptSyntaxCheckResultPrivate;
class Q_JAVASCRIPT_EXPORT QScriptSyntaxCheckResult {
public:
    enum State {
        Error,
        Intermediate,
        Valid
    };

    QScriptSyntaxCheckResult(const QScriptSyntaxCheckResult& other);
    ~QScriptSyntaxCheckResult();
    QScriptSyntaxCheckResult& operator=(const QScriptSyntaxCheckResult& other);

    State state() const;
    int errorLineNumber() const;
    int errorColumnNumber() const;
    QString errorMessage() const;

private:
    QScriptSyntaxCheckResult(QScriptSyntaxCheckResultPrivate* d);
    QExplicitlySharedDataPointer<QScriptSyntaxCheckResultPrivate> d_ptr;

    friend class QScriptSyntaxCheckResultPrivate;
};
#endif // qscriptsyntaxcheckresult_h
