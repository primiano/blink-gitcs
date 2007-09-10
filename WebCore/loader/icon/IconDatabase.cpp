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

#include "config.h"
#include "IconDatabase.h"

#include "AutodrainedPool.h"
#include "CString.h"
#include "DocumentLoader.h"
#include "FileSystem.h"
#include "IconDatabaseClient.h"
#include "IconRecord.h"
#include "Image.h"
#include "IntSize.h"
#include "KURL.h"
#include "Logging.h"
#include "PageURLRecord.h"
#include "SQLStatement.h"
#include "SQLTransaction.h"
#include "SystemTime.h"

#if PLATFORM(WIN_OS)
#include <windows.h>
#include <winbase.h>
#include <shlobj.h>
#else
#include <sys/stat.h>
#endif

#include <errno.h>

// For methods that are meant to support API from the main thread - should not be called internally
#define ASSERT_NOT_SYNC_THREAD() ASSERT(!m_syncThreadRunning || !IS_ICON_SYNC_THREAD())

// For methods that are meant to support the sync thread ONLY
#define IS_ICON_SYNC_THREAD() pthread_equal(pthread_self(), m_syncThread)
#define ASSERT_ICON_SYNC_THREAD() ASSERT(IS_ICON_SYNC_THREAD())

namespace WebCore {

static IconDatabase* sharedIconDatabase = 0;
static int databaseCleanupCounter = 0;

// This version number is in the DB and marks the current generation of the schema
// Currently, a mismatched schema causes the DB to be wiped and reset.  This isn't 
// so bad during development but in the future, we would need to write a conversion
// function to advance older released schemas to "current"
const int currentDatabaseVersion = 6;

// Icons expire once every 4 days
const int iconExpirationTime = 60*60*24*4; 

const int updateTimerDelay = 5; 

static bool checkIntegrityOnOpen = false;

#ifndef NDEBUG
static String urlForLogging(const String& url)
{
    static unsigned urlTruncationLength = 120;

    if (url.length() < urlTruncationLength)
        return url;
    return url.substring(0, urlTruncationLength) + "...";
}
#endif

static IconDatabaseClient* defaultClient() 
{
    static IconDatabaseClient* defaultClient = new IconDatabaseClient();
    return defaultClient;
}

IconDatabase* iconDatabase()
{
    if (!sharedIconDatabase)
        sharedIconDatabase = new IconDatabase;
    return sharedIconDatabase;
}

// ************************
// *** Main Thread Only ***
// ************************

void IconDatabase::setClient(IconDatabaseClient* client)
{
    // We don't allow a null client, because we never null check it anywhere in this code
    // Also don't allow a client change after the thread has already began 
    // (setting the client should occur before the database is opened)
    ASSERT(client);
    ASSERT(!m_syncThreadRunning);
    if (!client || m_syncThreadRunning)
        return;
        
    m_client = client;
}

bool makeAllDirectories(const String& path)
{
#if PLATFORM(WIN_OS)
    String fullPath = path;
    if (!SHCreateDirectoryEx(0, fullPath.charactersWithNullTermination(), 0)) {
        DWORD error = GetLastError();
        if (error != ERROR_FILE_EXISTS && error != ERROR_ALREADY_EXISTS) {
            LOG_ERROR("Failed to create path %s", path.utf8().data());
            return false;
        }
    }
#else
    CString fullPath = path.utf8();
    if (!access(fullPath.data(), F_OK))
        return true;
        
    char* p = fullPath.mutableData() + 1;
    int length = fullPath.length();
    
    if(p[length - 1] == '/')
        p[length - 1] = '\0';
    for (; *p; ++p)
        if (*p == '/') {
            *p = '\0';
            if (access(fullPath.data(), F_OK))
                if (mkdir(fullPath.data(), S_IRWXU))
                    return false;
            *p = '/';
        }
    if (access(fullPath.data(), F_OK))        
        if (mkdir(fullPath.data(), S_IRWXU))
            return false;
#endif   
    return true;
}

bool IconDatabase::open(const String& databasePath)
{
    ASSERT_NOT_SYNC_THREAD();

    if (!m_isEnabled)
        return false;

    if (isOpen()) {
        LOG_ERROR("Attempt to reopen the IconDatabase which is already open.  Must close it first.");
        return false;
    }
 
    // Need to create the database path if it doesn't already exist
    makeAllDirectories(databasePath);
    
    // First we'll formulate the full path for the database file
    String dbFilename;
#if PLATFORM(WIN_OS)
    if (databasePath[databasePath.length()] == '\\')
        dbFilename = databasePath + defaultDatabaseFilename();
    else
        dbFilename = databasePath + "\\" + defaultDatabaseFilename();
#else
    if (databasePath[databasePath.length()] == '/')
        dbFilename = databasePath + defaultDatabaseFilename();
    else
        dbFilename = databasePath + "/" + defaultDatabaseFilename();
#endif

    m_completeDatabasePath = dbFilename.copy();
    
    // Lock here as well as first thing in the thread so the tread doesn't actually commence until the pthread_create() call 
    // completes and m_syncThreadRunning is properly set
    m_syncLock.lock();
    m_syncThreadRunning = !pthread_create(&m_syncThread, NULL, IconDatabase::iconDatabaseSyncThreadStart, this);
    m_syncLock.unlock();

    return m_syncThreadRunning;
}

void IconDatabase::close()
{
    ASSERT_NOT_SYNC_THREAD();
    
    if (m_syncThreadRunning) {
        // Set the flag to tell the sync thread to wrap it up
        m_threadTerminationRequested = true;

        // Wake up the sync thread if it's waiting
        wakeSyncThread();
        
        // Wait for the sync thread to terminate
        if (pthread_join(m_syncThread, NULL) == EDEADLK)
            LOG_ERROR("m_syncThread was found to be deadlocked trying to quit");
    }

    m_syncThreadRunning = false;    
    m_threadTerminationRequested = false;
    m_removeIconsRequested = false;
}

void IconDatabase::removeAllIcons()
{
    ASSERT_NOT_SYNC_THREAD();
    
    if (!isOpen())
        return;

    LOG(IconDatabase, "Requesting background thread to remove all icons");
 
    m_removeIconsRequested = true;
    wakeSyncThread();
}

Image* IconDatabase::iconForPageURL(const String& pageURLOriginal, const IntSize& size, bool cache)
{   
    ASSERT_NOT_SYNC_THREAD();
    
    // pageURLOriginal can not be stored without being deep copied first.  
    // We should go our of our way to only copy it if we have to store it
    
    if (!isOpen())
        return defaultIcon(size);
                
    MutexLocker locker(m_urlAndIconLock);
    
    String pageURLCopy; // Creates a null string for easy testing
    
    PageURLRecord* pageRecord = m_pageURLToRecordMap.get(pageURLOriginal);
    if (!pageRecord) {
        pageURLCopy = pageURLOriginal.copy();
        pageRecord = getOrCreatePageURLRecord(pageURLCopy);
    }
    
    // If pageRecord is NULL, one of two things is true -
    // 1 - The initial url import is incomplete and this pageURL was marked to be notified once it is complete if an iconURL exists
    // 2 - The initial url import IS complete and this pageURL has no icon
    if (!pageRecord) {
        MutexLocker locker(m_pendingReadingLock);
        
        // Import is ongoing, there might be an icon.  In this case, register to be notified when the icon comes in
        // If we ever reach this condition, we know we've already made the pageURL copy
        if (!m_iconURLImportComplete)
            m_pageURLsInterestedInIcons.add(pageURLCopy);
        
        return 0;
    }

    IconRecord* iconRecord = pageRecord->iconRecord();
    
    // If the initial URL import isn't complete, it's possible to have a PageURL record without an associated icon
    // In this case, the pageURL is already in the set to alert the client when the iconURL mapping is complete so
    // we can just bail now
    if (!m_iconURLImportComplete && !iconRecord)
        return 0;
    
    // The only way we should *not* have an icon record is if this pageURL is retained but has no icon yet - make sure of that
    ASSERT(iconRecord || m_retainedPageURLs.contains(pageURLOriginal));
    
    if (!iconRecord)
        return 0;
        
    // If it's a new IconRecord object that doesn't have its imageData set yet,
    // mark it to be read by the background thread
    if (iconRecord->imageDataStatus() == ImageDataStatusUnknown) {
        if (pageURLCopy.isNull())
            pageURLCopy = pageURLOriginal.copy();
    
        MutexLocker locker(m_pendingReadingLock);
        m_pageURLsInterestedInIcons.add(pageURLCopy);
        m_iconsPendingReading.add(iconRecord);
        wakeSyncThread();
        return 0;
    }
    
    // If the size parameter was (0, 0) that means the caller of this method just wanted the read from disk to be kicked off
    // and isn't actually interested in the image return value
    if (size == IntSize(0, 0))
        return 0;
        
    // PARANOID DISCUSSION: This method makes some assumptions.  It returns a WebCore::image which the icon database might dispose of at anytime in the future,
    // and Images aren't ref counted.  So there is no way for the client to guarantee continued existence of the image.
    // This has *always* been the case, but in practice clients would always create some other platform specific representation of the image
    // and drop the raw Image*.  On Mac an NSImage, and on windows drawing into an HBITMAP.
    // The async aspect adds a huge question - what if the image is deleted before the platform specific API has a chance to create its own
    // representation out of it?
    // If an image is read in from the icondatabase, we do *not* overwrite any image data that exists in the in-memory cache.  
    // This is because we make the assumption that anything in memory is newer than whatever is in the database.
    // So the only time the data will be set from the second thread is when it is INITIALLY being read in from the database, but we would never 
    // delete the image on the secondary thread if the image already exists.
    return iconRecord->image(size);
}

void IconDatabase::readIconForPageURLFromDisk(const String& pageURL)
{
    // The effect of asking for an Icon for a pageURL automatically queues it to be read from disk
    // if it hasn't already been set in memory.  The special IntSize (0, 0) is a special way of telling 
    // that method "I don't care about the actual Image, i just want you to make sure you're getting it from disk.
    iconForPageURL(pageURL, IntSize(0,0));
}

String IconDatabase::iconURLForPageURL(const String& pageURLOriginal)
{    
    ASSERT_NOT_SYNC_THREAD(); 
        
    // Cannot do anything with pageURLOriginal that would end up storing it without deep copying first
    // Also, in the case we have a real answer for the caller, we must deep copy that as well
    
    if (!isOpen() || pageURLOriginal.isEmpty())
        return String();
        
    MutexLocker locker(m_urlAndIconLock);
    
    PageURLRecord* pageRecord = m_pageURLToRecordMap.get(pageURLOriginal);
    if (!pageRecord)
        pageRecord = getOrCreatePageURLRecord(pageURLOriginal.copy());
    
    // If pageRecord is NULL, one of two things is true -
    // 1 - The initial url import is incomplete and this pageURL has already been marked to be notified once it is complete if an iconURL exists
    // 2 - The initial url import IS complete and this pageURL has no icon
    if (!pageRecord)
        return String();
    
    // Possible the pageRecord is around because it's a retained pageURL with no iconURL, so we have to check
    return pageRecord->iconRecord() ? pageRecord->iconRecord()->iconURL().copy() : String();
}

Image* IconDatabase::defaultIcon(const IntSize& size)
{
    ASSERT_NOT_SYNC_THREAD();

    if (!m_defaultIconRecord) {
        m_defaultIconRecord = new IconRecord("urlIcon");
        m_defaultIconRecord->loadImageFromResource("urlIcon");
    }
    
    return m_defaultIconRecord->image(size);
}


void IconDatabase::retainIconForPageURL(const String& pageURLOriginal)
{    
    ASSERT_NOT_SYNC_THREAD();
    
    // Cannot do anything with pageURLOriginal that would end up storing it without deep copying first
    
    if (!isOpen() || pageURLOriginal.isEmpty())
        return;
       
    MutexLocker locker(m_urlAndIconLock);

    PageURLRecord* record = m_pageURLToRecordMap.get(pageURLOriginal);
    
    String pageURL;
    
    if (!record) {
        pageURL = pageURLOriginal.copy();

        record = new PageURLRecord(pageURL);
        m_pageURLToRecordMap.set(pageURL, record);
        m_retainedPageURLs.add(pageURL);
    }
    
    if (record->retain()) {
        // If we read the iconURLs yet, we want to avoid any pageURL->iconURL lookups and the pageURLsPendingDeletion is moot, 
        // so we bail here and skip those steps
        if (!m_iconURLImportComplete)
            return;

        MutexLocker locker(m_pendingSyncLock);
        // If this pageURL waiting to be sync'ed, update the sync record
        // This saves us in the case where a page was ready to be deleted from the database but was just retained - so theres no need to delete it!
        if (!m_privateBrowsingEnabled && m_pageURLsPendingSync.contains(pageURLOriginal)) {
            if (pageURL.isNull())
                pageURL = pageURLOriginal.copy();
        
            LOG(IconDatabase, "Bringing %s back from the brink", pageURL.utf8().data());
            m_pageURLsPendingSync.set(pageURL, record->snapshot());
        }
    }
}

void IconDatabase::releaseIconForPageURL(const String& pageURLOriginal)
{
    ASSERT_NOT_SYNC_THREAD();
        
    // Cannot do anything with pageURLOriginal that would end up storing it without deep copying first
    
    if (!isOpen() || pageURLOriginal.isEmpty())
        return;
    
    MutexLocker locker(m_urlAndIconLock);

    // Check if this pageURL is actually retained
    if (!m_retainedPageURLs.contains(pageURLOriginal)) {
        LOG_ERROR("Attempting to release icon for URL %s which is not retained", urlForLogging(pageURLOriginal).utf8().data());
        return;
    }
    
    // Get its retain count - if it's retained, we'd better have a PageURLRecord for it
    PageURLRecord* pageRecord = m_pageURLToRecordMap.get(pageURLOriginal);
    ASSERT(pageRecord);
    LOG(IconDatabase, "Releasing pageURL %s to a retain count of %i", urlForLogging(pageURLOriginal).utf8().data(), pageRecord->retainCount() - 1);
    ASSERT(pageRecord->retainCount() > 0);
        
    // If it still has a positive retain count, store the new count and bail
    if (pageRecord->release())
        return;
        
    // This pageRecord has now been fully released.  Do the appropriate cleanup
    LOG(IconDatabase, "No more retainers for PageURL %s", urlForLogging(pageURLOriginal).utf8().data());
    m_pageURLToRecordMap.remove(pageURLOriginal);
    m_retainedPageURLs.remove(pageURLOriginal);       
    
    // Grab the iconRecord for later use (and do a sanity check on it for kicks)
    IconRecord* iconRecord = pageRecord->iconRecord();
    
    ASSERT(!iconRecord || (iconRecord && m_iconURLToRecordMap.get(iconRecord->iconURL()) == iconRecord));

    {
        MutexLocker locker(m_pendingReadingLock);
        
        // Since this pageURL is going away, there's no reason anyone would ever be interested in its read results    
        if (!m_iconURLImportComplete)
            m_pageURLsPendingImport.remove(pageURLOriginal);
        m_pageURLsInterestedInIcons.remove(pageURLOriginal);
        
        // If this icon is down to it's last retainer, we don't care about reading it in from disk anymore
        if (iconRecord && iconRecord->hasOneRef()) {
            m_iconURLToRecordMap.remove(iconRecord->iconURL());
            m_iconsPendingReading.remove(iconRecord);
        }
    }
    
    // Mark stuff for deletion from the database only if we're not in private browsing
    if (!m_privateBrowsingEnabled) {
        MutexLocker locker(m_pendingSyncLock);
        m_pageURLsPendingSync.set(pageURLOriginal.copy(), pageRecord->snapshot(true));
    
        // If this page is the last page to refer to a particular IconRecord, that IconRecord needs to
        // be marked for deletion
        if (iconRecord && iconRecord->hasOneRef())
            m_iconsPendingSync.set(iconRecord->iconURL(), iconRecord->snapshot(true));
    }
    
    delete pageRecord;

    scheduleOrDeferSyncTimer();
}

void IconDatabase::setIconDataForIconURL(PassRefPtr<SharedBuffer> dataOriginal, const String& iconURLOriginal)
{    
    ASSERT_NOT_SYNC_THREAD();
    
    // Cannot do anything with dataOriginal or iconURLOriginal that would end up storing them without deep copying first
    
    if (!isOpen() || iconURLOriginal.isEmpty())
        return;
    
    RefPtr<SharedBuffer> data = dataOriginal ? dataOriginal->copy() : 0;
    String iconURL = iconURLOriginal.copy();
    
    Vector<String> pageURLs;
    {
        MutexLocker locker(m_urlAndIconLock);
    
        // If this icon was pending a read, remove it from that set because this new data should override what is on disk
        IconRecord* icon = m_iconURLToRecordMap.get(iconURL);
        if (icon) {
            MutexLocker locker(m_pendingReadingLock);
            m_iconsPendingReading.remove(icon);
        } else
            icon = getOrCreateIconRecord(iconURL);
    
        // Update the data and set the time stamp
        icon->setImageData(data);
        icon->setTimestamp((int)currentTime());
        
        // Copy the current retaining pageURLs - if any - to notify them of the change
        pageURLs.appendRange(icon->retainingPageURLs().begin(), icon->retainingPageURLs().end());
        
        // Mark the IconRecord as requiring an update to the database only if private browsing is disabled
        if (!m_privateBrowsingEnabled) {
            MutexLocker locker(m_pendingSyncLock);
            m_iconsPendingSync.set(iconURL, icon->snapshot());
        }
    }

    // Send notification out regarding all PageURLs that retain this icon
    // But not if we're on the sync thread because that implies this mapping
    // comes from the initial import which we don't want notifications for
    if (!IS_ICON_SYNC_THREAD()) {
        // Start the timer to commit this change - or further delay the timer if it was already started
        scheduleOrDeferSyncTimer();

        // Informal testing shows that draining the autorelease pool every 25 iterations is about as low as we can go
        // before performance starts to drop off, but we don't want to increase this number because then accumulated memory usage will go up        
        AutodrainedPool pool(25);

        for (unsigned i = 0; i < pageURLs.size(); ++i) {
            LOG(IconDatabase, "Dispatching notification that retaining pageURL %s has a new icon", urlForLogging(pageURLs[i]).utf8().data());
            m_client->dispatchDidAddIconForPageURL(pageURLs[i]);

            pool.cycle();
        }
    }
}

void IconDatabase::setIconURLForPageURL(const String& iconURLOriginal, const String& pageURLOriginal)
{    
    ASSERT_NOT_SYNC_THREAD();

    // Cannot do anything with iconURLOriginal or pageURLOriginal that would end up storing them without deep copying first
    
    ASSERT(!iconURLOriginal.isEmpty());
        
    if (!isOpen() || pageURLOriginal.isEmpty())
        return;
    
    String iconURL, pageURL;
    
    {
        MutexLocker locker(m_urlAndIconLock);

        PageURLRecord* pageRecord = m_pageURLToRecordMap.get(pageURLOriginal);
        
        // If the urls already map to each other, bail.
        // This happens surprisingly often, and seems to cream iBench performance
        if (pageRecord && pageRecord->iconRecord() && pageRecord->iconRecord()->iconURL() == iconURLOriginal)
            return;
            
        pageURL = pageURLOriginal.copy();
        iconURL = iconURLOriginal.copy();

        if (!pageRecord) {
            pageRecord = new PageURLRecord(pageURL);
            m_pageURLToRecordMap.set(pageURL, pageRecord);
        }

        RefPtr<IconRecord> iconRecord = pageRecord->iconRecord();

        // Otherwise, set the new icon record for this page
        IconRecord* newIconRecord = getOrCreateIconRecord(iconURL);
        pageRecord->setIconRecord(newIconRecord);

        // If the current icon has only a single ref left, it is about to get wiped out. 
        // Remove it from the in-memory records and don't bother reading it in from disk anymore
        if (iconRecord && iconRecord->hasOneRef()) {
            ASSERT(iconRecord->retainingPageURLs().size() == 0);
            LOG(IconDatabase, "Icon for icon url %s is about to be destroyed - removing mapping for it", urlForLogging(iconRecord->iconURL()).utf8().data());
            m_iconURLToRecordMap.remove(iconRecord->iconURL());
            MutexLocker locker(m_pendingReadingLock);
            m_iconsPendingReading.remove(iconRecord.get());
        }
        
        // And mark this mapping to be added to the database
        if (!m_privateBrowsingEnabled) {
            MutexLocker locker(m_pendingSyncLock);
            m_pageURLsPendingSync.set(pageURL, pageRecord->snapshot());
            
            // If the icon is on it's last ref, mark it for deletion
            if (iconRecord && iconRecord->hasOneRef())
                m_iconsPendingSync.set(iconRecord->iconURL(), iconRecord->snapshot(true));
        }
    }

    // Since this mapping is new, send the notification out - but not if we're on the sync thread because that implies this mapping
    // comes from the initial import which we don't want notifications for
    if (!IS_ICON_SYNC_THREAD()) {
        // Start the timer to commit this change - or further delay the timer if it was already started
        scheduleOrDeferSyncTimer();
        
        LOG(IconDatabase, "Dispatching notification that we changed an icon mapping for url %s", urlForLogging(pageURL).utf8().data());
        AutodrainedPool pool;
        m_client->dispatchDidAddIconForPageURL(pageURL);
    }
}

IconLoadDecision IconDatabase::loadDecisionForIconURL(const String& iconURL, DocumentLoader* notificationDocumentLoader)
{
    ASSERT_NOT_SYNC_THREAD();

    if (!isOpen() || iconURL.isEmpty())
        return IconLoadNo;
    
    // If we have a IconRecord, it should also have its timeStamp marked because there is only two times when we create the IconRecord:
    // 1 - When we read the icon urls from disk, getting the timeStamp at the same time
    // 2 - When we get a new icon from the loader, in which case the timestamp is set at that time
    {
        MutexLocker locker(m_urlAndIconLock);
        if (IconRecord* icon = m_iconURLToRecordMap.get(iconURL)) {
            LOG(IconDatabase, "Found expiration time on a present icon based on existing IconRecord");
            return (int)currentTime() - icon->getTimestamp() > iconExpirationTime ? IconLoadYes : IconLoadNo;
        }
    }
    
    // If we don't have a record for it, but we *have* imported all iconURLs from disk, then we should load it now
    MutexLocker readingLocker(m_pendingReadingLock);
    if (m_iconURLImportComplete)
        return IconLoadYes;
        
    // Otherwise - since we refuse to perform I/O on the main thread to find out for sure - we return the answer that says
    // "You might be asked to load this later, so flag that"
    LOG(IconDatabase, "Don't know if we should load %s or not - adding %p to the set of document loaders waiting on a decision", iconURL.utf8().data(), notificationDocumentLoader);
    m_loadersPendingDecision.add(notificationDocumentLoader);    

    return IconLoadUnknown;
}

bool IconDatabase::iconDataKnownForIconURL(const String& iconURL)
{
    ASSERT_NOT_SYNC_THREAD();
    
    MutexLocker locker(m_urlAndIconLock);
    if (IconRecord* icon = m_iconURLToRecordMap.get(iconURL))
        return icon->imageDataStatus() != ImageDataStatusUnknown;

    return false;
}

void IconDatabase::setEnabled(bool enabled)
{
    ASSERT_NOT_SYNC_THREAD();
    
    if (!enabled && isOpen())
        close();
    m_isEnabled = enabled;
}

bool IconDatabase::isEnabled() const
{
    ASSERT_NOT_SYNC_THREAD();
    
     return m_isEnabled;
}

void IconDatabase::setPrivateBrowsingEnabled(bool flag)
{
    m_privateBrowsingEnabled = flag;
}

bool IconDatabase::isPrivateBrowsingEnabled() const
{
    return m_privateBrowsingEnabled;
}

void IconDatabase::delayDatabaseCleanup()
{
    ++databaseCleanupCounter;
    if (databaseCleanupCounter == 1)
        LOG(IconDatabase, "Database cleanup is now DISABLED");
}

void IconDatabase::allowDatabaseCleanup()
{
    if (--databaseCleanupCounter < 0)
        databaseCleanupCounter = 0;
    if (databaseCleanupCounter == 0)
        LOG(IconDatabase, "Database cleanup is now ENABLED");
}

void IconDatabase::checkIntegrityBeforeOpening()
{
    checkIntegrityOnOpen = true;
}

size_t IconDatabase::pageURLMappingCount()
{
    MutexLocker locker(m_urlAndIconLock);
    return m_pageURLToRecordMap.size();
}

size_t IconDatabase::retainedPageURLCount()
{
    MutexLocker locker(m_urlAndIconLock);
    return m_retainedPageURLs.size();
}

size_t IconDatabase::iconRecordCount()
{
    MutexLocker locker(m_urlAndIconLock);
    return m_iconURLToRecordMap.size();
}

size_t IconDatabase::iconRecordCountWithData()
{
    MutexLocker locker(m_urlAndIconLock);
    size_t result = 0;
    
    HashMap<String, IconRecord*>::iterator i = m_iconURLToRecordMap.begin();
    HashMap<String, IconRecord*>::iterator end = m_iconURLToRecordMap.end();
    
    for (; i != end; ++i)
        result += ((*i).second->imageDataStatus() == ImageDataStatusPresent);
            
    return result;
}

IconDatabase::IconDatabase()
    : m_syncTimer(this, &IconDatabase::syncTimerFired)
    , m_syncThreadRunning(false)
    , m_defaultIconRecord(0)
    , m_isEnabled(false)
    , m_privateBrowsingEnabled(false)
    , m_threadTerminationRequested(false)
    , m_removeIconsRequested(false)
    , m_iconURLImportComplete(false)
    , m_initialPruningComplete(false)
    , m_client(defaultClient())
    , m_imported(false)
    , m_isImportedSet(false)
{
#if PLATFORM(DARWIN)
    ASSERT(pthread_main_np());
#endif
}

IconDatabase::~IconDatabase()
{
    ASSERT_NOT_REACHED();
}

void IconDatabase::notifyPendingLoadDecisions()
{    
    iconDatabase()->notifyPendingLoadDecisionsInternal();
}

void IconDatabase::notifyPendingLoadDecisionsInternal()
{
    ASSERT_NOT_SYNC_THREAD();
    
    // This method should only be called upon completion of the initial url import from the database
    ASSERT(m_iconURLImportComplete);
    LOG(IconDatabase, "Notifying all DocumentLoaders that were waiting on a load decision for thier icons");
    
    HashSet<RefPtr<DocumentLoader> >::iterator i = m_loadersPendingDecision.begin();
    HashSet<RefPtr<DocumentLoader> >::iterator end = m_loadersPendingDecision.end();
    
    for (; i != end; ++i)
        if ((*i)->refCount() > 1)
            (*i)->iconLoadDecisionAvailable();
            
    m_loadersPendingDecision.clear();
}

void IconDatabase::wakeSyncThread()
{
    MutexLocker locker(m_syncLock);
    m_syncCondition.signal();
}

void IconDatabase::scheduleOrDeferSyncTimer()
{
    ASSERT_NOT_SYNC_THREAD();
    
    m_syncTimer.startOneShot(updateTimerDelay);
}

void IconDatabase::syncTimerFired(Timer<IconDatabase>*)
{
    ASSERT_NOT_SYNC_THREAD();
    wakeSyncThread();
}

// ******************
// *** Any Thread ***
// ******************

bool IconDatabase::isOpen() const
{
    MutexLocker locker(m_syncLock);
    return m_syncDB.isOpen();
}

String IconDatabase::databasePath() const
{
    MutexLocker locker(m_syncLock);
    return m_completeDatabasePath.copy();
}

String IconDatabase::defaultDatabaseFilename()
{
    static String defaultDatabaseFilename = "Icons.db";
    return defaultDatabaseFilename.copy();
}

// Unlike getOrCreatePageURLRecord(), getOrCreateIconRecord() does not mark the icon as "interested in import"
IconRecord* IconDatabase::getOrCreateIconRecord(const String& iconURL)
{
    // Clients of getOrCreateIconRecord() are required to acquire the m_urlAndIconLock before calling this method
    ASSERT(m_urlAndIconLock.tryLock() == EBUSY);

    if (IconRecord* icon = m_iconURLToRecordMap.get(iconURL))
        return icon;
        
    IconRecord* newIcon = new IconRecord(iconURL);
    m_iconURLToRecordMap.set(iconURL, newIcon);
        
    return newIcon;    
}

// This method retrieves the existing PageURLRecord, or creates a new one and marks it as "interested in the import" for later notification
PageURLRecord* IconDatabase::getOrCreatePageURLRecord(const String& pageURL)
{
    // Clients of getOrCreatePageURLRecord() are required to acquire the m_urlAndIconLock before calling this method
    ASSERT(m_urlAndIconLock.tryLock() == EBUSY);

    PageURLRecord* pageRecord = m_pageURLToRecordMap.get(pageURL);
    
    MutexLocker locker(m_pendingReadingLock);
    if (!m_iconURLImportComplete) {
        // If the initial import of all URLs hasn't completed and we have no page record, we assume we *might* know about this later and create a record for it
        if (!pageRecord) {
            LOG(IconDatabase, "Creating new PageURLRecord for pageURL %s", urlForLogging(pageURL).utf8().data());
            pageRecord = new PageURLRecord(pageURL);
            m_pageURLToRecordMap.set(pageURL, pageRecord);
        }

        // If the pageRecord for this page does not have an iconRecord attached to it, then it is a new pageRecord still awaiting the initial import
        // Mark the URL as "interested in the result of the import" then bail
        if (!pageRecord->iconRecord()) {
            m_pageURLsPendingImport.add(pageURL);
            return 0;
        }
    }

    // We've done the initial import of all URLs known in the database.  If this record doesn't exist now, it never will    
     return pageRecord;
}


// ************************
// *** Sync Thread Only ***
// ************************

void IconDatabase::importIconURLForPageURL(const String& iconURL, const String& pageURL)
{
    ASSERT_ICON_SYNC_THREAD();
    
    // This function is only for setting actual existing url mappings so assert that neither of these URLs are empty
    ASSERT(!iconURL.isEmpty() && !pageURL.isEmpty());
    
    setIconURLForPageURLInSQLDatabase(iconURL, pageURL);    
}

void IconDatabase::importIconDataForIconURL(PassRefPtr<SharedBuffer> data, const String& iconURL)
{
    ASSERT_ICON_SYNC_THREAD();
    
    ASSERT(!iconURL.isEmpty());

    writeIconSnapshotToSQLDatabase(IconSnapshot(iconURL, (int)currentTime(), data.get()));
}

bool IconDatabase::shouldStopThreadActivity() const
{
    ASSERT_ICON_SYNC_THREAD();
    
    return m_threadTerminationRequested || m_removeIconsRequested;
}

void* IconDatabase::iconDatabaseSyncThreadStart(void* vIconDatabase)
{    
    IconDatabase* iconDB = static_cast<IconDatabase*>(vIconDatabase);
    
    return iconDB->iconDatabaseSyncThread();
}

void* IconDatabase::iconDatabaseSyncThread()
{
    // The call to create this thread might not complete before the thread actually starts, so we might fail this ASSERT_ICON_SYNC_THREAD() because the pointer 
    // to our thread structure hasn't been filled in yet.
    // To fix this, the main thread acquires this lock before creating us, then releases the lock after creation is complete.  A quick lock/unlock cycle here will 
    // prevent us from running before that call completes
    m_syncLock.lock();
    m_syncLock.unlock();

    ASSERT_ICON_SYNC_THREAD();
    
    LOG(IconDatabase, "(THREAD) IconDatabase sync thread started");

#ifndef NDEBUG
    double startTime = currentTime();
#endif

    // Existence of a journal file is evidence of a previous crash/force quit and automatically qualifies
    // us to do an integrity check
    String journalFilename = m_completeDatabasePath + "-journal";
    if (!checkIntegrityOnOpen) {
        AutodrainedPool pool;
        checkIntegrityOnOpen = fileExists(journalFilename);
    }
    
    {
        MutexLocker locker(m_syncLock);
        if (!m_syncDB.open(m_completeDatabasePath)) {
            LOG_ERROR("Unable to open icon database at path %s - %s", m_completeDatabasePath.utf8().data(), m_syncDB.lastErrorMsg());
            return 0;
        }
    }
    
    if (shouldStopThreadActivity())
        return syncThreadMainLoop();
        
#ifndef NDEBUG
    double timeStamp = currentTime();
    LOG(IconDatabase, "(THREAD) Open took %.4f seconds", timeStamp - startTime);
#endif    

    performOpenInitialization();
    if (shouldStopThreadActivity())
        return syncThreadMainLoop();
        
#ifndef NDEBUG
    double newStamp = currentTime();
    LOG(IconDatabase, "(THREAD) performOpenInitialization() took %.4f seconds, now %.4f seconds from thread start", newStamp - timeStamp, newStamp - startTime);
    timeStamp = newStamp;
#endif 

    if (!imported()) {
        LOG(IconDatabase, "(THREAD) Performing Safari2 import procedure");
        SQLTransaction importTransaction(m_syncDB);
        importTransaction.begin();
        
        // Commit the transaction only if the import completes (the import should be atomic)
        if (m_client->performImport()) {
            setImported(true);
            importTransaction.commit();
        } else {
            LOG(IconDatabase, "(THREAD) Safari 2 import was cancelled");
            importTransaction.rollback();
        }
        
        if (shouldStopThreadActivity())
            return syncThreadMainLoop();
            
#ifndef NDEBUG
        newStamp = currentTime();
        LOG(IconDatabase, "(THREAD) performImport() took %.4f seconds, now %.4f seconds from thread start", newStamp - timeStamp, newStamp - startTime);
        timeStamp = newStamp;
#endif 
    }
        
    // Uncomment the following line to simulate a long lasting URL import (*HUGE* icon databases, or network home directories)
    // while (currentTime() - timeStamp < 10);

    // Read in URL mappings from the database          
    LOG(IconDatabase, "(THREAD) Starting iconURL import");
    performURLImport();
    
    if (shouldStopThreadActivity())
        return syncThreadMainLoop();

#ifndef NDEBUG
    newStamp = currentTime();
    LOG(IconDatabase, "(THREAD) performURLImport() took %.4f seconds.  Entering main loop %.4f seconds from thread start", newStamp - timeStamp, newStamp - startTime);
#endif 

    LOG(IconDatabase, "(THREAD) Beginning sync");
    return syncThreadMainLoop();
}

static int databaseVersionNumber(SQLDatabase& db)
{
    return SQLStatement(db, "SELECT value FROM IconDatabaseInfo WHERE key = 'Version';").getColumnInt(0);
}

static bool isValidDatabase(SQLDatabase& db)
{

    // These four tables should always exist in a valid db
    if (!db.tableExists("IconInfo") || !db.tableExists("IconData") || !db.tableExists("PageURL") || !db.tableExists("IconDatabaseInfo"))
        return false;
    
    if (databaseVersionNumber(db) != currentDatabaseVersion) {
        LOG(IconDatabase, "DB version is not found or below expected valid version");
        return false;
    }
    
    return true;
}

static void createDatabaseTables(SQLDatabase& db)
{
    if (!db.executeCommand("CREATE TABLE PageURL (url TEXT NOT NULL ON CONFLICT FAIL UNIQUE ON CONFLICT REPLACE,iconID INTEGER NOT NULL ON CONFLICT FAIL);")) {
        LOG_ERROR("Could not create PageURL table in database (%i) - %s", db.lastError(), db.lastErrorMsg());
        db.close();
        return;
    }
    if (!db.executeCommand("CREATE INDEX PageURLIndex ON PageURL (url);")) {
        LOG_ERROR("Could not create PageURL index in database (%i) - %s", db.lastError(), db.lastErrorMsg());
        db.close();
        return;
    }
    if (!db.executeCommand("CREATE TABLE IconInfo (iconID INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE ON CONFLICT REPLACE, url TEXT NOT NULL ON CONFLICT FAIL UNIQUE ON CONFLICT FAIL, stamp INTEGER);")) {
        LOG_ERROR("Could not create IconInfo table in database (%i) - %s", db.lastError(), db.lastErrorMsg());
        db.close();
        return;
    }
    if (!db.executeCommand("CREATE INDEX IconInfoIndex ON IconInfo (url, iconID);")) {
        LOG_ERROR("Could not create PageURL index in database (%i) - %s", db.lastError(), db.lastErrorMsg());
        db.close();
        return;
    }
    if (!db.executeCommand("CREATE TABLE IconData (iconID INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE ON CONFLICT REPLACE, data BLOB);")) {
        LOG_ERROR("Could not create IconData table in database (%i) - %s", db.lastError(), db.lastErrorMsg());
        db.close();
        return;
    }
    if (!db.executeCommand("CREATE INDEX IconDataIndex ON IconData (iconID);")) {
        LOG_ERROR("Could not create PageURL index in database (%i) - %s", db.lastError(), db.lastErrorMsg());
        db.close();
        return;
    }
    if (!db.executeCommand("CREATE TABLE IconDatabaseInfo (key TEXT NOT NULL ON CONFLICT FAIL UNIQUE ON CONFLICT REPLACE,value TEXT NOT NULL ON CONFLICT FAIL);")) {
        LOG_ERROR("Could not create IconDatabaseInfo table in database (%i) - %s", db.lastError(), db.lastErrorMsg());
        db.close();
        return;
    }
    if (!db.executeCommand(String("INSERT INTO IconDatabaseInfo VALUES ('Version', ") + String::number(currentDatabaseVersion) + ");")) {
        LOG_ERROR("Could not insert icon database version into IconDatabaseInfo table (%i) - %s", db.lastError(), db.lastErrorMsg());
        db.close();
        return;
    }
}    

void IconDatabase::performOpenInitialization()
{
    ASSERT_ICON_SYNC_THREAD();
    
    if (!isOpen())
        return;
    
    if (checkIntegrityOnOpen) {
        checkIntegrityOnOpen = false;
        if (!checkIntegrity()) {
            LOG(IconDatabase, "Integrity check was bad - dumping IconDatabase");

            close();
            
            {
                MutexLocker locker(m_syncLock);
                // Should've been consumed by SQLite, delete just to make sure we don't see it again in the future;
                deleteFile(m_completeDatabasePath + "-journal");
                deleteFile(m_completeDatabasePath);
            }
            
            // Reopen the write database, creating it from scratch
            if (!m_syncDB.open(m_completeDatabasePath)) {
                LOG_ERROR("Unable to open icon database at path %s - %s", m_completeDatabasePath.utf8().data(), m_syncDB.lastErrorMsg());
                return;
            }          
        }
    }
        
    if (!isValidDatabase(m_syncDB)) {
        LOG(IconDatabase, "%s is missing or in an invalid state - reconstructing", m_syncDB.path().utf8().data());
        m_syncDB.clearAllTables();
        createDatabaseTables(m_syncDB);
    }

    // Reduce sqlite RAM cache size from default 2000 pages (~1.5kB per page). 3MB of cache for icon database is overkill
    if (!SQLStatement(m_syncDB, "PRAGMA cache_size = 200;").executeCommand())         
        LOG_ERROR("SQLite database could not set cache_size");
}

bool IconDatabase::checkIntegrity()
{
    ASSERT_ICON_SYNC_THREAD();
    
    SQLStatement integrity(m_syncDB, "PRAGMA integrity_check;");
    if (integrity.prepare() != SQLResultOk) {
        LOG_ERROR("checkIntegrity failed to execute");
        return false;
    }
    
    int resultCode = integrity.step();
    if (resultCode == SQLResultOk)
        return true;
        
    if (resultCode != SQLResultRow)
        return false;

    int columns = integrity.columnCount();
    if (columns != 1) {
        LOG_ERROR("Received %i columns performing integrity check, should be 1", columns);
        return false;
    }
        
    String resultText = integrity.getColumnText16(0);
        
    // A successful, no-error integrity check will be "ok" - all other strings imply failure
    if (resultText == "ok")
        return true;
    
    LOG_ERROR("Icon database integrity check failed - \n%s", resultText.utf8().data());
    return false;
}

void IconDatabase::performURLImport()
{
    ASSERT_ICON_SYNC_THREAD();

    SQLStatement query(m_syncDB, "SELECT PageURL.url, IconInfo.url, IconInfo.stamp FROM PageURL INNER JOIN IconInfo ON PageURL.iconID=IconInfo.iconID;");
    
    if (query.prepare() != SQLResultOk) {
        LOG_ERROR("Unable to prepare icon url import query");
        return;
    }
    
    // Informal testing shows that draining the autorelease pool every 25 iterations is about as low as we can go
    // before performance starts to drop off, but we don't want to increase this number because then accumulated memory usage will go up
    AutodrainedPool pool(25);
        
    int result = query.step();
    while (result == SQLResultRow) {
        String pageURL = query.getColumnText16(0);
        String iconURL = query.getColumnText16(1);

        {
            MutexLocker locker(m_urlAndIconLock);
            
            PageURLRecord* pageRecord = m_pageURLToRecordMap.get(pageURL);
            
            // If the pageRecord doesn't exist in this map, then no one has retained this pageURL
            // If the s_databaseCleanupCounter count is non-zero, then we're not supposed to be pruning the database in any manner,
            // so go ahead and actually create a pageURLRecord for this url even though it's not retained.
            // If database cleanup *is* allowed, we don't want to bother pulling in a page url from disk that noone is actually interested
            // in - we'll prune it later instead!
            if (!pageRecord && databaseCleanupCounter) {
                pageRecord = new PageURLRecord(pageURL);
                m_pageURLToRecordMap.set(pageURL, pageRecord);
            }
            
            if (pageRecord) {
                IconRecord* currentIcon = pageRecord->iconRecord();

                if (!currentIcon || currentIcon->iconURL() != iconURL) {
                    currentIcon = getOrCreateIconRecord(iconURL);
                    pageRecord->setIconRecord(currentIcon);
                }
            
                // Regardless, the time stamp from disk still takes precedence.  Until we read this icon from disk, we didn't think we'd seen it before
                // so we marked the timestamp as "now", but it's really much older
                currentIcon->setTimestamp(query.getColumnInt(2));
            }            
        }
        
        // FIXME: Currently the WebKit API supports 1 type of notification that is sent whenever we get an Icon URL for a Page URL.  We might want to re-purpose it to work for 
        // getting the actually icon itself also (so each pageurl would get this notification twice) or we might want to add a second type of notification -
        // one for the URL and one for the Image itself
        // Note that WebIconDatabase is not neccessarily API so we might be able to make this change
        {
            MutexLocker locker(m_pendingReadingLock);
            if (m_pageURLsPendingImport.contains(pageURL)) {
                m_client->dispatchDidAddIconForPageURL(pageURL);
                m_pageURLsPendingImport.remove(pageURL);
            
                pool.cycle();
            }
        }
        
        // Stop the import at any time of the thread has been asked to shutdown
        if (shouldStopThreadActivity()) {
            LOG(IconDatabase, "IconDatabase asked to terminate during performURLImport()");
            return;
        }
        
        result = query.step();
    }
    
    if (result != SQLResultDone)
        LOG(IconDatabase, "Error reading page->icon url mappings from database");

    // Clear the m_pageURLsPendingImport set - either the page URLs ended up with an iconURL (that we'll notify about) or not, 
    // but after m_iconURLImportComplete is set to true, we don't care about this set anymore
    Vector<String> urls;
    {
        MutexLocker locker(m_pendingReadingLock);

        urls.appendRange(m_pageURLsPendingImport.begin(), m_pageURLsPendingImport.end());
        m_pageURLsPendingImport.clear();        
        m_iconURLImportComplete = true;
    }
    
    Vector<String> urlsToNotify;
    
    // Loop through the urls pending import
    // Remove unretained ones if database cleanup is allowed
    // Keep a set of ones that are retained and pending notification
    
    {
        MutexLocker locker(m_urlAndIconLock);
        
        for (unsigned i = 0; i < urls.size(); ++i) {
            if (!m_retainedPageURLs.contains(urls[i])) {
                PageURLRecord* record = m_pageURLToRecordMap.get(urls[i]);
                if (record && !databaseCleanupCounter) {
                    m_pageURLToRecordMap.remove(urls[i]);
                    IconRecord* iconRecord = record->iconRecord();
                    
                    // If this page is the only remaining retainer of its icon, mark that icon for deletion and don't bother
                    // reading anything related to it 
                    if (iconRecord && iconRecord->hasOneRef()) {
                        m_iconURLToRecordMap.remove(iconRecord->iconURL());
                        
                        {
                            MutexLocker locker(m_pendingReadingLock);
                            m_pageURLsInterestedInIcons.remove(urls[i]);
                            m_iconsPendingReading.remove(iconRecord);
                        }
                        {
                            MutexLocker locker(m_pendingSyncLock);
                            m_iconsPendingSync.set(iconRecord->iconURL(), iconRecord->snapshot(true));                    
                        }
                    }
                    
                    delete record;
                }
            } else {
                urlsToNotify.append(urls[i]);
            }
        }
    }

    LOG(IconDatabase, "Notifying %i interested page URLs that their icon URL is known due to the import", urlsToNotify.size());
    // Now that we don't hold any locks, perform the actual notifications
    for (unsigned i = 0; i < urlsToNotify.size(); ++i) {
        LOG(IconDatabase, "Notifying icon info known for pageURL %s", urlsToNotify[i].utf8().data());
        m_client->dispatchDidAddIconForPageURL(urlsToNotify[i]);
        if (shouldStopThreadActivity())
            return;

        pool.cycle();
    }
    
    // Notify all DocumentLoaders that were waiting for an icon load decision on the main thread
    callOnMainThread(notifyPendingLoadDecisions);
}

void* IconDatabase::syncThreadMainLoop()
{
    ASSERT_ICON_SYNC_THREAD();
    static bool prunedUnretainedIcons = false;

    while (true) {
        m_syncLock.unlock();

#ifndef NDEBUG
        double timeStamp = currentTime();
#endif
        LOG(IconDatabase, "(THREAD) Main work loop starting");

        // If we should remove all icons, do it now.  This is an uninteruptible procedure that we will always do before quitting if it is requested
        if (m_removeIconsRequested) {
            removeAllIconsOnThread();
            m_removeIconsRequested = false;
        }
        
        // Then, if the thread should be quitting, quit now!
        if (m_threadTerminationRequested)
            break;
        
        bool didAnyWork = true;
        while (didAnyWork) {
            didAnyWork = readFromDatabase();
            if (shouldStopThreadActivity())
                break;
                
            bool didWrite = writeToDatabase();
            if (shouldStopThreadActivity())
                break;
                
            // Prune unretained icons after the first time we sync anything out to the database
            // This way, pruning won't be the only operation we perform to the database by itself
            // We also don't want to bother doing this if the thread should be terminating (the user is quitting)
            // or if private browsing is enabled
            // We also don't want to prune if the m_databaseCleanupCounter count is non-zero - that means someone
            // has asked to delay pruning
            if (didWrite && !m_privateBrowsingEnabled && !prunedUnretainedIcons && !databaseCleanupCounter) {
#ifndef NDEBUG
                double time = currentTime();
#endif
                LOG(IconDatabase, "(THREAD) Starting pruneUnretainedIcons()");
                
                pruneUnretainedIcons();
                
                LOG(IconDatabase, "(THREAD) pruneUnretainedIcons() took %.4f seconds", currentTime() - time);
                
                // If pruneUnretainedIcons() returned early due to requested thread termination, its still okay
                // to mark prunedUnretainedIcons true because we're about to terminate anyway
                prunedUnretainedIcons = true;
            }
            
            didAnyWork = didAnyWork || didWrite;
            if (shouldStopThreadActivity())
                break;
        }
        
#ifndef NDEBUG
        double newstamp = currentTime();
        LOG(IconDatabase, "(THREAD) Main work loop ran for %.4f seconds, %s requested to terminate", newstamp - timeStamp, shouldStopThreadActivity() ? "was" : "was not");
#endif
                    
        m_syncLock.lock();
        
        // There is some condition that is asking us to stop what we're doing now and handle a special case
        // This is either removing all icons, or shutting down the thread to quit the app
        // We handle those at the top of this main loop so continue to jump back up there
        if (shouldStopThreadActivity())
            continue;
            
        m_syncCondition.wait(m_syncLock); 
    }
    m_syncLock.unlock();
    
    // Thread is terminating at this point
    return cleanupSyncThread();
}

bool IconDatabase::readFromDatabase()
{
    ASSERT_ICON_SYNC_THREAD();
    
#ifndef NDEBUG
    double timeStamp = currentTime();
#endif

    bool didAnyWork = false;

    // We'll make a copy of the sets of things that need to be read.  Then we'll verify at the time of updating the record that it still wants to be updated
    // This way we won't hold the lock for a long period of time
    Vector<IconRecord*> icons;
    {
        MutexLocker locker(m_pendingReadingLock);
        icons.appendRange(m_iconsPendingReading.begin(), m_iconsPendingReading.end());
    }
    
    // Keep track of icons we actually read to notify them of the new icon    
    HashSet<String> urlsToNotify;
    
    for (unsigned i = 0; i < icons.size(); ++i) {
        didAnyWork = true;
        RefPtr<SharedBuffer> imageData = getImageDataForIconURLFromSQLDatabase(icons[i]->iconURL());

        // Verify this icon still wants to be read from disk
        {
            MutexLocker urlLocker(m_urlAndIconLock);
            {
                MutexLocker readLocker(m_pendingReadingLock);
                
                if (m_iconsPendingReading.contains(icons[i])) {
                    // Set the new data
                    icons[i]->setImageData(imageData.get());
                    
                    // Remove this icon from the set that needs to be read
                    m_iconsPendingReading.remove(icons[i]);
                    
                    // We have a set of all Page URLs that retain this icon as well as all PageURLs waiting for an icon
                    // We want to find the intersection of these two sets to notify them
                    // Check the sizes of these two sets to minimize the number of iterations
                    const HashSet<String>* outerHash;
                    const HashSet<String>* innerHash;
                    
                    if (icons[i]->retainingPageURLs().size() > m_pageURLsInterestedInIcons.size()) {
                        outerHash = &m_pageURLsInterestedInIcons;
                        innerHash = &(icons[i]->retainingPageURLs());
                    } else {
                        innerHash = &m_pageURLsInterestedInIcons;
                        outerHash = &(icons[i]->retainingPageURLs());
                    }
                    
                    HashSet<String>::const_iterator iter = outerHash->begin();
                    HashSet<String>::const_iterator end = outerHash->end();
                    for (; iter != end; ++iter) {
                        if (innerHash->contains(*iter)) {
                            LOG(IconDatabase, "%s is interesting in the icon we just read.  Adding it to the list and removing it from the interested set", urlForLogging(*iter).utf8().data());
                            urlsToNotify.add(*iter);
                        }
                        
                        // If we ever get to the point were we've seen every url interested in this icon, break early
                        if (urlsToNotify.size() == m_pageURLsInterestedInIcons.size())
                            break;
                    }
                    
                    // We don't need to notify a PageURL twice, so all the ones we're about to notify can be removed from the interested set
                    if (urlsToNotify.size() == m_pageURLsInterestedInIcons.size())
                        m_pageURLsInterestedInIcons.clear();
                    else {
                        iter = urlsToNotify.begin();
                        end = urlsToNotify.end();
                        for (; iter != end; ++iter)
                            m_pageURLsInterestedInIcons.remove(*iter);
                    }
                }
            }
        }
    
        if (shouldStopThreadActivity())
            return didAnyWork;
        
        // Informal testing shows that draining the autorelease pool every 25 iterations is about as low as we can go
        // before performance starts to drop off, but we don't want to increase this number because then accumulated memory usage will go up
        AutodrainedPool pool(25);

        // Now that we don't hold any locks, perform the actual notifications
        HashSet<String>::iterator iter = urlsToNotify.begin();
        HashSet<String>::iterator end = urlsToNotify.end();
        for (unsigned iteration = 0; iter != end; ++iter, ++iteration) {
            LOG(IconDatabase, "Notifying icon received for pageURL %s", urlForLogging(*iter).utf8().data());
            m_client->dispatchDidAddIconForPageURL(*iter);
            if (shouldStopThreadActivity())
                return didAnyWork;
            
            pool.cycle();
        }

        LOG(IconDatabase, "Done notifying %i pageURLs who just received their icons", urlsToNotify.size());
        urlsToNotify.clear();
        
        if (shouldStopThreadActivity())
            return didAnyWork;
    }

    LOG(IconDatabase, "Reading from database took %.4f seconds", currentTime() - timeStamp);

    return didAnyWork;
}

bool IconDatabase::writeToDatabase()
{
    ASSERT_ICON_SYNC_THREAD();

#ifndef NDEBUG
    double timeStamp = currentTime();
#endif

    bool didAnyWork = false;
    
    // We can copy the current work queue then clear it out - If any new work comes in while we're writing out,
    // we'll pick it up on the next pass.  This greatly simplifies the locking strategy for this method and remains cohesive with changes
    // asked for by the database on the main thread
    Vector<IconSnapshot> iconSnapshots;
    Vector<PageURLSnapshot> pageSnapshots;
    {
        MutexLocker locker(m_pendingSyncLock);
        
        iconSnapshots.appendRange(m_iconsPendingSync.begin().values(), m_iconsPendingSync.end().values());
        m_iconsPendingSync.clear();
        
        pageSnapshots.appendRange(m_pageURLsPendingSync.begin().values(), m_pageURLsPendingSync.end().values());
        m_pageURLsPendingSync.clear();
    }
    
    if (iconSnapshots.size() || pageSnapshots.size())
        didAnyWork = true;
        
    SQLTransaction syncTransaction(m_syncDB);
    syncTransaction.begin();
    
    for (unsigned i = 0; i < iconSnapshots.size(); ++i) {
        writeIconSnapshotToSQLDatabase(iconSnapshots[i]);
        LOG(IconDatabase, "Wrote IconRecord for IconURL %s with timeStamp of %li to the DB", urlForLogging(iconSnapshots[i].iconURL).utf8().data(), iconSnapshots[i].timestamp);
    }
    
    for (unsigned i = 0; i < pageSnapshots.size(); ++i) {
        String iconURL = pageSnapshots[i].iconURL;

        // If the icon URL is empty, this page is meant to be deleted
        // ASSERTs are sanity checks to make sure the mappings exist if they should and don't if they shouldn't
        if (pageSnapshots[i].iconURL.isEmpty())
            removePageURLFromSQLDatabase(pageSnapshots[i].pageURL);
        else {
            setIconURLForPageURLInSQLDatabase(pageSnapshots[i].iconURL, pageSnapshots[i].pageURL);
        }
        LOG(IconDatabase, "Committed IconURL for PageURL %s to database", urlForLogging(pageSnapshots[i].pageURL).utf8().data());
    }

    syncTransaction.commit();
    
    // Check to make sure there are no dangling PageURLs - If there are, we want to output one log message but not spam the console potentially every few seconds
    checkForDanglingPageURLs(false);

    LOG(IconDatabase, "Updating the database took %.4f seconds", currentTime() - timeStamp);

    return didAnyWork;
}

void IconDatabase::pruneUnretainedIcons()
{
    ASSERT_ICON_SYNC_THREAD();

    if (!isOpen())
        return;        
    
    // This method should only be called once per run
    ASSERT(!m_initialPruningComplete);

    // This method relies on having read in all page URLs from the database earlier.
    ASSERT(m_iconURLImportComplete);

    // Get the known PageURLs from the db, and record the ID of any that are not in the retain count set.
    Vector<int64_t> pageIDsToDelete; 

    SQLStatement pageSQL(m_syncDB, "SELECT rowid, url FROM PageURL;");
    pageSQL.prepare();
    
    int result;
    while ((result = pageSQL.step()) == SQLResultRow) {
        MutexLocker locker(m_urlAndIconLock);
        if (!m_pageURLToRecordMap.contains(pageSQL.getColumnText16(1)))
            pageIDsToDelete.append(pageSQL.getColumnInt64(0));
    }
    
    if (result != SQLResultDone)
        LOG_ERROR("Error reading PageURL table from on-disk DB");
    pageSQL.finalize();
    
    // Delete page URLs that were in the table, but not in our retain count set.
    size_t numToDelete = pageIDsToDelete.size();
    if (numToDelete) {
        SQLTransaction pruningTransaction(m_syncDB);
        pruningTransaction.begin();
        
        SQLStatement pageDeleteSQL(m_syncDB, "DELETE FROM PageURL WHERE rowid = (?);");
        pageDeleteSQL.prepare();
        for (size_t i = 0; i < numToDelete; ++i) {
            LOG(IconDatabase, "Pruning page with rowid %lli from disk", pageIDsToDelete[i]);
            pageDeleteSQL.bindInt64(1, pageIDsToDelete[i]);
            int result = pageDeleteSQL.step();
            if (result != SQLResultDone)
                LOG_ERROR("Unabled to delete page with id %lli from disk", pageIDsToDelete[i]);
            pageDeleteSQL.reset();
            
            // If the thread was asked to terminate, we should commit what pruning we've done so far, figuring we can
            // finish the rest later (hopefully)
            if (shouldStopThreadActivity()) {
                pruningTransaction.commit();
                return;
            }
        }
        pruningTransaction.commit();
        pageDeleteSQL.finalize();
    }
    
    // Deleting unreferenced icons from the Icon tables has to be atomic - 
    // If the user quits while these are taking place, they might have to wait.  Thankfully this will rarely be an issue
    // A user on a network home directory with a wildly inconsistent database might see quite a pause...

    SQLTransaction pruningTransaction(m_syncDB);
    pruningTransaction.begin();
    
    // Wipe Icons that aren't retained
    if (!m_syncDB.executeCommand("DELETE FROM IconData WHERE iconID NOT IN (SELECT iconID FROM PageURL);"))
        LOG_ERROR("Failed to execute SQL to prune unretained icons from the on-disk IconData table");    
    if (!m_syncDB.executeCommand("DELETE FROM IconInfo WHERE iconID NOT IN (SELECT iconID FROM PageURL);"))
        LOG_ERROR("Failed to execute SQL to prune unretained icons from the on-disk IconInfo table");    
    
    pruningTransaction.commit();
        
    checkForDanglingPageURLs(true);

    m_initialPruningComplete = true;
}

void IconDatabase::checkForDanglingPageURLs(bool pruneIfFound)
{
    ASSERT_ICON_SYNC_THREAD();
    
    // We don't want to keep performing this check and reporting this error if it has already found danglers so we keep track
    static bool danglersFound = false;
    
    // However, if the caller wants us to prune the danglers, we will reset this flag and prune every time
    if (pruneIfFound)
        danglersFound = false;
        
    if (!danglersFound && SQLStatement(m_syncDB, "SELECT url FROM PageURL WHERE PageURL.iconID NOT IN (SELECT iconID FROM IconInfo) LIMIT 1;").returnsAtLeastOneResult()) {
        danglersFound = true;
        LOG_ERROR("Dangling PageURL entries found");
        if (pruneIfFound && !m_syncDB.executeCommand("DELETE FROM PageURL WHERE iconID NOT IN (SELECT iconID FROM IconInfo);"))
            LOG_ERROR("Unable to prune dangling PageURLs");
    }
}

void IconDatabase::removeAllIconsOnThread()
{
    ASSERT_ICON_SYNC_THREAD();

    LOG(IconDatabase, "Removing all icons on the sync thread");

    //Delete all the prepared statements so they can start over
    deleteAllPreparedStatements();    
        
    {
        MutexLocker locker(m_urlAndIconLock);
        
        // Clear all in-memory records of pages and icons
        m_iconURLToRecordMap.clear();
        // Deleting the PageURLRecords derefs the IconRecords automagically
        deleteAllValues(m_pageURLToRecordMap); 
        m_pageURLToRecordMap.clear();
        
        // Clear all in-memory records of things that need to be synced out to disk
        {
            MutexLocker locker(m_pendingSyncLock);
            m_pageURLsPendingSync.clear();
            m_iconsPendingSync.clear();
        }
        
        // Clear all in-memory records of things that need to be read in from disk
        {
            MutexLocker locker(m_pendingReadingLock);
            m_pageURLsPendingImport.clear();
            m_pageURLsInterestedInIcons.clear();
            m_iconsPendingReading.clear();
            m_loadersPendingDecision.clear();
        }
    }
    
    // To reset the on-disk database, we'll wipe all its tables then vacuum it
    // This is easier and safer than closing it, deleting the file, and recreating from scratch
    m_syncDB.clearAllTables();
    m_syncDB.runVacuumCommand();
    createDatabaseTables(m_syncDB);
    
    LOG(IconDatabase, "Dispatching notification that we removed all icons");
    m_client->dispatchDidRemoveAllIcons();    
}

void IconDatabase::deleteAllPreparedStatements()
{        
    ASSERT_ICON_SYNC_THREAD();
    
    m_setIconIDForPageURLStatement.set(0);
    m_removePageURLStatement.set(0);
    m_getIconIDForIconURLStatement.set(0);
    m_getImageDataForIconURLStatement.set(0);
    m_addIconToIconInfoStatement.set(0);
    m_addIconToIconDataStatement.set(0);
    m_getImageDataStatement.set(0);
    m_deletePageURLsForIconURLStatement.set(0);
    m_deleteIconFromIconInfoStatement.set(0);
    m_deleteIconFromIconDataStatement.set(0);
    m_updateIconInfoStatement.set(0);
    m_updateIconDataStatement.set(0);
    m_setIconInfoStatement.set(0);
    m_setIconDataStatement.set(0);
}

void* IconDatabase::cleanupSyncThread()
{
    ASSERT_ICON_SYNC_THREAD();
    
#ifndef NDEBUG
    double timeStamp = currentTime();
#endif 

    // If the removeIcons flag is set, remove all icons from the db.
    // Otherwise, do a final write an sync out to disk
    if (m_removeIconsRequested)
        removeAllIconsOnThread();
    else {
        LOG(IconDatabase, "(THREAD) Doing final writeout and closure of sync thread");
        writeToDatabase();
    }
    
    // Close the database
    MutexLocker locker(m_syncLock);
    
    m_completeDatabasePath = String();
    deleteAllPreparedStatements();    
    m_syncDB.close();
    
#ifndef NDEBUG
    LOG(IconDatabase, "(THREAD) Final closure took %.4f seconds", currentTime() - timeStamp);
#endif
    
    return 0;
}

bool IconDatabase::imported()
{
    ASSERT_ICON_SYNC_THREAD();
    
    if (m_isImportedSet)
        return m_imported;
        
    SQLStatement query(m_syncDB, "SELECT IconDatabaseInfo.value FROM IconDatabaseInfo WHERE IconDatabaseInfo.key = \"ImportedSafari2Icons\";");
    if (query.prepare() != SQLResultOk) {
        LOG_ERROR("Unable to prepare imported statement");
        return false;
    }
    
    int result = query.step();
    if (result == SQLResultRow)
        result = query.getColumnInt(0);
    else {
        if (result != SQLResultDone)
            LOG_ERROR("imported statement failed");
        result = 0;
    }
    
    m_isImportedSet = true;
    return m_imported = result;
}

void IconDatabase::setImported(bool import)
{
    ASSERT_ICON_SYNC_THREAD();

    m_imported = import;
    m_isImportedSet = true;
    
    String queryString = import ?
        "INSERT INTO IconDatabaseInfo (key, value) VALUES (\"ImportedSafari2Icons\", 1);" :
        "INSERT INTO IconDatabaseInfo (key, value) VALUES (\"ImportedSafari2Icons\", 0);";
        
    SQLStatement query(m_syncDB, queryString);
    
    if (query.prepare() != SQLResultOk) {
        LOG_ERROR("Unable to prepare set imported statement");
        return;
    }    
    
    if (query.step() != SQLResultDone)
        LOG_ERROR("set imported statement failed");
}

// readySQLStatement() handles two things
// 1 - If the SQLDatabase& argument is different, the statement must be destroyed and remade.  This happens when the user
//     switches to and from private browsing
// 2 - Lazy construction of the Statement in the first place, in case we've never made this query before
inline void readySQLStatement(OwnPtr<SQLStatement>& statement, SQLDatabase& db, const String& str)
{
    if (statement && (statement->database() != &db || statement->isExpired())) {
        if (statement->isExpired())
            LOG(IconDatabase, "SQLStatement associated with %s is expired", str.utf8().data());
        statement.set(0);
    }
    if (!statement) {
        statement.set(new SQLStatement(db, str));
        int result;
        result = statement->prepare();
        ASSERT(result == SQLResultOk);
    }
}

void IconDatabase::setIconURLForPageURLInSQLDatabase(const String& iconURL, const String& pageURL)
{
    ASSERT_ICON_SYNC_THREAD();
    
    int64_t iconID = getIconIDForIconURLFromSQLDatabase(iconURL);

    if (!iconID)
        iconID = addIconURLToSQLDatabase(iconURL);
    
    if (!iconID) {
        LOG_ERROR("Failed to establish an ID for iconURL %s", urlForLogging(iconURL).utf8().data());
        ASSERT(false);
        return;
    }
    
    setIconIDForPageURLInSQLDatabase(iconID, pageURL);
}

void IconDatabase::setIconIDForPageURLInSQLDatabase(int64_t iconID, const String& pageURL)
{
    ASSERT_ICON_SYNC_THREAD();
    
    readySQLStatement(m_setIconIDForPageURLStatement, m_syncDB, "INSERT INTO PageURL (url, iconID) VALUES ((?), ?);");
    m_setIconIDForPageURLStatement->bindText16(1, pageURL, false);
    m_setIconIDForPageURLStatement->bindInt64(2, iconID);

    int result = m_setIconIDForPageURLStatement->step();
    if (result != SQLResultDone) {
        ASSERT(false);
        LOG_ERROR("setIconIDForPageURLQuery failed for url %s", urlForLogging(pageURL).utf8().data());
    }

    m_setIconIDForPageURLStatement->reset();
}

void IconDatabase::removePageURLFromSQLDatabase(const String& pageURL)
{
    ASSERT_ICON_SYNC_THREAD();
    
    readySQLStatement(m_removePageURLStatement, m_syncDB, "DELETE FROM PageURL WHERE url = (?);");
    m_removePageURLStatement->bindText16(1, pageURL, false);

    if (m_removePageURLStatement->step() != SQLResultDone)
        LOG_ERROR("removePageURLFromSQLDatabase failed for url %s", urlForLogging(pageURL).utf8().data());
    
    m_removePageURLStatement->reset();
}


int64_t IconDatabase::getIconIDForIconURLFromSQLDatabase(const String& iconURL)
{
    ASSERT_ICON_SYNC_THREAD();
    
    readySQLStatement(m_getIconIDForIconURLStatement, m_syncDB, "SELECT IconInfo.iconID FROM IconInfo WHERE IconInfo.url = (?);");
    m_getIconIDForIconURLStatement->bindText16(1, iconURL, false);
    
    int64_t result = m_getIconIDForIconURLStatement->step();
    if (result == SQLResultRow)
        result = m_getIconIDForIconURLStatement->getColumnInt64(0);
    else {
        if (result != SQLResultDone)
            LOG_ERROR("getIconIDForIconURLFromSQLDatabase failed for url %s", urlForLogging(iconURL).utf8().data());
        result = 0;
    }

    m_getIconIDForIconURLStatement->reset();
    return result;
}

int64_t IconDatabase::addIconURLToSQLDatabase(const String& iconURL)
{
    ASSERT_ICON_SYNC_THREAD();
    
    // There would be a transaction here to make sure these two inserts are atomic
    // In practice the only caller of this method is always wrapped in a transaction itself so placing another
    // here is unnecessary
    
    readySQLStatement(m_addIconToIconInfoStatement, m_syncDB, "INSERT INTO IconInfo (url, stamp) VALUES (?, 0);");
    m_addIconToIconInfoStatement->bindText16(1, iconURL);
    
    int result = m_addIconToIconInfoStatement->step();
    m_addIconToIconInfoStatement->reset();
    if (result != SQLResultDone) {
        LOG_ERROR("addIconURLToSQLDatabase failed to insert %s into IconInfo", urlForLogging(iconURL).utf8().data());
        return 0;
    }
    int64_t iconID = m_syncDB.lastInsertRowID();
    
    readySQLStatement(m_addIconToIconDataStatement, m_syncDB, "INSERT INTO IconData (iconID, data) VALUES (?, ?);");
    m_addIconToIconDataStatement->bindInt64(1, iconID);
    
    result = m_addIconToIconDataStatement->step();
    m_addIconToIconDataStatement->reset();
    if (result != SQLResultDone) {
        LOG_ERROR("addIconURLToSQLDatabase failed to insert %s into IconData", urlForLogging(iconURL).utf8().data());
        return 0;
    }
    
    return iconID;
}

PassRefPtr<SharedBuffer> IconDatabase::getImageDataForIconURLFromSQLDatabase(const String& iconURL)
{
    ASSERT_ICON_SYNC_THREAD();
    
    RefPtr<SharedBuffer> imageData;
    
    readySQLStatement(m_getImageDataForIconURLStatement, m_syncDB, "SELECT IconData.data FROM IconData WHERE IconData.iconID IN (SELECT iconID FROM IconInfo WHERE IconInfo.url = (?));");
    m_getImageDataForIconURLStatement->bindText16(1, iconURL, false);
    
    int result = m_getImageDataForIconURLStatement->step();
    if (result == SQLResultRow) {
        Vector<char> data;
        m_getImageDataForIconURLStatement->getColumnBlobAsVector(0, data);
        imageData = new SharedBuffer;
        imageData->append(data.data(), data.size());
    } else if (result != SQLResultDone)
        LOG_ERROR("getImageDataForIconURLFromSQLDatabase failed for url %s", urlForLogging(iconURL).utf8().data());

    m_getImageDataForIconURLStatement->reset();
    
    return imageData.release();
}

void IconDatabase::removeIconFromSQLDatabase(const String& iconURL)
{
    ASSERT_ICON_SYNC_THREAD();
    
    if (iconURL.isEmpty())
        return;

    // There would be a transaction here to make sure these removals are atomic
    // In practice the only caller of this method is always wrapped in a transaction itself so placing another here is unnecessary
    
    int64_t iconID = getIconIDForIconURLFromSQLDatabase(iconURL);
    ASSERT(iconID);
    if (!iconID) {
        LOG_ERROR("Unable to get iconID for icon URL %s to delete it from the database", urlForLogging(iconURL).utf8().data());
        return;
    }
    
    readySQLStatement(m_deletePageURLsForIconURLStatement, m_syncDB, "DELETE FROM PageURL WHERE PageURL.iconID = (?);");
    m_deletePageURLsForIconURLStatement->bindInt64(1, iconID);
    
    if (m_deletePageURLsForIconURLStatement->step() != SQLResultDone)
        LOG_ERROR("m_deletePageURLsForIconURLStatement failed for url %s", urlForLogging(iconURL).utf8().data());
    
    readySQLStatement(m_deleteIconFromIconInfoStatement, m_syncDB, "DELETE FROM IconInfo WHERE IconInfo.iconID = (?);");
    m_deleteIconFromIconInfoStatement->bindInt64(1, iconID);
    
    if (m_deleteIconFromIconInfoStatement->step() != SQLResultDone)
        LOG_ERROR("m_deleteIconFromIconInfoStatement failed for url %s", urlForLogging(iconURL).utf8().data());
        
    readySQLStatement(m_deleteIconFromIconDataStatement, m_syncDB, "DELETE FROM IconData WHERE IconData.iconID = (?);");
    m_deleteIconFromIconDataStatement->bindInt64(1, iconID);
    
    if (m_deleteIconFromIconDataStatement->step() != SQLResultDone)
        LOG_ERROR("m_deleteIconFromIconDataStatement failed for url %s", urlForLogging(iconURL).utf8().data());
        
    m_deletePageURLsForIconURLStatement->reset();
    m_deleteIconFromIconInfoStatement->reset();
    m_deleteIconFromIconDataStatement->reset();
}

void IconDatabase::writeIconSnapshotToSQLDatabase(const IconSnapshot& snapshot)
{
    ASSERT_ICON_SYNC_THREAD();
    
    if (snapshot.iconURL.isEmpty())
        return;
        
    // A nulled out timestamp and data means this icon is destined to be deleted - do that instead of writing it out
    if (!snapshot.timestamp && !snapshot.data) {
        LOG(IconDatabase, "Removing %s from on-disk database", urlForLogging(snapshot.iconURL).utf8().data());
        removeIconFromSQLDatabase(snapshot.iconURL);
        return;
    }

    // There would be a transaction here to make sure these removals are atomic
    // In practice the only caller of this method is always wrapped in a transaction itself so placing another here is unnecessary
        
    // Get the iconID for this url
    int64_t iconID = getIconIDForIconURLFromSQLDatabase(snapshot.iconURL);
    
    // If there is already an iconID in place, update the database.  
    // Otherwise, insert new records
    if (iconID) {    
        readySQLStatement(m_updateIconInfoStatement, m_syncDB, "UPDATE IconInfo SET stamp = ?, url = ? WHERE iconID = ?;");
        m_updateIconInfoStatement->bindInt64(1, snapshot.timestamp);
        m_updateIconInfoStatement->bindText16(2, snapshot.iconURL);
        m_updateIconInfoStatement->bindInt64(3, iconID);

        if (m_updateIconInfoStatement->step() != SQLResultDone)
            LOG_ERROR("Failed to update icon info for url %s", urlForLogging(snapshot.iconURL).utf8().data());
        
        m_updateIconInfoStatement->reset();
        
        readySQLStatement(m_updateIconDataStatement, m_syncDB, "UPDATE IconData SET data = ? WHERE iconID = ?;");
        m_updateIconDataStatement->bindInt64(2, iconID);
                
        // If we *have* image data, bind it to this statement - Otherwise the DB will get "null" for the blob data, 
        // signifying that this icon doesn't have any data    
        if (snapshot.data && snapshot.data->size())
            m_updateIconDataStatement->bindBlob(1, snapshot.data->data(), snapshot.data->size());
        
        if (m_updateIconDataStatement->step() != SQLResultDone)
            LOG_ERROR("Failed to update icon data for url %s", urlForLogging(snapshot.iconURL).utf8().data());

        m_updateIconDataStatement->reset();
    } else {    
        readySQLStatement(m_setIconInfoStatement, m_syncDB, "INSERT INTO IconInfo (url,stamp) VALUES (?, ?);");
        m_setIconInfoStatement->bindText16(1, snapshot.iconURL);
        m_setIconInfoStatement->bindInt64(2, snapshot.timestamp);

        if (m_setIconInfoStatement->step() != SQLResultDone)
            LOG_ERROR("Failed to set icon info for url %s", urlForLogging(snapshot.iconURL).utf8().data());
        
        m_setIconInfoStatement->reset();
        
        int64_t iconID = m_syncDB.lastInsertRowID();

        readySQLStatement(m_setIconDataStatement, m_syncDB, "INSERT INTO IconData (iconID, data) VALUES (?, ?);");
        m_setIconDataStatement->bindInt64(1, iconID);

        // If we *have* image data, bind it to this statement - Otherwise the DB will get "null" for the blob data, 
        // signifying that this icon doesn't have any data    
        if (snapshot.data && snapshot.data->size())
            m_setIconDataStatement->bindBlob(2, snapshot.data->data(), snapshot.data->size());
        
        if (m_setIconDataStatement->step() != SQLResultDone)
            LOG_ERROR("Failed to set icon data for url %s", urlForLogging(snapshot.iconURL).utf8().data());

        m_setIconDataStatement->reset();
    }
}

} // namespace WebCore
