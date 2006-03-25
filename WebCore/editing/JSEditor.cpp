/*
 * Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.  All rights reserved.
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
#include "JSEditor.h"

#include "CreateLinkCommand.h"
#include "UnlinkCommand.h"
#include "Document.h"
#include "Frame.h"
#include "SelectionController.h"
#include "css_valueimpl.h"
#include "CSSPropertyNames.h"
#include "htmlediting.h"
#include "HTMLNames.h"
#include "html_imageimpl.h"
#include "markup.h"
#include "ReplaceSelectionCommand.h"
#include "TypingCommand.h"
#include <kxmlcore/HashMap.h>

namespace WebCore {

class Document;

using namespace HTMLNames;

namespace {

bool supportsPasteCommand = false;

struct CommandImp {
    bool (*execFn)(Frame *frame, bool userInterface, const String &value);
    bool (*enabledFn)(Frame *frame);
    Frame::TriState (*stateFn)(Frame *frame);
    String (*valueFn)(Frame *frame);
};

typedef HashMap<StringImpl *, const CommandImp *, CaseInsensitiveHash> CommandMap;

CommandMap *createCommandDictionary();

const CommandImp *commandImp(const String &command)
{
    static CommandMap *commandDictionary = createCommandDictionary();
    return commandDictionary->get(command.impl());
}

} // anonymous namespace

bool JSEditor::execCommand(const String &command, bool userInterface, const String &value)
{
    const CommandImp *cmd = commandImp(command);
    if (!cmd)
        return false;
    Frame *frame = m_doc->frame();
    if (!frame)
        return false;
    m_doc->updateLayoutIgnorePendingStylesheets();
    return cmd->enabledFn(frame) && cmd->execFn(frame,  userInterface, value);
}

bool JSEditor::queryCommandEnabled(const String &command)
{
    const CommandImp *cmd = commandImp(command);
    if (!cmd)
        return false;
    Frame *frame = m_doc->frame();
    if (!frame)
        return false;
    m_doc->updateLayoutIgnorePendingStylesheets();
    return cmd->enabledFn(frame);
}

bool JSEditor::queryCommandIndeterm(const String &command)
{
    const CommandImp *cmd = commandImp(command);
    if (!cmd)
        return false;
    Frame *frame = m_doc->frame();
    if (!frame)
        return false;
    m_doc->updateLayoutIgnorePendingStylesheets();
    return cmd->stateFn(frame) == Frame::mixedTriState;
}

bool JSEditor::queryCommandState(const String &command)
{
    const CommandImp *cmd = commandImp(command);
    if (!cmd)
        return false;
    Frame *frame = m_doc->frame();
    if (!frame)
        return false;
    m_doc->updateLayoutIgnorePendingStylesheets();
    return cmd->stateFn(frame) != Frame::falseTriState;
}

bool JSEditor::queryCommandSupported(const String &command)
{
    if (!supportsPasteCommand && command.lower() == "paste")
        return false;
    return commandImp(command) != 0;
}

String JSEditor::queryCommandValue(const String &command)
{
    const CommandImp *cmd = commandImp(command);
    if (!cmd)
        return String();
    Frame *frame = m_doc->frame();
    if (!frame)
        return String();
    m_doc->updateLayoutIgnorePendingStylesheets();
    return cmd->valueFn(frame);
}

void JSEditor::setSupportsPasteCommand(bool flag)
{
    supportsPasteCommand = flag;
}

// =============================================================================================

// Private stuff, all inside an anonymous namespace.

namespace {

bool execStyleChange(Frame *frame, int propertyID, const String &propertyValue)
{
    RefPtr<CSSMutableStyleDeclaration> style = new CSSMutableStyleDeclaration;
    style->setProperty(propertyID, propertyValue);
    frame->applyStyle(style.get());
    return true;
}

bool execStyleChange(Frame *frame, int propertyID, const char *propertyValue)
{
    return execStyleChange(frame,  propertyID, String(propertyValue));
}

Frame::TriState stateStyle(Frame *frame, int propertyID, const char *desiredValue)
{
    RefPtr<CSSMutableStyleDeclaration> style = new CSSMutableStyleDeclaration;
    style->setProperty(propertyID, desiredValue);
    return frame->selectionHasStyle(style.get());
}

bool selectionStartHasStyle(Frame *frame, int propertyID, const char *desiredValue)
{
    RefPtr<CSSMutableStyleDeclaration> style = new CSSMutableStyleDeclaration;
    style->setProperty(propertyID, desiredValue);
    return frame->selectionStartHasStyle(style.get());
}

String valueStyle(Frame *frame, int propertyID)
{
    return frame->selectionStartStylePropertyValue(propertyID);
}

// =============================================================================================
//
// execCommand implementations
//

bool execBackColor(Frame *frame, bool userInterface, const String &value)
{
    return execStyleChange(frame,  CSS_PROP_BACKGROUND_COLOR, value);
}

bool execBold(Frame *frame, bool userInterface, const String &value)
{
    bool isBold = selectionStartHasStyle(frame,  CSS_PROP_FONT_WEIGHT, "bold");
    return execStyleChange(frame,  CSS_PROP_FONT_WEIGHT, isBold ? "normal" : "bold");
}

bool execCopy(Frame *frame, bool userInterface, const String &value)
{
    frame->copyToPasteboard();
    return true;
}

bool execCreateLink(Frame* frame, bool userInterface, const String& value)
{
    // FIXME: If userInterface is true, we should display a dialog box to let the user enter a url.
    if (userInterface)
        LOG_ERROR("A dialog box for link creation is not yet implemented.\n");
    
    if (value.isEmpty())
        return false;
    
    EditCommandPtr(new CreateLinkCommand(frame->document(), value)).apply();
    return true;
}

bool execCut(Frame *frame, bool userInterface, const String &value)
{
    frame->cutToPasteboard();
    return true;
}

bool execDelete(Frame *frame, bool userInterface, const String &value)
{
    TypingCommand::deleteKeyPressed(frame->document(), frame->selectionGranularity() == WordGranularity);
    return true;
}

bool execForwardDelete(Frame *frame, bool userInterface, const String &value)
{
    TypingCommand::forwardDeleteKeyPressed(frame->document());
    return true;
}

bool execFontName(Frame *frame, bool userInterface, const String &value)
{
    return execStyleChange(frame,  CSS_PROP_FONT_FAMILY, value);
}

bool execFontSize(Frame *frame, bool userInterface, const String &value)
{
    return execStyleChange(frame,  CSS_PROP_FONT_SIZE, value);
}

bool execFontSizeDelta(Frame *frame, bool userInterface, const String &value)
{
    return execStyleChange(frame,  CSS_PROP__KHTML_FONT_SIZE_DELTA, value);
}

bool execForeColor(Frame *frame, bool userInterface, const String &value)
{
    return execStyleChange(frame,  CSS_PROP_COLOR, value);
}

bool execInsertHTML(Frame* frame, bool userInterface, const String& value)
{
    DeprecatedString baseURL = "";
    bool selectReplacement = false;
    RefPtr<DocumentFragment> fragment = createFragmentFromMarkup(frame->document(), value.deprecatedString(), baseURL);
    EditCommandPtr cmd(new ReplaceSelectionCommand(frame->document(), fragment.get(), selectReplacement));
    cmd.apply();
    return true;
}

bool execInsertImage(Frame* frame, bool userInterface, const String& value)
{
    // FIXME: If userInterface is true, we should display a dialog box and let the user choose a local image.
    if (userInterface)
        LOG_ERROR("A dialog box for image insertion is not yet implemented.\n");
    
    RefPtr<HTMLImageElement> image = new HTMLImageElement(imgTag, frame->document());
    image->setSrc(value);
    RefPtr<DocumentFragment> fragment = new DocumentFragment(frame->document());
    ExceptionCode ec = 0;
    fragment->appendChild(image, ec);
    if (ec)
        return false;
    
    EditCommandPtr(new ReplaceSelectionCommand(frame->document(), fragment.get(), false)).apply();
    return true;
}

bool execIndent(Frame *frame, bool userInterface, const String &value)
{
    // FIXME: Implement.
    return false;
}

bool execInsertLineBreak(Frame *frame, bool userInterface, const String &value)
{
    TypingCommand::insertLineBreak(frame->document());
    return true;
}

bool execInsertParagraph(Frame *frame, bool userInterface, const String &value)
{
    TypingCommand::insertParagraphSeparator(frame->document());
    return true;
}

bool execInsertNewlineInQuotedContent(Frame *frame, bool userInterface, const String &value)
{
    TypingCommand::insertParagraphSeparatorInQuotedContent(frame->document());
    return true;
}

bool execInsertText(Frame *frame, bool userInterface, const String &value)
{
    TypingCommand::insertText(frame->document(), value);
    return true;
}

bool execItalic(Frame *frame, bool userInterface, const String &value)
{
    bool isItalic = selectionStartHasStyle(frame,  CSS_PROP_FONT_STYLE, "italic");
    return execStyleChange(frame,  CSS_PROP_FONT_STYLE, isItalic ? "normal" : "italic");
}

bool execJustifyCenter(Frame *frame, bool userInterface, const String &value)
{
    return execStyleChange(frame,  CSS_PROP_TEXT_ALIGN, "center");
}

bool execJustifyFull(Frame *frame, bool userInterface, const String &value)
{
    return execStyleChange(frame,  CSS_PROP_TEXT_ALIGN, "justify");
}

bool execJustifyLeft(Frame *frame, bool userInterface, const String &value)
{
    return execStyleChange(frame,  CSS_PROP_TEXT_ALIGN, "left");
}

bool execJustifyRight(Frame *frame, bool userInterface, const String &value)
{
    return execStyleChange(frame,  CSS_PROP_TEXT_ALIGN, "right");
}

bool execOutdent(Frame *frame, bool userInterface, const String &value)
{
    // FIXME: Implement.
    return false;
}

bool execPaste(Frame *frame, bool userInterface, const String &value)
{
    frame->pasteFromPasteboard();
    return true;
}

bool execPasteAndMatchStyle(Frame *frame, bool userInterface, const String &value)
{
    frame->pasteAndMatchStyle();
    return true;
}

bool execPrint(Frame *frame, bool userInterface, const String &value)
{
    frame->print();
    return true;
}

bool execRedo(Frame *frame, bool userInterface, const String &value)
{
    frame->redo();
    return true;
}

bool execSelectAll(Frame *frame, bool userInterface, const String &value)
{
    frame->selectAll();
    return true;
}

bool execStrikethrough(Frame *frame, bool userInterface, const String &value)
{
    bool isStrikethrough = selectionStartHasStyle(frame,  CSS_PROP__KHTML_TEXT_DECORATIONS_IN_EFFECT, "line-through");
    return execStyleChange(frame,  CSS_PROP__KHTML_TEXT_DECORATIONS_IN_EFFECT, isStrikethrough ? "none" : "line-through");
}

bool execSubscript(Frame *frame, bool userInterface, const String &value)
{
    return execStyleChange(frame,  CSS_PROP_VERTICAL_ALIGN, "sub");
}

bool execSuperscript(Frame *frame, bool userInterface, const String &value)
{
    return execStyleChange(frame,  CSS_PROP_VERTICAL_ALIGN, "super");
}

bool execTranspose(Frame *frame, bool userInterface, const String &value)
{
    frame->transpose();
    return true;
}

bool execUnderline(Frame *frame, bool userInterface, const String &value)
{
    bool isUnderlined = selectionStartHasStyle(frame,  CSS_PROP__KHTML_TEXT_DECORATIONS_IN_EFFECT, "underline");
    return execStyleChange(frame,  CSS_PROP__KHTML_TEXT_DECORATIONS_IN_EFFECT, isUnderlined ? "none" : "underline");
}

bool execUndo(Frame *frame, bool userInterface, const String &value)
{
    frame->undo();
    return true;
}

bool execUnlink(Frame *frame, bool userInterface, const String &value)
{
    EditCommandPtr(new UnlinkCommand(frame->document())).apply();
    return true;
}

bool execUnselect(Frame *frame, bool userInterface, const String &value)
{
    // FIXME: 6498 Should just be able to call m_frame->selection().clear()
    frame->setSelection(WebCore::SelectionController());
    return true;
}

// =============================================================================================
//
// queryCommandEnabled implementations
//
// It's a bit difficult to get a clear notion of the difference between
// "supported" and "enabled" from reading the Microsoft documentation, but
// what little I could glean from that seems to make some sense.
//     Supported = The command is supported by this object.
//     Enabled =   The command is available and enabled.

bool enabled(Frame *frame)
{
    return true;
}

bool enabledAnySelection(Frame *frame)
{
    return frame->selection().isCaretOrRange();
}

bool enabledPaste(Frame *frame)
{
    return supportsPasteCommand && frame->canPaste();
}

bool enabledPasteAndMatchStyle(Frame *frame)
{
    return supportsPasteCommand && frame->canPaste();
}

bool enabledRangeSelection(Frame *frame)
{
    return frame->selection().isRange();
}

bool enabledRedo(Frame *frame)
{
    return frame->canRedo();
}

bool enabledUndo(Frame *frame)
{
    return frame->canUndo();
}

// =============================================================================================
//
// queryCommandIndeterm/State implementations
//
// It's a bit difficult to get a clear notion of what these methods are supposed
// to do from reading the Microsoft documentation, but my current guess is this:
//
//     queryCommandState and queryCommandIndeterm work in concert to return
//     the two bits of information that are needed to tell, for instance,
//     if the text of a selection is bold. The answer can be "yes", "no", or
//     "partially".
//
// If this is so, then queryCommandState should return "yes" in the case where
// all the text is bold and "no" for non-bold or partially-bold text.
// Then, queryCommandIndeterm should return "no" in the case where
// all the text is either all bold or not-bold and and "yes" for partially-bold text.

Frame::TriState stateNone(Frame *frame)
{
    return Frame::falseTriState;
}

Frame::TriState stateBold(Frame *frame)
{
    return stateStyle(frame,  CSS_PROP_FONT_WEIGHT, "bold");
}

Frame::TriState stateItalic(Frame *frame)
{
    return stateStyle(frame,  CSS_PROP_FONT_STYLE, "italic");
}

Frame::TriState stateStrikethrough(Frame *frame)
{
    return stateStyle(frame,  CSS_PROP_TEXT_DECORATION, "line-through");
}

Frame::TriState stateSubscript(Frame *frame)
{
    return stateStyle(frame,  CSS_PROP_VERTICAL_ALIGN, "sub");
}

Frame::TriState stateSuperscript(Frame *frame)
{
    return stateStyle(frame,  CSS_PROP_VERTICAL_ALIGN, "super");
}

Frame::TriState stateUnderline(Frame *frame)
{
    return stateStyle(frame,  CSS_PROP_TEXT_DECORATION, "underline");
}

// =============================================================================================
//
// queryCommandValue implementations
//

String valueNull(Frame *frame)
{
    return String();
}

String valueBackColor(Frame *frame)
{
    return valueStyle(frame,  CSS_PROP_BACKGROUND_COLOR);
}

String valueFontName(Frame *frame)
{
    return valueStyle(frame,  CSS_PROP_FONT_FAMILY);
}

String valueFontSize(Frame *frame)
{
    return valueStyle(frame,  CSS_PROP_FONT_SIZE);
}

String valueFontSizeDelta(Frame *frame)
{
    return valueStyle(frame,  CSS_PROP__KHTML_FONT_SIZE_DELTA);
}

String valueForeColor(Frame *frame)
{
    return valueStyle(frame,  CSS_PROP_COLOR);
}

// =============================================================================================

CommandMap *createCommandDictionary()
{
    struct EditorCommand { const char *name; CommandImp imp; };

    static const EditorCommand commands[] = {

        { "BackColor", { execBackColor, enabled, stateNone, valueBackColor } },
        { "Bold", { execBold, enabledAnySelection, stateBold, valueNull } },
        { "Copy", { execCopy, enabledRangeSelection, stateNone, valueNull } },
        { "CreateLink", { execCreateLink, enabledRangeSelection, stateNone, valueNull } },
        { "Cut", { execCut, enabledRangeSelection, stateNone, valueNull } },
        { "Delete", { execDelete, enabledAnySelection, stateNone, valueNull } },
        { "FontName", { execFontName, enabledAnySelection, stateNone, valueFontName } },
        { "FontSize", { execFontSize, enabledAnySelection, stateNone, valueFontSize } },
        { "FontSizeDelta", { execFontSizeDelta, enabledAnySelection, stateNone, valueFontSizeDelta } },
        { "ForeColor", { execForeColor, enabledAnySelection, stateNone, valueForeColor } },
        { "ForwardDelete", { execForwardDelete, enabledAnySelection, stateNone, valueNull } },
        { "Indent", { execIndent, enabledAnySelection, stateNone, valueNull } },
        { "InsertHTML", { execInsertHTML, enabledAnySelection, stateNone, valueNull } },
        { "InsertImage", { execInsertImage, enabledAnySelection, stateNone, valueNull } },
        { "InsertLineBreak", { execInsertLineBreak, enabledAnySelection, stateNone, valueNull } },
        { "InsertParagraph", { execInsertParagraph, enabledAnySelection, stateNone, valueNull } },
        { "InsertNewlineInQuotedContent", { execInsertNewlineInQuotedContent, enabledAnySelection, stateNone, valueNull } },
        { "InsertText", { execInsertText, enabledAnySelection, stateNone, valueNull } },
        { "Italic", { execItalic, enabledAnySelection, stateItalic, valueNull } },
        { "JustifyCenter", { execJustifyCenter, enabledAnySelection, stateNone, valueNull } },
        { "JustifyFull", { execJustifyFull, enabledAnySelection, stateNone, valueNull } },
        { "JustifyLeft", { execJustifyLeft, enabledAnySelection, stateNone, valueNull } },
        { "JustifyNone", { execJustifyLeft, enabledAnySelection, stateNone, valueNull } },
        { "JustifyRight", { execJustifyRight, enabledAnySelection, stateNone, valueNull } },
        { "Outdent", { execOutdent, enabledAnySelection, stateNone, valueNull } },
        { "Paste", { execPaste, enabledPaste, stateNone, valueNull } },
        { "PasteAndMatchStyle", { execPasteAndMatchStyle, enabledPasteAndMatchStyle, stateNone, valueNull } },
        { "Print", { execPrint, enabled, stateNone, valueNull } },
        { "Redo", { execRedo, enabledRedo, stateNone, valueNull } },
        { "SelectAll", { execSelectAll, enabled, stateNone, valueNull } },
        { "Strikethrough", { execStrikethrough, enabledAnySelection, stateStrikethrough, valueNull } },
        { "Subscript", { execSubscript, enabledAnySelection, stateSubscript, valueNull } },
        { "Superscript", { execSuperscript, enabledAnySelection, stateSuperscript, valueNull } },
        { "Transpose", { execTranspose, enabled, stateNone, valueNull } },
        { "Underline", { execUnderline, enabledAnySelection, stateUnderline, valueNull } },
        { "Undo", { execUndo, enabledUndo, stateNone, valueNull } },
        { "Unlink", { execUnlink, enabledRangeSelection, stateNone, valueNull } },
        { "Unselect", { execUnselect, enabledAnySelection, stateNone, valueNull } }

        //
        // The "unsupported" commands are listed here since they appear in the Microsoft
        // documentation used as the basis for the list.
        //

        // 2D-Position (not supported)
        // AbsolutePosition (not supported)
        // BlockDirLTR (not supported)
        // BlockDirRTL (not supported)
        // BrowseMode (not supported)
        // ClearAuthenticationCache (not supported)
        // CreateBookmark (not supported)
        // DirLTR (not supported)
        // DirRTL (not supported)
        // EditMode (not supported)
        // FormatBlock (not supported)
        // InlineDirLTR (not supported)
        // InlineDirRTL (not supported)
        // InsertButton (not supported)
        // InsertFieldSet (not supported)
        // InsertHorizontalRule (not supported)
        // InsertIFrame (not supported)
        // InsertInputButton (not supported)
        // InsertInputCheckbox (not supported)
        // InsertInputFileUpload (not supported)
        // InsertInputHidden (not supported)
        // InsertInputImage (not supported)
        // InsertInputPassword (not supported)
        // InsertInputRadio (not supported)
        // InsertInputReset (not supported)
        // InsertInputSubmit (not supported)
        // InsertInputText (not supported)
        // InsertMarquee (not supported)
        // InsertOrderedList (not supported)
        // InsertSelectDropDown (not supported)
        // InsertSelectListBox (not supported)
        // InsertTextArea (not supported)
        // InsertUnorderedList (not supported)
        // LiveResize (not supported)
        // MultipleSelection (not supported)
        // Open (not supported)
        // Overwrite (not supported)
        // PlayImage (not supported)
        // Refresh (not supported)
        // RemoveFormat (not supported)
        // RemoveParaFormat (not supported)
        // SaveAs (not supported)
        // SizeToControl (not supported)
        // SizeToControlHeight (not supported)
        // SizeToControlWidth (not supported)
        // Stop (not supported)
        // StopImage (not supported)
        // Unbookmark (not supported)
    };

    CommandMap *commandMap = new CommandMap;

    const int numCommands = sizeof(commands) / sizeof(commands[0]);
    for (int i = 0; i < numCommands; ++i) {
        StringImpl *name = new StringImpl(commands[i].name);
        name->ref();
        commandMap->set(name, &commands[i].imp);
    }
#ifndef NDEBUG
    supportsPasteCommand = true;
#endif
    return commandMap;
}

} // anonymous namespace

} // namespace WebCore
