/*
 * Copyright (C) 2005 Apple Computer, Inc.  All rights reserved.
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

#include "config.h"
#include "typing_command.h"

#include "insert_text_command.h"
#include "insert_line_break_command.h"
#include "insert_paragraph_separator_command.h"
#include "break_blockquote_command.h"

#include "MacFrame.h"
#include "DocumentImpl.h"
#include "visible_position.h"
#include "visible_units.h"

#include <kxmlcore/Assertions.h>

using DOM::DocumentImpl;
using DOM::DOMString;
using DOM::Position;

namespace khtml {

TypingCommand::TypingCommand(DocumentImpl *document, ETypingCommand commandType, const DOMString &textToInsert, bool selectInsertedText)
    : CompositeEditCommand(document), 
      m_commandType(commandType), 
      m_textToInsert(textToInsert), 
      m_openForMoreTyping(true), 
      m_applyEditing(false), 
      m_selectInsertedText(selectInsertedText),
      m_smartDelete(false)
{
}

void TypingCommand::deleteKeyPressed(DocumentImpl *document, bool smartDelete)
{
    ASSERT(document);
    
    Frame *frame = document->frame();
    ASSERT(frame);
    
    EditCommandPtr lastEditCommand = frame->lastEditCommand();
    if (isOpenForMoreTypingCommand(lastEditCommand)) {
        static_cast<TypingCommand *>(lastEditCommand.get())->deleteKeyPressed();
    }
    else {
        SelectionController selection = frame->selection();
        if (selection.isCaret() && VisiblePosition(selection.start(), selection.affinity()).previous().isNull()) {
            // do nothing for a delete key at the start of an editable element.
        }
        else {
            TypingCommand *typingCommand = new TypingCommand(document, DeleteKey);
            typingCommand->setSmartDelete(smartDelete);
            EditCommandPtr cmd(typingCommand);
            cmd.apply();
        }
    }
}

void TypingCommand::forwardDeleteKeyPressed(DocumentImpl *document, bool smartDelete)
{
    ASSERT(document);
    
    Frame *frame = document->frame();
    ASSERT(frame);
    
    EditCommandPtr lastEditCommand = frame->lastEditCommand();
    if (isOpenForMoreTypingCommand(lastEditCommand)) {
        static_cast<TypingCommand *>(lastEditCommand.get())->forwardDeleteKeyPressed();
    }
    else {
        SelectionController selection = frame->selection();
        if (selection.isCaret() && isEndOfDocument(VisiblePosition(selection.start(), selection.affinity()))) {
            // do nothing for a delete key at the start of an editable element.
        }
        else {
            TypingCommand *typingCommand = new TypingCommand(document, ForwardDeleteKey);
            typingCommand->setSmartDelete(smartDelete);
            EditCommandPtr cmd(typingCommand);
            cmd.apply();
        }
    }
}

void TypingCommand::insertText(DocumentImpl *document, const DOMString &text, bool selectInsertedText)
{
    ASSERT(document);
    
    Frame *frame = document->frame();
    ASSERT(frame);
    
    EditCommandPtr lastEditCommand = frame->lastEditCommand();
    if (isOpenForMoreTypingCommand(lastEditCommand)) {
        static_cast<TypingCommand *>(lastEditCommand.get())->insertText(text, selectInsertedText);
    }
    else {
        EditCommandPtr cmd(new TypingCommand(document, InsertText, text, selectInsertedText));
        cmd.apply();
    }
}

void TypingCommand::insertLineBreak(DocumentImpl *document)
{
    ASSERT(document);
    
    Frame *frame = document->frame();
    ASSERT(frame);
    
    EditCommandPtr lastEditCommand = frame->lastEditCommand();
    if (isOpenForMoreTypingCommand(lastEditCommand)) {
        static_cast<TypingCommand *>(lastEditCommand.get())->insertLineBreak();
    }
    else {
        EditCommandPtr cmd(new TypingCommand(document, InsertLineBreak));
        cmd.apply();
    }
}

void TypingCommand::insertParagraphSeparatorInQuotedContent(DocumentImpl *document)
{
    ASSERT(document);
    
    Frame *frame = document->frame();
    ASSERT(frame);
    
    EditCommandPtr lastEditCommand = frame->lastEditCommand();
    if (isOpenForMoreTypingCommand(lastEditCommand)) {
        static_cast<TypingCommand *>(lastEditCommand.get())->insertParagraphSeparatorInQuotedContent();
    }
    else {
        EditCommandPtr cmd(new TypingCommand(document, InsertParagraphSeparatorInQuotedContent));
        cmd.apply();
    }
}

void TypingCommand::insertParagraphSeparator(DocumentImpl *document)
{
    ASSERT(document);
    
    Frame *frame = document->frame();
    ASSERT(frame);
    
    EditCommandPtr lastEditCommand = frame->lastEditCommand();
    if (isOpenForMoreTypingCommand(lastEditCommand)) {
        static_cast<TypingCommand *>(lastEditCommand.get())->insertParagraphSeparator();
    }
    else {
        EditCommandPtr cmd(new TypingCommand(document, InsertParagraphSeparator));
        cmd.apply();
    }
}

bool TypingCommand::isOpenForMoreTypingCommand(const EditCommandPtr &cmd)
{
    return cmd.isTypingCommand() &&
        static_cast<const TypingCommand *>(cmd.get())->openForMoreTyping();
}

void TypingCommand::closeTyping(const EditCommandPtr &cmd)
{
    if (isOpenForMoreTypingCommand(cmd))
        static_cast<TypingCommand *>(cmd.get())->closeTyping();
}

void TypingCommand::doApply()
{
    if (endingSelection().isNone())
        return;

    switch (m_commandType) {
        case DeleteKey:
            deleteKeyPressed();
            return;
        case ForwardDeleteKey:
            forwardDeleteKeyPressed();
            return;
        case InsertLineBreak:
            insertLineBreak();
            return;
        case InsertParagraphSeparator:
            insertParagraphSeparator();
            return;
        case InsertParagraphSeparatorInQuotedContent:
            insertParagraphSeparatorInQuotedContent();
            return;
        case InsertText:
            insertText(m_textToInsert, m_selectInsertedText);
            return;
    }

    ASSERT_NOT_REACHED();
}

EditAction TypingCommand::editingAction() const
{
    return EditActionTyping;
}

void TypingCommand::markMisspellingsAfterTyping()
{
    // Take a look at the selection that results after typing and determine whether we need to spellcheck. 
    // Since the word containing the current selection is never marked, this does a check to
    // see if typing made a new word that is not in the current selection. Basically, you
    // get this by being at the end of a word and typing a space.    
    VisiblePosition start(endingSelection().start(), endingSelection().affinity());
    VisiblePosition previous = start.previous();
    if (previous.isNotNull()) {
        VisiblePosition p1 = startOfWord(previous, LeftWordIfOnBoundary);
        VisiblePosition p2 = startOfWord(start, LeftWordIfOnBoundary);
        if (p1 != p2)
            Mac(document()->frame())->markMisspellingsInAdjacentWords(p1);
    }
}

void TypingCommand::typingAddedToOpenCommand()
{
    markMisspellingsAfterTyping();
    // Do not apply editing to the frame on the first time through.
    // The frame will get told in the same way as all other commands.
    // But since this command stays open and is used for additional typing, 
    // we need to tell the frame here as other commands are added.
    if (m_applyEditing) {
        EditCommandPtr cmd(this);
        document()->frame()->appliedEditing(cmd);
    }
    m_applyEditing = true;
}

void TypingCommand::insertText(const DOMString &text, bool selectInsertedText)
{
    // FIXME: Need to implement selectInsertedText for cases where more than one insert is involved.
    // This requires support from insertTextRunWithoutNewlines and insertParagraphSeparator for extending
    // an existing selection; at the moment they can either put the caret after what's inserted or
    // select what's inserted, but there's no way to "extend selection" to include both an old selection
    // that ends just before where we want to insert text and the newly inserted text.
    int offset = 0;
    int newline;
    while ((newline = text.find('\n', offset)) != -1) {
        if (newline != offset) {
            insertTextRunWithoutNewlines(text.substring(offset, newline - offset), false);
        }
        insertParagraphSeparator();
        offset = newline + 1;
    }
    if (offset == 0) {
        insertTextRunWithoutNewlines(text, selectInsertedText);
    } else {
        int length = text.length();
        if (length != offset) {
            insertTextRunWithoutNewlines(text.substring(offset, length - offset), selectInsertedText);
        }
    }
}

void TypingCommand::insertTextRunWithoutNewlines(const DOMString &text, bool selectInsertedText)
{
    // FIXME: Improve typing style.
    // See this bug: <rdar://problem/3769899> Implementation of typing style needs improvement
    if (document()->frame()->typingStyle() || m_cmds.count() == 0) {
        InsertTextCommand *impl = new InsertTextCommand(document());
        EditCommandPtr cmd(impl);
        applyCommandToComposite(cmd);
        impl->input(text, selectInsertedText);
    }
    else {
        EditCommandPtr lastCommand = m_cmds.last();
        if (lastCommand.isInsertTextCommand()) {
            InsertTextCommand *impl = static_cast<InsertTextCommand *>(lastCommand.get());
            impl->input(text, selectInsertedText);
        }
        else {
            InsertTextCommand *impl = new InsertTextCommand(document());
            EditCommandPtr cmd(impl);
            applyCommandToComposite(cmd);
            impl->input(text, selectInsertedText);
        }
    }
    typingAddedToOpenCommand();
}

void TypingCommand::insertLineBreak()
{
    EditCommandPtr cmd(new InsertLineBreakCommand(document()));
    applyCommandToComposite(cmd);
    typingAddedToOpenCommand();
}

void TypingCommand::insertParagraphSeparator()
{
    EditCommandPtr cmd(new InsertParagraphSeparatorCommand(document()));
    applyCommandToComposite(cmd);
    typingAddedToOpenCommand();
}

void TypingCommand::insertParagraphSeparatorInQuotedContent()
{
    EditCommandPtr cmd(new BreakBlockquoteCommand(document()));
    applyCommandToComposite(cmd);
    typingAddedToOpenCommand();
}

void TypingCommand::deleteKeyPressed()
{
    SelectionController selectionToDelete;
    
    switch (endingSelection().state()) {
        case khtml::Selection::RANGE:
            selectionToDelete = endingSelection();
            break;
        case khtml::Selection::CARET: {
            // Handle delete at beginning-of-block case.
            // Do nothing in the case that the caret is at the start of a
            // root editable element or at the start of a document.
            Position pos(endingSelection().start());
            Position start = VisiblePosition(pos, endingSelection().affinity()).previous().deepEquivalent();
            Position end = VisiblePosition(pos, endingSelection().affinity()).deepEquivalent();
            if (start.isNotNull() && end.isNotNull() && start.node()->rootEditableElement() == end.node()->rootEditableElement())
                selectionToDelete = SelectionController(start, end, SEL_DEFAULT_AFFINITY);
            break;
        }
        case khtml::Selection::NONE:
            ASSERT_NOT_REACHED();
            break;
    }
    
    if (selectionToDelete.isCaretOrRange()) {
        deleteSelection(selectionToDelete, m_smartDelete);
        setSmartDelete(false);
        typingAddedToOpenCommand();
    }
}

void TypingCommand::forwardDeleteKeyPressed()
{
    SelectionController selectionToDelete;
    
    switch (endingSelection().state()) {
        case khtml::Selection::RANGE:
            selectionToDelete = endingSelection();
            break;
        case khtml::Selection::CARET: {
            // Handle delete at beginning-of-block case.
            // Do nothing in the case that the caret is at the start of a
            // root editable element or at the start of a document.
            Position pos(endingSelection().start());
            Position start = VisiblePosition(pos, endingSelection().affinity()).next().deepEquivalent();
            Position end = VisiblePosition(pos, endingSelection().affinity()).deepEquivalent();
            if (start.isNotNull() && end.isNotNull() && start.node()->rootEditableElement() == end.node()->rootEditableElement())
                selectionToDelete = SelectionController(start, end, SEL_DEFAULT_AFFINITY);
            break;
        }
        case khtml::Selection::NONE:
            ASSERT_NOT_REACHED();
            break;
    }
    
    if (selectionToDelete.isCaretOrRange()) {
        deleteSelection(selectionToDelete, m_smartDelete);
        setSmartDelete(false);
        typingAddedToOpenCommand();
    }
}

bool TypingCommand::preservesTypingStyle() const
{
    switch (m_commandType) {
        case DeleteKey:
        case ForwardDeleteKey:
        case InsertParagraphSeparator:
        case InsertLineBreak:
            return true;
        case InsertParagraphSeparatorInQuotedContent:
        case InsertText:
            return false;
    }
    ASSERT_NOT_REACHED();
    return false;
}

bool TypingCommand::isTypingCommand() const
{
    return true;
}

} // namespace khtml
