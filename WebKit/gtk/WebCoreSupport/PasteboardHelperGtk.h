/*
 * Copyright (C) 2007 Luca Bruno <lethalman88@gmail.com>
 * Copyright (C) 2009 Holger Hans Peter Freyther
 * All rights reserved.
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
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef PasteboardHelperGtk_h
#define PasteboardHelperGtk_h

/*
 * FIXME: this is for WebCore support and must be removed once
 * a better solution is found
 */

#include "Frame.h"
#include "PasteboardHelper.h"

#include <gtk/gtk.h>

using namespace WebCore;

namespace WebKit {

class PasteboardHelperGtk : public PasteboardHelper {
public:
    PasteboardHelperGtk();
    ~PasteboardHelperGtk();
    virtual GtkClipboard* getCurrentTarget(Frame*) const;
    virtual GtkClipboard* getClipboard(Frame*) const;
    virtual GtkClipboard* getPrimary(Frame*) const;
    virtual GtkTargetList* targetList() const;
    virtual gint getWebViewTargetInfoHtml() const;

    void writeClipboardContents(GtkClipboard* clipboard, gpointer data = 0);

private:
    GtkTargetList* m_targetList;
};

}

#endif // PasteboardHelperGtk_h
