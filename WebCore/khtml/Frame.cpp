/* This file is part of the KDE project
 *
 * Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
 *                     1999 Lars Knoll <knoll@kde.org>
 *                     1999 Antti Koivisto <koivisto@kde.org>
 *                     2000 Simon Hausmann <hausmann@kde.org>
 *                     2000 Stefan Schimanski <1Stein@gmx.de>
 *                     2001 George Staikos <staikos@kde.org>
 * Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.
 * Copyright (C) 2005 Alexey Proskuryakov <ap@nypop.com>
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
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include "Frame.h"

#include "css/csshelper.h"
#include "cssproperties.h"
#include "css/cssstyleselector.h"
#include "css/css_computedstyle.h"
#include "css/css_valueimpl.h"
#include "dom/dom_string.h"
#include "editing/markup.h"
#include "editing/htmlediting.h"
#include "editing/SelectionController.h"
#include "editing/visible_position.h"
#include "editing/visible_text.h"
#include "editing/visible_units.h"
#include "html/html_documentimpl.h"
#include "html/html_baseimpl.h"
#include "html/html_miscimpl.h"
#include "html/html_imageimpl.h"
#include "html/html_objectimpl.h"
#include "rendering/render_block.h"
#include "RenderText.h"
#include "rendering/render_frames.h"
#include "misc/Cache.h"
#include "misc/CachedCSSStyleSheet.h"
#include "misc/DocLoader.h"
#include "misc/loader.h"
#include "xml/dom2_eventsimpl.h"
#include "xml/dom2_rangeimpl.h"
#include "xml/EventNames.h"
#include "xml/xml_tokenizer.h"
#if SVG_SUPPORT
#include "SVGNames.h"
#include "XLinkNames.h"
#endif

using namespace DOM;
using namespace HTMLNames;

#include "khtmlview.h"
#include "ecma/kjs_proxy.h"
#include "ecma/xmlhttprequest.h"
#include "khtml_settings.h"
#include "khtmlpart_p.h"

#include <sys/types.h>
#include <assert.h>
#include <unistd.h>

#include <kstandarddirs.h>
#include <kio/job.h>
#include <kio/global.h>
#include <kdebug.h>
#include <klocale.h>
#include <kglobalsettings.h>

#include <qfile.h>
#include <qptrlist.h>

using namespace DOM::EventNames;
using namespace khtml;

using KParts::BrowserInterface;

const int CARET_BLINK_FREQUENCY = 500;

namespace khtml {
    class PartStyleSheetLoader : public CachedObjectClient
    {
    public:
        PartStyleSheetLoader(Frame *frame, DOM::DOMString url, DocLoader* dl)
        {
            m_frame = frame;
            m_cachedSheet = Cache::requestStyleSheet(dl, url );
            if (m_cachedSheet)
		m_cachedSheet->ref( this );
        }
        virtual ~PartStyleSheetLoader()
        {
            if ( m_cachedSheet ) m_cachedSheet->deref(this);
        }
        virtual void setStyleSheet(const DOM::DOMString&, const DOM::DOMString &sheet)
        {
          if ( m_frame )
            m_frame->setUserStyleSheet( sheet.qstring() );

            delete this;
        }
        QGuardedPtr<Frame> m_frame;
        khtml::CachedCSSStyleSheet *m_cachedSheet;
    };
}

FrameList::Iterator FrameList::find( const QString &name )
{
    Iterator it = begin();
    Iterator e = end();

    for (; it!=e; ++it )
        if ( (*it).m_name==name )
            break;

    return it;
}

Frame::Frame(QWidget *parentWidget, const char *widgetname, QObject *parent, const char *name)
    : KParts::ReadOnlyPart(parent, name), d(0)
{
}

void Frame::init(KHTMLView *view)
{
  AtomicString::init();
  QualifiedName::init();
  EventNames::init();
  HTMLNames::init(); // FIXME: We should make this happen only when HTML is used.
#if SVG_SUPPORT
  KSVG::SVGNames::init();
  XLinkNames::init();
#endif

  frameCount = 0;

  d = new KHTMLPartPrivate(parent());

  d->m_view = view;
  setWidget( d->m_view );
  
  d->m_extension = new KHTMLPartBrowserExtension( this );

  d->m_bSecurityInQuestion = false;
  d->m_bMousePressed = false;

  // The java, javascript, and plugin settings will be set after the settings
  // have been initialized.
  d->m_bJScriptEnabled = true;
  d->m_bJavaEnabled = true;
  d->m_bPluginsEnabled = true;

  connect( khtml::Cache::loader(), SIGNAL( requestDone( khtml::DocLoader*, khtml::CachedObject *) ),
           this, SLOT( slotLoaderRequestDone( khtml::DocLoader*, khtml::CachedObject *) ) );
  connect( khtml::Cache::loader(), SIGNAL( requestFailed( khtml::DocLoader*, khtml::CachedObject *) ),
           this, SLOT( slotLoaderRequestDone( khtml::DocLoader*, khtml::CachedObject *) ) );

  connect(&d->m_redirectionTimer, SIGNAL(timeout()), this, SLOT(slotRedirect()));
  connect(&d->m_lifeSupportTimer, SIGNAL(timeout()), this, SLOT(slotEndLifeSupport()));
}

Frame::~Frame()
{
  cancelRedirection();

  if (!d->m_bComplete)
    closeURL();

  disconnect( khtml::Cache::loader(), SIGNAL( requestDone( khtml::DocLoader*, khtml::CachedObject *) ),
           this, SLOT( slotLoaderRequestDone( khtml::DocLoader*, khtml::CachedObject *) ) );
  disconnect( khtml::Cache::loader(), SIGNAL( requestFailed( khtml::DocLoader*, khtml::CachedObject *) ),
           this, SLOT( slotLoaderRequestDone( khtml::DocLoader*, khtml::CachedObject *) ) );

  clear();

  if (d->m_view) {
    d->m_view->hide();
    d->m_view->viewport()->hide();
    d->m_view->m_frame = 0;
  }
  
  delete d; d = 0;
}

bool Frame::restoreURL( const KURL &url )
{
  cancelRedirection();

  /*
   * That's not a good idea as it will call closeURL() on all
   * child frames, preventing them from further loading. This
   * method gets called from restoreState() in case of a full frameset
   * restoral, and restoreState() calls closeURL() before restoring
   * anyway.
  kdDebug( 6050 ) << "closing old URL" << endl;
  closeURL();
  */

  d->m_bComplete = false;
  d->m_bLoadEventEmitted = false;
  d->m_workingURL = url;

  // set the java(script) flags according to the current host.
  d->m_bJScriptEnabled = d->m_settings->isJavaScriptEnabled(url.host());
  d->m_bJavaEnabled = d->m_settings->isJavaEnabled(url.host());
  d->m_bPluginsEnabled = d->m_settings->isPluginsEnabled(url.host());

  m_url = url;

  emit started( 0L );

  return true;
}


bool Frame::didOpenURL(const KURL &url)
{
  if (d->m_scheduledRedirection == locationChangeScheduledDuringLoad) {
    // We're about to get a redirect that happened before the document was
    // created.  This can happen when one frame may change the location of a 
    // sibling.
    return false;
  }
  
  cancelRedirection();
  
  // clear last edit command
  d->m_lastEditCommand = EditCommandPtr();
  Mac(this)->clearUndoRedoOperations();
  
  KParts::URLArgs args( d->m_extension->urlArgs() );

  if (!d->m_restored)
    closeURL();

  if (d->m_restored)
     d->m_cachePolicy = KIO::CC_Cache;
  else if (args.reload)
     d->m_cachePolicy = KIO::CC_Refresh;
  else
     d->m_cachePolicy = KIO::CC_Verify;

  if ( args.doPost() && (url.protocol().startsWith("http")) )
  {
      d->m_job = KIO::http_post( url, args.postData, false );
      d->m_job->addMetaData("content-type", args.contentType() );
  }
  else
  {
      d->m_job = KIO::get( url, false, false );
      d->m_job->addMetaData("cache", KIO::getCacheControlString(d->m_cachePolicy));
  }

  d->m_job->addMetaData(args.metaData());

  connect( d->m_job, SIGNAL( result( KIO::Job * ) ),
           SLOT( slotFinished( KIO::Job * ) ) );

  connect( d->m_job, SIGNAL(redirection(KIO::Job*, const KURL&) ),
           SLOT( slotRedirection(KIO::Job*,const KURL&) ) );

  d->m_bComplete = false;
  d->m_bLoadingMainResource = true;
  d->m_bLoadEventEmitted = false;

  // delete old status bar msg's from kjs (if it _was_ activated on last URL)
  if( d->m_bJScriptEnabled )
  {
     d->m_kjsStatusBarText = QString::null;
     d->m_kjsDefaultStatusBarText = QString::null;
  }

  // set the javascript flags according to the current url
  d->m_bJavaEnabled = d->m_settings->isJavaEnabled(url.host());
  d->m_bPluginsEnabled = d->m_settings->isPluginsEnabled(url.host());

  // initializing m_url to the new url breaks relative links when opening such a link after this call and _before_ begin() is called (when the first
  // data arrives) (Simon)
  m_url = url;
  if(m_url.protocol().startsWith("http") && !m_url.host().isEmpty() && m_url.path().isEmpty())
    m_url.setPath("/");
  // copy to m_workingURL after fixing m_url above
  d->m_workingURL = m_url;

  connect( d->m_job, SIGNAL( speed( KIO::Job*, unsigned long ) ),
           this, SLOT( slotJobSpeed( KIO::Job*, unsigned long ) ) );

  connect( d->m_job, SIGNAL( percent( KIO::Job*, unsigned long ) ),
           this, SLOT( slotJobPercent( KIO::Job*, unsigned long ) ) );

  emit started( 0L );

  return true;
}

void Frame::didExplicitOpen()
{
  d->m_bComplete = false;
  d->m_bLoadEventEmitted = false;
    
  // Prevent window.open(url) -- eg window.open("about:blank") -- from blowing away results
  // from a subsequent window.document.open / window.document.write call. 
  // Cancelling redirection here works for all cases because document.open 
  // implicitly precedes document.write.
  cancelRedirection(); 
}

void Frame::stopLoading(bool sendUnload)
{
  if (d->m_doc && d->m_doc->tokenizer())
    d->m_doc->tokenizer()->stopParsing();
    
  if ( d->m_job )
  {
    d->m_job->kill();
    d->m_job = 0;
  }

  if (sendUnload) {
    if ( d->m_doc && d->m_doc->isHTMLDocument() ) {
      HTMLDocumentImpl* hdoc = static_cast<HTMLDocumentImpl*>( d->m_doc );
      
      if ( hdoc->body() && d->m_bLoadEventEmitted && !d->m_bUnloadEventEmitted ) {
        hdoc->body()->dispatchWindowEvent( unloadEvent, false, false );
        if ( d->m_doc )
          d->m_doc->updateRendering();
        d->m_bUnloadEventEmitted = true;
      }
    }
    
    if (d->m_doc && !d->m_doc->inPageCache())
      d->m_doc->removeAllEventListenersFromAllNodes();
  }

  d->m_bComplete = true; // to avoid emitting completed() in slotFinishedParsing() (David)
  d->m_bLoadingMainResource = false;
  d->m_bLoadEventEmitted = true; // don't want that one either
  d->m_cachePolicy = KIO::CC_Verify; // Why here?

  if ( d->m_doc && d->m_doc->parsing() )
  {
    kdDebug( 6050 ) << " was still parsing... calling end " << endl;
    slotFinishedParsing();
    d->m_doc->setParsing(false);
  }
  
  d->m_workingURL = KURL();

  if (DocumentImpl *doc = d->m_doc) {
    if (DocLoader *docLoader = doc->docLoader())
      khtml::Cache::loader()->cancelRequests(docLoader);
    KJS::XMLHttpRequest::cancelRequests(doc);
  }

  // tell all subframes to stop as well
  ConstFrameIt it = d->m_frames.begin();
  ConstFrameIt end = d->m_frames.end();
  for (; it != end; ++it ) {
      KParts::ReadOnlyPart *part = (*it).m_frame;
      if (part) {
          Frame *frame = static_cast<Frame *>(part);

          if (frame->inherits("Frame"))
              frame->stopLoading(sendUnload);
          else
              frame->closeURL();
      }
  }

  d->m_bPendingChildRedirection = false;

  // Stop any started redirections as well!! (DA)
  cancelRedirection();
}

bool Frame::closeURL()
{    
  stopLoading(true);

  return true;
}

KParts::BrowserExtension *Frame::browserExtension() const
{
  return d->m_extension;
}

KHTMLView *Frame::view() const
{
  return d->m_view;
}

void Frame::setJScriptEnabled( bool enable )
{
  if ( !enable && jScriptEnabled() && d->m_jscript ) {
    d->m_jscript->clear();
  }
  d->m_bJScriptForce = enable;
  d->m_bJScriptOverride = true;
}

bool Frame::jScriptEnabled() const
{
  if ( d->m_bJScriptOverride )
      return d->m_bJScriptForce;
  return d->m_bJScriptEnabled;
}

void Frame::setMetaRefreshEnabled( bool enable )
{
  d->m_metaRefreshEnabled = enable;
}

bool Frame::metaRefreshEnabled() const
{
  return d->m_metaRefreshEnabled;
}

KJSProxyImpl *Frame::jScript()
{
    if (!jScriptEnabled())
        return 0;

    if (!d->m_jscript)
        d->m_jscript = new KJSProxyImpl(this);

    return d->m_jscript;
}

void Frame::replaceContentsWithScriptResult( const KURL &url )
{
  QString script = KURL::decode_string(url.url().mid(strlen("javascript:")));
  QVariant ret = executeScript(script);
  
  if (ret.type() == QVariant::String) {
    begin();
    write(ret.asString());
    end();
  }
}

QVariant Frame::executeScript( const QString &script, bool forceUserGesture )
{
    return executeScript( 0, script, forceUserGesture );
}

QVariant Frame::executeScript( DOM::NodeImpl *n, const QString &script, bool forceUserGesture )
{
  KJSProxyImpl *proxy = jScript();

  if (!proxy)
    return QVariant();
  d->m_runningScripts++;
  // If forceUserGesture is true, then make the script interpreter
  // treat it as if triggered by a user gesture even if there is no
  // current DOM event being processed.
  QVariant ret = proxy->evaluate( forceUserGesture ? QString::null : m_url.url(), 0, script, n );
  d->m_runningScripts--;
  if (!d->m_runningScripts && d->m_doc && !d->m_doc->parsing() && d->m_submitForm )
      submitFormAgain();
    DocumentImpl::updateDocumentsRendering();

  return ret;
}

bool Frame::scheduleScript(DOM::NodeImpl *n, const QString& script)
{
    d->scheduledScript = script;
    d->scheduledScriptNode = n;

    return true;
}

QVariant Frame::executeScheduledScript()
{
  if( d->scheduledScript.isEmpty() )
    return QVariant();

  QVariant ret = executeScript( d->scheduledScriptNode.get(), d->scheduledScript );
  d->scheduledScript = QString();
  d->scheduledScriptNode = 0;

  return ret;
}

void Frame::setJavaEnabled( bool enable )
{
  d->m_bJavaForce = enable;
  d->m_bJavaOverride = true;
}

bool Frame::javaEnabled() const
{
#ifndef Q_WS_QWS
  if( d->m_bJavaOverride )
      return d->m_bJavaForce;
  return d->m_bJavaEnabled;
#else
  return false;
#endif
}

void Frame::setPluginsEnabled( bool enable )
{
  d->m_bPluginsForce = enable;
  d->m_bPluginsOverride = true;
}

bool Frame::pluginsEnabled() const
{
  if ( d->m_bPluginsOverride )
      return d->m_bPluginsForce;
  return d->m_bPluginsEnabled;
}


void Frame::slotDebugDOMTree()
{
  if ( d->m_doc && d->m_doc->firstChild() )
    qDebug("%s", createMarkup(d->m_doc->firstChild()).latin1());
}

void Frame::slotDebugRenderTree()
{
#ifndef NDEBUG
  if ( d->m_doc )
    d->m_doc->renderer()->printTree();
#endif
}

void Frame::setAutoloadImages( bool enable )
{
  if ( d->m_doc && d->m_doc->docLoader()->autoloadImages() == enable )
    return;

  if ( d->m_doc )
    d->m_doc->docLoader()->setAutoloadImages( enable );
}

bool Frame::autoloadImages() const
{
  if ( d->m_doc )
    return d->m_doc->docLoader()->autoloadImages();

  return true;
}

void Frame::clear()
{
  if ( d->m_bCleared )
    return;
  d->m_bCleared = true;
  d->m_bClearing = true;
  d->m_mousePressNode = 0;

  if ( d->m_doc )
    d->m_doc->detach();

  // Moving past doc so that onUnload works.
  if ( d->m_jscript )
    d->m_jscript->clear();

  if ( d->m_view )
    d->m_view->clear();

  // do not dereference the document before the jscript and view are cleared, as some destructors
  // might still try to access the document.
  if ( d->m_doc )
    d->m_doc->deref();
  d->m_doc = 0;
  d->m_decoder = 0;

  {
    ConstFrameIt it = d->m_frames.begin();
    ConstFrameIt end = d->m_frames.end();
    for(; it != end; ++it )
    {
      if ( (*it).m_frame )
      {
        disconnectChild(&*it);
        (*it).m_frame->deref();
      }
    }
  }
  d->m_frames.clear();

  {
    ConstFrameIt it = d->m_objects.begin();
    ConstFrameIt end = d->m_objects.end();
    for(; it != end; ++it ) {
      if ( (*it).m_frame )
        (*it).m_frame->deref();
    }
  }
  d->m_objects.clear();

  d->m_scheduledRedirection = noRedirectionScheduled;
  d->m_delayRedirect = 0;
  d->m_redirectURL = QString::null;
  d->m_redirectReferrer = QString::null;
  d->m_redirectLockHistory = true;
  d->m_redirectUserGesture = false;
  d->m_bHTTPRefresh = false;
  d->m_bClearing = false;
  d->m_frameNameId = 1;
  d->m_bFirstData = true;

  d->m_bMousePressed = false;

  if ( !d->m_haveEncoding )
    d->m_encoding = QString::null;
#ifdef SPEED_DEBUG
  d->m_parsetime.restart();
#endif
}

bool Frame::openFile()
{
  return true;
}

DOM::DocumentImpl *Frame::xmlDocImpl() const
{
    if ( d )
        return d->m_doc;
    return 0;
}

void Frame::replaceDocImpl(DocumentImpl* newDoc)
{
    if (d) {
        if (d->m_doc) {
            d->m_doc->detach();
            d->m_doc->deref();
        }
        d->m_doc = newDoc;
        if (newDoc) {
            newDoc->ref();
            newDoc->attach();
        }
    }
}

void Frame::receivedFirstData()
{
    begin( d->m_workingURL, d->m_extension->urlArgs().xOffset, d->m_extension->urlArgs().yOffset );

    d->m_doc->docLoader()->setCachePolicy(d->m_cachePolicy);
    d->m_workingURL = KURL();

    // When the first data arrives, the metadata has just been made available
    QString qData;

    // Support for http-refresh
    qData = d->m_job->queryMetaData("http-refresh");
    if( !qData.isEmpty() && d->m_metaRefreshEnabled )
    {
      kdDebug(6050) << "HTTP Refresh Request: " << qData << endl;
      double delay;
      int pos = qData.find( ';' );
      if ( pos == -1 )
        pos = qData.find( ',' );

      if( pos == -1 )
      {
        delay = qData.stripWhiteSpace().toDouble();
        // We want a new history item if the refresh timeout > 1 second
        scheduleRedirection( delay, m_url.url(), delay <= 1);
      }
      else
      {
        int end_pos = qData.length();
        delay = qData.left(pos).stripWhiteSpace().toDouble();
        while ( qData[++pos] == ' ' );
        if ( qData.find( "url", pos, false ) == pos )
        {
          pos += 3;
          while (qData[pos] == ' ' || qData[pos] == '=' )
              pos++;
          if ( qData[pos] == '"' )
          {
              pos++;
              int index = end_pos-1;
              while( index > pos )
              {
                if ( qData[index] == '"' )
                    break;
                index--;
              }
              if ( index > pos )
                end_pos = index;
          }
        }
        // We want a new history item if the refresh timeout > 1 second
        scheduleRedirection( delay, d->m_doc->completeURL( qData.mid( pos, end_pos ) ), delay <= 1);
      }
      d->m_bHTTPRefresh = true;
    }

    // Support for http last-modified
    d->m_lastModified = d->m_job->queryMetaData("modified");
}

void Frame::slotFinished( KIO::Job * job )
{
  if (job->error())
  {
    d->m_job = 0L;
    // TODO: what else ?
    checkCompleted();
    return;
  }

  d->m_workingURL = KURL();
  d->m_job = 0L;

  if (d->m_doc->parsing())
      end(); //will emit completed()
}

void Frame::childBegin()
{
    // We need to do this when the child is created so as to avoid the bogus state of the parent's
    // child->m_bCompleted being false but the child's m_bComplete being true.  If the child gets
    // an error early on, we had trouble where checkingComplete on the child was a NOP because
    // it thought it was already complete, and thus the parent was never signaled, and never set
    // its child->m_bComplete.
    d->m_bComplete = false;
}

void Frame::begin( const KURL &url, int xOffset, int yOffset )
{
  // If we aren't loading an actual URL, then we need to make sure
  // that we have at least an empty document. createEmptyDocument will
  // do that if we don't have a document already.
  if (d->m_workingURL.isEmpty()) {
    Mac(this)->createEmptyDocument();
  }

  clear();

  Mac(this)->partClearedInBegin();

  // Only do this after clearing the part, so that JavaScript can
  // clean up properly if it was on for the last load.
  d->m_bJScriptEnabled = d->m_settings->isJavaScriptEnabled(url.host());

  d->m_bCleared = false;
  d->m_bComplete = false;
  d->m_bLoadEventEmitted = false;
  d->m_bLoadingMainResource = true;

  KParts::URLArgs args( d->m_extension->urlArgs() );
  args.xOffset = xOffset;
  args.yOffset = yOffset;
  d->m_extension->setURLArgs( args );

  KURL ref(url);
  ref.setUser(QSTRING_NULL);
  ref.setPass(QSTRING_NULL);
  ref.setRef(QSTRING_NULL);
  d->m_referrer = ref.url();
  m_url = url;
  KURL baseurl;

  // We don't need KDE chained URI handling or window caption setting
  if (!m_url.isEmpty())
    baseurl = m_url;

  if (DOMImplementationImpl::isXMLMIMEType(args.serviceType))
    d->m_doc = DOMImplementationImpl::instance()->createDocument( d->m_view );
  else
    d->m_doc = DOMImplementationImpl::instance()->createHTMLDocument( d->m_view );

  d->m_doc->ref();
  if (!d->m_doc->attached())
    d->m_doc->attach( );
  d->m_doc->setURL( m_url.url() );
  // We prefer m_baseURL over m_url because m_url changes when we are
  // about to load a new page.
  d->m_doc->setBaseURL( baseurl.url() );
  if (d->m_decoder)
    d->m_doc->setDecoder(d->m_decoder.get());
  d->m_doc->docLoader()->setShowAnimations( d->m_settings->showAnimations() );

  Mac(this)->updatePolicyBaseURL();

  setAutoloadImages( d->m_settings->autoLoadImages() );
  QString userStyleSheet = d->m_settings->userStyleSheet();

  if ( !userStyleSheet.isEmpty() )
    setUserStyleSheet( KURL( userStyleSheet ) );

  Mac(this)->restoreDocumentState();

  d->m_doc->implicitOpen();
  // clear widget
  if (d->m_view)
    d->m_view->resizeContents( 0, 0 );
  connect(d->m_doc,SIGNAL(finishedParsing()),this,SLOT(slotFinishedParsing()));
}

void Frame::write( const char *str, int len )
{
    if ( !d->m_decoder ) {
        d->m_decoder = new Decoder;
        if (!d->m_encoding.isNull())
            d->m_decoder->setEncoding(d->m_encoding.latin1(),
                d->m_haveEncoding ? Decoder::UserChosenEncoding : Decoder::EncodingFromHTTPHeader);
        else
            d->m_decoder->setEncoding(settings()->encoding().latin1(), Decoder::DefaultEncoding);

        if (d->m_doc)
            d->m_doc->setDecoder(d->m_decoder.get());
    }
  if ( len == 0 )
    return;

  if ( len == -1 )
    len = strlen( str );

  QString decoded = d->m_decoder->decode( str, len );

  if(decoded.isEmpty()) return;

  if(d->m_bFirstData) {
      // determine the parse mode
      d->m_doc->determineParseMode( decoded );
      d->m_bFirstData = false;

      // ### this is still quite hacky, but should work a lot better than the old solution
      if(d->m_decoder->visuallyOrdered()) d->m_doc->setVisuallyOrdered();
      d->m_doc->recalcStyle( NodeImpl::Force );
  }

  if (Tokenizer* t = d->m_doc->tokenizer())
      t->write( decoded, true );
}

void Frame::write( const QString &str )
{
  if ( str.isNull() )
    return;

  if(d->m_bFirstData) {
      // determine the parse mode
      d->m_doc->setParseMode( DocumentImpl::Strict );
      d->m_bFirstData = false;
  }
  Tokenizer* t = d->m_doc->tokenizer();
  if(t)
    t->write( str, true );
}

void Frame::end()
{
    d->m_bLoadingMainResource = false;
    endIfNotLoading();
}

void Frame::endIfNotLoading()
{
    if (d->m_bLoadingMainResource)
        return;

    // make sure nothing's left in there...
    if (d->m_decoder)
        write(d->m_decoder->flush());
    if (d->m_doc)
        d->m_doc->finishParsing();
    else
        // WebKit partially uses WebCore when loading non-HTML docs.  In these cases doc==nil, but
        // WebCore is enough involved that we need to checkCompleted() in order for m_bComplete to
        // become true.  An example is when a subframe is a pure text doc, and that subframe is the
        // last one to complete.
        checkCompleted();
}

void Frame::stop()
{
    // make sure nothing's left in there...
    if (d->m_doc) {
        if (d->m_doc->tokenizer())
            d->m_doc->tokenizer()->stopParsing();
        d->m_doc->finishParsing();
    } else {
        // WebKit partially uses WebCore when loading non-HTML docs.  In these cases doc==nil, but
        // WebCore is enough involved that we need to checkCompleted() in order for m_bComplete to
        // become true.  An example is when a subframe is a pure text doc, and that subframe is the
        // last one to complete.
        checkCompleted();
    }
}


void Frame::stopAnimations()
{
  if ( d->m_doc )
    d->m_doc->docLoader()->setShowAnimations( KHTMLSettings::KAnimationDisabled );

  ConstFrameIt it = d->m_frames.begin();
  ConstFrameIt end = d->m_frames.end();
  for (; it != end; ++it )
    if ( !( *it ).m_frame.isNull() && ( *it ).m_frame->inherits( "Frame" ) ) {
      KParts::ReadOnlyPart* p = ( *it ).m_frame;
      static_cast<Frame*>( p )->stopAnimations();
    }
}

void Frame::gotoAnchor()
{
    // If our URL has no ref, then we have no place we need to jump to.
    if (!m_url.hasRef())
        return;

    QString ref = m_url.encodedHtmlRef();
    if (!gotoAnchor(ref)) {
        // Can't use htmlRef() here because it doesn't know which encoding to use to decode.
        // Decoding here has to match encoding in completeURL, which means it has to use the
        // page's encoding rather than UTF-8.
        if (d->m_decoder)
            gotoAnchor(KURL::decode_string(ref, d->m_decoder->codec()));
    }
}

void Frame::slotFinishedParsing()
{
  d->m_doc->setParsing(false);

  if (!d->m_view)
    return; // We are probably being destructed.

  RefPtr<Frame> protector(this);
  checkCompleted();

  if (!d->m_view)
    return; // We are being destroyed by something checkCompleted called.

  // check if the scrollbars are really needed for the content
  // if not, remove them, relayout, and repaint

  d->m_view->restoreScrollBar();
  gotoAnchor();
}

void Frame::slotLoaderRequestDone( khtml::DocLoader* dl, khtml::CachedObject *obj )
{
  // We really only need to call checkCompleted when our own resources are done loading.
  // So we should check that d->m_doc->docLoader() == dl here.
  // That might help with performance by skipping some unnecessary work, but it's too
  // risky to make that change right now (2005-02-07), because we might be accidentally
  // depending on the extra checkCompleted calls.
  if (d->m_doc) {
    checkCompleted();
  }
}

void Frame::checkCompleted()
{
  // Any frame that hasn't completed yet ?
  ConstFrameIt it = d->m_frames.begin();
  ConstFrameIt end = d->m_frames.end();
  for (; it != end; ++it )
    if ( !(*it).m_bCompleted )
      return;

  // Have we completed before?
  if ( d->m_bComplete )
    return;

  // Are we still parsing?
  if ( d->m_doc && d->m_doc->parsing() )
    return;

  // Still waiting for images/scripts from the loader ?
  int requests = 0;
  if ( d->m_doc && d->m_doc->docLoader() )
    requests = khtml::Cache::loader()->numRequests( d->m_doc->docLoader() );

  if ( requests > 0 )
    return;

  // OK, completed.
  // Now do what should be done when we are really completed.
  d->m_bComplete = true;

  checkEmitLoadEvent(); // if we didn't do it before

  if ( d->m_scheduledRedirection != noRedirectionScheduled )
  {
    // Do not start redirection for frames here! That action is
    // deferred until the parent emits a completed signal.
    if ( parentFrame() == 0 )
      d->m_redirectionTimer.start( (int)(1000 * d->m_delayRedirect), true );

    emit completed( true );
  }
  else
  {
    if ( d->m_bPendingChildRedirection )
      emit completed ( true );
    else
      emit completed();
  }
}

void Frame::checkEmitLoadEvent()
{
  if ( d->m_bLoadEventEmitted || !d->m_doc || d->m_doc->parsing() ) return;

  ConstFrameIt it = d->m_frames.begin();
  ConstFrameIt end = d->m_frames.end();
  for (; it != end; ++it )
    if ( !(*it).m_bCompleted ) // still got a frame running -> too early
      return;


  // All frames completed -> set their domain to the frameset's domain
  // This must only be done when loading the frameset initially (#22039),
  // not when following a link in a frame (#44162).
  if (d->m_doc) {
    DOMString domain = d->m_doc->domain();
    ConstFrameIt it = d->m_frames.begin();
    ConstFrameIt end = d->m_frames.end();
    for (; it != end; ++it ) {
      KParts::ReadOnlyPart *p = (*it).m_frame;
      if (p && p->inherits("Frame")) {
        Frame* htmlFrame = static_cast<Frame *>(p);
        if (htmlFrame->d->m_doc)
          htmlFrame->d->m_doc->setDomain(domain);
      }
    }
  }

  d->m_bLoadEventEmitted = true;
  d->m_bUnloadEventEmitted = false;
  if (d->m_doc)
    d->m_doc->implicitClose();
}

const KHTMLSettings *Frame::settings() const
{
  return d->m_settings;
}

KURL Frame::baseURL() const
{
    if (!d->m_doc)
        return KURL();
    return d->m_doc->baseURL();
}

QString Frame::baseTarget() const
{
    if (!d->m_doc)
        return QString();
    return d->m_doc->baseTarget();
}

KURL Frame::completeURL( const QString &url )
{
    if (!d->m_doc)
        return url;

    return KURL(d->m_doc->completeURL(url));
}

void Frame::scheduleRedirection( double delay, const QString &url, bool doLockHistory)
{
    if (delay < 0 || delay > INT_MAX / 1000)
      return;
    if ( d->m_scheduledRedirection == noRedirectionScheduled || delay <= d->m_delayRedirect )
    {
       d->m_scheduledRedirection = redirectionScheduled;
       d->m_delayRedirect = delay;
       d->m_redirectURL = url;
       d->m_redirectReferrer = QString::null;
       d->m_redirectLockHistory = doLockHistory;
       d->m_redirectUserGesture = false;

       d->m_redirectionTimer.stop();
       if ( d->m_bComplete )
         d->m_redirectionTimer.start( (int)(1000 * d->m_delayRedirect), true );
    }
}

void Frame::scheduleLocationChange(const QString &url, const QString &referrer, bool lockHistory, bool userGesture)
{
    // Handle a location change of a page with no document as a special case.
    // This may happen when a frame changes the location of another frame.
    d->m_scheduledRedirection = d->m_doc ? locationChangeScheduled : locationChangeScheduledDuringLoad;
    
    // If a redirect was scheduled during a load, then stop the current load.
    // Otherwise when the current load transitions from a provisional to a 
    // committed state, pending redirects may be cancelled. 
    if (d->m_scheduledRedirection == locationChangeScheduledDuringLoad) {
        stopLoading(true);   
    }
    
    d->m_delayRedirect = 0;
    d->m_redirectURL = url;
    d->m_redirectReferrer = referrer;
    d->m_redirectLockHistory = lockHistory;
    d->m_redirectUserGesture = userGesture;
    d->m_redirectionTimer.stop();
    if (d->m_bComplete)
        d->m_redirectionTimer.start(0, true);
}

bool Frame::isScheduledLocationChangePending() const
{
    switch (d->m_scheduledRedirection) {
        case noRedirectionScheduled:
        case redirectionScheduled:
            return false;
        case historyNavigationScheduled:
        case locationChangeScheduled:
        case locationChangeScheduledDuringLoad:
            return true;
    }
    return false;
}

void Frame::scheduleHistoryNavigation( int steps )
{
    // navigation will always be allowed in the 0 steps case, which is OK because
    // that's supposed to force a reload.
    if (!Mac(this)->canGoBackOrForward(steps)) {
        cancelRedirection();
        return;
    }

    d->m_scheduledRedirection = historyNavigationScheduled;
    d->m_delayRedirect = 0;
    d->m_redirectURL = QString::null;
    d->m_redirectReferrer = QString::null;
    d->m_scheduledHistoryNavigationSteps = steps;
    d->m_redirectionTimer.stop();
    if (d->m_bComplete)
        d->m_redirectionTimer.start(0, true);
}

void Frame::cancelRedirection(bool cancelWithLoadInProgress)
{
    if (d) {
        d->m_cancelWithLoadInProgress = cancelWithLoadInProgress;
        d->m_scheduledRedirection = noRedirectionScheduled;
        d->m_redirectionTimer.stop();
    }
}

void Frame::changeLocation(const QString &URL, const QString &referrer, bool lockHistory, bool userGesture)
{
    if (URL.find("javascript:", 0, false) == 0) {
        QString script = KURL::decode_string(URL.mid(11));
        QVariant result = executeScript(script, userGesture);
        if (result.type() == QVariant::String) {
            begin(url());
            write(result.asString());
            end();
        }
        return;
    }

    KParts::URLArgs args;

    args.setLockHistory(lockHistory);
    if (!referrer.isEmpty())
        args.metaData()["referrer"] = referrer;

    urlSelected(URL, 0, 0, "_self", args);
}

void Frame::slotRedirect()
{
    if (d->m_scheduledRedirection == historyNavigationScheduled) {
        d->m_scheduledRedirection = noRedirectionScheduled;

        // Special case for go(0) from a frame -> reload only the frame
        // go(i!=0) from a frame navigates into the history of the frame only,
        // in both IE and NS (but not in Mozilla).... we can't easily do that
        // in Konqueror...
        if (d->m_scheduledHistoryNavigationSteps == 0) // add && parentFrame() to get only frames, but doesn't matter
            openURL( url() ); /// ## need args.reload=true?
        else {
            if (d->m_extension) {
                BrowserInterface *interface = d->m_extension->browserInterface();
                if (interface)
                    interface->callMethod( "goHistory(int)", d->m_scheduledHistoryNavigationSteps );
            }
        }
        return;
    }

    QString URL = d->m_redirectURL;
    QString referrer = d->m_redirectReferrer;
    bool lockHistory = d->m_redirectLockHistory;
    bool userGesture = d->m_redirectUserGesture;

    d->m_scheduledRedirection = noRedirectionScheduled;
    d->m_delayRedirect = 0;
    d->m_redirectURL = QString::null;
    d->m_redirectReferrer = QString::null;

    changeLocation(URL, referrer, lockHistory, userGesture);
}

void Frame::slotRedirection(KIO::Job*, const KURL& url)
{
    d->m_workingURL = url;
}

QString Frame::encoding() const
{
    if(d->m_haveEncoding && !d->m_encoding.isEmpty())
        return d->m_encoding;

    if(d->m_decoder && d->m_decoder->encoding())
        return QString(d->m_decoder->encoding());

    return(settings()->encoding());
}

void Frame::setUserStyleSheet(const KURL &url)
{
  if ( d->m_doc && d->m_doc->docLoader() )
    (void) new khtml::PartStyleSheetLoader(this, url.url(), d->m_doc->docLoader());
}

void Frame::setUserStyleSheet(const QString &styleSheet)
{
  if ( d->m_doc )
    d->m_doc->setUserStyleSheet( styleSheet );
}

bool Frame::gotoAnchor( const QString &name )
{
  if (!d->m_doc)
    return false;

  NodeImpl *n = d->m_doc->getElementById(name);
  if (!n) {
    HTMLCollectionImpl *anchors =
        new HTMLCollectionImpl( d->m_doc, HTMLCollectionImpl::DOC_ANCHORS);
    anchors->ref();
    n = anchors->namedItem(name, !d->m_doc->inCompatMode());
    anchors->deref();
  }

  d->m_doc->setCSSTarget(n); // Setting to null will clear the current target.
  
  // Implement the rule that "" and "top" both mean top of page as in other browsers.
  if (!n && !(name.isEmpty() || name.lower() == "top")) {
    kdDebug(6050) << "Frame::gotoAnchor node '" << name << "' not found" << endl;
    return false;
  }

  // We need to update the layout before scrolling, otherwise we could
  // really mess things up if an anchor scroll comes at a bad moment.
  if ( d->m_doc ) {
    d->m_doc->updateRendering();
    // Only do a layout if changes have occurred that make it necessary.      
    if ( d->m_view && d->m_doc->renderer() && d->m_doc->renderer()->needsLayout() ) {
      d->m_view->layout();
    }
  }
  
  // Scroll nested layers and frames to reveal the anchor.
  RenderObject *renderer;
  QRect rect;
  if (n) {
      renderer = n->renderer();
      rect = n->getRect();
  } else {
    // If there's no node, we should scroll to the top of the document.
      renderer = d->m_doc->renderer();
      rect = QRect();
  }

  if (renderer) {
    // Align to the top and to the closest side (this matches other browsers).
    renderer->enclosingLayer()->scrollRectToVisible(rect, RenderLayer::gAlignToEdgeIfNeeded, RenderLayer::gAlignTopAlways);
  }
  
  return true;
}

void Frame::setStandardFont( const QString &name )
{
    d->m_settings->setStdFontName(name);
}

void Frame::setFixedFont( const QString &name )
{
    d->m_settings->setFixedFontName(name);
}

QCursor Frame::urlCursor() const
{
  // Don't load the link cursor until it's actually used.
  // Also, we don't need setURLCursor.
  // This speeds up startup time.
  return KCursor::handCursor();
}

bool Frame::onlyLocalReferences() const
{
  return d->m_onlyLocalReferences;
}

void Frame::setOnlyLocalReferences(bool enable)
{
  d->m_onlyLocalReferences = enable;
}

QString Frame::selectedText() const
{
    return plainText(selection().toRange().get());
}

bool Frame::hasSelection() const
{
    return d->m_selection.isCaretOrRange();
}

const SelectionController &Frame::selection() const
{
    return d->m_selection;
}

ETextGranularity Frame::selectionGranularity() const
{
    return d->m_selectionGranularity;
}

void Frame::setSelectionGranularity(ETextGranularity granularity) const
{
    d->m_selectionGranularity = granularity;
}

const SelectionController &Frame::dragCaret() const
{
    return d->m_dragCaret;
}

const SelectionController &Frame::mark() const
{
    return d->m_mark;
}

void Frame::setMark(const SelectionController &s)
{
    d->m_mark = s;
}

void Frame::setSelection(const SelectionController &s, bool closeTyping, bool keepTypingStyle)
{
    if (d->m_selection == s) {
        return;
    }
    
    clearCaretRectIfNeeded();

    SelectionController oldSelection = d->m_selection;

    d->m_selection = s;
    if (!s.isNone())
        setFocusNodeIfNeeded();
    
    selectionLayoutChanged();

    // Always clear the x position used for vertical arrow navigation.
    // It will be restored by the vertical arrow navigation code if necessary.
    d->m_xPosForVerticalArrowNavigation = NoXPosForVerticalArrowNavigation;

    if (closeTyping)
        TypingCommand::closeTyping(lastEditCommand());

    if (!keepTypingStyle)
        clearTypingStyle();
    
    Mac(this)->respondToChangedSelection(oldSelection, closeTyping);
}

void Frame::setDragCaret(const SelectionController &dragCaret)
{
    if (d->m_dragCaret != dragCaret) {
        d->m_dragCaret.needsCaretRepaint();
        d->m_dragCaret = dragCaret;
        d->m_dragCaret.needsCaretRepaint();
    }
}

void Frame::clearSelection()
{
    setSelection(SelectionController());
}

void Frame::invalidateSelection()
{
    clearCaretRectIfNeeded();
    d->m_selection.setNeedsLayout();
    selectionLayoutChanged();
}

void Frame::setCaretVisible(bool flag)
{
    if (d->m_caretVisible == flag)
        return;
    clearCaretRectIfNeeded();
    if (flag)
        setFocusNodeIfNeeded();
    d->m_caretVisible = flag;
    selectionLayoutChanged();
}


void Frame::clearCaretRectIfNeeded()
{
    if (d->m_caretPaint) {
        d->m_caretPaint = false;
        d->m_selection.needsCaretRepaint();
    }        
}

// Helper function that tells whether a particular node is an element that has an entire
// Frame and KHTMLView, a <frame>, <iframe>, or <object>.
static bool isFrameElement(const NodeImpl *n)
{
    if (!n)
        return false;
    RenderObject *renderer = n->renderer();
    if (!renderer || !renderer->isWidget())
        return false;
    QWidget *widget = static_cast<RenderWidget *>(renderer)->widget();
    return widget && widget->inherits("KHTMLView");
}

void Frame::setFocusNodeIfNeeded()
{
    if (!xmlDocImpl() || d->m_selection.isNone() || !d->m_isFocused)
        return;

    NodeImpl *startNode = d->m_selection.start().node();
    NodeImpl *target = startNode ? startNode->rootEditableElement() : 0;
    
    if (target) {
        for ( ; target; target = target->parentNode()) {
            // We don't want to set focus on a subframe when selecting in a parent frame,
            // so add the !isFrameElement check here. There's probably a better way to make this
            // work in the long term, but this is the safest fix at this time.
            if (target->isMouseFocusable() && !::isFrameElement(target)) {
                xmlDocImpl()->setFocusNode(target);
                return;
            }
        }
        xmlDocImpl()->setFocusNode(0);
    }
}

void Frame::selectionLayoutChanged()
{
    // kill any caret blink timer now running
    if (d->m_caretBlinkTimer >= 0) {
        killTimer(d->m_caretBlinkTimer);
        d->m_caretBlinkTimer = -1;
    }

    // see if a new caret blink timer needs to be started
    if (d->m_caretVisible && d->m_caretBlinks && 
        d->m_selection.isCaret() && d->m_selection.start().node()->isContentEditable()) {
        d->m_caretBlinkTimer = startTimer(CARET_BLINK_FREQUENCY);
        d->m_caretPaint = true;
        d->m_selection.needsCaretRepaint();
    }

    if (d->m_doc)
        d->m_doc->updateSelection();
}

void Frame::setXPosForVerticalArrowNavigation(int x)
{
    d->m_xPosForVerticalArrowNavigation = x;
}

int Frame::xPosForVerticalArrowNavigation() const
{
    return d->m_xPosForVerticalArrowNavigation;
}

void Frame::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == d->m_caretBlinkTimer && 
        d->m_caretVisible && 
        d->m_caretBlinks && 
        d->m_selection.isCaret()) {
        if (d->m_bMousePressed) {
            if (!d->m_caretPaint) {
                d->m_caretPaint = true;
                d->m_selection.needsCaretRepaint();
            }
        }
        else {
            d->m_caretPaint = !d->m_caretPaint;
            d->m_selection.needsCaretRepaint();
        }
    }
}

void Frame::paintCaret(QPainter *p, const QRect &rect) const
{
    if (d->m_caretPaint)
        d->m_selection.paintCaret(p, rect);
}

void Frame::paintDragCaret(QPainter *p, const QRect &rect) const
{
    d->m_dragCaret.paintCaret(p, rect);
}

void Frame::urlSelected( const QString &url, int button, int state, const QString &_target,
                             KParts::URLArgs args )
{
  bool hasTarget = false;

  QString target = _target;
  if ( target.isEmpty() && d->m_doc )
    target = d->m_doc->baseTarget();
  if ( !target.isEmpty() )
      hasTarget = true;

  if ( url.find( QString::fromLatin1( "javascript:" ), 0, false ) == 0 )
  {
    executeScript( KURL::decode_string( url.right( url.length() - 11) ), true );
    return;
  }

  KURL cURL = completeURL(url);

  if ( !cURL.isValid() )
    // ### ERROR HANDLING
    return;

  //kdDebug( 6000 ) << "urlSelected: complete URL:" << cURL.url() << " target = " << target << endl;


  args.frameName = target;

  if ( d->m_bHTTPRefresh )
  {
    d->m_bHTTPRefresh = false;
    args.metaData()["cache"] = "refresh";
  }


  if (!d->m_referrer.isEmpty())
    args.metaData()["referrer"] = d->m_referrer;
  Mac(this)->urlSelected(cURL, button, state, args);
}


bool Frame::requestFrame( khtml::RenderPart *part, const QString &url, const QString &frameName,
                              const QStringList &paramNames, const QStringList &paramValues, bool isIFrame )
{
  FrameIt it = d->m_frames.find( frameName );
  if (it == d->m_frames.end()) {
    khtml::ChildFrame child;
    child.m_name = frameName;
    it = d->m_frames.append( child );
  }

  (*it).m_type = isIFrame ? khtml::ChildFrame::IFrame : khtml::ChildFrame::Frame;
  (*it).m_renderer = part;
  (*it).m_paramValues = paramNames;
  (*it).m_paramNames = paramValues;

  // Support for <frame src="javascript:string">
  if (url.startsWith("javascript:", false)) {
    if (!processObjectRequest(&(*it), "about:blank", "text/html" ))
      return false;

    Frame *newPart = static_cast<Frame *>(&*(*it).m_frame); 
    newPart->replaceContentsWithScriptResult( url );

    return true;
  }

  return requestObject( &(*it), completeURL( url ));
}

QString Frame::requestFrameName()
{
    return Mac(this)->generateFrameName();
}

bool Frame::requestObject( khtml::RenderPart *frame, const QString &url, const QString &serviceType,
                               const QStringList &paramNames, const QStringList &paramValues )
{
  khtml::ChildFrame child;
  QValueList<khtml::ChildFrame>::Iterator it = d->m_objects.append( child );
  (*it).m_renderer = frame;
  (*it).m_type = khtml::ChildFrame::Object;
  (*it).m_paramNames = paramNames;
  (*it).m_paramValues = paramValues;
  (*it).m_hasFallbackContent = frame->hasFallbackContent();

  KURL completedURL;
  if (!url.isEmpty())
    completedURL = completeURL(url);

  KParts::URLArgs args;
  args.serviceType = serviceType;
  return requestObject( &(*it), completedURL, args );
}

bool Frame::requestObject( khtml::ChildFrame *child, const KURL &url, const KParts::URLArgs &_args )
{
  if ( child->m_bPreloaded )
  {
    // kdDebug(6005) << "Frame::requestObject preload" << endl;
    if ( child->m_renderer && child->m_frame && child->m_renderer->widget() )
      child->m_renderer->setWidget( child->m_renderer->widget() );

    child->m_bPreloaded = false;
    return true;
  }

  KParts::URLArgs args( _args );


  if ( child->m_frame && !args.reload && urlcmp( child->m_frame->url().url(), url.url(), true, true ) )
    args.serviceType = child->m_serviceType;

  child->m_args = args;
  child->m_args.reload = (d->m_cachePolicy == KIO::CC_Reload) || (d->m_cachePolicy == KIO::CC_Refresh);
  child->m_serviceName = QString::null;
  if (!d->m_referrer.isEmpty() && !child->m_args.metaData().contains( "referrer" ))
    child->m_args.metaData()["referrer"] = d->m_referrer;


  // We want a Frame if the HTML says <frame src=""> or <frame src="about:blank">
  if ((url.isEmpty() || url.url() == "about:blank") && args.serviceType.isEmpty())
    args.serviceType = QString::fromLatin1( "text/html" );

  return processObjectRequest( child, url, args.serviceType );
}

bool Frame::processObjectRequest( khtml::ChildFrame *child, const KURL &_url, const QString &mimetype )
{
  //kdDebug( 6050 ) << "Frame::processObjectRequest trying to create part for " << mimetype << endl;

  // IMPORTANT: create a copy of the url here, because it is just a reference, which was likely to be given
  // by an emitting frame part (emit openURLRequest( blahurl, ... ) . A few lines below we delete the part
  // though -> the reference becomes invalid -> crash is likely
  KURL url( _url );

  // khtmlrun called us this way to indicate a loading error
  if ( d->m_onlyLocalReferences || ( url.isEmpty() && mimetype.isEmpty() ) )
  {
      checkEmitLoadEvent();
      child->m_bCompleted = true;
      return true;
  }

  if (child->m_bNotify)
  {
      child->m_bNotify = false;
      if ( !child->m_args.lockHistory() )
          emit d->m_extension->openURLNotify();
  }

  if (child->m_frame)
  {
    Frame *frame = static_cast<Frame *>(&*child->m_frame);
    if (frame && frame->inherits("Frame")) {
      KParts::URLArgs args;
      if (!d->m_referrer.isEmpty())
        args.metaData()["referrer"] = d->m_referrer;
      Mac(frame)->openURLRequest(url, args);
    }
  }
  else
  {
    KParts::ReadOnlyPart *part = Mac(this)->createPart(*child, url, mimetype);
    Frame *frame = static_cast<Frame *>(part);
    if (frame && frame->inherits("Frame"))
      frame->childBegin();

    if (!part) {
        checkEmitLoadEvent();
        return false;
    }

    //CRITICAL STUFF
    if (child->m_frame)
    {
      disconnectChild(child);
      child->m_frame->deref();
    }

    child->m_serviceType = mimetype;
    if (child->m_renderer && frame->widget() )
      child->m_renderer->setWidget( frame->widget() );


    child->m_frame = part;
    assert( ((void*) child->m_frame) != 0);

    connectChild(child);

  }

  checkEmitLoadEvent();
  // Some JS code in the load event may have destroyed the part
  // In that case, abort
  if (!child->m_renderer)
    return false;

  if (child->m_bPreloaded)
  {
    if (child->m_renderer && child->m_renderer)
      child->m_renderer->setWidget(child->m_renderer->widget());

    child->m_bPreloaded = false;
    return true;
  }

  child->m_args.reload = (d->m_cachePolicy == KIO::CC_Reload) || (d->m_cachePolicy == KIO::CC_Refresh);

  // make sure the part has a way to find out about the mimetype.
  // we actually set it in child->m_args in requestObject already,
  // but it's useless if we had to use a KHTMLRun instance, as the
  // point the run object is to find out exactly the mimetype.
  child->m_args.serviceType = mimetype;

  child->m_bCompleted = false;
  if ( child->m_extension )
    child->m_extension->setURLArgs( child->m_args );

  // In these cases, the synchronous load would have finished
  // before we could connect the signals, so make sure to send the 
  // completed() signal for the child by hand
  // FIXME: In this case the Frame will have finished loading before 
  // it's being added to the child list.  It would be a good idea to
  // create the child first, then invoke the loader separately  
  if (url.isEmpty() || url.url() == "about:blank") {
      ReadOnlyPart *readOnlyPart = child->m_frame;
      Frame *frame = static_cast<Frame *>(readOnlyPart);
      if (frame && frame->inherits("Frame")) {
          frame->completed();
          frame->checkCompleted();
      }
  }
      return true;
}


void Frame::submitFormAgain()
{
  if( d->m_doc && !d->m_doc->parsing() && d->m_submitForm)
    Frame::submitForm( d->m_submitForm->submitAction, d->m_submitForm->submitUrl, d->m_submitForm->submitFormData, d->m_submitForm->target, d->m_submitForm->submitContentType, d->m_submitForm->submitBoundary );

  delete d->m_submitForm;
  d->m_submitForm = 0;
  disconnect(this, SIGNAL(completed()), this, SLOT(submitFormAgain()));
}

void Frame::submitForm( const char *action, const QString &url, const FormData &formData, const QString &_target, const QString& contentType, const QString& boundary )
{
  kdDebug(6000) << this << ": Frame::submitForm target=" << _target << " url=" << url << endl;
  KURL u = completeURL( url );

  if (!u.isValid())
    // ### ERROR HANDLING!
    return;

  QString urlstring = u.url();
  if (urlstring.startsWith("javascript:", false)) {
    urlstring = KURL::decode_string(urlstring);
    d->m_executingJavaScriptFormAction = true;
    executeScript( urlstring.right( urlstring.length() - 11) );
    d->m_executingJavaScriptFormAction = false;
    return;
  }

  KParts::URLArgs args;

  if (!d->m_referrer.isEmpty())
     args.metaData()["referrer"] = d->m_referrer;

  args.frameName = _target.isEmpty() ? d->m_doc->baseTarget() : _target ;

  // Handle mailto: forms
  if (u.protocol() == "mailto") {
      // 1)  Check for attach= and strip it
      QString q = u.query().mid(1);
      QStringList nvps = QStringList::split("&", q);
      bool triedToAttach = false;

      for (QStringList::Iterator nvp = nvps.begin(); nvp != nvps.end(); ++nvp) {
         QStringList pair = QStringList::split("=", *nvp);
         if (pair.count() >= 2) {
            if (pair.first().lower() == "attach") {
               nvp = nvps.remove(nvp);
               triedToAttach = true;
            }
         }
      }


      // 2)  Append body=
      QString bodyEnc;
      if (contentType.lower() == "multipart/form-data") {
         // FIXME: is this correct?  I suspect not
         bodyEnc = KURL::encode_string(formData.flattenToString());
      } else if (contentType.lower() == "text/plain") {
         // Convention seems to be to decode, and s/&/\n/
         QString tmpbody = formData.flattenToString();
         tmpbody.replace('&', '\n');
         tmpbody.replace('+', ' ');
         tmpbody = KURL::decode_string(tmpbody);  // Decode the rest of it
         bodyEnc = KURL::encode_string(tmpbody);  // Recode for the URL
      } else {
         bodyEnc = KURL::encode_string(formData.flattenToString());
      }

      nvps.append(QString("body=%1").arg(bodyEnc));
      q = nvps.join("&");
      u.setQuery(q);
  } 

  if ( strcmp( action, "get" ) == 0 ) {
    if (u.protocol() != "mailto")
       u.setQuery( formData.flattenToString() );
    args.setDoPost( false );
  }
  else {
    args.postData = formData;
    args.setDoPost( true );

    // construct some user headers if necessary
    if (contentType.isNull() || contentType == "application/x-www-form-urlencoded")
      args.setContentType( "Content-Type: application/x-www-form-urlencoded" );
    else // contentType must be "multipart/form-data"
      args.setContentType( "Content-Type: " + contentType + "; boundary=" + boundary );
  }

  if ( d->m_doc->parsing() || d->m_runningScripts > 0 ) {
    if( d->m_submitForm ) {
      kdDebug(6000) << "Frame::submitForm ABORTING!" << endl;
      return;
    }
    d->m_submitForm = new KHTMLPartPrivate::SubmitForm;
    d->m_submitForm->submitAction = action;
    d->m_submitForm->submitUrl = url;
    d->m_submitForm->submitFormData = formData;
    d->m_submitForm->target = _target;
    d->m_submitForm->submitContentType = contentType;
    d->m_submitForm->submitBoundary = boundary;
    connect(this, SIGNAL(completed()), this, SLOT(submitFormAgain()));
  }
  else
  {
    Mac(this)->submitForm( u, args);
  }
}


void Frame::slotParentCompleted()
{
  if ( d->m_scheduledRedirection != noRedirectionScheduled && !d->m_redirectionTimer.isActive() )
  {
    // kdDebug(6050) << this << ": Child redirection -> " << d->m_redirectURL << endl;
    d->m_redirectionTimer.start( (int)(1000 * d->m_delayRedirect), true );
  }
}

void Frame::slotChildStarted( KIO::Job *job )
{
  khtml::ChildFrame *child = childFrame( sender() );

  assert( child );

  child->m_bCompleted = false;

  if ( d->m_bComplete )
  {
    d->m_bComplete = false;
    emit started( job );
  }
}

void Frame::slotChildCompleted()
{
  slotChildCompleted( false );
}

void Frame::slotChildCompleted( bool complete )
{
  khtml::ChildFrame *child = childFrame( sender() );

  assert( child );

  child->m_bCompleted = true;
  child->m_args = KParts::URLArgs();

  if ( complete && parentFrame() == 0 )
    d->m_bPendingChildRedirection = true;

  checkCompleted();
}


khtml::ChildFrame *Frame::childFrame( const QObject *obj )
{
    assert( obj->inherits( "KParts::ReadOnlyPart" ) );
    const ReadOnlyPart *part = static_cast<const ReadOnlyPart *>( obj );

    FrameIt it = d->m_frames.begin();
    FrameIt end = d->m_frames.end();
    for (; it != end; ++it )
      if (static_cast<ReadOnlyPart *>((*it).m_frame) == part)
        return &(*it);

    it = d->m_objects.begin();
    end = d->m_objects.end();
    for (; it != end; ++it )
      if (static_cast<ReadOnlyPart *>((*it).m_frame) == part)
        return &(*it);

    return 0L;
}

Frame *Frame::findFrame( const QString &f )
{

  // ### http://www.w3.org/TR/html4/appendix/notes.html#notes-frames
  ConstFrameIt it = d->m_frames.find( f );
  if ( it == d->m_frames.end() )
  {
    //kdDebug() << "Frame::findFrame frame " << f << " not found" << endl;
    return 0L;
  }
  else {
    KParts::ReadOnlyPart *p = (*it).m_frame;
    if ( p && p->inherits( "Frame" ))
    {
      //kdDebug() << "Frame::findFrame frame " << f << " is a Frame, ok" << endl;
      return (Frame*)p;
    }
    else
    {
      return 0L;
    }
  }
}


bool Frame::frameExists( const QString &frameName )
{
  ConstFrameIt it = d->m_frames.find( frameName );
  if ( it == d->m_frames.end() )
    return false;

  // WABA: We only return true if the child actually has a frame
  // set. Otherwise we might find our preloaded-selve.
  // This happens when we restore the frameset.
  return (!(*it).m_renderer.isNull());
}

Frame *Frame::parentFrame() const
{
  if ( !parent() || !parent()->inherits( "Frame" ) )
    return 0L;

  return (Frame *)parent();
}

int Frame::zoomFactor() const
{
  return d->m_zoomFactor;
}

// ### make the list configurable ?
static const int zoomSizes[] = { 20, 40, 60, 80, 90, 95, 100, 105, 110, 120, 140, 160, 180, 200, 250, 300 };
static const int zoomSizeCount = (sizeof(zoomSizes) / sizeof(int));
static const int minZoom = 20;
static const int maxZoom = 300;

void Frame::slotIncZoom()
{
  int zoomFactor = d->m_zoomFactor;

  if (zoomFactor < maxZoom) {
    // find the entry nearest to the given zoomsizes
    for (int i = 0; i < zoomSizeCount; ++i)
      if (zoomSizes[i] > zoomFactor) {
        zoomFactor = zoomSizes[i];
        break;
      }
    setZoomFactor(zoomFactor);
  }
}

void Frame::slotDecZoom()
{
    int zoomFactor = d->m_zoomFactor;
    if (zoomFactor > minZoom) {
      // find the entry nearest to the given zoomsizes
      for (int i = zoomSizeCount-1; i >= 0; --i)
        if (zoomSizes[i] < zoomFactor) {
          zoomFactor = zoomSizes[i];
          break;
        }
      setZoomFactor(zoomFactor);
    }
}

void Frame::setZoomFactor (int percent)
{
  
  if (d->m_zoomFactor == percent) return;
  d->m_zoomFactor = percent;

  if(d->m_doc) {
    d->m_doc->recalcStyle( NodeImpl::Force );
  }

  ConstFrameIt it = d->m_frames.begin();
  ConstFrameIt end = d->m_frames.end();
  for (; it != end; ++it )
    if (!(*it).m_frame.isNull() && (*it).m_frame->inherits( "Frame" ) ) {
      KParts::ReadOnlyPart* p = ( *it ).m_frame;
      static_cast<Frame*>( p )->setZoomFactor(d->m_zoomFactor);
    }

  if (d->m_doc && d->m_doc->renderer() && d->m_doc->renderer()->needsLayout())
    view()->layout();
}

void Frame::setJSStatusBarText( const QString &text )
{
   d->m_kjsStatusBarText = text;
   emit setStatusBarText( d->m_kjsStatusBarText );
}

void Frame::setJSDefaultStatusBarText( const QString &text )
{
   d->m_kjsDefaultStatusBarText = text;
   emit setStatusBarText( d->m_kjsDefaultStatusBarText );
}

QString Frame::jsStatusBarText() const
{
    return d->m_kjsStatusBarText;
}

QString Frame::jsDefaultStatusBarText() const
{
   return d->m_kjsDefaultStatusBarText;
}

QString Frame::referrer() const
{
   return d->m_referrer;
}

QString Frame::lastModified() const
{
  return d->m_lastModified;
}


void Frame::reparseConfiguration()
{
  setAutoloadImages( d->m_settings->autoLoadImages() );
  if (d->m_doc)
     d->m_doc->docLoader()->setShowAnimations( d->m_settings->showAnimations() );

  d->m_bJScriptEnabled = d->m_settings->isJavaScriptEnabled(m_url.host());
  d->m_bJavaEnabled = d->m_settings->isJavaEnabled(m_url.host());
  d->m_bPluginsEnabled = d->m_settings->isPluginsEnabled(m_url.host());

  QString userStyleSheet = d->m_settings->userStyleSheet();
  if ( !userStyleSheet.isEmpty() )
    setUserStyleSheet( KURL( userStyleSheet ) );
  else
    setUserStyleSheet( QString() );

  if(d->m_doc) d->m_doc->updateStyleSelector();
}

QStringList Frame::frameNames() const
{
  QStringList res;

  ConstFrameIt it = d->m_frames.begin();
  ConstFrameIt end = d->m_frames.end();
  for (; it != end; ++it )
    if (!(*it).m_bPreloaded)
      res += (*it).m_name;

  return res;
}

QPtrList<KParts::ReadOnlyPart> Frame::frames() const
{
  QPtrList<KParts::ReadOnlyPart> res;

  ConstFrameIt it = d->m_frames.begin();
  ConstFrameIt end = d->m_frames.end();
  for (; it != end; ++it )
    if (!(*it).m_bPreloaded)
      res.append((*it).m_frame);

  return res;
}

Frame *Frame::childFrameNamed(const QString &name) const
{
  FrameList::Iterator it = d->m_frames.find(name);
  if (it != d->m_frames.end())
    return static_cast<Frame *>(&*(*it).m_frame);
  return NULL;
}

bool Frame::shouldDragAutoNode(DOM::NodeImpl *node, int x, int y) const
{
    // No KDE impl yet
    return false;
}

void Frame::customEvent( QCustomEvent *event )
{
  if ( khtml::MousePressEvent::test( event ) )
  {
    khtmlMousePressEvent( static_cast<khtml::MousePressEvent *>( event ) );
    return;
  }

  if ( khtml::MouseDoubleClickEvent::test( event ) )
  {
    khtmlMouseDoubleClickEvent( static_cast<khtml::MouseDoubleClickEvent *>( event ) );
    return;
  }

  if ( khtml::MouseMoveEvent::test( event ) )
  {
    khtmlMouseMoveEvent( static_cast<khtml::MouseMoveEvent *>( event ) );
    return;
  }

  if ( khtml::MouseReleaseEvent::test( event ) )
  {
    khtmlMouseReleaseEvent( static_cast<khtml::MouseReleaseEvent *>( event ) );
    return;
  }

  if ( khtml::DrawContentsEvent::test( event ) )
  {
    khtmlDrawContentsEvent( static_cast<khtml::DrawContentsEvent *>( event ) );
    return;
  }

  KParts::ReadOnlyPart::customEvent( event );
}

bool Frame::isPointInsideSelection(int x, int y)
{
    // Treat a collapsed selection like no selection.
    if (!d->m_selection.isRange())
        return false;
    if (!xmlDocImpl()->renderer()) 
        return false;
    
    RenderObject::NodeInfo nodeInfo(true, true);
    xmlDocImpl()->renderer()->layer()->hitTest(nodeInfo, x, y);
    NodeImpl *innerNode = nodeInfo.innerNode();
    if (!innerNode || !innerNode->renderer())
        return false;
    
    Position pos(innerNode->renderer()->positionForCoordinates(x, y).deepEquivalent());
    if (pos.isNull())
        return false;

    NodeImpl *n = d->m_selection.start().node();
    while (n) {
        if (n == pos.node()) {
            if ((n == d->m_selection.start().node() && pos.offset() < d->m_selection.start().offset()) ||
                (n == d->m_selection.end().node() && pos.offset() > d->m_selection.end().offset())) {
                return false;
            }
            return true;
        }
        if (n == d->m_selection.end().node())
            break;
        n = n->traverseNextNode();
    }

   return false;
}

void Frame::selectClosestWordFromMouseEvent(QMouseEvent *mouse, NodeImpl *innerNode, int x, int y)
{
    SelectionController selection;

    if (innerNode && innerNode->renderer() && mouseDownMayStartSelect() && innerNode->renderer()->shouldSelect()) {
        VisiblePosition pos(innerNode->renderer()->positionForCoordinates(x, y));
        if (pos.isNotNull()) {
            selection.moveTo(pos);
            selection.expandUsingGranularity(WORD);
        }
    }
    
    if (selection.isRange()) {
        d->m_selectionGranularity = WORD;
        d->m_beganSelectingText = true;
    }
    
    if (shouldChangeSelection(selection))
        setSelection(selection);
}

void Frame::handleMousePressEventDoubleClick(khtml::MousePressEvent *event)
{
    if (event->qmouseEvent()->button() == LeftButton) {
        if (selection().isRange()) {
            // A double-click when range is already selected
            // should not change the selection.  So, do not call
            // selectClosestWordFromMouseEvent, but do set
            // m_beganSelectingText to prevent khtmlMouseReleaseEvent
            // from setting caret selection.
            d->m_beganSelectingText = true;
        } else {
            selectClosestWordFromMouseEvent(event->qmouseEvent(), event->innerNode(), event->x(), event->y());
        }
    }
}

void Frame::handleMousePressEventTripleClick(khtml::MousePressEvent *event)
{
    QMouseEvent *mouse = event->qmouseEvent();
    NodeImpl *innerNode = event->innerNode();
    
    if (mouse->button() == LeftButton && innerNode && innerNode->renderer() &&
        mouseDownMayStartSelect() && innerNode->renderer()->shouldSelect()) {
        SelectionController selection;
        VisiblePosition pos(innerNode->renderer()->positionForCoordinates(event->x(), event->y()));
        if (pos.isNotNull()) {
            selection.moveTo(pos);
            selection.expandUsingGranularity(PARAGRAPH);
        }
        if (selection.isRange()) {
            d->m_selectionGranularity = PARAGRAPH;
            d->m_beganSelectingText = true;
        }
        
        if (shouldChangeSelection(selection))
            setSelection(selection);
    }
}

void Frame::handleMousePressEventSingleClick(khtml::MousePressEvent *event)
{
    QMouseEvent *mouse = event->qmouseEvent();
    NodeImpl *innerNode = event->innerNode();
    
    if (mouse->button() == LeftButton) {
        if (innerNode && innerNode->renderer() &&
            mouseDownMayStartSelect() && innerNode->renderer()->shouldSelect()) {
            SelectionController sel;
            
            // Extend the selection if the Shift key is down, unless the click is in a link.
            bool extendSelection = (mouse->state() & ShiftButton) && (event->url().isNull());

            // Don't restart the selection when the mouse is pressed on an
            // existing selection so we can allow for text dragging.
            if (!extendSelection && isPointInsideSelection(event->x(), event->y())) {
                return;
            }

            VisiblePosition visiblePos(innerNode->renderer()->positionForCoordinates(event->x(), event->y()));
            if (visiblePos.isNull())
                visiblePos = VisiblePosition(innerNode, innerNode->caretMinOffset(), khtml::DOWNSTREAM);
            Position pos = visiblePos.deepEquivalent();
            
            sel = selection();
            if (extendSelection && sel.isCaretOrRange()) {
                sel.clearModifyBias();
                
                // See <rdar://problem/3668157> REGRESSION (Mail): shift-click deselects when selection 
                // was created right-to-left
                Position start = sel.start();
                short before = RangeImpl::compareBoundaryPoints(pos.node(), pos.offset(), start.node(), start.offset());
                if (before <= 0) {
                    sel.setBaseAndExtent(pos.node(), pos.offset(), sel.end().node(), sel.end().offset());
                } else {
                    sel.setBaseAndExtent(start.node(), start.offset(), pos.node(), pos.offset());
                }

                if (d->m_selectionGranularity != CHARACTER) {
                    sel.expandUsingGranularity(d->m_selectionGranularity);
                }
                d->m_beganSelectingText = true;
            } else {
                sel = SelectionController(visiblePos);
                d->m_selectionGranularity = CHARACTER;
            }
            
            if (shouldChangeSelection(sel))
                setSelection(sel);
        }
    }
}

void Frame::khtmlMousePressEvent(khtml::MousePressEvent *event)
{
    DOM::DOMString url = event->url();
    QMouseEvent *mouse = event->qmouseEvent();
    NodeImpl *innerNode = event->innerNode();

    d->m_mousePressNode = innerNode;
    d->m_dragStartPos = mouse->pos();

    if (!event->url().isNull()) {
        d->m_strSelectedURL = d->m_strSelectedURLTarget = QString::null;
    } else {
        d->m_strSelectedURL = event->url().qstring();
        d->m_strSelectedURLTarget = event->target().qstring();
    }

    if (mouse->button() == LeftButton || mouse->button() == MidButton) {
        d->m_bMousePressed = true;
        d->m_beganSelectingText = false;

        if (mouse->clickCount() == 2) {
            handleMousePressEventDoubleClick(event);
            return;
        } else if (mouse->clickCount() >= 3) {
            handleMousePressEventTripleClick(event);
            return;
        }
        handleMousePressEventSingleClick(event);
    }
}

void Frame::handleMouseMoveEventSelection(khtml::MouseMoveEvent *event)
{
    // Mouse not pressed. Do nothing.
    if (!d->m_bMousePressed)
        return;

    QMouseEvent *mouse = event->qmouseEvent();
    NodeImpl *innerNode = event->innerNode();

    if (mouse->state() != LeftButton || !innerNode || !innerNode->renderer() ||
        !mouseDownMayStartSelect() || !innerNode->renderer()->shouldSelect())
    	return;

    // handle making selection
    VisiblePosition pos(innerNode->renderer()->positionForCoordinates(event->x(), event->y()));

    // Don't modify the selection if we're not on a node.
    if (pos.isNull())
        return;

    // Restart the selection if this is the first mouse move. This work is usually
    // done in khtmlMousePressEvent, but not if the mouse press was on an existing selection.
    SelectionController sel = selection();
    sel.clearModifyBias();
    
    if (!d->m_beganSelectingText) {
        d->m_beganSelectingText = true;
        sel.moveTo(pos);
    }

    sel.setExtent(pos);
    if (d->m_selectionGranularity != CHARACTER)
        sel.expandUsingGranularity(d->m_selectionGranularity);

    if (shouldChangeSelection(sel))
        setSelection(sel);
}

void Frame::khtmlMouseMoveEvent(khtml::MouseMoveEvent *event)
{
    handleMouseMoveEventSelection(event);		
}

void Frame::khtmlMouseReleaseEvent( khtml::MouseReleaseEvent *event )
{
    // Used to prevent mouseMoveEvent from initiating a drag before
    // the mouse is pressed again.
    d->m_bMousePressed = false;
  
    // Clear the selection if the mouse didn't move after the last mouse press.
    // We do this so when clicking on the selection, the selection goes away.
    // However, if we are editing, place the caret.
    if (mouseDownMayStartSelect() && !d->m_beganSelectingText
            && d->m_dragStartPos.x() == event->qmouseEvent()->x()
            && d->m_dragStartPos.y() == event->qmouseEvent()->y()
            && d->m_selection.isRange()) {
        SelectionController selection;
        NodeImpl *node = event->innerNode();
        if (node && node->isContentEditable() && node->renderer()) {
            VisiblePosition pos = node->renderer()->positionForCoordinates(event->x(), event->y());
            selection.moveTo(pos);
        }
        if (shouldChangeSelection(selection))
            setSelection(selection);
    }
    selectFrameElementInParentIfFullySelected();
}

void Frame::selectAll()
{
    if (!d->m_doc)
        return;
    
    NodeImpl *startNode = d->m_selection.start().node();
    NodeImpl *root = startNode && startNode->isContentEditable() ? startNode->rootEditableElement() : d->m_doc->documentElement();
    
    selectContentsOfNode(root);
    selectFrameElementInParentIfFullySelected();
}

bool Frame::selectContentsOfNode(NodeImpl* node)
{
    SelectionController sel = SelectionController(Position(node, 0), khtml::DOWNSTREAM, Position(node, node->maxDeepOffset()), khtml::DOWNSTREAM);    
    if (shouldChangeSelection(sel)) {
        setSelection(sel);
        return true;
    }
    return false;
}

bool Frame::shouldChangeSelection(const SelectionController &newselection) const
{
    return Mac(this)->shouldChangeSelection(d->m_selection, newselection, newselection.startAffinity(), false);
}

bool Frame::shouldBeginEditing(const RangeImpl *range) const
{
    return true;
}

bool Frame::shouldEndEditing(const RangeImpl *range) const
{
    return true;
}

bool Frame::isContentEditable() const 
{
    if (!d->m_doc)
        return false;
    return d->m_doc->inDesignMode();
}

EditCommandPtr Frame::lastEditCommand()
{
    return d->m_lastEditCommand;
}

void Frame::appliedEditing(EditCommandPtr &cmd)
{
    if (shouldChangeSelection(cmd.endingSelection())) {
        setSelection(cmd.endingSelection(), false);
    }

    // Now set the typing style from the command. Clear it when done.
    // This helps make the case work where you completely delete a piece
    // of styled text and then type a character immediately after.
    // That new character needs to take on the style of the just-deleted text.
    // FIXME: Improve typing style.
    // See this bug: <rdar://problem/3769899> Implementation of typing style needs improvement
    if (cmd.typingStyle()) {
        setTypingStyle(cmd.typingStyle());
        cmd.setTypingStyle(0);
    }

    // Command will be equal to last edit command only in the case of typing
    if (d->m_lastEditCommand == cmd) {
        assert(cmd.isTypingCommand());
    }
    else {
        // Only register a new undo command if the command passed in is
        // different from the last command
        Mac(this)->registerCommandForUndo(cmd);
        d->m_lastEditCommand = cmd;
    }
    Mac(this)->respondToChangedContents();
}

void Frame::unappliedEditing(EditCommandPtr &cmd)
{
    if (shouldChangeSelection(cmd.startingSelection())) {
        setSelection(cmd.startingSelection(), true);
    }
    Mac(this)->registerCommandForRedo(cmd);
    Mac(this)->respondToChangedContents();
    d->m_lastEditCommand = EditCommandPtr::emptyCommand();
}

void Frame::reappliedEditing(EditCommandPtr &cmd)
{
    if (shouldChangeSelection(cmd.endingSelection())) {
        setSelection(cmd.endingSelection(), true);
    }
    Mac(this)->registerCommandForUndo(cmd);
    Mac(this)->respondToChangedContents();
    d->m_lastEditCommand = EditCommandPtr::emptyCommand();
}

CSSMutableStyleDeclarationImpl *Frame::typingStyle() const
{
    return d->m_typingStyle;
}

void Frame::setTypingStyle(CSSMutableStyleDeclarationImpl *style)
{
    if (d->m_typingStyle == style)
        return;
        
    CSSMutableStyleDeclarationImpl *old = d->m_typingStyle;
    d->m_typingStyle = style;
    if (d->m_typingStyle)
        d->m_typingStyle->ref();
    if (old)
        old->deref();
}

void Frame::clearTypingStyle()
{
    setTypingStyle(0);
}


QVariant Frame::executeScript(QString filename, int baseLine, NodeImpl *n, const QString &script)
{
#ifdef KJS_VERBOSE
  kdDebug(6070) << "executeScript: filename=" << filename << " baseLine=" << baseLine << " script=" << script << endl;
#endif
  KJSProxyImpl *proxy = jScript();

  if (!proxy)
    return QVariant();
  QVariant ret = proxy->evaluate(filename,baseLine,script, n );
  DocumentImpl::updateDocumentsRendering();
  return ret;
}

void Frame::slotPartRemoved(KParts::Part *part)
{
    if (part == d->m_activeFrame)
        d->m_activeFrame = 0;
}

DOM::EventListener *Frame::createHTMLEventListener(const DOMString& code, NodeImpl *node)
{
    if (KJSProxyImpl *proxy = jScript())
        return proxy->createHTMLEventHandler(code, node);
    return 0;
}

Frame *Frame::opener()
{
    return d->m_opener;
}

void Frame::setOpener(Frame *_opener)
{
    d->m_opener = _opener;
}

bool Frame::openedByJS()
{
    return d->m_openedByJS;
}

void Frame::setOpenedByJS(bool _openedByJS)
{
    d->m_openedByJS = _openedByJS;
}

void Frame::preloadStyleSheet(const QString &url, const QString &stylesheet)
{
    khtml::Cache::preloadStyleSheet(url, stylesheet);
}

void Frame::preloadScript(const QString &url, const QString &script)
{
    khtml::Cache::preloadScript(url, script);
}


bool Frame::restored() const
{
  return d->m_restored;
}

void Frame::incrementFrameCount()
{
  frameCount++;
  if (parentFrame()) {
    parentFrame()->incrementFrameCount();
  }
}

void Frame::decrementFrameCount()
{
  frameCount--;
  if (parentFrame()) {
    parentFrame()->decrementFrameCount();
  }
}

int Frame::topLevelFrameCount()
{
  if (parentFrame()) {
    return parentFrame()->topLevelFrameCount();
  }

  return frameCount;
}

bool Frame::tabsToLinks() const
{
    return true;
}

bool Frame::tabsToAllControls() const
{
    return true;
}

void Frame::copyToPasteboard()
{
    Mac(this)->issueCopyCommand();
}

void Frame::cutToPasteboard()
{
    Mac(this)->issueCutCommand();
}

void Frame::pasteFromPasteboard()
{
    Mac(this)->issuePasteCommand();
}

void Frame::pasteAndMatchStyle()
{
    Mac(this)->issuePasteAndMatchStyleCommand();
}

void Frame::transpose()
{
    Mac(this)->issueTransposeCommand();
}

void Frame::redo()
{
    Mac(this)->issueRedoCommand();
}

void Frame::undo()
{
    Mac(this)->issueUndoCommand();
}


void Frame::computeAndSetTypingStyle(CSSStyleDeclarationImpl *style, EditAction editingAction)
{
    if (!style || style->length() == 0) {
        clearTypingStyle();
        return;
    }

    // Calculate the current typing style.
    CSSMutableStyleDeclarationImpl *mutableStyle = style->makeMutable();
    mutableStyle->ref();
    if (typingStyle()) {
        typingStyle()->merge(mutableStyle);
        mutableStyle->deref();
        mutableStyle = typingStyle();
        mutableStyle->ref();
    }

    NodeImpl *node = VisiblePosition(selection().start(), selection().startAffinity()).deepEquivalent().node();
    CSSComputedStyleDeclarationImpl computedStyle(node);
    computedStyle.diff(mutableStyle);
    
    // Handle block styles, substracting these from the typing style.
    CSSMutableStyleDeclarationImpl *blockStyle = mutableStyle->copyBlockProperties();
    blockStyle->ref();
    blockStyle->diff(mutableStyle);
    if (xmlDocImpl() && blockStyle->length() > 0) {
        EditCommandPtr cmd(new ApplyStyleCommand(xmlDocImpl(), blockStyle, editingAction));
        cmd.apply();
    }
    blockStyle->deref();
    
    // Set the remaining style as the typing style.
    setTypingStyle(mutableStyle);
    mutableStyle->deref();
}

void Frame::applyStyle(CSSStyleDeclarationImpl *style, EditAction editingAction)
{
    switch (selection().state()) {
        case SelectionController::NONE:
            // do nothing
            break;
        case SelectionController::CARET: {
            computeAndSetTypingStyle(style, editingAction);
            break;
        }
        case SelectionController::RANGE:
            if (xmlDocImpl() && style) {
                EditCommandPtr cmd(new ApplyStyleCommand(xmlDocImpl(), style, editingAction));
                cmd.apply();
            }
            break;
    }
}

void Frame::applyParagraphStyle(CSSStyleDeclarationImpl *style, EditAction editingAction)
{
    switch (selection().state()) {
        case SelectionController::NONE:
            // do nothing
            break;
        case SelectionController::CARET:
        case SelectionController::RANGE:
            if (xmlDocImpl() && style) {
                EditCommandPtr cmd(new ApplyStyleCommand(xmlDocImpl(), style, editingAction, ApplyStyleCommand::ForceBlockProperties));
                cmd.apply();
            }
            break;
    }
}

static void updateState(CSSMutableStyleDeclarationImpl *desiredStyle, CSSComputedStyleDeclarationImpl *computedStyle, bool &atStart, Frame::TriState &state)
{
    QValueListConstIterator<CSSProperty> end;
    for (QValueListConstIterator<CSSProperty> it = desiredStyle->valuesIterator(); it != end; ++it) {
        int propertyID = (*it).id();
        DOMString desiredProperty = desiredStyle->getPropertyValue(propertyID);
        DOMString computedProperty = computedStyle->getPropertyValue(propertyID);
        Frame::TriState propertyState = equalIgnoringCase(desiredProperty, computedProperty)
            ? Frame::trueTriState : Frame::falseTriState;
        if (atStart) {
            state = propertyState;
            atStart = false;
        } else if (state != propertyState) {
            state = Frame::mixedTriState;
            break;
        }
    }
}

Frame::TriState Frame::selectionHasStyle(CSSStyleDeclarationImpl *style) const
{
    bool atStart = true;
    TriState state = falseTriState;

    CSSMutableStyleDeclarationImpl *mutableStyle = style->makeMutable();
    RefPtr<CSSStyleDeclarationImpl> protectQueryStyle(mutableStyle);

    if (!d->m_selection.isRange()) {
        NodeImpl *nodeToRemove;
        CSSComputedStyleDeclarationImpl *selectionStyle = selectionComputedStyle(nodeToRemove);
        if (!selectionStyle)
            return falseTriState;
        selectionStyle->ref();
        updateState(mutableStyle, selectionStyle, atStart, state);
        selectionStyle->deref();
        if (nodeToRemove) {
            int exceptionCode = 0;
            nodeToRemove->remove(exceptionCode);
            assert(exceptionCode == 0);
        }
    } else {
        for (NodeImpl *node = d->m_selection.start().node(); node; node = node->traverseNextNode()) {
            CSSComputedStyleDeclarationImpl *computedStyle = new CSSComputedStyleDeclarationImpl(node);
            if (computedStyle) {
                computedStyle->ref();
                updateState(mutableStyle, computedStyle, atStart, state);
                computedStyle->deref();
            }
            if (state == mixedTriState)
                break;
            if (node == d->m_selection.end().node())
                break;
        }
    }

    return state;
}

bool Frame::selectionStartHasStyle(CSSStyleDeclarationImpl *style) const
{
    NodeImpl *nodeToRemove;
    CSSStyleDeclarationImpl *selectionStyle = selectionComputedStyle(nodeToRemove);
    if (!selectionStyle)
        return false;

    CSSMutableStyleDeclarationImpl *mutableStyle = style->makeMutable();

    RefPtr<CSSStyleDeclarationImpl> protectSelectionStyle(selectionStyle);
    RefPtr<CSSStyleDeclarationImpl> protectQueryStyle(mutableStyle);

    bool match = true;
    QValueListConstIterator<CSSProperty> end;
    for (QValueListConstIterator<CSSProperty> it = mutableStyle->valuesIterator(); it != end; ++it) {
        int propertyID = (*it).id();
        if (!equalIgnoringCase(mutableStyle->getPropertyValue(propertyID), selectionStyle->getPropertyValue(propertyID))) {
            match = false;
            break;
        }
    }

    if (nodeToRemove) {
        int exceptionCode = 0;
        nodeToRemove->remove(exceptionCode);
        assert(exceptionCode == 0);
    }

    return match;
}

DOMString Frame::selectionStartStylePropertyValue(int stylePropertyID) const
{
    NodeImpl *nodeToRemove;
    CSSStyleDeclarationImpl *selectionStyle = selectionComputedStyle(nodeToRemove);
    if (!selectionStyle)
        return DOMString();

    selectionStyle->ref();
    DOMString value = selectionStyle->getPropertyValue(stylePropertyID);
    selectionStyle->deref();

    if (nodeToRemove) {
        int exceptionCode = 0;
        nodeToRemove->remove(exceptionCode);
        assert(exceptionCode == 0);
    }

    return value;
}

CSSComputedStyleDeclarationImpl *Frame::selectionComputedStyle(NodeImpl *&nodeToRemove) const
{
    nodeToRemove = 0;

    if (!xmlDocImpl())
        return 0;

    if (d->m_selection.isNone())
        return 0;

    RefPtr<RangeImpl> range(d->m_selection.toRange());
    Position pos = range->editingStartPosition();

    ElementImpl *elem = pos.element();
    if (!elem)
        return 0;
    
    ElementImpl *styleElement = elem;
    int exceptionCode = 0;

    if (d->m_typingStyle) {
        styleElement = xmlDocImpl()->createElementNS(xhtmlNamespaceURI, "span", exceptionCode);
        assert(exceptionCode == 0);

        styleElement->ref();
        
        styleElement->setAttribute(styleAttr, d->m_typingStyle->cssText().impl(), exceptionCode);
        assert(exceptionCode == 0);
        
        TextImpl *text = xmlDocImpl()->createEditingTextNode("");
        styleElement->appendChild(text, exceptionCode);
        assert(exceptionCode == 0);

        if (elem->renderer() && elem->renderer()->canHaveChildren()) {
            elem->appendChild(styleElement, exceptionCode);
        } else {
            NodeImpl *parent = elem->parent();
            NodeImpl *next = elem->nextSibling();

            if (next) {
                parent->insertBefore(styleElement, next, exceptionCode);
            } else {
                parent->appendChild(styleElement, exceptionCode);
            }
        }
        assert(exceptionCode == 0);

        styleElement->deref();

        nodeToRemove = styleElement;
    }

    return new CSSComputedStyleDeclarationImpl(styleElement);
}

static CSSMutableStyleDeclarationImpl *editingStyle()
{
    static CSSMutableStyleDeclarationImpl *editingStyle = 0;
    if (!editingStyle) {
        editingStyle = new CSSMutableStyleDeclarationImpl;
        int exceptionCode;
        editingStyle->setCssText("word-wrap: break-word; -khtml-nbsp-mode: space; -khtml-line-break: after-white-space;", exceptionCode);
    }
    return editingStyle;
}

void Frame::applyEditingStyleToBodyElement() const
{
    if (!d->m_doc)
        return;
        
    RefPtr<NodeListImpl> list = d->m_doc->getElementsByTagName("body");
    unsigned len = list->length();
    for (unsigned i = 0; i < len; i++) {
        applyEditingStyleToElement(static_cast<ElementImpl *>(list->item(i)));    
    }
}

void Frame::removeEditingStyleFromBodyElement() const
{
    if (!d->m_doc)
        return;
        
    RefPtr<NodeListImpl> list = d->m_doc->getElementsByTagName("body");
    unsigned len = list->length();
    for (unsigned i = 0; i < len; i++) {
        removeEditingStyleFromElement(static_cast<ElementImpl *>(list->item(i)));    
    }
}

void Frame::applyEditingStyleToElement(ElementImpl *element) const
{
    if (!element || !element->isHTMLElement())
        return;
        
    RenderObject *renderer = element->renderer();
    if (!renderer || !renderer->isBlockFlow())
        return;
    
    CSSMutableStyleDeclarationImpl *currentStyle = static_cast<HTMLElementImpl *>(element)->getInlineStyleDecl();
    CSSMutableStyleDeclarationImpl *mergeStyle = editingStyle();
    if (mergeStyle) {
        currentStyle->merge(mergeStyle);
        element->setAttribute(styleAttr, currentStyle->cssText());
    }
}

void Frame::removeEditingStyleFromElement(ElementImpl *element) const
{
    if (!element || !element->isHTMLElement())
        return;
        
    RenderObject *renderer = element->renderer();
    if (!renderer || !renderer->isBlockFlow())
        return;
    
    CSSMutableStyleDeclarationImpl *currentStyle = static_cast<HTMLElementImpl *>(element)->getInlineStyleDecl();
    bool changed = false;
    changed |= !currentStyle->removeProperty(CSS_PROP_WORD_WRAP, false).isNull();
    changed |= !currentStyle->removeProperty(CSS_PROP__KHTML_NBSP_MODE, false).isNull();
    changed |= !currentStyle->removeProperty(CSS_PROP__KHTML_LINE_BREAK, false).isNull();
    if (changed)
        currentStyle->setChanged();

    element->setAttribute(styleAttr, currentStyle->cssText());
}


bool Frame::isCharacterSmartReplaceExempt(const QChar &, bool)
{
    // no smart replace
    return true;
}

void Frame::connectChild(const khtml::ChildFrame *child) const
{
    ReadOnlyPart *part = child->m_frame;
    if (part && child->m_type != ChildFrame::Object)
    {
        connect( part, SIGNAL( started( KIO::Job *) ),
                 this, SLOT( slotChildStarted( KIO::Job *) ) );
        connect( part, SIGNAL( completed() ),
                 this, SLOT( slotChildCompleted() ) );
        connect( part, SIGNAL( completed(bool) ),
                 this, SLOT( slotChildCompleted(bool) ) );
        connect( part, SIGNAL( setStatusBarText( const QString & ) ),
                 this, SIGNAL( setStatusBarText( const QString & ) ) );
        connect( this, SIGNAL( completed() ),
                 part, SLOT( slotParentCompleted() ) );
        connect( this, SIGNAL( completed(bool) ),
                 part, SLOT( slotParentCompleted() ) );
    }
}

void Frame::disconnectChild(const khtml::ChildFrame *child) const
{
    ReadOnlyPart *part = child->m_frame;
    if (part && child->m_type != ChildFrame::Object)
    {
        disconnect( part, SIGNAL( started( KIO::Job *) ),
                    this, SLOT( slotChildStarted( KIO::Job *) ) );
        disconnect( part, SIGNAL( completed() ),
                    this, SLOT( slotChildCompleted() ) );
        disconnect( part, SIGNAL( completed(bool) ),
                    this, SLOT( slotChildCompleted(bool) ) );
        disconnect( part, SIGNAL( setStatusBarText( const QString & ) ),
                    this, SIGNAL( setStatusBarText( const QString & ) ) );
        disconnect( this, SIGNAL( completed() ),
                    part, SLOT( slotParentCompleted() ) );
        disconnect( this, SIGNAL( completed(bool) ),
                    part, SLOT( slotParentCompleted() ) );
    }
}

void Frame::keepAlive()
{
    if (d->m_lifeSupportTimer.isActive())
        return;
    ref();
    d->m_lifeSupportTimer.start(0, true);
}

void Frame::slotEndLifeSupport()
{
    d->m_lifeSupportTimer.stop();
    deref();
}

// Workaround for the fact that it's hard to delete a frame.
// Call this after doing user-triggered selections to make it easy to delete the frame you entirely selected.
// Can't do this implicitly as part of every setSelection call because in some contexts it might not be good
// for the focus to move to another frame. So instead we call it from places where we are selecting with the
// mouse or the keyboard after setting the selection.
void Frame::selectFrameElementInParentIfFullySelected()
{
    // Find the parent frame; if there is none, then we have nothing to do.
    Frame *parent = parentFrame();
    if (!parent)
        return;
    KHTMLView *parentView = parent->view();
    if (!parentView)
        return;

    // Check if the selection contains the entire frame contents; if not, then there is nothing to do.
    if (!d->m_selection.isRange())
        return;
    if (!isStartOfDocument(VisiblePosition(d->m_selection.start(), d->m_selection.startAffinity())))
        return;
    if (!isEndOfDocument(VisiblePosition(d->m_selection.end(), d->m_selection.endAffinity())))
        return;

    // Get to the <iframe> or <frame> (or even <object>) element in the parent frame.
    DocumentImpl *document = xmlDocImpl();
    if (!document)
        return;
    ElementImpl *ownerElement = document->ownerElement();
    if (!ownerElement)
        return;
    NodeImpl *ownerElementParent = ownerElement->parentNode();
    if (!ownerElementParent)
        return;

    // Create compute positions before and after the element.
    unsigned ownerElementNodeIndex = ownerElement->nodeIndex();
    VisiblePosition beforeOwnerElement(VisiblePosition(ownerElementParent, ownerElementNodeIndex, khtml::SEL_DEFAULT_AFFINITY));
    VisiblePosition afterOwnerElement(VisiblePosition(ownerElementParent, ownerElementNodeIndex + 1, khtml::VP_UPSTREAM_IF_POSSIBLE));

    // Focus on the parent frame, and then select from before this element to after.
    if (parent->shouldChangeSelection(SelectionController(beforeOwnerElement, afterOwnerElement))) {
        parentView->setFocus();
        parent->setSelection(SelectionController(beforeOwnerElement, afterOwnerElement));
    }
}

void Frame::handleFallbackContent()
{
    Frame *parent = parentFrame();
    if (!parent)
        return;
    ChildFrame *childFrame = parent->childFrame(this);
    if (!childFrame || childFrame->m_type != ChildFrame::Object)
        return;
    khtml::RenderPart *renderPart = childFrame->m_renderer;
    if (!renderPart)
        return;
    NodeImpl *node = renderPart->element();
    if (!node || !node->hasTagName(objectTag))
        return;
    static_cast<HTMLObjectElementImpl *>(node)->renderFallbackContent();
}
