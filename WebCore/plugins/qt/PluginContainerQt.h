/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)

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
#ifndef PluginContainerQt_H
#define PluginContainerQt_H

#include <QX11EmbedContainer>

namespace WebCore {

    class PluginView;

    class PluginContainerQt : public QX11EmbedContainer
    {
        Q_OBJECT
    public:
        PluginContainerQt(PluginView*, QWidget* parent);

        void requestGeometry(const QRect&, const QRegion& clip = QRegion());
        void adjustGeometry();

    protected:
        virtual void focusInEvent(QFocusEvent*);
        virtual void focusOutEvent(QFocusEvent*);

    private:
        PluginView* m_pluginView;

        QRect m_windowRect;
        QRegion m_clipRegion;

        bool m_hasPendingGeometryChange;
    };

}

#endif // PluginContainerQt_H
