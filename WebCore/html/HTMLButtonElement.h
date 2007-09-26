/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
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

#ifndef HTMLButtonElement_h
#define HTMLButtonElement_h

#include "HTMLGenericFormElement.h"

namespace WebCore {

class HTMLButtonElement : public HTMLGenericFormElement {
public:
    HTMLButtonElement(Document*, HTMLFormElement* = 0);
    virtual ~HTMLButtonElement();

    virtual const AtomicString& type() const;
        
    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);

    virtual void parseMappedAttribute(MappedAttribute*);
    virtual void defaultEventHandler(Event*);
    virtual bool appendFormData(FormDataList&, bool);

    virtual bool isEnumeratable() const { return true; } 

    virtual bool isSuccessfulSubmitButton() const;
    virtual bool isActivatedSubmit() const;
    virtual void setActivatedSubmit(bool flag);

    virtual void accessKeyAction(bool sendToAnyElement);

    virtual bool canStartSelection() const { return false; }

    String accessKey() const;
    void setAccessKey(const String&);

    String value() const;
    void setValue(const String&);
    
private:
    enum Type { SUBMIT, RESET, BUTTON };

    Type m_type;
    bool m_activeSubmit;
};

} // namespace

#endif
