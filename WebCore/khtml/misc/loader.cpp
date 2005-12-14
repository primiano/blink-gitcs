/*
    This file is part of the KDE libraries

    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2002 Waldo Bastian (bastian@kde.org)
    Copyright (C) 2004 Apple Computer, Inc.

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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.

    This class provides all functionality needed for loading images, style sheets and html
    pages from the web. It has a memory cache for these objects.
*/

#undef CACHE_DEBUG
//#define CACHE_DEBUG
#include "config.h"
#include <assert.h>

#include "loader.h"

// up to which size is a picture for sure cacheable
#define MAXCACHEABLE 40*1024
// default cache size
#define DEFCACHESIZE 4096*1024

#include <qasyncio.h>
#include <qasyncimageio.h>
#include <qpainter.h>
#include <qbitmap.h>
#include <qmovie.h>

#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kimageio.h>
#include <kcharsets.h>
#include <kiconloader.h>
#include <scheduler.h>
#include <kdebug.h>
#include "khtml_factory.h"
#include "khtml_part.h"
#include "decoder.h"

#include "html/html_documentimpl.h"
#include "css/css_stylesheetimpl.h"

#ifndef KHTML_NO_XBL
#include "xbl/xbl_docimpl.h"
#endif

#include <kxmlcore/Assertions.h>
#include "KWQLoader.h"

using namespace khtml;
using namespace DOM;

static bool cacheDisabled;

// Call this "walker" instead of iterator so people won't expect Qt or STL-style iterator interface.
// Just keep calling next() on this. It's safe from deletions of the current item
class CachedObjectClientWalker {
public:
    CachedObjectClientWalker(const QPtrDict<CachedObjectClient> &clients) : _current(0), _iterator(clients) { }
    CachedObjectClient *next();
private:
    CachedObjectClient *_current;
    QPtrDictIterator<CachedObjectClient> _iterator;
};

CachedObject::~CachedObject()
{
    if(m_deleted) abort();
    Cache::removeFromLRUList(this);
    m_deleted = true;
    setResponse(0);
    setAllData(0);
}

void CachedObject::finish()
{
    if (m_size > Cache::maxCacheableObjectSize())
        m_status = Uncacheable;
    else
        m_status = Cached;
    KURL url(m_url.qstring());
    if (m_expireDateChanged && url.protocol().startsWith("http"))
    {
        m_expireDateChanged = false;
        KIO::http_update_cache(url, false, m_expireDate);
#ifdef CACHE_DEBUG
        kdDebug(6060) << " Setting expire date for image "<<m_url.qstring()<<" to " << m_expireDate << endl;
#endif
    }
#ifdef CACHE_DEBUG
    else kdDebug(6060) << " No expire date for image "<<m_url.qstring()<<endl;
#endif
}

void CachedObject::setExpireDate(time_t _expireDate, bool changeHttpCache)
{
    if ( _expireDate == m_expireDate)
        return;

    if (m_status == Uncacheable || m_status == Cached)
    {
        finish();
    }
    m_expireDate = _expireDate;
    if (changeHttpCache && m_expireDate)
       m_expireDateChanged = true;
}

bool CachedObject::isExpired() const
{
    if (!m_expireDate) return false;
    time_t now = time(0);
    return (difftime(now, m_expireDate) >= 0);
}

void CachedObject::setRequest(Request *_request)
{
    if ( _request && !m_request )
        m_status = Pending;
    m_request = _request;
    if (canDelete() && m_free)
        delete this;
    else if (allowInLRUList())
        Cache::insertInLRUList(this);
}

void CachedObject::ref(CachedObjectClient *c)
{
    m_clients.insert(c, c);
    Cache::removeFromLRUList(this);
    increaseAccessCount();
}

void CachedObject::deref(CachedObjectClient *c)
{
    m_clients.remove(c);
    if (allowInLRUList())
        Cache::insertInLRUList(this);
}

void CachedObject::setSize(int size)
{
    bool sizeChanged = Cache::adjustSize(this, size - m_size);

    // The object must now be moved to a different queue, since its size has been changed.
    if (sizeChanged && allowInLRUList())
        Cache::removeFromLRUList(this);

    m_size = size;
    
    if (sizeChanged && allowInLRUList())
        Cache::insertInLRUList(this);
}

// -------------------------------------------------------------------------------------------

CachedCSSStyleSheet::CachedCSSStyleSheet(DocLoader* dl, const DOMString &url, KIO::CacheControl _cachePolicy, time_t _expireDate, const QString& charset)
    : CachedObject(url, CSSStyleSheet, _cachePolicy, _expireDate)
{
    // It's css we want.
    setAccept( QString::fromLatin1("text/css") );
    // load the file
    Cache::loader()->load(dl, this, false);
    m_loading = true;
    if(!charset.isEmpty())
	m_codec = QTextCodec::codecForName(charset.latin1());
    else
        m_codec = QTextCodec::codecForName("iso8859-1");
}

CachedCSSStyleSheet::CachedCSSStyleSheet(const DOMString &url, const QString &stylesheet_data)
    : CachedObject(url, CSSStyleSheet, KIO::CC_Verify, 0, stylesheet_data.length())
{
    m_loading = false;
    m_status = Persistent;
    m_codec = 0;
    m_sheet = DOMString(stylesheet_data);
}


CachedCSSStyleSheet::~CachedCSSStyleSheet()
{
}

void CachedCSSStyleSheet::ref(CachedObjectClient *c)
{
    CachedObject::ref(c);

    if(!m_loading) c->setStyleSheet( m_url, m_sheet );
}

void CachedCSSStyleSheet::deref(CachedObjectClient *c)
{
    Cache::flush();
    CachedObject::deref(c);
    if ( canDelete() && m_free )
      delete this;
}

void CachedCSSStyleSheet::setCharset( const QString &chs )
{
    if (!chs.isEmpty()) {
        QTextCodec *codec = QTextCodec::codecForName(chs.latin1());
        if (codec)
            m_codec = codec;
    }
}

void CachedCSSStyleSheet::data( QBuffer &buffer, bool eof )
{
    if(!eof) return;
    buffer.close();
    setSize(buffer.buffer().size());
    QString data = m_codec->toUnicode( buffer.buffer().data(), size() );
    m_sheet = DOMString(data);
    m_loading = false;

    checkNotify();
}

void CachedCSSStyleSheet::checkNotify()
{
    if(m_loading) return;

#ifdef CACHE_DEBUG
    kdDebug( 6060 ) << "CachedCSSStyleSheet:: finishedLoading " << m_url.qstring() << endl;
#endif

    CachedObjectClientWalker w(m_clients);
    while (CachedObjectClient *c = w.next()) {
        if (m_response && !KWQIsResponseURLEqualToURL(m_response, m_url))
            c->setStyleSheet(DOMString(KWQResponseURL(m_response)), m_sheet);
        else
            c->setStyleSheet(m_url, m_sheet);
    }
}


void CachedCSSStyleSheet::error( int /*err*/, const char */*text*/ )
{
    m_loading = false;
    checkNotify();
}

// -------------------------------------------------------------------------------------------

CachedScript::CachedScript(DocLoader* dl, const DOMString &url, KIO::CacheControl _cachePolicy, time_t _expireDate, const QString& charset)
    : CachedObject(url, Script, _cachePolicy, _expireDate)
{
    // It's javascript we want.
    // But some websites think their scripts are <some wrong mimetype here>
    // and refuse to serve them if we only accept application/x-javascript.
    setAccept( QString::fromLatin1("*/*") );
    // load the file
    Cache::loader()->load(dl, this, false);
    m_loading = true;
    if(!charset.isEmpty())
        m_codec = QTextCodec::codecForName(charset.latin1());
    else
	m_codec = QTextCodec::codecForName("iso8859-1");
}

CachedScript::CachedScript(const DOMString &url, const QString &script_data)
    : CachedObject(url, Script, KIO::CC_Verify, 0, script_data.length())
{
    m_loading = false;
    m_status = Persistent;
    m_codec = 0;
    m_script = DOMString(script_data);
}

CachedScript::~CachedScript()
{
}

void CachedScript::ref(CachedObjectClient *c)
{
    CachedObject::ref(c);

    if(!m_loading) c->notifyFinished(this);
}

void CachedScript::deref(CachedObjectClient *c)
{
    Cache::flush();
    CachedObject::deref(c);
    if ( canDelete() && m_free )
      delete this;
}

void CachedScript::setCharset( const QString &chs )
{
    if (!chs.isEmpty()) {
        QTextCodec *codec = QTextCodec::codecForName(chs.latin1());
        if (codec)
            m_codec = codec;
    }
}

void CachedScript::data( QBuffer &buffer, bool eof )
{
    if(!eof) return;
    buffer.close();
    setSize(buffer.buffer().size());
    QString data = m_codec->toUnicode( buffer.buffer().data(), size() );
    m_script = DOMString(data);
    m_loading = false;
    checkNotify();
}

void CachedScript::checkNotify()
{
    if(m_loading) return;

    CachedObjectClientWalker w(m_clients);
    while (CachedObjectClient *c = w.next())
        c->notifyFinished(this);
}


void CachedScript::error( int /*err*/, const char */*text*/ )
{
    m_loading = false;
    checkNotify();
}

// ------------------------------------------------------------------------------------------


static bool crossDomain(const QString &a, const QString &b)
{
    if (a == b) return false;

    QStringList l1 = QStringList::split('.', a);
    QStringList l2 = QStringList::split('.', b);

    while(l1.count() > l2.count())
        l1.pop_front();

    while(l2.count() > l1.count())
        l2.pop_front();

    while(l2.count() >= 2)
    {
        if (l1 == l2)
           return false;

        l1.pop_front();
        l2.pop_front();
    }
    return true;
}

// -------------------------------------------------------------------------------------
void CachedImageCallback::notifyUpdate() 
{ 
    if (cachedImage) {
        cachedImage->do_notify (cachedImage->pixmap(), cachedImage->pixmap().rect()); 
        QSize s = cachedImage->pixmap_size();
        cachedImage->setSize(s.width() * s.height() * 2);

        // After receiving the image header we are guaranteed to know
        // the image size.  Although all of the data may not have arrived or
        // been decoded we can consider the image loaded for purposed of
        // layout and dispatching the image's onload handler.  Removing the request from
        // the list of background decoding requests will ensure that Loader::numRequests() 
        // does not count this background request.  Further numRequests() can
        // be correctly used by the part to determine if loading is sufficiently
        // complete to dispatch the page's onload handler.
        Request *r = cachedImage->m_request;
        DocLoader *dl = r->m_docLoader;

        khtml::Cache::loader()->removeBackgroundDecodingRequest(r);

        // Poke the part to get it to do a checkCompleted().  Only do this for
        // the first update to minimize work.  Note that we are guaranteed to have
        // read the header when we received this first update, which is triggered
        // by the first kCGImageStatusIncomplete status from CG. kCGImageStatusIncomplete
        // really means that the CG decoder is waiting for more data, but has already
        // read the header.
        if (!headerReceived) {
            emit khtml::Cache::loader()->requestDone( dl, cachedImage );
            headerReceived = true;
        }
    }
}

void CachedImageCallback::notifyFinished()
{
    if (cachedImage) {
        cachedImage->do_notify (cachedImage->pixmap(), cachedImage->pixmap().rect()); 
        cachedImage->m_loading = false;
        cachedImage->checkNotify();
        QSize s = cachedImage->pixmap_size();
        cachedImage->setSize(s.width() * s.height() * 2);
	
        Request *r = cachedImage->m_request;
        DocLoader *dl = r->m_docLoader;

        khtml::Cache::loader()->removeBackgroundDecodingRequest(r);

        // Poke the part to get it to do a checkCompleted().
        emit khtml::Cache::loader()->requestDone( dl, cachedImage );
        
        delete r;
    }
}

void CachedImageCallback::notifyDecodingError()
{
    if (cachedImage) {
        handleError();
    }
}

void CachedImageCallback::handleError()
{
    if (cachedImage) {
        cachedImage->errorOccured = true;
        QPixmap ep = cachedImage->pixmap();
        cachedImage->do_notify (ep, ep.rect());
        Cache::removeCacheEntry (cachedImage);

        clear();
    }
}

void CachedImageCallback::clear() 
{
    if (cachedImage && cachedImage->m_request) {
        Request *r = cachedImage->m_request;
        DocLoader *dl = r->m_docLoader;

        khtml::Cache::loader()->removeBackgroundDecodingRequest(r);

        // Poke the part to get it to do a checkCompleted().
        emit khtml::Cache::loader()->requestFailed( dl, cachedImage );

        delete r;
    }
    cachedImage = 0;
}

CachedImage::CachedImage(DocLoader* dl, const DOMString &url, KIO::CacheControl _cachePolicy, time_t _expireDate)
    : QObject(), CachedObject(url, Image, _cachePolicy, _expireDate)
    , m_dataSize(0)
{

    m = 0;
    p = 0;
    pixPart = 0;
    bg = 0;
    isFullyTransparent = false;
    errorOccured = false;
    monochrome = false;
    formatType = 0;
    m_status = Unknown;
    imgSource = 0;
    m_loading = true;
    m_showAnimations = dl->showAnimations();
    if (QPixmap::shouldUseThreadedDecoding())
        m_decoderCallback = new CachedImageCallback(this);
    else
        m_decoderCallback = 0;
}

CachedImage::~CachedImage()
{
    clear();
}

void CachedImage::ref( CachedObjectClient *c )
{
#ifdef CACHE_DEBUG
    kdDebug( 6060 ) << this << " CachedImage::ref(" << c << ") " << endl;
#endif

    CachedObject::ref(c);

    if( m ) {
        m->unpause();
        if( m->finished() || m_clients.count() == 1 )
            m->restart();
    }

    // for mouseovers, dynamic changes
    if (!valid_rect().isNull())
        c->setPixmap( pixmap(), valid_rect(), this);

    if(!m_loading) c->notifyFinished(this);
}

void CachedImage::deref( CachedObjectClient *c )
{
#ifdef CACHE_DEBUG
    kdDebug( 6060 ) << this << " CachedImage::deref(" << c << ") " << endl;
#endif
    Cache::flush();
    CachedObject::deref(c);
    if(m && m_clients.isEmpty() && m->running())
        m->pause();
    if ( canDelete() && m_free )
        delete this;
}

#define BGMINWIDTH      32
#define BGMINHEIGHT     32

const QPixmap &CachedImage::tiled_pixmap(const QColor& newc)
{
    return pixmap();
}

const QPixmap &CachedImage::pixmap( ) const
{
    if(errorOccured)
        return *Cache::brokenPixmap;

    if (p)
        return *p;

    return *Cache::nullPixmap;
}


QSize CachedImage::pixmap_size() const
{
    return (m ? m->framePixmap().size() : ( p ? p->size() : QSize()));
}


QRect CachedImage::valid_rect() const
{
    return m ? m->getValidRect() : ( p ? p->rect() : QRect());
}


void CachedImage::do_notify(const QPixmap& p, const QRect& r)
{
    CachedObjectClientWalker w(m_clients);
    while (CachedObjectClient *c = w.next())
        c->setPixmap(p, r, this);
}


void CachedImage::setShowAnimations( KHTMLSettings::KAnimationAdvice showAnimations )
{
    m_showAnimations = showAnimations;
}


void CachedImage::clear()
{
    delete m;   m = 0;
    delete p;   p = 0;
    delete bg;  bg = 0;
    delete pixPart; pixPart = 0;

    formatType = 0;

    setSize(0);

    // No need to delete imageSource - QMovie does it for us
    imgSource = 0;

    if (m_decoderCallback) {
        m_decoderCallback->clear();
        m_decoderCallback->deref();
        m_decoderCallback = 0;
    }
}

void CachedImage::data ( QBuffer &_buffer, bool eof )
{
#ifdef CACHE_DEBUG
    kdDebug( 6060 ) << this << "in CachedImage::data(buffersize " << _buffer.buffer().size() <<", eof=" << eof << endl;
#endif

    bool canDraw = false;
    
    m_dataSize = _buffer.size();
        
    // If we're at eof and don't have a pixmap yet, the data
    // must have arrived in one chunk.  This avoids the attempt
    // to perform incremental decoding.
    if (eof && !p) {
        p = new QPixmap(_buffer.buffer(), KWQResponseMIMEType(m_response));
        if (m_decoderCallback)
            m_decoderCallback->notifyFinished();
        canDraw = true;
    } else {
        // Always attempt to load the image incrementally.
        if (!p)
            p = new QPixmap(KWQResponseMIMEType(m_response));
        canDraw = p->receivedData(_buffer.buffer(), eof, m_decoderCallback);
    }
    
    // If we have a decoder, we'll be notified when decoding has completed.
    if (!m_decoderCallback) {
        if (canDraw || eof) {
            if (p->isNull()) {
                errorOccured = true;
                QPixmap ep = pixmap();
                do_notify (ep, ep.rect());
                Cache::removeCacheEntry (this);
            }
            else
                do_notify(*p, p->rect());

            QSize s = pixmap_size();
            setSize(s.width() * s.height() * 2);
        }
        if (eof) {
            m_loading = false;
            checkNotify();
        }
    }
}

void CachedImage::error( int /*err*/, const char */*text*/ )
{
#ifdef CACHE_DEBUG
    kdDebug(6060) << "CahcedImage::error" << endl;
#endif

    clear();
    errorOccured = true;
    do_notify(pixmap(), QRect(0, 0, 16, 16));
    m_loading = false;
    checkNotify();
}

void CachedImage::checkNotify()
{
    if(m_loading) return;

    CachedObjectClientWalker w(m_clients);
    while (CachedObjectClient *c = w.next()) {
        c->notifyFinished(this);
    }
}

// -------------------------------------------------------------------------------------------

#ifdef KHTML_XSLT

CachedXSLStyleSheet::CachedXSLStyleSheet(DocLoader* dl, const DOMString &url, KIO::CacheControl _cachePolicy, time_t _expireDate)
: CachedObject(url, XSLStyleSheet, _cachePolicy, _expireDate)
{
    // It's XML we want.
    setAccept(QString::fromLatin1("text/xml, application/xml, application/xhtml+xml, text/xsl, application/rss+xml, application/atom+xml"));
    
    // load the file
    Cache::loader()->load(dl, this, false);
    m_loading = true;
    m_decoder = new Decoder;
}

void CachedXSLStyleSheet::ref(CachedObjectClient *c)
{
    CachedObject::ref(c);
    
    if (!m_loading)
        c->setStyleSheet(m_url, m_sheet);
}

void CachedXSLStyleSheet::deref(CachedObjectClient *c)
{
    Cache::flush();
    CachedObject::deref(c);
    if (canDelete() && m_free)
        delete this;
}

void CachedXSLStyleSheet::setCharset( const QString &chs )
{
    if (!chs.isEmpty())
        m_decoder->setEncoding(chs.latin1(), Decoder::EncodingFromHTTPHeader);
}

void CachedXSLStyleSheet::data(QBuffer &buffer, bool eof)
{
    if(!eof) return;
    buffer.close();
    setSize(buffer.buffer().size());
    QString data = m_decoder->decode(buffer.buffer().data(), size());
    m_sheet = DOMString(data);
    m_loading = false;
    
    checkNotify();
}

void CachedXSLStyleSheet::checkNotify()
{
    if (m_loading)
        return;
    
#ifdef CACHE_DEBUG
    kdDebug( 6060 ) << "CachedCSSStyleSheet:: finishedLoading " << m_url.qstring() << endl;
#endif
    
    CachedObjectClientWalker w(m_clients);
    while (CachedObjectClient *c = w.next())
        c->setStyleSheet(m_url, m_sheet);
}


void CachedXSLStyleSheet::error( int /*err*/, const char */*text*/ )
{
    m_loading = false;
    checkNotify();
}

#endif

#ifndef KHTML_NO_XBL
CachedXBLDocument::CachedXBLDocument(DocLoader* dl, const DOMString &url, KIO::CacheControl _cachePolicy, time_t _expireDate)
: CachedObject(url, XBL, _cachePolicy, _expireDate), m_document(0)
{
    // It's XML we want.
    setAccept( QString::fromLatin1("text/xml, application/xml, application/xhtml+xml, text/xsl, application/rss+xml, application/atom+xml") );
    
    // Load the file
    Cache::loader()->load(dl, this, false);
    m_loading = true;
    m_decoder = new Decoder;
}

CachedXBLDocument::~CachedXBLDocument()
{
    if (m_document)
        m_document->deref();
}

void CachedXBLDocument::ref(CachedObjectClient *c)
{
    CachedObject::ref(c);
    if (!m_loading)
        c->setXBLDocument(m_url, m_document);
}

void CachedXBLDocument::deref(CachedObjectClient *c)
{
    Cache::flush();
    CachedObject::deref(c);
    if (canDelete() && m_free)
        delete this;
}

void CachedXBLDocument::setCharset( const QString &chs )
{
    if (!chs.isEmpty())
        m_decoder->setEncoding(chs.latin1(), Decoder::EncodingFromHTTPHeader);
}

void CachedXBLDocument::data( QBuffer &buffer, bool eof )
{
    if (!eof) return;
    
    assert(!m_document);
    
    m_document =  new XBL::XBLDocumentImpl();
    m_document->ref();
    m_document->open();
    
    QString data = m_decoder->decode(buffer.buffer().data(), buffer.buffer().size());
    m_document->write(data);
    setSize(buffer.buffer().size());
    buffer.close();
    
    m_document->finishParsing();
    m_document->close();
    m_loading = false;
    checkNotify();
}

void CachedXBLDocument::checkNotify()
{
    if(m_loading) return;
    
#ifdef CACHE_DEBUG
    kdDebug( 6060 ) << "CachedXBLDocument:: finishedLoading " << m_url.qstring() << endl;
#endif
    
    CachedObjectClientWalker w(m_clients);
    while (CachedObjectClient *c = w.next())
        c->setXBLDocument(m_url, m_document);
}


void CachedXBLDocument::error( int /*err*/, const char */*text*/ )
{
    m_loading = false;
    checkNotify();
}
#endif

// ------------------------------------------------------------------------------------------

Request::Request(DocLoader* dl, CachedObject *_object, bool _incremental)
{
    object = _object;
    object->setRequest(this);
    incremental = _incremental;
    m_docLoader = dl;
    multipart = false;
}

Request::~Request()
{
    object->setRequest(0);
}

// ------------------------------------------------------------------------------------------

DocLoader::DocLoader(KHTMLPart* part, DocumentImpl* doc)
{
    m_cachePolicy = KIO::CC_Verify;
    m_expireDate = 0;
    m_bautoloadImages = true;
    m_showAnimations = KHTMLSettings::KAnimationEnabled;
    m_part = part;
    m_doc = doc;
    m_loadInProgress = false;

    Cache::init();
    Cache::docloader->append( this );
}

DocLoader::~DocLoader()
{
    Cache::docloader->remove( this );
}

void DocLoader::setExpireDate(time_t _expireDate)
{
    m_expireDate = _expireDate;
}

bool DocLoader::needReload(const KURL &fullURL)
{
    bool reload = false;
    if (m_cachePolicy == KIO::CC_Verify)
    {
       if (!m_reloadedURLs.contains(fullURL.url()))
       {
          CachedObject *existing = Cache::cache->find(fullURL.url());
          if (existing && existing->isExpired())
          {
             Cache::removeCacheEntry(existing);
             m_reloadedURLs.append(fullURL.url());
             reload = true;
          }
       }
    }
    else if ((m_cachePolicy == KIO::CC_Reload) || (m_cachePolicy == KIO::CC_Refresh))
    {
       if (!m_reloadedURLs.contains(fullURL.url()))
       {
          CachedObject *existing = Cache::cache->find(fullURL.url());
          if (existing)
          {
             Cache::removeCacheEntry(existing);
          }
          m_reloadedURLs.append(fullURL.url());
          reload = true;
       }
    }
    return reload;
}

CachedImage *DocLoader::requestImage( const DOM::DOMString &url)
{
    KURL fullURL = m_doc->completeURL( url.qstring() );
    if ( m_part && m_part->onlyLocalReferences() && fullURL.protocol() != "file") return 0;

    if (KWQCheckIfReloading(this)) {
        setCachePolicy(KIO::CC_Reload);
    }

    bool reload = needReload(fullURL);

    CachedImage *cachedObject = Cache::requestImage(this, fullURL, reload, m_expireDate);
    KWQCheckCacheObjectStatus(this, cachedObject);
    return cachedObject;
}

CachedCSSStyleSheet *DocLoader::requestStyleSheet( const DOM::DOMString &url, const QString& charset)
{
    KURL fullURL = m_doc->completeURL( url.qstring() );
    if ( m_part && m_part->onlyLocalReferences() && fullURL.protocol() != "file") return 0;

    if (KWQCheckIfReloading(this)) {
        setCachePolicy(KIO::CC_Reload);
    }

    bool reload = needReload(fullURL);

    CachedCSSStyleSheet *cachedObject = Cache::requestStyleSheet(this, url, reload, m_expireDate, charset);
    KWQCheckCacheObjectStatus(this, cachedObject);
    return cachedObject;
}

CachedScript *DocLoader::requestScript( const DOM::DOMString &url, const QString& charset)
{
    KURL fullURL = m_doc->completeURL( url.qstring() );
    if ( m_part && m_part->onlyLocalReferences() && fullURL.protocol() != "file") return 0;

    if (KWQCheckIfReloading(this)) {
        setCachePolicy(KIO::CC_Reload);
    }

    bool reload = needReload(fullURL);

    CachedScript *cachedObject = Cache::requestScript(this, url, reload, m_expireDate, charset);
    KWQCheckCacheObjectStatus(this, cachedObject);
    return cachedObject;
}

#ifdef KHTML_XSLT
CachedXSLStyleSheet* DocLoader::requestXSLStyleSheet(const DOM::DOMString &url)
{
    KURL fullURL = m_doc->completeURL(url.qstring());
    
    if (m_part && m_part->onlyLocalReferences() && fullURL.protocol() != "file") return 0;
    
    if (KWQCheckIfReloading(this))
        setCachePolicy(KIO::CC_Reload);
    
    bool reload = needReload(fullURL);
    
    CachedXSLStyleSheet *cachedObject = Cache::requestXSLStyleSheet(this, url, reload, m_expireDate);
    KWQCheckCacheObjectStatus(this, cachedObject);
    return cachedObject;
}
#endif

#ifndef KHTML_NO_XBL
CachedXBLDocument* DocLoader::requestXBLDocument(const DOM::DOMString &url)
{
    KURL fullURL = m_doc->completeURL(url.qstring());
    
    // FIXME: Is this right for XBL?
    if (m_part && m_part->onlyLocalReferences() && fullURL.protocol() != "file") return 0;
    
    if (KWQCheckIfReloading(this)) {
        setCachePolicy(KIO::CC_Reload);
    }
    
    bool reload = needReload(fullURL);
    
    CachedXBLDocument *cachedObject = Cache::requestXBLDocument(this, url, reload, m_expireDate);
    KWQCheckCacheObjectStatus(this, cachedObject);
    return cachedObject;
}
#endif

void DocLoader::setAutoloadImages( bool enable )
{
    if ( enable == m_bautoloadImages )
        return;

    m_bautoloadImages = enable;

    if ( !m_bautoloadImages ) return;

    for ( const CachedObject* co=m_docObjects.first(); co; co=m_docObjects.next() )
        if ( co->type() == CachedObject::Image )
        {
            CachedImage *img = const_cast<CachedImage*>( static_cast<const CachedImage *>( co ) );

            CachedObject::Status status = img->status();
            if ( status != CachedObject::Unknown )
                continue;

            Cache::loader()->load(this, img, true);
        }
}

void DocLoader::setCachePolicy( KIO::CacheControl cachePolicy )
{
    m_cachePolicy = cachePolicy;
}

void DocLoader::setShowAnimations( KHTMLSettings::KAnimationAdvice showAnimations )
{
    if ( showAnimations == m_showAnimations ) return;
    m_showAnimations = showAnimations;

    const CachedObject* co;
    for ( co=m_docObjects.first(); co; co=m_docObjects.next() )
        if ( co->type() == CachedObject::Image )
        {
            CachedImage *img = const_cast<CachedImage*>( static_cast<const CachedImage *>( co ) );

            img->setShowAnimations( showAnimations );
        }
}

void DocLoader::removeCachedObject( CachedObject* o ) const
{
    m_docObjects.removeRef( o );
}

void DocLoader::setLoadInProgress(bool load)
{
    m_loadInProgress = load;
}

// ------------------------------------------------------------------------------------------

Loader::Loader() : QObject()
{
    m_requestsPending.setAutoDelete( true );
    m_requestsLoading.setAutoDelete( true );
    kwq = new KWQLoader(this);
}

Loader::~Loader()
{
    delete kwq;
}

void Loader::load(DocLoader* dl, CachedObject *object, bool incremental)
{
    Request *req = new Request(dl, object, incremental);
    m_requestsPending.append(req);

    emit requestStarted( req->m_docLoader, req->object );

    servePendingRequests();
}

void Loader::servePendingRequests()
{
  if ( m_requestsPending.count() == 0 )
      return;

  // get the first pending request
  Request *req = m_requestsPending.take(0);

#ifdef CACHE_DEBUG
  kdDebug( 6060 ) << "starting Loader url=" << req->object->url().qstring() << endl;
#endif

  KURL u(req->object->url().qstring());
  KIO::TransferJob* job = KIO::get( u, false, false /*no GUI*/, true);
  
  job->addMetaData("cache", getCacheControlString(req->object->cachePolicy()));
  if (!req->object->accept().isEmpty())
      job->addMetaData("accept", req->object->accept());
  if ( req->m_docLoader )  {
      KURL r = req->m_docLoader->doc()->URL();
      if ( r.protocol().startsWith( "http" ) && r.path().isEmpty() )
          r.setPath( "/" );

      job->addMetaData("referrer", r.url());
      QString domain = r.host();
      if (req->m_docLoader->doc()->isHTMLDocument())
         domain = static_cast<HTMLDocumentImpl*>(req->m_docLoader->doc())->domain().qstring();
      if (crossDomain(u.host(), domain))
         job->addMetaData("cross-domain", "true");
  }

  connect( job, SIGNAL( result( KIO::Job *, NSData *) ), this, SLOT( slotFinished( KIO::Job *, NSData *) ) );
  
  connect( job, SIGNAL( data( KIO::Job*, const char *, int)),
           SLOT( slotData( KIO::Job*, const char *, int)));
  connect( job, SIGNAL( receivedResponse( KIO::Job *, NSURLResponse *)), SLOT( slotReceivedResponse( KIO::Job *, NSURLResponse *)) );

  if (KWQServeRequest(this, req, job)) {
      if (req->object->type() == CachedObject::Image) {
	CachedImage *ci = static_cast<CachedImage*>(req->object);
	if (ci->decoderCallback()) {
	    m_requestsBackgroundDecoding.append(req);
	}
      }
      m_requestsLoading.insert(job, req);
  }
}

void Loader::slotFinished( KIO::Job* job, NSData *allData)
{
    Request *r = m_requestsLoading.take( job );
    KIO::TransferJob* j = static_cast<KIO::TransferJob*>(job);

    if ( !r )
        return;

    CachedObject *object = r->object;
    DocLoader *docLoader = r->m_docLoader;
    
    bool backgroundImageDecoding = (object->type() == CachedObject::Image && 
	static_cast<CachedImage*>(object)->decoderCallback());
	
    if (j->error() || j->isErrorPage()) {
        // Use the background image decoder's callback to handle the error.
        if (backgroundImageDecoding) {
            CachedImageCallback *callback = static_cast<CachedImage*>(object)->decoderCallback();
            callback->handleError();
        }
        else {
            docLoader->setLoadInProgress(true);
            r->object->error( job->error(), job->errorText().ascii() );
            docLoader->setLoadInProgress(false);
            emit requestFailed( docLoader, object );
            Cache::removeCacheEntry( object );
        }
    }
    else {
        docLoader->setLoadInProgress(true);
        object->data(r->m_buffer, true);
        r->object->setAllData(allData);
        docLoader->setLoadInProgress(false);
        
        // Let the background image decoder trigger the done signal.
        if (!backgroundImageDecoding)
            emit requestDone( docLoader, object );

        object->finish();
    }

    // Let the background image decoder release the request when it is
    // finished.
    if (!backgroundImageDecoding) {
        delete r;
    }

    servePendingRequests();
}


void Loader::slotReceivedResponse(KIO::Job* job, NSURLResponse *response)
{
    Request *r = m_requestsLoading[job];
    ASSERT(r);
    ASSERT(response);
    r->object->setResponse(response);
    r->object->setExpireDate(KWQCacheObjectExpiresTime(r->m_docLoader, response), false);
    
    QString chs = static_cast<KIO::TransferJob*>(job)->queryMetaData("charset");
    if (!chs.isNull())
        r->object->setCharset(chs);
    
    if (r->multipart) {
        ASSERT(r->object->isImage());
        static_cast<CachedImage *>(r->object)->clear();
        r->m_buffer = QBuffer();
        if (r->m_docLoader->part())
            r->m_docLoader->part()->checkCompleted();
        
    } else if (KWQResponseIsMultipart(response)) {
        r->multipart = true;
        if (!r->object->isImage())
            static_cast<KIO::TransferJob*>(job)->cancel();
    }
}


void Loader::slotData( KIO::Job*job, const char *data, int size )
{
    Request *r = m_requestsLoading[job];
    if(!r) {
        kdDebug( 6060 ) << "got data for unknown request!" << endl;
        return;
    }

    if ( !r->m_buffer.isOpen() )
        r->m_buffer.open( IO_WriteOnly );

    r->m_buffer.writeBlock( data, size );

    if (r->multipart)
        r->object->data( r->m_buffer, true ); // the loader delivers the data in a multipart section all at once, send eof
    else if(r->incremental)
        r->object->data( r->m_buffer, false );
}

int Loader::numRequests( DocLoader* dl ) const
{
    int res = 0;

    QPtrListIterator<Request> pIt( m_requestsPending );
    for (; pIt.current(); ++pIt )
        if ( pIt.current()->m_docLoader == dl )
            res++;

    QPtrDictIterator<Request> lIt( m_requestsLoading );
    for (; lIt.current(); ++lIt )
        if ( lIt.current()->m_docLoader == dl )
            if (!lIt.current()->multipart)
                res++;

    QPtrListIterator<Request> bdIt( m_requestsBackgroundDecoding );
    for (; bdIt.current(); ++bdIt )
        if ( bdIt.current()->m_docLoader == dl )
            res++;

    if (dl->loadInProgress())
        res++;
        
    return res;
}

void Loader::cancelRequests( DocLoader* dl )
{
    //kdDebug( 6060 ) << "void Loader::cancelRequests()" << endl;
    //kdDebug( 6060 ) << "got " << m_requestsPending.count() << " pending requests" << endl;
    QPtrListIterator<Request> pIt( m_requestsPending );
    while ( pIt.current() )
    {
        if ( pIt.current()->m_docLoader == dl )
        {
            kdDebug( 6060 ) << "cancelling pending request for " << pIt.current()->object->url().qstring() << endl;
            //emit requestFailed( dl, pIt.current()->object );
            Cache::removeCacheEntry( pIt.current()->object );
            m_requestsPending.remove( pIt );
        }
        else
            ++pIt;
    }

    //kdDebug( 6060 ) << "got " << m_requestsLoading.count() << "loading requests" << endl;

    QPtrDictIterator<Request> lIt( m_requestsLoading );
    while ( lIt.current() )
    {
        if ( lIt.current()->m_docLoader == dl )
        {
            //kdDebug( 6060 ) << "cancelling loading request for " << lIt.current()->object->url().qstring() << endl;
            KIO::Job *job = static_cast<KIO::Job *>( lIt.currentKey() );
            Cache::removeCacheEntry( lIt.current()->object );
            m_requestsLoading.remove( lIt.currentKey() );
            job->kill();
            //emit requestFailed( dl, pIt.current()->object );
        }
        else
            ++lIt;
    }

    QPtrListIterator<Request> bdIt( m_requestsBackgroundDecoding );
    while ( bdIt.current() )
    {
        if ( bdIt.current()->m_docLoader == dl )
        {
            kdDebug( 6060 ) << "cancelling pending request for " << bdIt.current()->object->url().qstring() << endl;
            //emit requestFailed( dl, bdIt.current()->object );
            Cache::removeCacheEntry( bdIt.current()->object );
            m_requestsBackgroundDecoding.remove( bdIt );
        }
        else
            ++bdIt;
    }
}

void Loader::removeBackgroundDecodingRequest (Request *r)
{
    bool present = m_requestsBackgroundDecoding.containsRef(r);
    if (present) {
	m_requestsBackgroundDecoding.remove (r);
    }
}

KIO::Job *Loader::jobForRequest( const DOM::DOMString &url ) const
{
    QPtrDictIterator<Request> it( m_requestsLoading );

    for (; it.current(); ++it )
    {
        CachedObject *obj = it.current()->object;

        if ( obj && obj->url() == url )
            return static_cast<KIO::Job *>( it.currentKey() );
    }

    return 0;
}


bool Loader::isKHTMLLoader() const
{
    return true;
}


// ----------------------------------------------------------------------------


LRUList::LRUList()
:m_head(0), m_tail(0) 
{}

LRUList::~LRUList()
{}

QDict<CachedObject> *Cache::cache = 0;
QPtrList<DocLoader>* Cache::docloader = 0;
Loader *Cache::m_loader = 0;

int Cache::maxSize = DEFCACHESIZE;
int Cache::maxCacheable = MAXCACHEABLE;
int Cache::flushCount = 0;

QPixmap *Cache::nullPixmap = 0;
QPixmap *Cache::brokenPixmap = 0;

CachedObject *Cache::m_headOfUncacheableList = 0;
int Cache::m_totalSizeOfLRULists = 0;
int Cache::m_countOfLRUAndUncacheableLists;
LRUList *Cache::m_LRULists = 0;

void Cache::init()
{
    if ( !cache )
        cache = new QDict<CachedObject>(401, true);

    if ( !docloader )
        docloader = new QPtrList<DocLoader>;

    if ( !nullPixmap )
        nullPixmap = new QPixmap;

    if ( !brokenPixmap )
        brokenPixmap = KWQLoadPixmap("missing_image");

    if ( !m_loader )
        m_loader = new Loader();
}

void Cache::clear()
{
    if ( !cache ) return;
#ifdef CACHE_DEBUG
    kdDebug( 6060 ) << "Cache: CLEAR!" << endl;
    statistics();
#endif
    cache->setAutoDelete( true );
    delete cache; cache = 0;
    delete nullPixmap; nullPixmap = 0;
    delete brokenPixmap; brokenPixmap = 0;
    delete m_loader;   m_loader = 0;
    delete docloader; docloader = 0;
}

CachedImage *Cache::requestImage( DocLoader* dl, const DOMString & url, bool reload, time_t _expireDate )
{
    // this brings the _url to a standard form...
    KURL kurl;
    if (dl)
        kurl = dl->m_doc->completeURL( url.qstring() );
    else
        kurl = url.qstring();
    return requestImage(dl, kurl, reload, _expireDate);
}

CachedImage *Cache::requestImage( DocLoader* dl, const KURL & url, bool reload, time_t _expireDate )
{
    KIO::CacheControl cachePolicy;
    if (dl)
        cachePolicy = dl->cachePolicy();
    else
        cachePolicy = KIO::CC_Verify;

    // Checking if the URL is malformed is lots of extra work for little benefit.

    if (!dl->doc()->shouldCreateRenderers()){
        return 0;
    }

    CachedObject *o = 0;
    if (!reload)
        o = cache->find(url.url());
    if(!o)
    {
#ifdef CACHE_DEBUG
        kdDebug( 6060 ) << "Cache: new: " << url.url() << endl;
#endif
        CachedImage *im = new CachedImage(dl, url.url(), cachePolicy, _expireDate);
        if ( dl && dl->autoloadImages() ) Cache::loader()->load(dl, im, true);
        if (cacheDisabled)
            im->setFree(true);
        else {
        cache->insert( url.url(), im );
        moveToHeadOfLRUList(im);
        }
        o = im;
    }

    
    if(o->type() != CachedObject::Image)
    {
#ifdef CACHE_DEBUG
        kdDebug( 6060 ) << "Cache::Internal Error in requestImage url=" << kurl.url() << "!" << endl;
#endif
        return 0;
    }

#ifdef CACHE_DEBUG
    if( o->status() == CachedObject::Pending )
        kdDebug( 6060 ) << "Cache: loading in progress: " << kurl.url() << endl;
    else
        kdDebug( 6060 ) << "Cache: using cached: " << kurl.url() << ", status " << o->status() << endl;
#endif

    moveToHeadOfLRUList(o);
    if ( dl ) {
        dl->m_docObjects.remove( o );
        if (!cacheDisabled)
        dl->m_docObjects.append( o );
    }
    return static_cast<CachedImage *>(o);
}

CachedCSSStyleSheet *Cache::requestStyleSheet( DocLoader* dl, const DOMString & url, bool reload, time_t _expireDate, const QString& charset)
{
    // this brings the _url to a standard form...
    KURL kurl;
    KIO::CacheControl cachePolicy;
    if ( dl )
    {
        kurl = dl->m_doc->completeURL( url.qstring() );
        cachePolicy = dl->cachePolicy();
    }
    else
    {
        kurl = url.qstring();
        cachePolicy = KIO::CC_Verify;
    }

    // Checking if the URL is malformed is lots of extra work for little benefit.

    CachedObject *o = cache->find(kurl.url());
    if(!o)
    {
#ifdef CACHE_DEBUG
        kdDebug( 6060 ) << "Cache: new: " << kurl.url() << endl;
#endif
        CachedCSSStyleSheet *sheet = new CachedCSSStyleSheet(dl, kurl.url(), cachePolicy, _expireDate, charset);
        if (cacheDisabled)
            sheet->setFree(true);
        else {
        cache->insert( kurl.url(), sheet );
        moveToHeadOfLRUList(sheet);
        }
        o = sheet;
    }

    
    if(o->type() != CachedObject::CSSStyleSheet)
    {
#ifdef CACHE_DEBUG
        kdDebug( 6060 ) << "Cache::Internal Error in requestStyleSheet url=" << kurl.url() << "!" << endl;
#endif
        return 0;
    }

#ifdef CACHE_DEBUG
    if( o->status() == CachedObject::Pending )
        kdDebug( 6060 ) << "Cache: loading in progress: " << kurl.url() << endl;
    else
        kdDebug( 6060 ) << "Cache: using cached: " << kurl.url() << endl;
#endif

    moveToHeadOfLRUList(o);
    if ( dl ) {
        dl->m_docObjects.remove( o );
        if (!cacheDisabled)
        dl->m_docObjects.append( o );
    }
    return static_cast<CachedCSSStyleSheet *>(o);
}

void Cache::preloadStyleSheet( const QString &url, const QString &stylesheet_data)
{
    CachedObject *o = cache->find(url);
    if(o)
        removeCacheEntry(o);

    CachedCSSStyleSheet *stylesheet = new CachedCSSStyleSheet(url, stylesheet_data);
    cache->insert( url, stylesheet );
}

CachedScript *Cache::requestScript( DocLoader* dl, const DOM::DOMString &url, bool reload, time_t _expireDate, const QString& charset)
{
    // this brings the _url to a standard form...
    KURL kurl;
    KIO::CacheControl cachePolicy;
    if ( dl )
    {
        kurl = dl->m_doc->completeURL( url.qstring() );
        cachePolicy = dl->cachePolicy();
    }
    else
    {
        kurl = url.qstring();
        cachePolicy = KIO::CC_Verify;
    }

    // Checking if the URL is malformed is lots of extra work for little benefit.

    CachedObject *o = cache->find(kurl.url());
    if(!o)
    {
#ifdef CACHE_DEBUG
        kdDebug( 6060 ) << "Cache: new: " << kurl.url() << endl;
#endif
        CachedScript *script = new CachedScript(dl, kurl.url(), cachePolicy, _expireDate, charset);
        if (cacheDisabled)
            script->setFree(true);
        else {
        cache->insert( kurl.url(), script );
        moveToHeadOfLRUList(script);
        }
        o = script;
    }

    
    if(!(o->type() == CachedObject::Script))
    {
#ifdef CACHE_DEBUG
        kdDebug( 6060 ) << "Cache::Internal Error in requestScript url=" << kurl.url() << "!" << endl;
#endif
        return 0;
    }
    
    
#ifdef CACHE_DEBUG
    if( o->status() == CachedObject::Pending )
        kdDebug( 6060 ) << "Cache: loading in progress: " << kurl.url() << endl;
    else
        kdDebug( 6060 ) << "Cache: using cached: " << kurl.url() << endl;
#endif

    moveToHeadOfLRUList(o);
    if ( dl ) {
        dl->m_docObjects.remove( o );
        if (!cacheDisabled)
        dl->m_docObjects.append( o );
    }
    return static_cast<CachedScript *>(o);
}

void Cache::preloadScript( const QString &url, const QString &script_data)
{
    CachedObject *o = cache->find(url);
    if(o)
        removeCacheEntry(o);

    CachedScript *script = new CachedScript(url, script_data);
    cache->insert( url, script );
}

#ifdef KHTML_XSLT
CachedXSLStyleSheet* Cache::requestXSLStyleSheet(DocLoader* dl, const DOMString & url, bool reload, 
                                                 time_t _expireDate)
{
    // this brings the _url to a standard form...
    KURL kurl;
    KIO::CacheControl cachePolicy;
    if (dl) {
        kurl = dl->m_doc->completeURL(url.qstring());
        cachePolicy = dl->cachePolicy();
    }
    else {
        kurl = url.qstring();
        cachePolicy = KIO::CC_Verify;
    }
    
    // Checking if the URL is malformed is lots of extra work for little benefit.
    
    CachedObject *o = cache->find(kurl.url());
    if (!o) {
#ifdef CACHE_DEBUG
        kdDebug( 6060 ) << "Cache: new: " << kurl.url() << endl;
#endif
        CachedXSLStyleSheet* doc = new CachedXSLStyleSheet(dl, kurl.url(), cachePolicy, _expireDate);
        if (cacheDisabled)
            doc->setFree(true);
        else {
            cache->insert(kurl.url(), doc);
            moveToHeadOfLRUList(doc);
        }
        o = doc;
    }
    
    
    if (o->type() != CachedObject::XSLStyleSheet) {
#ifdef CACHE_DEBUG
        kdDebug( 6060 ) << "Cache::Internal Error in requestXSLStyleSheet url=" << kurl.url() << "!" << endl;
#endif
        return 0;
    }
    
#ifdef CACHE_DEBUG
    if (o->status() == CachedObject::Pending)
        kdDebug( 6060 ) << "Cache: loading in progress: " << kurl.url() << endl;
    else
        kdDebug( 6060 ) << "Cache: using cached: " << kurl.url() << endl;
#endif
    
    moveToHeadOfLRUList(o);
    if (dl) {
        dl->m_docObjects.remove( o );
        if (!cacheDisabled)
            dl->m_docObjects.append( o );
    }
    return static_cast<CachedXSLStyleSheet*>(o);
}
#endif

#ifndef KHTML_NO_XBL
CachedXBLDocument* Cache::requestXBLDocument(DocLoader* dl, const DOMString & url, bool reload, 
                                             time_t _expireDate)
{
    // this brings the _url to a standard form...
    KURL kurl;
    KIO::CacheControl cachePolicy;
    if (dl) {
        kurl = dl->m_doc->completeURL(url.qstring());
        cachePolicy = dl->cachePolicy();
    }
    else {
        kurl = url.qstring();
        cachePolicy = KIO::CC_Verify;
    }
    
    // Checking if the URL is malformed is lots of extra work for little benefit.
    
    CachedObject *o = cache->find(kurl.url());
    if(!o)
    {
#ifdef CACHE_DEBUG
        kdDebug( 6060 ) << "Cache: new: " << kurl.url() << endl;
#endif
        CachedXBLDocument* doc = new CachedXBLDocument(dl, kurl.url(), cachePolicy, _expireDate);
        if (cacheDisabled)
            doc->setFree(true);
        else {
            cache->insert(kurl.url(), doc);
            moveToHeadOfLRUList(doc);
        }
        o = doc;
    }
    
    
    if(o->type() != CachedObject::XBL)
    {
#ifdef CACHE_DEBUG
        kdDebug( 6060 ) << "Cache::Internal Error in requestXBLDocument url=" << kurl.url() << "!" << endl;
#endif
        return 0;
    }
    
#ifdef CACHE_DEBUG
    if( o->status() == CachedObject::Pending )
        kdDebug( 6060 ) << "Cache: loading in progress: " << kurl.url() << endl;
    else
        kdDebug( 6060 ) << "Cache: using cached: " << kurl.url() << endl;
#endif
    
    moveToHeadOfLRUList(o);
    if ( dl ) {
        dl->m_docObjects.remove( o );
        if (!cacheDisabled)
            dl->m_docObjects.append( o );
    }
    return static_cast<CachedXBLDocument*>(o);
}
#endif

void Cache::flush(bool force)
{
    if (force)
       flushCount = 0;
    // Don't flush for every image.
    if (m_countOfLRUAndUncacheableLists < flushCount)
       return;

    init();

#ifdef CACHE_DEBUG
    statistics();
    kdDebug( 6060 ) << "Cache: flush()" << endl;
#endif

    while (m_headOfUncacheableList)
        removeCacheEntry(m_headOfUncacheableList);

    for (int i = MAX_LRU_LISTS-1; i>=0; i--) {
        if (m_totalSizeOfLRULists <= maxSize)
            break;
            
        while (m_totalSizeOfLRULists > maxSize && m_LRULists[i].m_tail)
            removeCacheEntry(m_LRULists[i].m_tail);
    }

    flushCount = m_countOfLRUAndUncacheableLists+10; // Flush again when the cache has grown.
#ifdef CACHE_DEBUG
    //statistics();
#endif
}


void Cache::setSize( int bytes )
{
    maxSize = bytes;
    maxCacheable = kMax(maxSize / 128, MAXCACHEABLE);

    // may be we need to clear parts of the cache
    flushCount = 0;
    flush(true);
}

void Cache::statistics()
{
    CachedObject *o;
    // this function is for debugging purposes only
    init();

    int size = 0;
    int msize = 0;
    int movie = 0;
    int stylesheets = 0;
    QDictIterator<CachedObject> it(*cache);
    for(it.toFirst(); it.current(); ++it)
    {
        o = it.current();
        if(o->type() == CachedObject::Image)
        {
            CachedImage *im = static_cast<CachedImage *>(o);
            if(im->m != 0)
            {
                movie++;
                msize += im->size();
            }
        }
        else
        {
            if(o->type() == CachedObject::CSSStyleSheet)
                stylesheets++;

        }
        size += o->size();
    }
    size /= 1024;

    kdDebug( 6060 ) << "------------------------- image cache statistics -------------------" << endl;
    kdDebug( 6060 ) << "Number of items in cache: " << cache->count() << endl;
    kdDebug( 6060 ) << "Number of items in lru  : " << m_countOfLRUAndUncacheableLists << endl;
    kdDebug( 6060 ) << "Number of cached images: " << cache->count()-movie << endl;
    kdDebug( 6060 ) << "Number of cached movies: " << movie << endl;
    kdDebug( 6060 ) << "Number of cached stylesheets: " << stylesheets << endl;
    kdDebug( 6060 ) << "pixmaps:   allocated space approx. " << size << " kB" << endl;
    kdDebug( 6060 ) << "movies :   allocated space approx. " << msize/1024 << " kB" << endl;
    kdDebug( 6060 ) << "--------------------------------------------------------------------" << endl;
}

void Cache::removeCacheEntry( CachedObject *object )
{
  QString key = object->url().qstring();

  // this indicates the deref() method of CachedObject to delete itself when the reference counter
  // drops down to zero
  object->setFree( true );

  cache->remove( key );
  removeFromLRUList(object);

  const DocLoader* dl;
  for ( dl=docloader->first(); dl; dl=docloader->next() )
      dl->removeCachedObject( object );

  if ( object->canDelete() )
     delete object;
}

#define FAST_LOG2(_log2,_n)   \
      unsigned int j_ = (unsigned int)(_n);   \
      (_log2) = 0;                    \
      if ((j_) & ((j_)-1))            \
      (_log2) += 1;               \
      if ((j_) >> 16)                 \
      (_log2) += 16, (j_) >>= 16; \
      if ((j_) >> 8)                  \
      (_log2) += 8, (j_) >>= 8;   \
      if ((j_) >> 4)                  \
      (_log2) += 4, (j_) >>= 4;   \
      if ((j_) >> 2)                  \
      (_log2) += 2, (j_) >>= 2;   \
      if ((j_) >> 1)                  \
      (_log2) += 1;

int FastLog2(unsigned int i) {
    int log2;
    FAST_LOG2(log2,i);
    return log2;
}

LRUList* Cache::getLRUListFor(CachedObject* o)
{
    int accessCount = o->accessCount();
    int queueIndex;
    if (accessCount == 0) {
        queueIndex = 0;
    } else {
        int sizeLog = FastLog2(o->size());
        queueIndex = sizeLog/o->accessCount() - 1;
        if (queueIndex < 0)
            queueIndex = 0;
        if (queueIndex >= MAX_LRU_LISTS)
            queueIndex = MAX_LRU_LISTS-1;
    }
    if (m_LRULists == 0) {
        m_LRULists = new LRUList [MAX_LRU_LISTS];
    }
    return &m_LRULists[queueIndex];
}

void Cache::removeFromLRUList(CachedObject *object)
{
    CachedObject *next = object->m_nextInLRUList;
    CachedObject *prev = object->m_prevInLRUList;
    bool uncacheable = object->status() == CachedObject::Uncacheable;
    
    LRUList* list = uncacheable ? 0 : getLRUListFor(object);
    CachedObject *&head = uncacheable ? m_headOfUncacheableList : list->m_head;
    
    if (next == 0 && prev == 0 && head != object) {
        return;
    }
    
    object->m_nextInLRUList = 0;
    object->m_prevInLRUList = 0;
    
    if (next)
        next->m_prevInLRUList = prev;
    else if (!uncacheable && list->m_tail == object)
        list->m_tail = prev;

    if (prev)
        prev->m_nextInLRUList = next;
    else if (head == object)
        head = next;
    
    --m_countOfLRUAndUncacheableLists;
    
    if (!uncacheable)
        m_totalSizeOfLRULists -= object->size();
}

void Cache::moveToHeadOfLRUList(CachedObject *object)
{
    insertInLRUList(object);
}

void Cache::insertInLRUList(CachedObject *object)
{
    removeFromLRUList(object);
    
    if (!object->allowInLRUList())
        return;
    
    LRUList* list = getLRUListFor(object);
    
    bool uncacheable = object->status() == CachedObject::Uncacheable;
    CachedObject *&head = uncacheable ? m_headOfUncacheableList : list->m_head;

    object->m_nextInLRUList = head;
    if (head)
        head->m_prevInLRUList = object;
    head = object;
    
    if (object->m_nextInLRUList == 0 && !uncacheable)
        list->m_tail = object;
    
    ++m_countOfLRUAndUncacheableLists;
    
    if (!uncacheable)
        m_totalSizeOfLRULists += object->size();
}

bool Cache::adjustSize(CachedObject *object, int delta)
{
    if (object->status() == CachedObject::Uncacheable)
        return false;

    if (object->m_nextInLRUList == 0 && object->m_prevInLRUList == 0 &&
        getLRUListFor(object)->m_head != object)
        return false;
    
    m_totalSizeOfLRULists += delta;
    return delta != 0;
}

// --------------------------------------

CachedObjectClient *CachedObjectClientWalker::next()
{
    // Only advance if we already returned this item.
    // This handles cases where the current item is removed, and prevents us from skipping the next item.
    // The iterator automatically gets advanced to the next item, and we make sure we return it.
    if (_current == _iterator.current())
        ++_iterator;
    _current = _iterator.current();
    return _current;
}

// --------------------------------------

CachedObjectClient::~CachedObjectClient() { }
void CachedObjectClient::setPixmap(const QPixmap &, const QRect&, CachedImage *) {}
void CachedObjectClient::setStyleSheet(const DOM::DOMString &/*url*/, const DOM::DOMString &/*sheet*/) {}
#ifndef KHTML_NO_XBL
void CachedObjectClient::setXBLDocument(const DOM::DOMString& url, XBL::XBLDocumentImpl* doc) {}
#endif
void CachedObjectClient::notifyFinished(CachedObject * /*finishedObj*/) {}

#include "loader.moc"



Cache::Statistics Cache::getStatistics()
{
    Statistics stats;

    if (!cache)
        return stats;

    QDictIterator<CachedObject> i(*cache);
    for (i.toFirst(); i.current(); ++i) {
        CachedObject *o = i.current();
        switch (o->type()) {
            case CachedObject::Image:
                if (static_cast<CachedImage *>(o)->m) {
                    stats.movies.count++;
                    stats.movies.size += o->size();
                } else {
                    stats.images.count++;
                    stats.images.size += o->size();
                }
                break;

            case CachedObject::CSSStyleSheet:
                stats.styleSheets.count++;
                stats.styleSheets.size += o->size();
                break;

            case CachedObject::Script:
                stats.scripts.count++;
                stats.scripts.size += o->size();
                break;
#ifdef KHTML_XSLT
            case CachedObject::XSLStyleSheet:
                stats.xslStyleSheets.count++;
                stats.xslStyleSheets.size += o->size();
                break;
#endif
#ifndef KHTML_NO_XBL
            case CachedObject::XBL:
                stats.xblDocs.count++;
                stats.xblDocs.size += o->size();
                break;
#endif
            default:
                stats.other.count++;
                stats.other.size += o->size();
        }
    }
    
    return stats;
}

void Cache::flushAll()
{
    if (!cache)
        return;

    for (;;) {
        QDictIterator<CachedObject> i(*cache);
        CachedObject *o = i.toFirst();
        if (!o)
            break;
        removeCacheEntry(o);
    }
}

void Cache::setCacheDisabled(bool disabled)
{
    cacheDisabled = disabled;
    if (disabled)
        flushAll();
}

