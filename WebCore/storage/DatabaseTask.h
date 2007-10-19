/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef DatabaseTask_h
#define DatabaseTask_h

#include "ExceptionCode.h"
#include "PlatformString.h"
#include "Threading.h"
#include <wtf/PassRefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class Database;
class DatabaseThread;
class SQLValue;
class SQLCallback;
class VersionChangeCallback;

class DatabaseTask : public ThreadSafeShared<DatabaseTask>
{
    friend class Database;
public:
    virtual ~DatabaseTask();

    void performTask(Database*);

    bool isComplete() const { return m_complete; }
protected:
    DatabaseTask();
    virtual void doPerformTask(Database* db) = 0;

private:
    void lockForSynchronousScheduling();
    void waitForSynchronousCompletion();

    bool m_complete;

    Mutex m_synchronousMutex;
    bool m_synchronous;
    ThreadCondition m_synchronousCondition;
};

class DatabaseOpenTask : public DatabaseTask
{
public:
    DatabaseOpenTask();

    ExceptionCode exceptionCode() const { return m_code; }
    bool openSuccessful() const { return m_success; }

protected:
    virtual void doPerformTask(Database* db);

private:
    ExceptionCode m_code;
    bool m_success;
};

class DatabaseChangeVersionTask : public DatabaseTask
{
public:
    DatabaseChangeVersionTask(const String& oldVersion, const String& newVersion, PassRefPtr<VersionChangeCallback>);

protected:
    virtual void doPerformTask(Database* db);

private:
    String m_oldVersion;
    String m_newVersion;
    RefPtr<VersionChangeCallback> m_callback;
};

class DatabaseExecuteSqlTask : public DatabaseTask
{
public:
    DatabaseExecuteSqlTask(const String& query, const Vector<SQLValue>& arguments, PassRefPtr<SQLCallback> callback);

protected:
    virtual void doPerformTask(Database* db);

private:
    String m_query;
    Vector<SQLValue> m_arguments;
    RefPtr<SQLCallback> m_callback;
};

class DatabaseCloseTransactionTask: public DatabaseTask
{
public:
    DatabaseCloseTransactionTask();

protected:
    virtual void doPerformTask(Database* db);
};

class DatabaseTableNamesTask : public DatabaseTask
{
public:
    DatabaseTableNamesTask();

    Vector<String>& tableNames() { return m_tableNames; }
protected:
    virtual void doPerformTask(Database* db);
private:
    Vector<String> m_tableNames;
};

} // namespace WebCore

#endif // DatabaseTask_h
