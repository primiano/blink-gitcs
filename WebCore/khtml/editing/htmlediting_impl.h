/*
 * Copyright (C) 2004 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef __htmleditingimpl_h__
#define __htmleditingimpl_h__

#include "htmlediting.h"

#include <qvaluelist.h>
#include "selection.h"

namespace DOM {
    class CSSProperty;
    class HTMLElementImpl;
}

namespace khtml {

//------------------------------------------------------------------------------------------
// StyleChange

class StyleChange {
public:
    StyleChange() : m_applyBold(false), m_applyItalic(false) { }
    explicit StyleChange(DOM::CSSStyleDeclarationImpl *);
    StyleChange(DOM::CSSStyleDeclarationImpl *, const DOM::Position &);

    DOM::DOMString cssStyle() const { return m_cssStyle; }
    bool applyBold() const { return m_applyBold; }
    bool applyItalic() const { return m_applyItalic; }

private:
    void init(DOM::CSSStyleDeclarationImpl *, const DOM::Position &);
    static bool currentlyHasStyle(const DOM::Position &, const DOM::CSSProperty *);
    
    DOM::DOMString m_cssStyle;
    bool m_applyBold;
    bool m_applyItalic;
};

//------------------------------------------------------------------------------------------
// EditCommandImpl

class EditCommandImpl : public Shared<EditCommandImpl>
{
public:
    EditCommandImpl(DOM::DocumentImpl *);
    virtual ~EditCommandImpl();

    bool isCompositeStep() const { return m_parent.notNull(); }
    EditCommandImpl *parent() const { return m_parent.get(); }
    void setParent(EditCommandImpl *parent) { m_parent = parent; }

    enum ECommandState { NotApplied, Applied };
    
    void apply();	
    void unapply();
    void reapply();

    virtual void doApply() = 0;
    virtual void doUnapply() = 0;
    virtual void doReapply();  // calls doApply()

    virtual DOM::DocumentImpl * const document() const { return m_document; }

    DOM::Selection startingSelection() const { return m_startingSelection; }
    DOM::Selection endingSelection() const { return m_endingSelection; }
        
    ECommandState state() const { return m_state; }
    void setState(ECommandState state) { m_state = state; }

    void setStartingSelection(const DOM::Selection &s);
    void setEndingSelection(const DOM::Selection &s);

    DOM::CSSStyleDeclarationImpl *typingStyle() const { return m_typingStyle; };
    void setTypingStyle(DOM::CSSStyleDeclarationImpl *);
    
    void markMisspellingsInSelection(const DOM::Selection &s);

    virtual bool isInputTextCommand() const;
    virtual bool isTypingCommand() const;

private:
    void assignTypingStyle(DOM::CSSStyleDeclarationImpl *);

    virtual bool preservesTypingStyle() const;

    DOM::DocumentImpl *m_document;
    ECommandState m_state;
    DOM::Selection m_startingSelection;
    DOM::Selection m_endingSelection;
    DOM::CSSStyleDeclarationImpl *m_typingStyle;
    EditCommand m_parent;
};

//------------------------------------------------------------------------------------------
// CompositeEditCommandImpl

class CompositeEditCommandImpl : public EditCommandImpl
{
public:
    CompositeEditCommandImpl(DOM::DocumentImpl *);
	
    virtual void doUnapply();
    virtual void doReapply();

protected:
    //
    // sugary-sweet convenience functions to help create and apply edit commands in composite commands
    //
    void appendNode(DOM::NodeImpl *appendChild, DOM::NodeImpl *parentNode);
    void applyCommandToComposite(EditCommand &);
    void deleteKeyPressed();
    void deleteSelection(bool smartDelete=false);
    void deleteSelection(const DOM::Selection &selection, bool smartDelete=false);
    void deleteText(DOM::TextImpl *node, long offset, long count);
    void inputText(const DOM::DOMString &text, bool selectInsertedText = false);
    void insertNodeAfter(DOM::NodeImpl *insertChild, DOM::NodeImpl *refChild);
    void insertNodeAt(DOM::NodeImpl *insertChild, DOM::NodeImpl *refChild, long offset);
    void insertNodeBefore(DOM::NodeImpl *insertChild, DOM::NodeImpl *refChild);
    void insertText(DOM::TextImpl *node, long offset, const DOM::DOMString &text);
    void joinTextNodes(DOM::TextImpl *text1, DOM::TextImpl *text2);
    void removeCSSProperty(DOM::CSSStyleDeclarationImpl *, int property);
    void removeNodeAttribute(DOM::ElementImpl *, int attribute);
    void removeNode(DOM::NodeImpl *removeChild);
    void removeNodePreservingChildren(DOM::NodeImpl *node);
    void replaceText(DOM::TextImpl *node, long offset, long count, const DOM::DOMString &replacementText);
    void setNodeAttribute(DOM::ElementImpl *, int attribute, const DOM::DOMString &);
    void splitTextNode(DOM::TextImpl *text, long offset);

    DOM::NodeImpl *applyTypingStyle(DOM::NodeImpl *) const;
    void deleteUnrenderedText(DOM::NodeImpl *);
    void deleteUnrenderedText(const DOM::Position &pos);

    QValueList<EditCommand> m_cmds;
};

//==========================================================================================
// Concrete commands
//------------------------------------------------------------------------------------------
// AppendNodeCommandImpl

class AppendNodeCommandImpl : public EditCommandImpl
{
public:
    AppendNodeCommandImpl(DOM::DocumentImpl *, DOM::NodeImpl *appendChild, DOM::NodeImpl *parentNode);
    virtual ~AppendNodeCommandImpl();

    virtual void doApply();
    virtual void doUnapply();

    DOM::NodeImpl *appendChild() const { return m_appendChild; }
    DOM::NodeImpl *parentNode() const { return m_parentNode; }

private:
    DOM::NodeImpl *m_appendChild;
    DOM::NodeImpl *m_parentNode;    
};

//------------------------------------------------------------------------------------------
// ApplyStyleCommandImpl

class ApplyStyleCommandImpl : public CompositeEditCommandImpl
{
public:
    ApplyStyleCommandImpl(DOM::DocumentImpl *, DOM::CSSStyleDeclarationImpl *style);
    virtual ~ApplyStyleCommandImpl();
	
    virtual void doApply();

    DOM::CSSStyleDeclarationImpl *style() const { return m_style; }

private:
    // style-removal helpers
    bool isHTMLStyleNode(DOM::HTMLElementImpl *);
    void removeHTMLStyleNode(DOM::HTMLElementImpl *);
    void removeCSSStyle(DOM::HTMLElementImpl *);
    void removeStyle(const DOM::Position &start, const DOM::Position &end);
    bool nodeFullySelected(const DOM::Position &, const DOM::NodeImpl *node) const;

    // style-application helpers
    bool splitTextAtStartIfNeeded(const DOM::Position &start, const DOM::Position &end);
    DOM::NodeImpl *splitTextAtEndIfNeeded(const DOM::Position &start, const DOM::Position &end);
    void surroundNodeRangeWithElement(DOM::NodeImpl *start, DOM::NodeImpl *end, DOM::ElementImpl *element);
    DOM::Position positionInsertionPoint(DOM::Position);
    void applyStyleIfNeeded(DOM::NodeImpl *start, DOM::NodeImpl *end);
    
    DOM::CSSStyleDeclarationImpl *m_style;
};

//------------------------------------------------------------------------------------------
// DeleteSelectionCommandImpl

class DeleteSelectionCommandImpl : public CompositeEditCommandImpl
{ 
public:
    DeleteSelectionCommandImpl(DOM::DocumentImpl *document, bool smartDelete=false);
    DeleteSelectionCommandImpl(DOM::DocumentImpl *document, const DOM::Selection &selection, bool smartDelete=false);
	
    virtual void doApply();
    
private:
    virtual bool preservesTypingStyle() const;

    void deleteDownstreamWS(const DOM::Position &start);
    bool containsOnlyWhitespace(const DOM::Position &start, const DOM::Position &end);
    void moveNodesAfterNode(DOM::NodeImpl *startNode, DOM::NodeImpl *dstNode);

    DOM::Selection m_selectionToDelete;
    bool m_hasSelectionToDelete;
    bool m_smartDelete;
};

//------------------------------------------------------------------------------------------
// DeleteTextCommandImpl

class DeleteTextCommandImpl : public EditCommandImpl
{
public:
    DeleteTextCommandImpl(DOM::DocumentImpl *document, DOM::TextImpl *node, long offset, long count);
    virtual ~DeleteTextCommandImpl();
	
    virtual void doApply();
    virtual void doUnapply();

    DOM::TextImpl *node() const { return m_node; }
    long offset() const { return m_offset; }
    long count() const { return m_count; }

private:
    DOM::TextImpl *m_node;
    long m_offset;
    long m_count;
    DOM::DOMString m_text;
};

//------------------------------------------------------------------------------------------
// InputNewlineCommandImpl

class InputNewlineCommandImpl : public CompositeEditCommandImpl
{
public:
    InputNewlineCommandImpl(DOM::DocumentImpl *document);

    virtual void doApply();

private:
    void insertNodeAfterPosition(DOM::NodeImpl *node, const DOM::Position &pos);
    void insertNodeBeforePosition(DOM::NodeImpl *node, const DOM::Position &pos);
};

//------------------------------------------------------------------------------------------
// InputTextCommandImpl

class InputTextCommandImpl : public CompositeEditCommandImpl
{
public:
    InputTextCommandImpl(DOM::DocumentImpl *document);

    virtual void doApply();

    void deleteCharacter();
    void input(const DOM::DOMString &text, bool selectInsertedText = false);
    
    unsigned long charactersAdded() const { return m_charactersAdded; }
    
private:
    virtual bool isInputTextCommand() const;

    DOM::Position prepareForTextInsertion(bool adjustDownstream);
    void insertSpace(DOM::TextImpl *textNode, unsigned long offset);

    unsigned long m_charactersAdded;
};

//------------------------------------------------------------------------------------------
// InsertNodeBeforeCommandImpl

class InsertNodeBeforeCommandImpl : public EditCommandImpl
{
public:
    InsertNodeBeforeCommandImpl(DOM::DocumentImpl *, DOM::NodeImpl *insertChild, DOM::NodeImpl *refChild);
    virtual ~InsertNodeBeforeCommandImpl();

    virtual void doApply();
    virtual void doUnapply();

    DOM::NodeImpl *insertChild() const { return m_insertChild; }
    DOM::NodeImpl *refChild() const { return m_refChild; }

private:
    DOM::NodeImpl *m_insertChild;
    DOM::NodeImpl *m_refChild; 
};

//------------------------------------------------------------------------------------------
// InsertTextCommandImpl

class InsertTextCommandImpl : public EditCommandImpl
{
public:
    InsertTextCommandImpl(DOM::DocumentImpl *document, DOM::TextImpl *, long, const DOM::DOMString &);
    virtual ~InsertTextCommandImpl();
	
    virtual void doApply();
    virtual void doUnapply();

    DOM::TextImpl *node() const { return m_node; }
    long offset() const { return m_offset; }
    DOM::DOMString text() const { return m_text; }

private:
    DOM::TextImpl *m_node;
    long m_offset;
    DOM::DOMString m_text;
};

//------------------------------------------------------------------------------------------
// JoinTextNodesCommandImpl

class JoinTextNodesCommandImpl : public EditCommandImpl
{
public:
    JoinTextNodesCommandImpl(DOM::DocumentImpl *, DOM::TextImpl *, DOM::TextImpl *);
    virtual ~JoinTextNodesCommandImpl();
	
    virtual void doApply();
    virtual void doUnapply();

    DOM::TextImpl *firstNode() const { return m_text1; }
    DOM::TextImpl *secondNode() const { return m_text2; }

private:
    DOM::TextImpl *m_text1;
    DOM::TextImpl *m_text2;
    unsigned long m_offset;
};

//------------------------------------------------------------------------------------------
// ReplaceSelectionCommandImpl

class ReplaceSelectionCommandImpl : public CompositeEditCommandImpl
{
public:
    ReplaceSelectionCommandImpl(DOM::DocumentImpl *document, DOM::DocumentFragmentImpl *fragment, bool selectReplacement=true, bool smartReplace=false);
    virtual ~ReplaceSelectionCommandImpl();
    
    virtual void doApply();

private:
    DOM::DocumentFragmentImpl *m_fragment;
    bool m_selectReplacement;
    bool m_smartReplace;
};

//------------------------------------------------------------------------------------------
// MoveSelectionCommandImpl

class MoveSelectionCommandImpl : public CompositeEditCommandImpl
{
public:
    MoveSelectionCommandImpl(DOM::DocumentImpl *document, DOM::DocumentFragmentImpl *fragment, DOM::Position &position, bool smartMove=false);
    virtual ~MoveSelectionCommandImpl();
    
    virtual void doApply();
    
private:
    DOM::DocumentFragmentImpl *m_fragment;
    DOM::Position m_position;
    bool m_smartMove;
};

//------------------------------------------------------------------------------------------
// RemoveCSSPropertyCommand

class RemoveCSSPropertyCommandImpl : public EditCommandImpl
{
public:
    RemoveCSSPropertyCommandImpl(DOM::DocumentImpl *, DOM::CSSStyleDeclarationImpl *, int property);
    virtual ~RemoveCSSPropertyCommandImpl();

    virtual void doApply();
    virtual void doUnapply();

    DOM::CSSStyleDeclarationImpl *styleDeclaration() const { return m_decl; }
    int property() const { return m_property; }
    
private:
    DOM::CSSStyleDeclarationImpl *m_decl;
    int m_property;
    DOM::DOMString m_oldValue;
    bool m_important;
};

//------------------------------------------------------------------------------------------
// RemoveNodeAttributeCommandImpl

class RemoveNodeAttributeCommandImpl : public EditCommandImpl
{
public:
    RemoveNodeAttributeCommandImpl(DOM::DocumentImpl *, DOM::ElementImpl *, DOM::NodeImpl::Id attribute);
    virtual ~RemoveNodeAttributeCommandImpl();

    virtual void doApply();
    virtual void doUnapply();

    DOM::ElementImpl *element() const { return m_element; }
    DOM::NodeImpl::Id attribute() const { return m_attribute; }
    
private:
    DOM::ElementImpl *m_element;
    DOM::NodeImpl::Id m_attribute;
    DOM::DOMString m_oldValue;
};

//------------------------------------------------------------------------------------------
// RemoveNodeCommandImpl

class RemoveNodeCommandImpl : public EditCommandImpl
{
public:
    RemoveNodeCommandImpl(DOM::DocumentImpl *, DOM::NodeImpl *);
    virtual ~RemoveNodeCommandImpl();
	
    virtual void doApply();
    virtual void doUnapply();

    DOM::NodeImpl *node() const { return m_removeChild; }

private:
    DOM::NodeImpl *m_parent;    
    DOM::NodeImpl *m_removeChild;
    DOM::NodeImpl *m_refChild;    
};

//------------------------------------------------------------------------------------------
// RemoveNodePreservingChildrenCommandImpl

class RemoveNodePreservingChildrenCommandImpl : public CompositeEditCommandImpl
{
public:
    RemoveNodePreservingChildrenCommandImpl(DOM::DocumentImpl *, DOM::NodeImpl *);
    virtual ~RemoveNodePreservingChildrenCommandImpl();
	
    virtual void doApply();

    DOM::NodeImpl *node() const { return m_node; }

private:
    DOM::NodeImpl *m_node;
};

//------------------------------------------------------------------------------------------
// SetNodeAttributeCommandImpl

class SetNodeAttributeCommandImpl : public EditCommandImpl
{
public:
    SetNodeAttributeCommandImpl(DOM::DocumentImpl *, DOM::ElementImpl *, DOM::NodeImpl::Id attribute, const DOM::DOMString &value);
    virtual ~SetNodeAttributeCommandImpl();

    virtual void doApply();
    virtual void doUnapply();

    DOM::ElementImpl *element() const { return m_element; }
    DOM::NodeImpl::Id attribute() const { return m_attribute; }
    DOM::DOMString value() const { return m_value; }
    
private:
    DOM::ElementImpl *m_element;
    DOM::NodeImpl::Id m_attribute;
    DOM::DOMString m_value;
    DOM::DOMString m_oldValue;
};

//------------------------------------------------------------------------------------------
// SplitTextNodeCommandImpl

class SplitTextNodeCommandImpl : public EditCommandImpl
{
public:
    SplitTextNodeCommandImpl(DOM::DocumentImpl *, DOM::TextImpl *, long);
    virtual ~SplitTextNodeCommandImpl();
	
    virtual void doApply();
    virtual void doUnapply();

    DOM::TextImpl *node() const { return m_text2; }
    long offset() const { return m_offset; }

private:
    DOM::TextImpl *m_text1;
    DOM::TextImpl *m_text2;
    unsigned long m_offset;
};

//------------------------------------------------------------------------------------------
// TypingCommandImpl

class TypingCommandImpl : public CompositeEditCommandImpl
{
public:
    TypingCommandImpl(DOM::DocumentImpl *document, TypingCommand::ETypingCommand, const DOM::DOMString &, bool selectInsertedText);
    
    virtual void doApply();

    bool openForMoreTyping() const { return m_openForMoreTyping; }
    void closeTyping() { m_openForMoreTyping = false; }

    void insertText(const DOM::DOMString &text, bool selectInsertedText);
    void insertNewline();
    void deleteKeyPressed();

private:
    virtual bool isTypingCommand() const;
    virtual bool preservesTypingStyle() const;

    void issueCommandForDeleteKey();
    void removeCommand(const EditCommand &);
    void markMisspellingsAfterTyping();
    void typingAddedToOpenCommand();
    
    TypingCommand::ETypingCommand m_commandType;
    DOM::DOMString m_textToInsert;
    bool m_openForMoreTyping;
    bool m_applyEditing;
    bool m_selectInsertedText;
};

//------------------------------------------------------------------------------------------

} // end namespace khtml

#endif

