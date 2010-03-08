/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AccessibilityUIElement_h
#define AccessibilityUIElement_h

#include "CppBoundClass.h"
#include "public/WebAccessibilityObject.h"
#include <wtf/Vector.h>

class AccessibilityUIElement : public CppBoundClass {
public:
    class Factory {
    public:
        virtual ~Factory() { }
        virtual AccessibilityUIElement* create(const WebKit::WebAccessibilityObject&) = 0;
    };

    AccessibilityUIElement(const WebKit::WebAccessibilityObject&, Factory*);

    virtual AccessibilityUIElement* getChildAtIndex(unsigned);
    virtual bool isRoot() const { return false; }

protected:
    const WebKit::WebAccessibilityObject& accessibilityObject() const { return m_accessibilityObject; }
    Factory* factory() const { return m_factory; }

private:
    // Bound methods and properties.
    void allAttributesCallback(const CppArgumentList&, CppVariant*);
    void attributesOfLinkedUIElementsCallback(const CppArgumentList&, CppVariant*);
    void attributesOfDocumentLinksCallback(const CppArgumentList&, CppVariant*);
    void attributesOfChildrenCallback(const CppArgumentList&, CppVariant*);
    void parametrizedAttributeNamesCallback(const CppArgumentList&, CppVariant*);
    void lineForIndexCallback(const CppArgumentList&, CppVariant*);
    void boundsForRangeCallback(const CppArgumentList&, CppVariant*);
    void stringForRangeCallback(const CppArgumentList&, CppVariant*);
    void childAtIndexCallback(const CppArgumentList&, CppVariant*);
    void elementAtPointCallback(const CppArgumentList&, CppVariant*);
    void attributesOfColumnHeadersCallback(const CppArgumentList&, CppVariant*);
    void attributesOfRowHeadersCallback(const CppArgumentList&, CppVariant*);
    void attributesOfColumnsCallback(const CppArgumentList&, CppVariant*);
    void attributesOfRowsCallback(const CppArgumentList&, CppVariant*);
    void attributesOfVisibleCellsCallback(const CppArgumentList&, CppVariant*);
    void attributesOfHeaderCallback(const CppArgumentList&, CppVariant*);
    void indexInTableCallback(const CppArgumentList&, CppVariant*);
    void rowIndexRangeCallback(const CppArgumentList&, CppVariant*);
    void columnIndexRangeCallback(const CppArgumentList&, CppVariant*);
    void cellForColumnAndRowCallback(const CppArgumentList&, CppVariant*);
    void titleUIElementCallback(const CppArgumentList&, CppVariant*);
    void setSelectedTextRangeCallback(const CppArgumentList&, CppVariant*);
    void attributeValueCallback(const CppArgumentList&, CppVariant*);
    void isAttributeSettableCallback(const CppArgumentList&, CppVariant*);
    void isActionSupportedCallback(const CppArgumentList&, CppVariant*);
    void parentElementCallback(const CppArgumentList&, CppVariant*);
    void incrementCallback(const CppArgumentList&, CppVariant*);
    void decrementCallback(const CppArgumentList&, CppVariant*);
    void fallbackCallback(const CppArgumentList&, CppVariant*);

    void childrenCountGetterCallback(CppVariant*);
    void descriptionGetterCallback(CppVariant*);
    void isEnabledGetterCallback(CppVariant*);
    void isSelectedGetterCallback(CppVariant*);
    void roleGetterCallback(CppVariant*);
    void titleGetterCallback(CppVariant*);

    CppVariant m_subrole;
    CppVariant m_language;
    CppVariant m_x;
    CppVariant m_y;
    CppVariant m_width;
    CppVariant m_height;
    CppVariant m_clickPointX;
    CppVariant m_clickPointY;
    CppVariant m_intValue;
    CppVariant m_minValue;
    CppVariant m_maxValue;
    CppVariant m_childrenCount;
    CppVariant m_insertionPointLineNumber;
    CppVariant m_selectedTextRange;
    CppVariant m_isRequired;
    CppVariant m_valueDescription;

    WebKit::WebAccessibilityObject m_accessibilityObject;
    Factory* m_factory;
};


class RootAccessibilityUIElement : public AccessibilityUIElement {
public:
    RootAccessibilityUIElement(const WebKit::WebAccessibilityObject&, Factory*);

    virtual AccessibilityUIElement* getChildAtIndex(unsigned);
    virtual bool isRoot() const { return true; }
};


// Provides simple lifetime management of the AccessibilityUIElement instances:
// all AccessibilityUIElements ever created from the controller are stored in
// a list and cleared explicitly.
class AccessibilityUIElementList : public AccessibilityUIElement::Factory {
public:
    AccessibilityUIElementList() { }
    virtual ~AccessibilityUIElementList();

    void clear();
    virtual AccessibilityUIElement* create(const WebKit::WebAccessibilityObject&);
    AccessibilityUIElement* createRoot(const WebKit::WebAccessibilityObject&);

private:
    typedef Vector<AccessibilityUIElement*> ElementList;
    ElementList m_elements;
};

#endif // AccessibilityUIElement_h
