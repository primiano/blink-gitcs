/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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
 
#ifndef ICONDATABASE_H
#define ICONDATABASE_H

#include "config.h"

#include "IntSize.h"
#include "PlatformString.h"
#include "SQLDatabase.h"
#include "StringHash.h"
#include "Timer.h"

#include <wtf/HashMap.h>
#include <wtf/HashSet.h>

namespace WTF {

    template<> struct IntHash<WebCore::IntSize> {
        static unsigned hash(const WebCore::IntSize& key) { return intHash((static_cast<uint64_t>(key.width()) << 32 | key.height())); }
        static bool equal(const WebCore::IntSize& a, const WebCore::IntSize& b) { return a == b; }
    };
    template<> struct DefaultHash<WebCore::IntSize> { typedef IntHash<WebCore::IntSize> Hash; };

} // namespace WTF

namespace WebCore { 

class Image;
class IconDataCache;
class SQLTransaction;
class SQLStatement;

class IconDatabase
{
//TODO - SiteIcon is never to be used outside of IconDatabase, so make it an internal and remove the friendness
friend class SiteIcon;
public:
    static IconDatabase* sharedIconDatabase();
    
    bool open(const String& path);
    bool isOpen() { return m_mainDB.isOpen() && m_privateBrowsingDB.isOpen(); }
    void close();
    
    bool isEmpty();
 
    Image* iconForPageURL(const String&, const IntSize&, bool cache = true);
    Image* iconForIconURL(const String&, const IntSize&, bool cache = true);
    String iconURLForPageURL(const String&);
    Image* defaultIcon(const IntSize&);

    void retainIconForPageURL(const String&);
    void releaseIconForPageURL(const String&);
    
    void setPrivateBrowsingEnabled(bool flag);
    bool getPrivateBrowsingEnabled() { return m_privateBrowsingEnabled; }

    bool hasEntryForIconURL(const String&);

    bool isIconExpiredForIconURL(const String&);
    
    // TODO - The following 3 methods were considered private in WebKit - analyze the impact of making them
    // public here in WebCore - I don't see any real badness with doing that...  after all if Chuck Norris wants to muck
    // around with the icons in his database, he's going to anyway
    void setIconDataForIconURL(const void* data, int size, const String&);
    void setHaveNoIconForIconURL(const String&);
    void setIconURLForPageURL(const String& iconURL, const String& pageURL);

    static const String& defaultDatabaseFilename();
    
    static const int currentDatabaseVersion;    
    static const int iconExpirationTime;
    static const int missingIconExpirationTime;
    static const int updateTimerDelay;
private:
    IconDatabase();
    ~IconDatabase();
    
    // This tries to get the iconID for the IconURL and, if it doesn't exist and createIfNecessary is true,
    // it will create the entry and return the new iconID
    int64_t establishIconIDForIconURL(SQLDatabase&, const String&, bool createIfNecessary = true);
    
    // This method returns the SiteIcon for the given IconURL and, if it doesn't exist it creates it first
    IconDataCache* getOrCreateIconDataCache(const String& iconURL);

    // Remove traces of the given pageURL
    void forgetPageURL(const String& pageURL);
    
    // Remove the current database entry for this IconURL
    void forgetIconForIconURLFromDatabase(const String&);
    
    void setIconURLForPageURLInDatabase(const String&, const String&);
    
    // Wipe all icons from the DB
    void removeAllIcons();
    
    // Called by the startup timer, this method removes all icons that are unretained
    // after initial retains are complete, and pageURLs that are dangling
    void pruneUnretainedIconsOnStartup(Timer<IconDatabase>*);
    
    // Called by the prune timer, this method periodically removes all the icons in the pending-deletion
    // queue
    void updateDatabase(Timer<IconDatabase>*);
    
    // This is called by updateDatabase and when private browsing shifts, and when the DB is closed down
    void syncDatabase();
    
    // Determine if an IconURL is still retained by anyone
    bool isIconURLRetained(const String&);
    
    // Do a quick check to make sure the database tables are in place and the db version is current
    bool isValidDatabase(SQLDatabase&);
    
    // Delete all tables from the given database
    void clearDatabaseTables(SQLDatabase&);
    
    // Create the tables and triggers for the given database.
    void createDatabaseTables(SQLDatabase&);
    
    // Returns the image data for the given IconURL, checking both databases if necessary
    void imageDataForIconURL(const String& iconURL, Vector<unsigned char>&);
    
    // Retains an iconURL, bringing it back from the brink if it was pending deletion
    void retainIconURL(const String& iconURL);
    
    // Releases an iconURL, putting it on the pending delete queue if it's totally released
    void releaseIconURL(const String& iconURL);
    
    // FIXME: This method is currently implemented in WebCoreIconDatabaseBridge so we can be in ObjC++ and fire off a loader in Webkit
    // Once all of the loader logic is sufficiently moved into WebCore we need to move this implementation to IconDatabase.cpp
    // using WebCore-style loaders
    void loadIconFromURL(const String&);
    
    static IconDatabase* m_sharedInstance;
        
    SQLDatabase m_mainDB;
    SQLDatabase m_privateBrowsingDB;
    SQLDatabase* m_currentDB;
    
    IconDataCache* m_defaultIconDataCache;
    
    bool m_privateBrowsingEnabled;
    
    Timer<IconDatabase> m_startupTimer;
    Timer<IconDatabase> m_updateTimer;
    
    bool m_initialPruningComplete;
    SQLTransaction* m_initialPruningTransaction;
    SQLStatement* m_preparedPageRetainInsertStatement;
    
    HashMap<String, IconDataCache*> m_iconURLToIconDataCacheMap;
    HashSet<IconDataCache*> m_iconDataCachesPendingUpdate;
    
    HashMap<String, String> m_pageURLToIconURLMap;
    HashSet<String> m_pageURLsPendingAddition;

    // This will keep track of the retaincount for each pageURL
    HashMap<String, int> m_pageURLToRetainCount;
    // This will keep track of the retaincount for each iconURL (ie - the number of pageURLs retaining this icon)
    HashMap<String, int> m_iconURLToRetainCount;

    HashSet<String> m_pageURLsPendingDeletion;
    HashSet<String> m_iconURLsPendingDeletion;
};

} //namespace WebCore

#endif
