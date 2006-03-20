/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2004, 2006 Apple Computer, Inc.
 *  Copyright (C) 2005, 2006 Alexey Proskuryakov <ap@nypop.com>
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

#include "config.h"
#include "xmlhttprequest.h"

#include "Cache.h"
#include "DOMImplementation.h"
#include "EventListener.h"
#include "EventNames.h"
#include "KWQLoader.h"
#include "dom2_eventsimpl.h"
#include "PlatformString.h"
#include "formdata.h"
#include "HTMLDocument.h"
#include "kjs_binding.h"
#include "TransferJob.h"
#include <kjs/protect.h>
#include <qregexp.h>
#include "TextEncoding.h"

using namespace KIO;

namespace WebCore {

using namespace EventNames;

typedef HashSet<XMLHttpRequest*> RequestsSet;

static HashMap<Document*, RequestsSet*>& requestsByDocument()
{
    static HashMap<Document*, RequestsSet*> map;
    return map;
}

static void addToRequestsByDocument(Document* doc, XMLHttpRequest* req)
{
    ASSERT(doc);
    ASSERT(req);

    RequestsSet* requests = requestsByDocument().get(doc);
    if (!requests) {
        requests = new RequestsSet;
        requestsByDocument().set(doc, requests);
    }

    ASSERT(!requests->contains(req));
    requests->add(req);
}

static void removeFromRequestsByDocument(Document* doc, XMLHttpRequest* req)
{
    ASSERT(doc);
    ASSERT(req);

    RequestsSet* requests = requestsByDocument().get(doc);
    ASSERT(requests);
    ASSERT(requests->contains(req));
    requests->remove(req);
    if (requests->isEmpty()) {
        requestsByDocument().remove(doc);
        delete requests;
    }
}

static inline DeprecatedString getMIMEType(const DeprecatedString& contentTypeString)
{
    return DeprecatedStringList::split(";", contentTypeString, true)[0].stripWhiteSpace();
}

static DeprecatedString getCharset(const DeprecatedString& contentTypeString)
{
    int pos = 0;
    int length = (int)contentTypeString.length();
    
    while (pos < length) {
        pos = contentTypeString.find("charset", pos, false);
        if (pos <= 0)
            return DeprecatedString();
        
        // is what we found a beginning of a word?
        if (contentTypeString[pos-1] > ' ' && contentTypeString[pos-1] != ';') {
            pos += 7;
            continue;
        }
        
        pos += 7;

        // skip whitespace
        while (pos != length && contentTypeString[pos] <= ' ')
            ++pos;
    
        if (contentTypeString[pos++] != '=') // this "charset" substring wasn't a parameter name, but there may be others
            continue;

        while (pos != length && (contentTypeString[pos] <= ' ' || contentTypeString[pos] == '"' || contentTypeString[pos] == '\''))
            ++pos;

        // we don't handle spaces within quoted parameter values, because charset names cannot have any
        int endpos = pos;
        while (pos != length && contentTypeString[endpos] > ' ' && contentTypeString[endpos] != '"' && contentTypeString[endpos] != '\'' && contentTypeString[endpos] != ';')
            ++endpos;
    
        return contentTypeString.mid(pos, endpos-pos);
    }
    
    return DeprecatedString();
}

XMLHttpRequestState XMLHttpRequest::getReadyState() const
{
    return state;
}

String XMLHttpRequest::getResponseText() const
{
    return response;
}

Document* XMLHttpRequest::getResponseXML() const
{
    if (state != Completed)
        return 0;

    if (!createdDocument) {
        if (responseIsXML()) {
            responseXML = doc->implementation()->createDocument();
            responseXML->open();
            responseXML->write(response);
            responseXML->finishParsing();
            responseXML->close();
        }
        createdDocument = true;
    }

    return responseXML.get();
}

EventListener* XMLHttpRequest::onReadyStateChangeListener() const
{
    return m_onReadyStateChangeListener.get();
}

void XMLHttpRequest::setOnReadyStateChangeListener(EventListener* eventListener)
{
    m_onReadyStateChangeListener = eventListener;
}

EventListener* XMLHttpRequest::onLoadListener() const
{
    return m_onLoadListener.get();
}

void XMLHttpRequest::setOnLoadListener(EventListener* eventListener)
{
    m_onLoadListener = eventListener;
}

XMLHttpRequest::XMLHttpRequest(Document *d)
    : doc(d)
    , async(true)
    , job(0)
    , state(Uninitialized)
    , createdDocument(false)
    , aborted(false)
{
    ASSERT(doc);
    addToRequestsByDocument(doc, this);
}

XMLHttpRequest::~XMLHttpRequest()
{
    if (doc)
        removeFromRequestsByDocument(doc, this);
}

void XMLHttpRequest::changeState(XMLHttpRequestState newState)
{
    if (state != newState) {
        state = newState;
        callReadyStateChangeListener();
    }
}

void XMLHttpRequest::callReadyStateChangeListener()
{
    if (doc && doc->frame() && m_onReadyStateChangeListener) {
        ExceptionCode ec;
        RefPtr<Event> ev = doc->createEvent("HTMLEvents", ec);
        ev->initEvent(readystatechangeEvent, true, true);
        m_onReadyStateChangeListener->handleEvent(ev.get(), true);
    }
    
    if (doc && doc->frame() && state == Completed && m_onLoadListener) {
        ExceptionCode ec;
        RefPtr<Event> ev = doc->createEvent("HTMLEvents", ec);
        ev->initEvent(loadEvent, true, true);
        m_onLoadListener->handleEvent(ev.get(), true);
    }
}

bool XMLHttpRequest::urlMatchesDocumentDomain(const KURL& _url) const
{
  KURL documentURL(doc->URL());

    // a local file can load anything
    if (documentURL.protocol().lower() == "file")
        return true;

    // but a remote document can only load from the same port on the server
    if (documentURL.protocol().lower() == _url.protocol().lower()
            && documentURL.host().lower() == _url.host().lower()
            && documentURL.port() == _url.port())
        return true;

    return false;
}

void XMLHttpRequest::open(const String& _method, const KURL& _url, bool _async, const String& user, const String& password)
{
    abort();
    aborted = false;

    // clear stuff from possible previous load
    requestHeaders = DeprecatedString();
    responseHeaders = DeprecatedString();
    response = DeprecatedString();
    createdDocument = false;
    responseXML = 0;

    changeState(Uninitialized);

    if (aborted)
        return;

    if (!urlMatchesDocumentDomain(_url))
        return;

    url = _url;

    if (!user.isNull())
        url.setUser(user.deprecatedString());

    if (!password.isNull())
        url.setPass(password.deprecatedString());

    method = _method.deprecatedString();
    async = _async;

    changeState(Loading);
}

void XMLHttpRequest::send(const String& _body)
{
    if (!doc)
        return;

    if (state != Loading)
        return;
  
    // FIXME: Should this abort instead if we already have a job going?
    if (job)
        return;

    aborted = false;

    if (!_body.isNull() && method.lower() != "get" && method.lower() != "head" && (url.protocol().lower() == "http" || url.protocol().lower() == "https")) {
        DeprecatedString contentType = getRequestHeader("Content-Type");
        DeprecatedString charset;
        if (contentType.isEmpty())
            setRequestHeader("Content-Type", "application/xml");
        else
            charset = getCharset(contentType);
      
        if (charset.isEmpty())
            charset = "UTF-8";
      
        TextEncoding encoding = TextEncoding(charset.latin1());
        if (!encoding.isValid())   // FIXME: report an error?
            encoding = TextEncoding(UTF8Encoding);

        job = new TransferJob(async ? this : 0, method, url, encoding.fromUnicode(_body.deprecatedString()));
    } else {
        // HEAD requests just crash; see <rdar://4460899> and the commented out tests in http/tests/xmlhttprequest/methods.html.
        if (method.lower() == "head")
            method = "GET";
        job = new TransferJob(async ? this : 0, method, url);
    }

    if (requestHeaders.length())
        job->addMetaData("customHTTPHeader", requestHeaders);

    if (!async) {
        DeprecatedByteArray data;
        KURL finalURL;
        DeprecatedString headers;

        {
            // avoid deadlock in case the loader wants to use JS on a background thread
            KJS::JSLock::DropAllLocks dropLocks;
            data = KWQServeSynchronousRequest(Cache::loader(), doc->docLoader(), job, finalURL, headers);
        }

        job = 0;
        processSyncLoadResults(data, finalURL, headers);
    
        return;
    }

    // Neither this object nor the JavaScript wrapper should be deleted while
    // a request is in progress because we need to keep the listeners alive,
    // and they are referenced by the JavaScript wrapper.
    ref();
    {
        KJS::JSLock lock;
        gcProtectNullTolerant(KJS::ScriptInterpreter::getDOMObject(this));
    }
  
    job->start(doc->docLoader());
}

void XMLHttpRequest::abort()
{
    bool hadJob = job;

    if (hadJob) {
        job->kill();
        job = 0;
    }
    decoder = 0;
    aborted = true;

    if (hadJob) {
        {
            KJS::JSLock lock;
            gcUnprotectNullTolerant(KJS::ScriptInterpreter::getDOMObject(this));
        }
        deref();
    }
}

void XMLHttpRequest::overrideMIMEType(const String& override)
{
    MIMETypeOverride = override.deprecatedString();
}

void XMLHttpRequest::setRequestHeader(const String& name, const String& value)
{
    if (requestHeaders.length() > 0)
        requestHeaders += "\r\n";
    requestHeaders += name.deprecatedString();
    requestHeaders += ": ";
    requestHeaders += value.deprecatedString();
}

DeprecatedString XMLHttpRequest::getRequestHeader(const DeprecatedString& name) const
{
    return getSpecificHeader(requestHeaders, name);
}

String XMLHttpRequest::getAllResponseHeaders() const
{
    if (responseHeaders.isEmpty())
        return String();

    int endOfLine = responseHeaders.find("\n");
    if (endOfLine == -1)
        return String();

    return responseHeaders.mid(endOfLine + 1) + "\n";
}

String XMLHttpRequest::getResponseHeader(const String& name) const
{
    return getSpecificHeader(responseHeaders, name.deprecatedString());
}

DeprecatedString XMLHttpRequest::getSpecificHeader(const DeprecatedString& headers, const DeprecatedString& name)
{
    if (headers.isEmpty())
        return DeprecatedString();

    RegularExpression headerLinePattern(name + ":", false);

    int matchLength;
    int headerLinePos = headerLinePattern.match(headers, 0, &matchLength);
    while (headerLinePos != -1) {
        if (headerLinePos == 0 || headers[headerLinePos-1] == '\n')
            break;
        headerLinePos = headerLinePattern.match(headers, headerLinePos + 1, &matchLength);
    }
    if (headerLinePos == -1)
        return DeprecatedString();
    
    int endOfLine = headers.find("\n", headerLinePos + matchLength);
    return headers.mid(headerLinePos + matchLength, endOfLine - (headerLinePos + matchLength)).stripWhiteSpace();
}

bool XMLHttpRequest::responseIsXML() const
{
    DeprecatedString mimeType = getMIMEType(MIMETypeOverride);
    if (mimeType.isEmpty())
        mimeType = getMIMEType(getResponseHeader("Content-Type").deprecatedString());
    if (mimeType.isEmpty())
        mimeType = "text/xml";
    return DOMImplementation::isXMLMIMEType(mimeType);
}

int XMLHttpRequest::getStatus() const
{
    if (responseHeaders.isEmpty())
        return -1;
  
    int endOfLine = responseHeaders.find("\n");
    DeprecatedString firstLine = endOfLine == -1 ? responseHeaders : responseHeaders.left(endOfLine);
    int codeStart = firstLine.find(" ");
    int codeEnd = firstLine.find(" ", codeStart + 1);
    if (codeStart == -1 || codeEnd == -1)
        return -1;
  
    DeprecatedString number = firstLine.mid(codeStart + 1, codeEnd - (codeStart + 1));
    bool ok = false;
    int code = number.toInt(&ok);
    if (!ok)
        return -1;
    return code;
}

String XMLHttpRequest::getStatusText() const
{
    if (responseHeaders.isEmpty())
        return String();
  
    int endOfLine = responseHeaders.find("\n");
    DeprecatedString firstLine = endOfLine == -1 ? responseHeaders : responseHeaders.left(endOfLine);
    int codeStart = firstLine.find(" ");
    int codeEnd = firstLine.find(" ", codeStart + 1);
    if (codeStart == -1 || codeEnd == -1)
        return String();
  
    DeprecatedString statusText = firstLine.mid(codeEnd + 1, endOfLine - (codeEnd + 1)).stripWhiteSpace();
    return String(statusText);
}

void XMLHttpRequest::processSyncLoadResults(const DeprecatedByteArray &data, const KURL &finalURL, const DeprecatedString &headers)
{
  if (!urlMatchesDocumentDomain(finalURL)) {
    abort();
    return;
  }
  
  responseHeaders = headers;
  changeState(Loaded);
  if (aborted)
    return;
  
  const char *bytes = (const char *)data.data();
  int len = (int)data.size();

  receivedData(0, bytes, len);
  if (aborted)
    return;

  receivedAllData(0);
}

void XMLHttpRequest::receivedAllData(TransferJob*)
{
    if (responseHeaders.isEmpty() && job)
        responseHeaders = job->queryMetaData("HTTP-Headers");

    if (state < Loaded)
        changeState(Loaded);

    if (decoder)
        response += decoder->flush();

    bool hadJob = job;
    job = 0;

    changeState(Completed);
    decoder = 0;

    if (hadJob) {
        {
            KJS::JSLock lock;
            gcUnprotectNullTolerant(KJS::ScriptInterpreter::getDOMObject(this));
        }
        deref();
    }
}

void XMLHttpRequest::receivedRedirect(TransferJob*, const KURL& url)
{
    if (!urlMatchesDocumentDomain(url))
        abort();
}

void XMLHttpRequest::receivedData(TransferJob*, const char *data, int len)
{
    if (responseHeaders.isEmpty() && job)
        responseHeaders = job->queryMetaData("HTTP-Headers");

    if (state < Loaded)
        changeState(Loaded);
  
    if (!decoder) {
        encoding = getCharset(MIMETypeOverride);
        if (encoding.isEmpty())
            encoding = getCharset(getResponseHeader("Content-Type").deprecatedString());
        if (encoding.isEmpty() && job)
            encoding = job->queryMetaData("charset");
    
        decoder = new Decoder;
        if (!encoding.isEmpty())
            decoder->setEncodingName(encoding.latin1(), Decoder::EncodingFromHTTPHeader);
        else
            // only allow Decoder to look inside the response if it's XML
            decoder->setEncodingName("UTF-8", responseIsXML() ? Decoder::DefaultEncoding : Decoder::EncodingFromHTTPHeader);
    }
    if (len == 0)
        return;

    if (len == -1)
        len = strlen(data);

    DeprecatedString decoded = decoder->decode(data, len);

    response += decoded;

    if (!aborted) {
        if (state != Interactive)
            changeState(Interactive);
        else
            // Firefox calls readyStateChanged every time it receives data, 4449442
            callReadyStateChangeListener();
    }
}

void XMLHttpRequest::cancelRequests(Document* doc)
{
    RequestsSet* requests = requestsByDocument().get(doc);
    if (!requests)
        return;
    RequestsSet copy = *requests;
    RequestsSet::const_iterator end = copy.end();
    for (RequestsSet::const_iterator it = copy.begin(); it != end; ++it)
        (*it)->abort();
}

void XMLHttpRequest::detachRequests(Document* doc)
{
    RequestsSet* requests = requestsByDocument().get(doc);
    if (!requests)
        return;
    requestsByDocument().remove(doc);
    RequestsSet::const_iterator end = requests->end();
    for (RequestsSet::const_iterator it = requests->begin(); it != end; ++it) {
        (*it)->doc = 0;
        (*it)->abort();
    }
    delete requests;
}

} // end namespace
