/*
 * Copyright (C) 2003 Apple Computer, Inc.  All rights reserved.
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

#include "htmlediting.h"

#if APPLE_CHANGES
#include "KWQAssertions.h"
#include "KWQLogging.h"
#endif

#include "khtmlview.h"
#include "khtml_part.h"
#include "khtml_selection.h"
#include "dom/dom_position.h"
#include "html/html_elementimpl.h"
#include "rendering/render_object.h"
#include "xml/dom_docimpl.h"
#include "xml/dom_elementimpl.h"
#include "xml/dom_nodeimpl.h"
#include "xml/dom2_rangeimpl.h"
#include "xml/dom_textimpl.h"

using DOM::DocumentFragmentImpl;
using DOM::DocumentImpl;
using DOM::DOMPosition;
using DOM::DOMString;
using DOM::ElementImpl;
using DOM::HTMLElementImpl;
using DOM::Node;
using DOM::NodeImpl;
using DOM::NodeListImpl;
using DOM::Range;
using DOM::RangeImpl;
using DOM::TextImpl;

using khtml::AppendNodeCommand;
using khtml::CompositeEditCommand;
using khtml::DeleteKeyCommand;
using khtml::DeleteSelectionCommand;
using khtml::DeleteTextCommand;
using khtml::EditCommand;
using khtml::InputTextCommand;
using khtml::InsertNodeBeforeCommand;
using khtml::InsertTextCommand;
using khtml::JoinTextNodesCommand;
using khtml::ModifyTextNodeCommand;
using khtml::RemoveNodeCommand;
using khtml::MoveSelectionToCommand;
using khtml::PasteHTMLCommand;
using khtml::SplitTextNodeCommand;

#if !APPLE_CHANGES
#define ASSERT(assertion) ((void)0)
#define ASSERT_WITH_MESSAGE(assertion, formatAndArgs...) ((void)0)
#define ASSERT_NOT_REACHED() ((void)0)
#define LOG(channel, formatAndArgs...) ((void)0)
#define ERROR(formatAndArgs...) ((void)0)
#endif

//------------------------------------------------------------------------------------------
// EditCommand

EditCommand::EditCommand(DocumentImpl *document) : m_document(document), m_state(NOT_APPLIED)
{
    ASSERT(m_document);
    ASSERT(m_document->part());
    m_document->ref();
    m_startingSelection = m_document->part()->selection();
    m_endingSelection = m_startingSelection;
}

EditCommand::~EditCommand()
{
    ASSERT(m_document);
    m_document->deref();
}

int EditCommand::commandID() const
{
    return EditCommandID;
}

void EditCommand::reapply()
{
    apply();
}

inline void EditCommand::beginApply()
{
    ASSERT(state() == NOT_APPLIED);
}

inline void EditCommand::endApply()
{
    m_state = APPLIED;
    moveToEndingSelection();
}

inline void EditCommand::beginUnapply()
{
    ASSERT(state() == APPLIED);
}

inline void EditCommand::endUnapply()
{
    m_state = NOT_APPLIED;
    moveToStartingSelection();
}

inline void EditCommand::beginReapply()
{
    beginApply();
}

inline void EditCommand::endReapply()
{
    endApply();
}

const KHTMLSelection &EditCommand::currentSelection() const
{
    ASSERT(m_document);
    ASSERT(m_document->part());
    return m_document->part()->selection();
}

bool EditCommand::coalesce(const EditCommandPtr &cmd)
{
    // default is no coalescing
    return false;
}

bool EditCommand::groupForUndo(const EditCommandPtr &) const
{
    // default is not continuing
    return false;
}

bool EditCommand::groupForRedo(const EditCommandPtr &) const
{
    // default is not continuing
    return false;
}

void EditCommand::moveToStartingSelection()
{
    ASSERT(m_document);
    ASSERT(m_document->part());
    m_document->part()->setSelection(m_startingSelection);
}

void EditCommand::moveToEndingSelection()
{
    ASSERT(m_document);
    ASSERT(m_document->part());
    m_document->part()->setSelection(m_endingSelection);
}

QString EditCommand::name() const
{
    switch (commandID()) {
        case EditCommandID:
            return "EditCommandID";
            break;
        case InputTextCommandID:
            return "InputTextCommandID";
            break;
        case DeleteKeyCommandID:
            return "DeleteKeyCommandID";
            break;
        case MoveSelectionToCommandID:
            return "MoveSelectionToCommand";
            break;
    }
    
    ASSERT_NOT_REACHED();
    return "";
}

//------------------------------------------------------------------------------------------
// CompositeEditCommand

CompositeEditCommand::CompositeEditCommand(DocumentImpl *document) 
    : EditCommand(document)
{
}

CompositeEditCommand::~CompositeEditCommand()
{
}

void CompositeEditCommand::unapply()
{
    if (m_cmds.count() == 0) {
        ERROR("Unapplying composite command containing zero steps");
    }
    for (int i = m_cmds.count() - 1; i >= 0; --i) {
        m_cmds[i]->unapply();
    }
    moveToStartingSelection();
    setState(NOT_APPLIED);
}

void CompositeEditCommand::reapply()
{
    if (m_cmds.count() == 0) {
        ERROR("Reapplying composite command containing zero steps");
    }
    for (QValueList<EditCommandPtr>::ConstIterator it = m_cmds.begin(); it != m_cmds.end(); ++it) {
        (*it)->reapply();
    }
    moveToEndingSelection();
    setState(APPLIED);
}

//
// sugary-sweet convenience functions to help create and apply edit commands in composite commands
//
void CompositeEditCommand::applyCommand(const EditCommandPtr &step)
{
    step->apply();
    m_cmds.append(step);
}

void CompositeEditCommand::insertNodeBefore(DOM::NodeImpl *insertChild, DOM::NodeImpl *refChild)
{
    applyCommand(EditCommandPtr(new InsertNodeBeforeCommand(document(), insertChild, refChild)));
}

void CompositeEditCommand::insertNodeAfter(DOM::NodeImpl *insertChild, DOM::NodeImpl *refChild)
{
    if (refChild->parentNode()->lastChild() == refChild) {
        appendNode(refChild->parentNode(), insertChild);
    }
    else {
        ASSERT(refChild->nextSibling());
        insertNodeBefore(insertChild, refChild->nextSibling());
    }
}

void CompositeEditCommand::appendNode(DOM::NodeImpl *parent, DOM::NodeImpl *appendChild)
{
    applyCommand(EditCommandPtr(new AppendNodeCommand(document(), parent, appendChild)));
}

void CompositeEditCommand::removeNode(DOM::NodeImpl *removeChild)
{
    applyCommand(EditCommandPtr(new RemoveNodeCommand(document(), removeChild)));
}

void CompositeEditCommand::splitTextNode(DOM::TextImpl *text, long offset)
{
    applyCommand(EditCommandPtr(new SplitTextNodeCommand(document(), text, offset)));
}

void CompositeEditCommand::joinTextNodes(DOM::TextImpl *text1, DOM::TextImpl *text2)
{
    applyCommand(EditCommandPtr(new JoinTextNodesCommand(document(), text1, text2)));
}

void CompositeEditCommand::insertText(DOM::TextImpl *node, long offset, const DOM::DOMString &text)
{
    applyCommand(EditCommandPtr(new InsertTextCommand(document(), node, offset, text)));
}

void CompositeEditCommand::deleteText(DOM::TextImpl *node, long offset, long count)
{
    applyCommand(EditCommandPtr(new DeleteTextCommand(document(), node, offset, count)));
}

void CompositeEditCommand::moveSelectionTo(const KHTMLSelection &selection)
{
    applyCommand(EditCommandPtr(new MoveSelectionToCommand(document(), selection)));
}

void CompositeEditCommand::moveSelectionTo(DOM::NodeImpl *node, long offset)
{
    applyCommand(EditCommandPtr(new MoveSelectionToCommand(document(), node, offset)));
}

void CompositeEditCommand::moveSelectionTo(const DOM::DOMPosition &pos)
{
    applyCommand(EditCommandPtr(new MoveSelectionToCommand(document(), pos)));
}

void CompositeEditCommand::deleteSelection()
{
    if (currentSelection().state() == KHTMLSelection::RANGE) {
        applyCommand(EditCommandPtr(new DeleteSelectionCommand(document())));
    }
}

void CompositeEditCommand::deleteSelection(const KHTMLSelection &selection)
{
    if (selection.state() == KHTMLSelection::RANGE) {
        applyCommand(EditCommandPtr(new DeleteSelectionCommand(document(), selection)));
    }
}

//------------------------------------------------------------------------------------------
// InsertNodeBeforeCommand

InsertNodeBeforeCommand::InsertNodeBeforeCommand(DocumentImpl *document, NodeImpl *insertChild, NodeImpl *refChild)
    : EditCommand(document), m_insertChild(insertChild), m_refChild(refChild)
{
    ASSERT(m_insertChild);
    m_insertChild->ref();

    ASSERT(m_refChild);
    m_refChild->ref();
}

InsertNodeBeforeCommand::~InsertNodeBeforeCommand()
{
    if (m_insertChild)
        m_insertChild->deref();
    if (m_refChild)
        m_refChild->deref();
}

void InsertNodeBeforeCommand::apply()
{
    beginApply();

    ASSERT(m_insertChild);
    ASSERT(m_refChild);
    ASSERT(m_refChild->parent());

    int exceptionCode;
    m_refChild->parent()->insertBefore(m_insertChild, m_refChild, exceptionCode);
    ASSERT(exceptionCode == 0);

    endApply();
}

void InsertNodeBeforeCommand::unapply()
{
    beginUnapply();

    ASSERT(m_insertChild);
    ASSERT(m_refChild);
    ASSERT(m_refChild->parent());

    int exceptionCode;
    m_refChild->parent()->removeChild(m_insertChild, exceptionCode);
    ASSERT(exceptionCode == 0);

    endUnapply();
}

//------------------------------------------------------------------------------------------
// AppendNodeCommand

AppendNodeCommand::AppendNodeCommand(DocumentImpl *document, NodeImpl *parent, NodeImpl *appendChild)
    : EditCommand(document), m_parent(parent), m_appendChild(appendChild)
{
    ASSERT(m_parent);
    m_parent->ref();

    ASSERT(m_appendChild);
    m_appendChild->ref();
}

AppendNodeCommand::~AppendNodeCommand()
{
    if (m_parent)
        m_parent->deref();
    if (m_appendChild)
        m_appendChild->deref();
}

void AppendNodeCommand::apply()
{
    beginApply();

    ASSERT(m_parent);
    ASSERT(m_appendChild);

    int exceptionCode;
    m_parent->appendChild(m_appendChild, exceptionCode);
    ASSERT(exceptionCode == 0);

    endApply();
}

void AppendNodeCommand::unapply()
{
    beginUnapply();

    ASSERT(m_parent);
    ASSERT(m_appendChild);
    ASSERT(state() == APPLIED);

    int exceptionCode;
    m_parent->removeChild(m_appendChild, exceptionCode);
    ASSERT(exceptionCode == 0);

    endUnapply();
}

//------------------------------------------------------------------------------------------
// RemoveNodeCommand

RemoveNodeCommand::RemoveNodeCommand(DocumentImpl *document, NodeImpl *removeChild)
    : EditCommand(document), m_parent(0), m_removeChild(removeChild), m_refChild(0)
{
    ASSERT(m_removeChild);
    m_removeChild->ref();

    m_parent = m_removeChild->parentNode();
    ASSERT(m_parent);
    m_parent->ref();
    
    NodeListImpl *children = m_parent->childNodes();
    for (int i = children->length(); i >= 0; i--) {
        NodeImpl *node = children->item(i);
        if (node == m_removeChild)
            break;
        m_refChild = node;
    }
    
    if (m_refChild)
        m_refChild->ref();
}

RemoveNodeCommand::~RemoveNodeCommand()
{
    if (m_parent)
        m_parent->deref();
    if (m_removeChild)
        m_removeChild->deref();
    if (m_refChild)
        m_refChild->deref();
}

void RemoveNodeCommand::apply()
{
    beginApply();

    ASSERT(m_parent);
    ASSERT(m_removeChild);

    int exceptionCode;
    m_parent->removeChild(m_removeChild, exceptionCode);
    ASSERT(exceptionCode == 0);

    endApply();
}

void RemoveNodeCommand::unapply()
{
    beginUnapply();

    ASSERT(m_parent);
    ASSERT(m_removeChild);

    int exceptionCode;
    if (m_refChild)
        m_parent->insertBefore(m_removeChild, m_refChild, exceptionCode);
    else
        m_parent->appendChild(m_removeChild, exceptionCode);
    ASSERT(exceptionCode == 0);

    endUnapply();
}

//------------------------------------------------------------------------------------------
// ModifyTextNodeCommand

ModifyTextNodeCommand::ModifyTextNodeCommand(DocumentImpl *document, TextImpl *text, long offset)
    : EditCommand(document), m_text1(0), m_text2(text), m_offset(offset)
{
    ASSERT(m_text2);
    ASSERT(m_text2->length() > 0);
    ASSERT(m_offset >= 0);

    m_text2->ref();
}

ModifyTextNodeCommand::ModifyTextNodeCommand(DocumentImpl *document, TextImpl *text1, TextImpl *text2)
    : EditCommand(document), m_text1(text1), m_text2(text2), m_offset(0)
{
    ASSERT(m_text1);
    ASSERT(m_text2);
    ASSERT(m_text1->nextSibling() == m_text2);
    ASSERT(m_text1->length() > 0);
    ASSERT(m_text2->length() > 0);

    m_text1->ref();
    m_text2->ref();
}

ModifyTextNodeCommand::~ModifyTextNodeCommand()
{
    if (m_text2)
        m_text2->deref();
    if (m_text1)
        m_text1->deref();
}

void ModifyTextNodeCommand::splitTextNode()
{
    ASSERT(m_text2);
    ASSERT(m_text1 == 0);
    ASSERT(m_offset > 0);
    ASSERT(state() == splitState());

    ASSERT(m_offset >= m_text2->caretMinOffset() && m_offset <= m_text2->caretMaxOffset());

    int exceptionCode;
    m_text1 = document()->createTextNode(m_text2->substringData(0, m_offset, exceptionCode));
    ASSERT(exceptionCode == 0);
    ASSERT(m_text1);
    m_text1->ref();

    m_text2->deleteData(0, m_offset, exceptionCode);
    ASSERT(exceptionCode == 0);

    m_text2->parentNode()->insertBefore(m_text1, m_text2, exceptionCode);
    ASSERT(exceptionCode == 0);
        
    ASSERT(m_text2->previousSibling()->isTextNode());
    m_text1 = static_cast<TextImpl *>(m_text2->previousSibling());
}

void ModifyTextNodeCommand::joinTextNodes()
{
    ASSERT(m_text1);
    ASSERT(m_text2);
    ASSERT(state() == joinState());
    
    ASSERT(m_text1->nextSibling() == m_text2);

    int exceptionCode;
    m_text2->insertData(0, m_text1->data(), exceptionCode);
    ASSERT(exceptionCode == 0);

    m_text2->parent()->removeChild(m_text1, exceptionCode);
    ASSERT(exceptionCode == 0);

    m_offset = m_text1->length();
    m_text1->deref();
    m_text1 = 0;
}

//------------------------------------------------------------------------------------------
// SplitTextNodeCommand

SplitTextNodeCommand::SplitTextNodeCommand(DocumentImpl *document, TextImpl *text, long offset)
    : ModifyTextNodeCommand(document, text, offset)
{
}

SplitTextNodeCommand::~SplitTextNodeCommand()
{
}

void SplitTextNodeCommand::apply()
{
    beginApply();
    splitTextNode();
    endApply();
}

void SplitTextNodeCommand::unapply()
{
    beginUnapply();
    joinTextNodes();
    endUnapply();
}

//------------------------------------------------------------------------------------------
// SplitTextNodeCommand

JoinTextNodesCommand::JoinTextNodesCommand(DocumentImpl *document, TextImpl *text1, TextImpl *text2)
    : ModifyTextNodeCommand(document, text1, text2)
{
}

JoinTextNodesCommand::~JoinTextNodesCommand()
{
}

void JoinTextNodesCommand::apply()
{
    beginApply();
    joinTextNodes();
    endApply();
}

void JoinTextNodesCommand::unapply()
{
    beginUnapply();
    splitTextNode();
    endUnapply();
}

//------------------------------------------------------------------------------------------
// InsertTextCommand

InsertTextCommand::InsertTextCommand(DocumentImpl *document, TextImpl *node, long offset, const DOMString &text)
    : EditCommand(document), m_node(node), m_offset(offset)
{
    ASSERT(m_node);
    ASSERT(m_offset >= 0);
    ASSERT(text.length() > 0);
    
    m_node->ref();
    m_text = text.copy(); // make a copy to ensure that the string never changes
}

InsertTextCommand::~InsertTextCommand()
{
    if (m_node)
        m_node->deref();
}

void InsertTextCommand::apply()
{
    beginApply();

    ASSERT(m_node);
    ASSERT(!m_text.isEmpty());

    int exceptionCode;
    m_node->insertData(m_offset, m_text, exceptionCode);
    ASSERT(exceptionCode == 0);

    endApply();
}

void InsertTextCommand::unapply()
{
    beginUnapply();

    ASSERT(m_node);
    ASSERT(!m_text.isEmpty());

    int exceptionCode;
    m_node->deleteData(m_offset, m_text.length(), exceptionCode);
    ASSERT(exceptionCode == 0);

    endUnapply();
}

//------------------------------------------------------------------------------------------
// DeleteTextCommand

DeleteTextCommand::DeleteTextCommand(DocumentImpl *document, TextImpl *node, long offset, long count)
    : EditCommand(document), m_node(node), m_offset(offset), m_count(count)
{
    ASSERT(m_node);
    ASSERT(m_offset >= 0);
    ASSERT(m_count >= 0);
    
    m_node->ref();
}

DeleteTextCommand::~DeleteTextCommand()
{
    if (m_node)
        m_node->deref();
}

void DeleteTextCommand::apply()
{
    beginApply();

    ASSERT(m_node);

    int exceptionCode;
    m_text = m_node->substringData(m_offset, m_count, exceptionCode);
    ASSERT(exceptionCode == 0);
    
    m_node->deleteData(m_offset, m_count, exceptionCode);
    ASSERT(exceptionCode == 0);

    endApply();
}

void DeleteTextCommand::unapply()
{
    beginUnapply();

    ASSERT(m_node);
    ASSERT(!m_text.isEmpty());

    int exceptionCode;
    m_node->insertData(m_offset, m_text, exceptionCode);
    ASSERT(exceptionCode == 0);

    endUnapply();
}

//------------------------------------------------------------------------------------------
// MoveSelectionToCommand

MoveSelectionToCommand::MoveSelectionToCommand(DocumentImpl *document, const KHTMLSelection &selection)
    : EditCommand(document)
{
    setEndingSelection(selection);
}

MoveSelectionToCommand::MoveSelectionToCommand(DocumentImpl *document, DOM::NodeImpl *node, long offset)
    : EditCommand(document)
{
    KHTMLSelection selection(node, offset);
    setEndingSelection(selection);
}

MoveSelectionToCommand::MoveSelectionToCommand(DOM::DocumentImpl *document, const DOM::DOMPosition &pos)
    : EditCommand(document)
{
    KHTMLSelection selection(pos);
    setEndingSelection(selection);
}

MoveSelectionToCommand::~MoveSelectionToCommand() 
{
}

int MoveSelectionToCommand::commandID() const
{
    return MoveSelectionToCommandID;
}


void MoveSelectionToCommand::apply()
{
    beginApply();
    endApply();
}

void MoveSelectionToCommand::unapply()
{
    beginUnapply();
    endUnapply();
}

//------------------------------------------------------------------------------------------
// DeleteSelectionCommand

DeleteSelectionCommand::DeleteSelectionCommand(DOM::DocumentImpl *document)
    : CompositeEditCommand(document)
{
}

DeleteSelectionCommand::DeleteSelectionCommand(DOM::DocumentImpl *document, const KHTMLSelection &selection)
    : CompositeEditCommand(document)
{
    setStartingSelection(selection);
}

DeleteSelectionCommand::~DeleteSelectionCommand()
{
}
	
void DeleteSelectionCommand::apply()
{
    beginApply();

    if (startingSelection().isEmpty()) {
        endApply();
        return;
    }

    KHTMLSelection selection = startingSelection();

    //
    // Figure out where to place the caret after doing the delete:
    //
    // 1. If the start node is not completely selected, use the start 
    //    node and start offset for the new position; else
    // 2. If the start and end nodes are completely selected:
    //       a. If there is an editable node following the end node, 
    //          place the caret in the min caret offset of that node; else
    //       b. If there is an editable node before the start node, 
    //          place the caret in the max caret offset of that node; else
    //       c. There is no more editable content in the document.
    //          EDIT FIXME: We do not handle this case now
    // 3. If the start node is completely selected and the end node is 
    //    different than the start node and it is not completely selected,
    //    use the end node and the end node min caret for the new position; else
    //

    DOMPosition deleteStart = DOMPosition(selection.startNode(), selection.startOffset());
    DOMPosition deleteEnd = DOMPosition(selection.endNode(), selection.endOffset());

    bool startIsCompletelySelected = 
        deleteStart.offset() == deleteStart.node()->caretMinOffset() &&
        ((deleteStart.node() != deleteEnd.node()) ||
         (deleteEnd.offset() == deleteEnd.node()->caretMaxOffset()));

    bool endIsCompletelySelected = 
        deleteEnd.offset() == deleteEnd.node()->caretMaxOffset() &&
        ((deleteStart.node() != deleteEnd.node()) ||
         (deleteStart.offset() == deleteStart.node()->caretMinOffset()));

    DOMPosition endingPosition;

    if (!startIsCompletelySelected) {
        // Case 1
        endingPosition = DOMPosition(deleteStart.node(), deleteStart.offset());
    }
    else if (endIsCompletelySelected) {
        DOMPosition pos = selection.nextCharacterPosition(deleteEnd);
        if (pos != deleteEnd) {
            // Case 2a
            endingPosition = DOMPosition(pos.node(), pos.node()->caretMinOffset());
        }
        else {
            pos = selection.previousCharacterPosition(deleteStart);
            if (pos != deleteStart) {
                // Case 2b
                endingPosition = DOMPosition(pos.node(), pos.node()->caretMaxOffset());
            }
            else {
                // Case 2c
                // EDIT FIXME
                endingPosition = DOMPosition();
            }
        }
    }
    else {
        // Case 3
        endingPosition = DOMPosition(deleteEnd.node(), deleteEnd.node()->caretMinOffset());
    }

    //
    // Do the delete
    //
    NodeImpl *n = deleteStart.node()->nextLeafNode();

    // work on start node
    if (startIsCompletelySelected) {
        removeNode(deleteStart.node());
    }
    else if (deleteStart.node()->isTextNode()) {
        TextImpl *text = static_cast<TextImpl *>(deleteStart.node());
        int endOffset = text == deleteEnd.node() ? deleteEnd.offset() : text->length();
        if (endOffset > deleteStart.offset()) {
            deleteText(text, deleteStart.offset(), endOffset - deleteStart.offset());
        }
    }
    else {
        ASSERT_NOT_REACHED();
    }

    if (deleteStart.node() != deleteEnd.node()) {
        // work on intermediate nodes
        while (n != deleteEnd.node()) {
            NodeImpl *d = n;
            n = n->nextLeafNode();
            removeNode(d);
        }
        
        // work on end node
        ASSERT(n == deleteEnd.node());
        if (endIsCompletelySelected) {
            removeNode(deleteEnd.node());
        }
        else if (deleteEnd.node()->isTextNode()) {
            if (deleteEnd.offset() > 0) {
                TextImpl *text = static_cast<TextImpl *>(deleteEnd.node());
                deleteText(text, 0, deleteEnd.offset());
            }
        }
        else {
            ASSERT_NOT_REACHED();
        }
    }

    //
    // set the ending selection
    //
    selection.moveTo(endingPosition);
    selection.moveToRenderedContent();
    setEndingSelection(selection);

    endApply();
}

//------------------------------------------------------------------------------------------
// InputTextCommand

InputTextCommand::InputTextCommand(DocumentImpl *document, const DOMString &text) 
    : CompositeEditCommand(document)
{
    ASSERT(!text.isEmpty());
    m_text = text; 
}

InputTextCommand::~InputTextCommand() 
{
}

int InputTextCommand::commandID() const
{
    return InputTextCommandID;
}

bool InputTextCommand::isLineBreak(const DOM::DOMString &text) const
{
    return text.length() == 1 && (text[0] == '\n' || text[0] == '\r');
}

bool InputTextCommand::isSpace(const DOM::DOMString &text) const
{
    return text.length() == 1 && (text[0] == ' ');
}

void InputTextCommand::apply()
{
    beginApply();
    execute(m_text);
    endApply();
}

bool InputTextCommand::coalesce(const EditCommandPtr &cmd)
{
    ASSERT(state() == APPLIED);
    
    if (cmd->commandID() == InputTextCommandID && endingSelection() == cmd->startingSelection()) {
        // InputTextCommands coalesce with each other if they 
        const InputTextCommand *c = static_cast<const InputTextCommand *>(cmd.get());
        execute(c->text());
        m_text += c->text();
        moveToEndingSelection();
        return true;
    }

    return false;
}

bool InputTextCommand::groupForUndo(const EditCommandPtr &cmd) const
{
    if ((cmd->commandID() == InputTextCommandID || cmd->commandID() == DeleteKeyCommandID) && startingSelection() == cmd->endingSelection()) {
        return true;
    }
    return false;
}

bool InputTextCommand::groupForRedo(const EditCommandPtr &cmd) const
{
    if ((cmd->commandID() == InputTextCommandID || cmd->commandID() == DeleteKeyCommandID) && endingSelection() == cmd->startingSelection()) {
        return true;
    }
    return false;
}

void InputTextCommand::execute(const DOM::DOMString &text)
{
    KHTMLSelection selection = currentSelection();

    if (!selection.startNode()->isTextNode())
        return;

    // Delete the current selection
    deleteSelection();
    
    TextImpl *textNode = static_cast<TextImpl *>(selection.startNode());
    
    if (isLineBreak(text)) {
        int exceptionCode;
        ElementImpl *breakNode = document()->createHTMLElement("BR", exceptionCode);

        bool atStart = selection.startOffset() == textNode->renderer()->caretMinOffset();
        bool atEnd = selection.startOffset() == textNode->renderer()->caretMaxOffset();
        if (atStart) {
            // Set the cursor at the beginning of text node now following the new BR.
            insertNodeBefore(breakNode, textNode);
            selection = KHTMLSelection(textNode, 0);
            setEndingSelection(selection);
        }
        else if (atEnd) {
            insertNodeAfter(breakNode, textNode);
            // Set the cursor at the beginning of the the BR.
            selection = selection.nextCharacterPosition();
            setEndingSelection(selection);
        }
        else {
            TextImpl *textBeforeNode = document()->createTextNode(textNode->substringData(0, selection.startOffset(), exceptionCode));
            deleteText(textNode, 0, selection.startOffset());
            insertNodeBefore(textBeforeNode, textNode);
            insertNodeBefore(breakNode, textNode);
            textBeforeNode->deref();
            // Set the cursor at the beginning of the node after the BR.
            selection = KHTMLSelection(textNode, 0);
            setEndingSelection(selection);
        }
        
        breakNode->deref();
    }
    else {
        insertText(textNode, selection.startOffset(), text);
        selection = KHTMLSelection(selection.startNode(), selection.startOffset() + text.length());
        setEndingSelection(selection);
    }
}

//------------------------------------------------------------------------------------------
// DeleteKeyCommand

DeleteKeyCommand::DeleteKeyCommand(DocumentImpl *document) 
    : CompositeEditCommand(document)
{
}

DeleteKeyCommand::~DeleteKeyCommand() 
{
}

int DeleteKeyCommand::commandID() const
{
    return DeleteKeyCommandID;
}

void DeleteKeyCommand::apply()
{
    beginApply();

    KHTMLPart *part = document()->part();
    ASSERT(part);

    KHTMLSelection selection = part->selection();
    ASSERT(!selection.isEmpty());

    // Delete the current selection
    if (selection.state() == KHTMLSelection::RANGE) {
        deleteSelection();
        endApply();
        return;
    }

    NodeImpl *caretNode = selection.startNode();

    if (caretNode->isTextNode()) {
        // Check if we can delete character at cursor
        int offset = selection.startOffset() - 1;
        if (offset >= caretNode->caretMinOffset()) {
            TextImpl *textNode = static_cast<TextImpl *>(caretNode);
            deleteText(textNode, offset, 1);
            selection = KHTMLSelection(textNode, offset);
            setEndingSelection(selection);
            endApply();
            return;
        }
        
        // Check if previous sibling is a BR element
        NodeImpl *previousSibling = caretNode->previousSibling();
        if (previousSibling->renderer() && previousSibling->renderer()->isBR()) {
            removeNode(previousSibling);
            endApply();
            return;
        }
        
        // Check if previous leaf node is a text node
        NodeImpl *previousLeafNode = caretNode->previousLeafNode();
        if (previousLeafNode->isTextNode()) {
            TextImpl *textNode = static_cast<TextImpl *>(previousLeafNode);
            offset = previousLeafNode->caretMaxOffset() - 1;
            deleteText(textNode, offset, 1);
            selection = KHTMLSelection(textNode, offset);
            setEndingSelection(selection);
            endApply();
            return;
        }
    }
}

bool DeleteKeyCommand::groupForUndo(const EditCommandPtr &cmd) const
{
    if ((cmd->commandID() == InputTextCommandID || cmd->commandID() == DeleteKeyCommandID) && startingSelection() == cmd->endingSelection()) {
        return true;
    }

    return false;
}

bool DeleteKeyCommand::groupForRedo(const EditCommandPtr &cmd) const
{
    if ((cmd->commandID() == InputTextCommandID || cmd->commandID() == DeleteKeyCommandID) && endingSelection() == cmd->startingSelection()) {
        return true;
    }

    return false;
}

PasteHTMLCommand::PasteHTMLCommand(DocumentImpl *document, const DOMString &HTMLString) 
    : CompositeEditCommand(document)
{
    ASSERT(!HTMLString.isEmpty());
    m_HTMLString = HTMLString; 
}

PasteHTMLCommand::~PasteHTMLCommand()
{
}

void PasteHTMLCommand::apply()
{
    beginApply();
    
    DOM::DocumentFragmentImpl *root = static_cast<HTMLElementImpl *>(document()->documentElement())->createContextualFragment(m_HTMLString);
    ASSERT(root);
    
    DOM::NodeImpl *firstChild = root->firstChild();
    DOM::NodeImpl *lastChild = root->lastChild();
    ASSERT(firstChild);
    ASSERT(lastChild);
    
    deleteSelection();
    
    KHTMLPart *part = document()->part();
    ASSERT(part);
    
    KHTMLSelection selection = part->selection();
    ASSERT(!selection.isEmpty());
    
    DOM::NodeImpl *startNode = selection.startNode();
    long startOffset = selection.startOffset();
    TextImpl *textNode = startNode->isTextNode() ? static_cast<TextImpl *>(startNode) : NULL;

    if (textNode && firstChild == lastChild && firstChild->isTextNode()) {
        // Simple text paste. Add the text to the text node with the caret.
        insertText(textNode, startOffset, static_cast<TextImpl *>(firstChild)->data());
        selection = KHTMLSelection(textNode, startOffset + static_cast<TextImpl *>(firstChild)->length());
        setEndingSelection(selection);
    } 
    else {
        // HTML tree paste.
        DOM::NodeImpl *child = firstChild;
        DOM::NodeImpl *beforeNode = NULL;
        if (startNode->caretMinOffset() == startOffset) {
            // Caret is at the beginning of the node. Insert before it.
            DOM::NodeImpl *nextSibling = child->nextSibling();
            insertNodeBefore(child, startNode);
            beforeNode = child;
            child = nextSibling;
        } 
        else if (textNode && textNode->caretMaxOffset() != startOffset) {
            // Caret is in middle of a text node. Split the text node and insert in between.
            splitTextNode(textNode, startOffset);
            beforeNode = textNode->previousSibling();
        } 
        else {
            // Caret is at the end of the node. Insert after it.
            beforeNode = startNode;
        }
        
        ASSERT(beforeNode);
		
        // Insert the nodes from the clipping.
        while (child) {
            DOM::NodeImpl *nextSibling = child->nextSibling();
            insertNodeAfter(child, beforeNode);
            beforeNode = child;
            child = nextSibling;
        }
		
		// Find the last leaf and place the caret after it.
        child = lastChild;
        while (1) {
            DOM::NodeImpl *nextChild = child->lastChild();
            if (!nextChild) {
                break;
            }
            child = nextChild;
        }
        selection = KHTMLSelection(child, child->caretMaxOffset());
        setEndingSelection(selection);
    }

    endApply();
}

