/*
  Copyright (C) 2007 Trolltech ASA
  
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
#ifndef QWEBNETWORKINTERFACE_P_H
#define QWEBNETWORKINTERFACE_P_H

#include "qwebnetworkinterface.h"
#include <qthread.h>

namespace WebCore {
    struct HostInfo;
};
uint qHash(const WebCore::HostInfo &info);
#include <qhash.h>

namespace WebCore {
    class ResourceHandle;
}

class QWebNetworkJobPrivate
{
public:
    int ref;
    QUrl url;
    QHttpRequestHeader request;
    QByteArray postData;

    QHttpResponseHeader response;

    WebCore::ResourceHandle *resourceHandle;
    bool redirected;

    void setURL(const QUrl &u);
};


class QWebNetworkManager : public QObject
{
    Q_OBJECT
public:
    static QWebNetworkManager *self();

    bool add(WebCore::ResourceHandle *resourceHandle);
    void cancel(WebCore::ResourceHandle *resourceHandle);

public slots:
    void started(QWebNetworkJob *);
    void data(QWebNetworkJob *, const QByteArray &data);
    void finished(QWebNetworkJob *, int errorCode);

signals:
    void fileRequest(QWebNetworkJob*);

private:
    friend class QWebNetworkInterface;
    QWebNetworkManager();
};


namespace WebCore {
    
    class LoaderThread : public QThread {
        Q_OBJECT
    public:
        LoaderThread(QWebNetworkInterface *manager);

        void waitForSetup() { while (!m_setup); }
    protected:
        void run();
    private:
        QObject* m_loader;
        QWebNetworkInterface* m_manager;
        volatile bool m_setup;
    };

    class FileLoader : public QObject {
        Q_OBJECT
    public:
        FileLoader();

            public slots:
            void request(QWebNetworkJob*);

        signals:
        void receivedResponse(QWebNetworkJob* resource);
        void receivedData(QWebNetworkJob* resource, const QByteArray &data);
        void receivedFinished(QWebNetworkJob* resource, int errorCode);

    private:
        void parseDataUrl(QWebNetworkJob* request);
        void sendData(QWebNetworkJob* request, int statusCode, const QByteArray &data);
    };

    class NetworkLoader;

    struct HostInfo {
        HostInfo() {}
        HostInfo(const QUrl& url);
        QString protocol;
        QString host;
        int port;
    };

    class WebCoreHttp : public QObject
    {
        Q_OBJECT
    public:
        WebCoreHttp(NetworkLoader* parent, const HostInfo&);
        ~WebCoreHttp();

        void request(QWebNetworkJob* resource);
        void cancel(QWebNetworkJob*);

    signals:
        void connectionClosed(const HostInfo &);

             private slots:
             void onResponseHeaderReceived(const QHttpResponseHeader& resp);
        void onReadyRead();
        void onRequestFinished(int, bool);
        void onStateChanged(int);

        void scheduleNextRequest();

        int getConnection();

    public:
        HostInfo info;
    private:
        NetworkLoader* m_loader;
        QList<QWebNetworkJob *> m_pendingRequests;
        struct HttpConnection {
            QHttp *http;
            QWebNetworkJob *current;
        };
        HttpConnection connection[2];
        bool m_inCancel;
    };

    class NetworkLoader : public QObject {
        Q_OBJECT
    public:
        NetworkLoader(QObject *parent);
        ~NetworkLoader();


            public slots:
            void request(QWebNetworkJob*);
        void cancel(QWebNetworkJob*);
        void connectionClosed(const HostInfo &);

    signals:
        void receivedResponse(QWebNetworkJob* resource);
        void receivedData(QWebNetworkJob* resource, const QByteArray &data);
        void receivedFinished(QWebNetworkJob* resource, int errorCode);
    private:
        friend class WebCoreHttp;
        QHash<HostInfo, WebCoreHttp *> m_hostMapping;
    };

}

class QWebNetworkInterfacePrivate
{
public:
    WebCore::NetworkLoader *networkLoader;
    WebCore::LoaderThread *fileLoader;
};

#endif
