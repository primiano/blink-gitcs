// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2003 Apple Computer, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "xmlhttprequest.h"
#include "xmlhttprequest.lut.h"
#include "kjs_window.h"
#include "kjs_events.h"

#include "dom/dom_string.h"
#include "misc/loader.h"
#include "html/html_documentimpl.h"
#include "xml/dom2_eventsimpl.h"

#include "khtml_part.h"
#include "khtmlview.h"

#include <kdebug.h>
#include <kio/job.h>
#include <qobject.h>

#ifdef APPLE_CHANGES
#include "KWQLoader.h"
#endif

using namespace KJS;
using khtml::Decoder;

////////////////////// XMLHttpRequest Object ////////////////////////

/* Source for XMLHttpRequestProtoTable.
@begin XMLHttpRequestProtoTable 7
  abort			XMLHttpRequest::Abort			DontDelete|Function 0
  getAllResponseHeaders	XMLHttpRequest::GetAllResponseHeaders	DontDelete|Function 0
  getResponseHeader	XMLHttpRequest::GetResponseHeader	DontDelete|Function 1
  open			XMLHttpRequest::Open			DontDelete|Function 5
  send			XMLHttpRequest::Send			DontDelete|Function 1
  setRequestHeader	XMLHttpRequest::SetRequestHeader	DontDelete|Function 2
@end
*/
DEFINE_PROTOTYPE("XMLHttpRequest",XMLHttpRequestProto)
IMPLEMENT_PROTOFUNC(XMLHttpRequestProtoFunc)
IMPLEMENT_PROTOTYPE(XMLHttpRequestProto,XMLHttpRequestProtoFunc)

namespace KJS {

XMLHttpRequestQObject::XMLHttpRequestQObject(XMLHttpRequest *_jsObject) 
{
  jsObject = _jsObject; 
}

#if APPLE_CHANGES
void XMLHttpRequestQObject::slotData( KIO::Job* job, const char *data, int size )
{
  jsObject->slotData(job, data, size);
}
#else
void XMLHttpRequestQObject::slotData( KIO::Job* job, const QByteArray &data )
{
  jsObject->slotData(job, data);
}
#endif

void XMLHttpRequestQObject::slotFinished( KIO::Job* job )
{
  jsObject->slotFinished(job); 
}

void XMLHttpRequestQObject::slotRedirection( KIO::Job* job, const KURL& url)
{ 
  jsObject->slotRedirection( job, url ); 
}

XMLHttpRequestConstructorImp::XMLHttpRequestConstructorImp(ExecState *, const DOM::Document &d)
    : ObjectImp(), doc(d)
{
}

bool XMLHttpRequestConstructorImp::implementsConstruct() const
{
  return true;
}

Object XMLHttpRequestConstructorImp::construct(ExecState *exec, const List &)
{
  return Object(new XMLHttpRequest(exec, doc));
}

const ClassInfo XMLHttpRequest::info = { "XMLHttpRequest", 0, &XMLHttpRequestTable, 0 };

/* Source for XMLHttpRequestTable.
@begin XMLHttpRequestTable 6
  readyState		XMLHttpRequest::ReadyState		DontDelete|ReadOnly
  responseText		XMLHttpRequest::ResponseText		DontDelete|ReadOnly
  responseXML		XMLHttpRequest::ResponseXML		DontDelete|ReadOnly
  status		XMLHttpRequest::Status			DontDelete|ReadOnly
  statusText		XMLHttpRequest::StatusText		DontDelete|ReadOnly
  onreadystatechange	XMLHttpRequest::Onreadystatechange	DontDelete
@end
*/

Value XMLHttpRequest::tryGet(ExecState *exec, const Identifier &propertyName) const
{
  return DOMObjectLookupGetValue<XMLHttpRequest,DOMObject>(exec, propertyName, &XMLHttpRequestTable, this);
}

Value XMLHttpRequest::getValueProperty(ExecState *, int token) const
{
  switch (token) {
  case ReadyState:
    return Number(state);
  case ResponseText:
    return getStringOrNull(DOM::DOMString(response));
  case ResponseXML:
    return Undefined();
  case Status:
    return Undefined();
  case StatusText:
    return Undefined();
  case Onreadystatechange:
    if (onReadyStateChangeListener && onReadyStateChangeListener->listenerObjImp()) {
      return onReadyStateChangeListener->listenerObj();
    } else {
      return Null();
    }
  default:
    kdWarning() << "XMLHttpRequest::getValueProperty unhandled token " << token << endl;
    return Value();
  }
}

void XMLHttpRequest::tryPut(ExecState *exec, const Identifier &propertyName, const Value& value, int attr)
{
  DOMObjectLookupPut<XMLHttpRequest,DOMObject>(exec, propertyName, value, attr, &XMLHttpRequestTable, this );
}

void XMLHttpRequest::putValue(ExecState *exec, int token, const Value& value, int /*attr*/)
{
  switch(token) {
  case Onreadystatechange:
    onReadyStateChangeListener = Window::retrieveActive(exec)->getJSEventListener(value, true);
    if (onReadyStateChangeListener) onReadyStateChangeListener->ref();
    break;
  default:
    kdWarning() << "HTMLDocument::putValue unhandled token " << token << endl;
  }
}

XMLHttpRequest::XMLHttpRequest(ExecState *exec, const DOM::Document &d)
  : DOMObject(XMLHttpRequestProto::self(exec)),
    qObject(new XMLHttpRequestQObject(this)),
    doc(static_cast<DOM::DocumentImpl*>(d.handle())),
    async(true),
    job(0),
    state(Uninitialized),
    onReadyStateChangeListener(0),
    decoder(0)
{
}

XMLHttpRequest::~XMLHttpRequest()
{
  if (decoder) {
    decoder->deref();
  }
}

void XMLHttpRequest::changeState(XMLHttpRequestState newState)
{
  if (state != newState) {
    state = newState;
    
    if (onReadyStateChangeListener != 0) {
      DOM::Event ev = doc->view()->part()->document().createEvent("HTMLEvents");
      ev.initEvent("readystatechange", true, true);
      onReadyStateChangeListener->handleEvent(ev, true);
    }
  }
}

void XMLHttpRequest::open(const QString& _method, const KURL& _url, bool _async)
{
  method = _method;
  url = _url;
  async = _async;

  changeState(Loading);
}

void XMLHttpRequest::send(const QString& _body)
{
  if (method.lower() == "post" && (url.protocol().lower() == "http" || url.protocol().lower() == "https") ) {
      // FIXME: determine post encoding correctly by looking in headers for charset
      job = KIO::http_post( url, QCString(_body.utf8()), false );
  }
  else
  {
     job = KIO::get( url, false, false );
  }

  qObject->connect( job, SIGNAL( result( KIO::Job* ) ),
		    SLOT( slotFinished( KIO::Job* ) ) );
#if APPLE_CHANGES
  qObject->connect( job, SIGNAL( data( KIO::Job*, const char*, int ) ),
		    SLOT( slotData( KIO::Job*, const char*, int ) ) );
#else
  qObject->connect( job, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
		    SLOT( slotData( KIO::Job*, const QByteArray& ) ) );
#endif
  qObject->connect( job, SIGNAL(redirection(KIO::Job*, const KURL& ) ),
		    SLOT( slotRedirection(KIO::Job*, const KURL&) ) );

#ifdef APPLE_CHANGES
  KWQServeRequest(khtml::Cache::loader(), doc->docLoader(), job);
#else 
  KIO::Scheduler::scheduleJob( job );
#endif
}

void XMLHttpRequest::abort()
{
  if (job) {
    job->kill();
    job = 0;
  }
}

void XMLHttpRequest::slotFinished(KIO::Job *)
{
  if (decoder) {
    response += decoder->flush();
  }

  changeState(Completed);
  job = 0;
  
  if (decoder) {
    decoder->deref();
    decoder = 0;
  }
}

void XMLHttpRequest::slotRedirection(KIO::Job*, const KURL& url)
{
}

#if APPLE_CHANGES
void XMLHttpRequest::slotData( KIO::Job* _job, const char *data, int len )
#else
void XMLHttpRequest::slotData(KIO::Job* _job, const QByteArray &_data)
#endif
{
  if (state < Loaded) {
    // handle http headers here
    changeState(Loaded);
  }
  
#if !APPLE_CHANGES
  const char *data = (const char *)_data.data();
  int len = (int)_data.size();
#endif

  if ( decoder == NULL ) {
    decoder = new Decoder;
    if (!encoding.isNull())
      decoder->setEncoding(encoding.latin1(), Decoder::EncodingFromHTTPHeader);
    else {
      // FIXME: Inherit the default encoding from the parent document?
    }
  }
  if (len == 0)
    return;

  if (len == -1)
    len = strlen(data);

  QString decoded = decoder->decode(data, len);

  response += decoded;

  if (job != 0) {
    changeState(Interactive);
  }
}

Value XMLHttpRequestProtoFunc::tryCall(ExecState *exec, Object &thisObj, const List &args)
{
  if (!thisObj.inherits(&XMLHttpRequest::info)) {
    Object err = Error::create(exec,TypeError);
    exec->setException(err);
    return err;
  }

  XMLHttpRequest *request = static_cast<XMLHttpRequest *>(thisObj.imp());

  switch (id) {
  case XMLHttpRequest::Abort:
    request->abort();
    return Undefined();
  case XMLHttpRequest::GetAllResponseHeaders:
    return Undefined();
  case XMLHttpRequest::GetResponseHeader:
    return Undefined();
  case XMLHttpRequest::Open: 
    {
      if (args.size() < 2 || args.size() > 5) {
	return Undefined();
      }
    
      if (request->state != Uninitialized) {
	return Undefined();
      }

      QString method = args[0].toString(exec).qstring();
      KURL url = KURL(Window::retrieveActive(exec)->part()->htmlDocument().completeURL(args[1].toString(exec).qstring()).string());

      bool async = true;
      if (args.size() >= 3) {
	async = args[2].toBoolean(exec);
      }
    
      if (args.size() >= 4) {
	url.setUser(args[3].toString(exec).qstring());
      }
      
      if (args.size() >= 5) {
	url.setPass(args[4].toString(exec).qstring());
      }

      request->open(method, url, async);

      return Undefined();
    }
  case XMLHttpRequest::Send:
    {
      if (args.size() > 1) {
	return Undefined();
      }

      if (request->state != Loading) {
	return Undefined();
      }

      QString body;

      if (args.size() >= 1) {
	// serializing document not handled yet
	if (!args[0].toObject(exec).inherits(&DOMDocument::info)) {
	  body = args[0].toString(exec).qstring();
	}
      }

      request->send(body);

      return Undefined();
    }
  case XMLHttpRequest::SetRequestHeader:
    return Undefined();
  }

  return Undefined();
}

}; // end namespace
