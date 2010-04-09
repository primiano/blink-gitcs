/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef DatabaseTracker_h
#define DatabaseTracker_h

#if ENABLE(DATABASE)

#include "PlatformString.h"
#include "StringHash.h"
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>

#if !PLATFORM(CHROMIUM)
#include "DatabaseDetails.h"
#include "SQLiteDatabase.h"
#include <wtf/OwnPtr.h>
#endif // !PLATFORM(CHROMIUM)

namespace WebCore {

class Database;
class ScriptExecutionContext;
class SecurityOrigin;

struct SecurityOriginHash;

#if !PLATFORM(CHROMIUM)
class DatabaseTrackerClient;
class OriginQuotaManager;

struct SecurityOriginTraits;
#endif // !PLATFORM(CHROMIUM)

class DatabaseTracker : public Noncopyable {
public:
    static DatabaseTracker& tracker();
    // This singleton will potentially be used from multiple worker threads and the page's context thread simultaneously.  To keep this safe, it's
    // currently using 4 locks.  In order to avoid deadlock when taking multiple locks, you must take them in the correct order:
    // originQuotaManager() before m_databaseGuard or m_openDatabaseMapGuard
    // m_databaseGuard before m_openDatabaseMapGuard
    // notificationMutex() is currently independent of the other locks.

    bool canEstablishDatabase(ScriptExecutionContext*, const String& name, const String& displayName, unsigned long estimatedSize);
    void setDatabaseDetails(SecurityOrigin*, const String& name, const String& displayName, unsigned long estimatedSize);
    String fullPathForDatabase(SecurityOrigin*, const String& name, bool createIfDoesNotExist = true);

    void addOpenDatabase(Database*);
    void removeOpenDatabase(Database*);
    void getOpenDatabases(SecurityOrigin* origin, const String& name, HashSet<RefPtr<Database> >* databases);

    unsigned long long getMaxSizeForDatabase(const Database*);

private:
    DatabaseTracker();

    typedef HashSet<Database*> DatabaseSet;
    typedef HashMap<String, DatabaseSet*> DatabaseNameMap;
    typedef HashMap<RefPtr<SecurityOrigin>, DatabaseNameMap*, SecurityOriginHash> DatabaseOriginMap;

    Mutex m_openDatabaseMapGuard;
    mutable OwnPtr<DatabaseOriginMap> m_openDatabaseMap;

#if !PLATFORM(CHROMIUM)
public:
    void setDatabaseDirectoryPath(const String&);
    String databaseDirectoryPath() const;

    void origins(Vector<RefPtr<SecurityOrigin> >& result);
    bool databaseNamesForOrigin(SecurityOrigin*, Vector<String>& result);

    DatabaseDetails detailsForNameAndOrigin(const String&, SecurityOrigin*);

    unsigned long long usageForDatabase(const String&, SecurityOrigin*);
    unsigned long long usageForOrigin(SecurityOrigin*);
    unsigned long long quotaForOrigin(SecurityOrigin*);
    void setQuota(SecurityOrigin*, unsigned long long);

    void deleteAllDatabases();
    bool deleteOrigin(SecurityOrigin*);
    bool deleteDatabase(SecurityOrigin*, const String& name);

    void setClient(DatabaseTrackerClient*);

    // From a secondary thread, must be thread safe with its data
    void scheduleNotifyDatabaseChanged(SecurityOrigin*, const String& name);

    OriginQuotaManager& originQuotaManager();


    bool hasEntryForOrigin(SecurityOrigin*);

private:
    OriginQuotaManager& originQuotaManagerNoLock();
    bool hasEntryForOriginNoLock(SecurityOrigin* origin);
    String fullPathForDatabaseNoLock(SecurityOrigin*, const String& name, bool createIfDoesNotExist);
    bool databaseNamesForOriginNoLock(SecurityOrigin* origin, Vector<String>& resultVector);
    unsigned long long usageForOriginNoLock(SecurityOrigin* origin);
    unsigned long long quotaForOriginNoLock(SecurityOrigin* origin);

    String trackerDatabasePath() const;
    void openTrackerDatabase(bool createIfDoesNotExist);

    String originPath(SecurityOrigin*) const;

    bool hasEntryForDatabase(SecurityOrigin*, const String& databaseIdentifier);

    bool addDatabase(SecurityOrigin*, const String& name, const String& path);
    void populateOrigins();

    bool deleteDatabaseFile(SecurityOrigin*, const String& name);

    // This lock protects m_database, m_quotaMap, m_proposedDatabases, m_databaseDirectoryPath, m_originsBeingDeleted, m_beingCreated, and m_beingDeleted.
    Mutex m_databaseGuard;
    SQLiteDatabase m_database;

    typedef HashMap<RefPtr<SecurityOrigin>, unsigned long long, SecurityOriginHash> QuotaMap;
    mutable OwnPtr<QuotaMap> m_quotaMap;

    OwnPtr<OriginQuotaManager> m_quotaManager;

    String m_databaseDirectoryPath;

    DatabaseTrackerClient* m_client;

    typedef std::pair<RefPtr<SecurityOrigin>, DatabaseDetails> ProposedDatabase;
    HashSet<ProposedDatabase*> m_proposedDatabases;

    typedef HashMap<String, long> NameCountMap;
    typedef HashMap<RefPtr<SecurityOrigin>, NameCountMap*, SecurityOriginHash> CreateSet;
    CreateSet m_beingCreated;
    typedef HashSet<String> NameSet;
    HashMap<RefPtr<SecurityOrigin>, NameSet*, SecurityOriginHash> m_beingDeleted;
    HashSet<RefPtr<SecurityOrigin>, SecurityOriginHash> m_originsBeingDeleted;
    bool canCreateDatabase(SecurityOrigin *origin, const String& name);
    void recordCreatingDatabase(SecurityOrigin *origin, const String& name);
    void doneCreatingDatabase(SecurityOrigin *origin, const String& name);
    bool creatingDatabase(SecurityOrigin *origin, const String& name);
    bool canDeleteDatabase(SecurityOrigin *origin, const String& name);
    void recordDeletingDatabase(SecurityOrigin *origin, const String& name);
    void doneDeletingDatabase(SecurityOrigin *origin, const String& name);
    bool deletingDatabase(SecurityOrigin *origin, const String& name);
    bool canDeleteOrigin(SecurityOrigin *origin);
    bool deletingOrigin(SecurityOrigin *origin);
    void recordDeletingOrigin(SecurityOrigin *origin);
    void doneDeletingOrigin(SecurityOrigin *origin);

    static void scheduleForNotification();
    static void notifyDatabasesChanged(void*);
#endif // !PLATFORM(CHROMIUM)
};

} // namespace WebCore

#endif // ENABLE(DATABASE)
#endif // DatabaseTracker_h
