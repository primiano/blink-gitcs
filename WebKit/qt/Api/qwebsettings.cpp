/*
    Copyright (C) 2007-2008 Trolltech ASA

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

    This class provides all functionality needed for loading images, style sheets and html
    pages from the web. It has a memory cache for these objects.
*/

#include "config.h"
#include "qwebsettings.h"

#include "qwebpage.h"
#include "qwebpage_p.h"

#include "Cache.h"
#include "Page.h"
#include "PageCache.h"
#include "Settings.h"
#include "KURL.h"
#include "PlatformString.h"
#include "IconDatabase.h"
#include "Image.h"
#include "IntSize.h"

#include <QHash>
#include <QSharedData>
#include <QUrl>
#include <QFileInfo>

class QWebSettingsPrivate
{
public:
    QWebSettingsPrivate(WebCore::Settings *wcSettings = 0)
        : settings(wcSettings)
    {
    }

    QHash<int, QString> fontFamilies;
    QHash<int, int> fontSizes;
    QHash<int, bool> attributes;
    QUrl userStyleSheetLocation;

    void apply();
    WebCore::Settings *settings;
};

typedef QHash<int, QPixmap> WebGraphicHash;
Q_GLOBAL_STATIC(WebGraphicHash, graphics)

Q_GLOBAL_STATIC(QList<QWebSettingsPrivate *>, allSettings);

void QWebSettingsPrivate::apply()
{
    if (settings) {
        QWebSettingsPrivate *global = QWebSettings::defaultSettings()->d;

        QString family = fontFamilies.value(QWebSettings::StandardFont,
                                            global->fontFamilies.value(QWebSettings::StandardFont));
        settings->setStandardFontFamily(family);

        family = fontFamilies.value(QWebSettings::FixedFont,
                                    global->fontFamilies.value(QWebSettings::FixedFont));
        settings->setFixedFontFamily(family);

        family = fontFamilies.value(QWebSettings::SerifFont,
                                    global->fontFamilies.value(QWebSettings::SerifFont));
        settings->setSerifFontFamily(family);

        family = fontFamilies.value(QWebSettings::SansSerifFont,
                                    global->fontFamilies.value(QWebSettings::SansSerifFont));
        settings->setSansSerifFontFamily(family);

        family = fontFamilies.value(QWebSettings::CursiveFont,
                                    global->fontFamilies.value(QWebSettings::CursiveFont));
        settings->setCursiveFontFamily(family);

        family = fontFamilies.value(QWebSettings::FantasyFont,
                                    global->fontFamilies.value(QWebSettings::FantasyFont));
        settings->setFantasyFontFamily(family);

        int size = fontSizes.value(QWebSettings::MinimumFontSize,
                                   global->fontSizes.value(QWebSettings::MinimumFontSize));
        settings->setMinimumFontSize(size);

        size = fontSizes.value(QWebSettings::MinimumLogicalFontSize,
                                   global->fontSizes.value(QWebSettings::MinimumLogicalFontSize));
        settings->setMinimumLogicalFontSize(size);

        size = fontSizes.value(QWebSettings::DefaultFontSize,
                                   global->fontSizes.value(QWebSettings::DefaultFontSize));
        settings->setDefaultFontSize(size);

        size = fontSizes.value(QWebSettings::DefaultFixedFontSize,
                                   global->fontSizes.value(QWebSettings::DefaultFixedFontSize));
        settings->setDefaultFixedFontSize(size);

        bool value = attributes.value(QWebSettings::AutoLoadImages,
                                      global->attributes.value(QWebSettings::AutoLoadImages));
        settings->setLoadsImagesAutomatically(value);

        value = attributes.value(QWebSettings::JavascriptEnabled,
                                      global->attributes.value(QWebSettings::JavascriptEnabled));
        settings->setJavaScriptEnabled(value);

        value = attributes.value(QWebSettings::JavascriptCanOpenWindows,
                                      global->attributes.value(QWebSettings::JavascriptCanOpenWindows));
        settings->setJavaScriptCanOpenWindowsAutomatically(value);

        value = attributes.value(QWebSettings::JavaEnabled,
                                      global->attributes.value(QWebSettings::JavaEnabled));
        settings->setJavaEnabled(value);

        value = attributes.value(QWebSettings::PluginsEnabled,
                                      global->attributes.value(QWebSettings::PluginsEnabled));
        settings->setPluginsEnabled(value);

        value = attributes.value(QWebSettings::PrivateBrowsingEnabled,
                                      global->attributes.value(QWebSettings::PrivateBrowsingEnabled));
        settings->setPrivateBrowsingEnabled(value);

        value = attributes.value(QWebSettings::JavascriptCanAccessClipboard,
                                      global->attributes.value(QWebSettings::JavascriptCanAccessClipboard));
        settings->setDOMPasteAllowed(value);

        value = attributes.value(QWebSettings::DeveloperExtrasEnabled,
                                      global->attributes.value(QWebSettings::DeveloperExtrasEnabled));
        settings->setDeveloperExtrasEnabled(value);

        QUrl location = !userStyleSheetLocation.isEmpty() ? userStyleSheetLocation : global->userStyleSheetLocation;
        settings->setUserStyleSheetLocation(WebCore::KURL(location));
    } else {
        QList<QWebSettingsPrivate *> settings = *::allSettings();
        for (int i = 0; i < settings.count(); ++i)
            settings[i]->apply();
    }
}

/*!
    Returns the global default settings object.

    Any setting changed on the default object is automatically applied to all
    QWebPage instances where the particular setting is not overriden already.
*/
QWebSettings *QWebSettings::defaultSettings()
{
    static QWebSettings *global = 0;
    if (!global)
        global = new QWebSettings;
    return global;
}

/*!
    \internal
*/
QWebSettings::QWebSettings()
    : d(new QWebSettingsPrivate)
{
    // Initialize our global defaults
    // changing any of those will likely break the LayoutTests
    d->fontSizes.insert(QWebSettings::MinimumFontSize, 5);
    d->fontSizes.insert(QWebSettings::MinimumLogicalFontSize, 5);
    d->fontSizes.insert(QWebSettings::DefaultFontSize, 14);
    d->fontSizes.insert(QWebSettings::DefaultFixedFontSize, 14);
    d->fontFamilies.insert(QWebSettings::StandardFont, QLatin1String("Arial"));
    d->fontFamilies.insert(QWebSettings::FixedFont, QLatin1String("Courier New"));
    d->fontFamilies.insert(QWebSettings::SerifFont, QLatin1String("Times New Roman"));
    d->fontFamilies.insert(QWebSettings::SansSerifFont, QLatin1String("Arial"));
    d->fontFamilies.insert(QWebSettings::CursiveFont, QLatin1String("Arial"));
    d->fontFamilies.insert(QWebSettings::FantasyFont, QLatin1String("Arial"));

    d->attributes.insert(QWebSettings::AutoLoadImages, true);
    d->attributes.insert(QWebSettings::JavascriptEnabled, true);
    d->attributes.insert(QWebSettings::LinksIncludedInFocusChain, true);
}

/*!
    \internal
*/
QWebSettings::QWebSettings(WebCore::Settings *settings)
    : d(new QWebSettingsPrivate(settings))
{
    d->settings = settings;
    d->apply();
    allSettings()->append(d);
}

/*!
    \internal
*/
QWebSettings::~QWebSettings()
{
    if (d->settings)
        allSettings()->removeAll(d);

    delete d;
}

/*!
    Sets the font size for \a type to \a size.
*/
void QWebSettings::setFontSize(FontSize type, int size)
{
    d->fontSizes.insert(type, size);
    d->apply();
}

/*!
    Returns the default font size for \a type.
*/
int QWebSettings::fontSize(FontSize type) const
{
    int defaultValue = 0;
    if (d->settings) {
        QWebSettingsPrivate *global = QWebSettings::defaultSettings()->d;
        defaultValue = global->fontSizes.value(type);
    }
    return d->fontSizes.value(type, defaultValue);
}

/*!
    Resets the font size for \a type to the size specified in the default settings object.

    This function has not effect on the default QWebSettings instance.
*/
void QWebSettings::resetFontSize(FontSize type)
{
    if (d->settings) {
        d->fontSizes.remove(type);
        d->apply();
    }
}

/*!
    Specifies the location of a user stylesheet to load with every web page.

    The location can be a URL as well as a path on the local filesystem.

    \sa userStyleSheetLocation
*/
void QWebSettings::setUserStyleSheetLocation(const QUrl &location)
{
    d->userStyleSheetLocation = location;
    d->apply();
}

/*!
    Returns the location of the user stylesheet.

    \sa setUserStyleSheetLocation
*/
QUrl QWebSettings::userStyleSheetLocation() const
{
    return d->userStyleSheetLocation;
}

/*!
    Enables or disables the icon database. The icon database is used to store favicons
    associated with web sites.

    If \a enabled is true then \a location must be specified and point to an existing directory
    where the icons are stored.
*/
void QWebSettings::setIconDatabaseEnabled(bool enabled, const QString &location)
{
    WebCore::iconDatabase()->setEnabled(enabled);
    if (enabled) {
        QFileInfo info(location);
        if (info.isDir() && info.isWritable())
            WebCore::iconDatabase()->open(location);
    } else {
        WebCore::iconDatabase()->close();
    }
}

/*!
    Returns whether the icon database is enabled or not.

    \sa setIconDatabaseEnabled
*/
bool QWebSettings::iconDatabaseEnabled()
{
    return WebCore::iconDatabase()->isEnabled() && WebCore::iconDatabase()->isOpen();
}

/*!
    Returns the site icon for \a url
    If there is no icon for the url a null QIcon is returned.
*/
QPixmap QWebSettings::iconForUrl(const QUrl &url)
{
    WebCore::Image* image = WebCore::iconDatabase()->iconForPageURL(WebCore::KURL(url).string(),
                                WebCore::IntSize(16, 16));
    if (!image) {
        return QPixmap();
    }
    QPixmap *icon = image->getPixmap();
    if (!icon) {
        return QPixmap();
    }
    return *icon;
}

/*!
    Sets \a graphic to be drawn when QtWebKit needs to drawn an image of the given \a type.
*/
void QWebSettings::setWebGraphic(WebGraphic type, const QPixmap &graphic)
{
    WebGraphicHash *h = graphics();
    if (graphic.isNull())
        h->remove(type);
    else
        h->insert(type, graphic);
}

/*!
    Returns a previously set pixmap that is used to draw replacement graphics of the specified \a type.
*/
QPixmap QWebSettings::webGraphic(WebGraphic type)
{
    return graphics()->value(type);
}

/*!
 @internal
*/
void QWebSettings::setPageCacheCapacity(int numberOfPages)
{
    WebCore::pageCache()->setCapacity(qMax(0, numberOfPages));
}

/*!
 @internal
*/
void QWebSettings::setObjectCacheCapacities(int cacheMinDeadCapacity, int cacheMaxDead, int totalCapacity)
{
    WebCore::cache()->setCapacities(qMax(0, cacheMinDeadCapacity),
                                    qMax(0, cacheMaxDead),
                                    qMax(0, totalCapacity));
}

/*!
    Sets the default font family to \a family for the specified \a type of font.
*/
void QWebSettings::setFontFamily(FontType type, const QString &family)
{
    d->fontFamilies.insert(type, family);
    d->apply();
}

/*!
    Returns the default font family to \a family for the specified \a type of font.
*/
QString QWebSettings::fontFamily(FontType type) const
{
    QString defaultValue;
    if (d->settings) {
        QWebSettingsPrivate *global = QWebSettings::defaultSettings()->d;
        defaultValue = global->fontFamilies.value(type);
    }
    return d->fontFamilies.value(type, defaultValue);
}

/*!
    Resets the font family for specified \a type of fonts in a web page to the default.

    This function has not effect on the default QWebSettings instance.
*/
void QWebSettings::resetFontFamily(FontType type)
{
    if (d->settings) {
        d->fontFamilies.remove(type);
        d->apply();
    }
}

/*!
    Enables or disables the specified \a attr feature depending on the value of \a on.
*/
void QWebSettings::setAttribute(WebAttribute attr, bool on)
{
    d->attributes.insert(attr, on);
    d->apply();
}

/*!
    Returns true if \a attr is enabled; false otherwise.
*/
bool QWebSettings::testAttribute(WebAttribute attr) const
{
    bool defaultValue = false;
    if (d->settings) {
        QWebSettingsPrivate *global = QWebSettings::defaultSettings()->d;
        defaultValue = global->attributes.value(attr);
    }
    return d->attributes.value(attr, defaultValue);
}

/*!
    Clears the setting of \a attr. The global default for \a attr will be used instead.

    This function has not effect on the default QWebSettings instance.
*/
void QWebSettings::clearAttribute(WebAttribute attr)
{
    if (d->settings) {
        d->attributes.remove(attr);
        d->apply();
    }
}

