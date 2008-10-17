/**
 * This file is part of the theme implementation for form controls in WebCore.
 *
 * Copyright (C) 2005, 2006, 2007, 2008 Apple Computer, Inc.
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
 */

#include "config.h"
#include "RenderTheme.h"

#include "CSSValueKeywords.h"
#include "Document.h"
#include "FocusController.h"
#include "Frame.h"
#include "GraphicsContext.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "Page.h"
#include "RenderStyle.h"
#include "SelectionController.h"
#include "Settings.h"

// The methods in this file are shared by all themes on every platform.

namespace WebCore {

using namespace HTMLNames;

RenderTheme::RenderTheme()
#if USE(NEW_THEME)
    : m_theme(platformTheme())
#endif
{
}

void RenderTheme::adjustStyle(CSSStyleSelector* selector, RenderStyle* style, Element* e,
                              bool UAHasAppearance, const BorderData& border, const FillLayer& background, const Color& backgroundColor)
{
    // Force inline and table display styles to be inline-block (except for table- which is block)
    if (style->display() == INLINE || style->display() == INLINE_TABLE || style->display() == TABLE_ROW_GROUP ||
        style->display() == TABLE_HEADER_GROUP || style->display() == TABLE_FOOTER_GROUP ||
        style->display() == TABLE_ROW || style->display() == TABLE_COLUMN_GROUP || style->display() == TABLE_COLUMN ||
        style->display() == TABLE_CELL || style->display() == TABLE_CAPTION)
        style->setDisplay(INLINE_BLOCK);
    else if (style->display() == COMPACT || style->display() == RUN_IN || style->display() == LIST_ITEM || style->display() == TABLE)
        style->setDisplay(BLOCK);

    if (UAHasAppearance && theme()->isControlStyled(style, border, background, backgroundColor)) {
        if (style->appearance() == MenulistPart)
            style->setAppearance(MenulistButtonPart);
        else
            style->setAppearance(NoControlPart);
    }

    // Call the appropriate style adjustment method based off the appearance value.
    switch (style->appearance()) {
        case CheckboxPart:
            return adjustCheckboxStyle(selector, style, e);
        case RadioPart:
            return adjustRadioStyle(selector, style, e);
        case PushButtonPart:
        case SquareButtonPart:
        case DefaultButtonPart:
        case ButtonPart:
            return adjustButtonStyle(selector, style, e);
        case TextFieldPart:
            return adjustTextFieldStyle(selector, style, e);
        case TextAreaPart:
            return adjustTextAreaStyle(selector, style, e);
        case MenulistPart:
            return adjustMenuListStyle(selector, style, e);
        case MenulistButtonPart:
            return adjustMenuListButtonStyle(selector, style, e);
        case MediaSliderPart:
        case SliderHorizontalPart:
        case SliderVerticalPart:
            return adjustSliderTrackStyle(selector, style, e);
        case SliderThumbHorizontalPart:
        case SliderThumbVerticalPart:
            return adjustSliderThumbStyle(selector, style, e);
        case SearchFieldPart:
            return adjustSearchFieldStyle(selector, style, e);
        case SearchFieldCancelButtonPart:
            return adjustSearchFieldCancelButtonStyle(selector, style, e);
        case SearchFieldDecorationPart:
            return adjustSearchFieldDecorationStyle(selector, style, e);
        case SearchFieldResultsDecorationPart:
            return adjustSearchFieldResultsDecorationStyle(selector, style, e);
        case SearchFieldResultsButtonPart:
            return adjustSearchFieldResultsButtonStyle(selector, style, e);
        default:
            break;
    }
}

#if !PLATFORM(QT)
void RenderTheme::adjustDefaultStyleSheet(CSSStyleSheet*)
{
}
#endif

bool RenderTheme::paint(RenderObject* o, const RenderObject::PaintInfo& paintInfo, const IntRect& r)
{
    // If painting is disabled, but we aren't updating control tints, then just bail.
    // If we are updating control tints, just schedule a repaint if the theme supports tinting
    // for that control.
    if (paintInfo.context->updatingControlTints()) {
        if (controlSupportsTints(o))
            o->repaint();
        return false;
    }
    if (paintInfo.context->paintingDisabled())
        return false;

    // Call the appropriate paint method based off the appearance value.
    switch (o->style()->appearance()) {
        case CheckboxPart:
            return paintCheckbox(o, paintInfo, r);
        case RadioPart:
            return paintRadio(o, paintInfo, r);
        case PushButtonPart:
        case SquareButtonPart:
        case DefaultButtonPart:
        case ButtonPart:
            return paintButton(o, paintInfo, r);
        case MenulistPart:
            return paintMenuList(o, paintInfo, r);
        case SliderHorizontalPart:
        case SliderVerticalPart:
            return paintSliderTrack(o, paintInfo, r);
        case SliderThumbHorizontalPart:
        case SliderThumbVerticalPart:
            if (o->parent()->isSlider())
                return paintSliderThumb(o, paintInfo, r);
            // We don't support drawing a slider thumb without a parent slider
            break;
        case MediaFullscreenButtonPart:
            return paintMediaFullscreenButton(o, paintInfo, r);
        case MediaPlayButtonPart:
            return paintMediaPlayButton(o, paintInfo, r);
        case MediaMuteButtonPart:
            return paintMediaMuteButton(o, paintInfo, r);
        case MediaSeekBackButtonPart:
            return paintMediaSeekBackButton(o, paintInfo, r);
        case MediaSeekForwardButtonPart:
            return paintMediaSeekForwardButton(o, paintInfo, r);
        case MediaSliderPart:
            return paintMediaSliderTrack(o, paintInfo, r);
        case MediaSliderThumbPart:
            if (o->parent()->isSlider())
                return paintMediaSliderThumb(o, paintInfo, r);
            break;
        case MenulistButtonPart:
        case TextFieldPart:
        case TextAreaPart:
        case ListboxPart:
            return true;
        case SearchFieldPart:
            return paintSearchField(o, paintInfo, r);
        case SearchFieldCancelButtonPart:
            return paintSearchFieldCancelButton(o, paintInfo, r);
        case SearchFieldDecorationPart:
            return paintSearchFieldDecoration(o, paintInfo, r);
        case SearchFieldResultsDecorationPart:
            return paintSearchFieldResultsDecoration(o, paintInfo, r);
        case SearchFieldResultsButtonPart:
            return paintSearchFieldResultsButton(o, paintInfo, r);
        default:
            break;
    }

    return true; // We don't support the appearance, so let the normal background/border paint.
}

bool RenderTheme::paintBorderOnly(RenderObject* o, const RenderObject::PaintInfo& paintInfo, const IntRect& r)
{
    if (paintInfo.context->paintingDisabled())
        return false;

    // Call the appropriate paint method based off the appearance value.
    switch (o->style()->appearance()) {
        case TextFieldPart:
            return paintTextField(o, paintInfo, r);
        case ListboxPart:
        case TextAreaPart:
            return paintTextArea(o, paintInfo, r);
        case MenulistButtonPart:
            return true;
        case CheckboxPart:
        case RadioPart:
        case PushButtonPart:
        case SquareButtonPart:
        case DefaultButtonPart:
        case ButtonPart:
        case MenulistPart:
        case SliderHorizontalPart:
        case SliderVerticalPart:
        case SliderThumbHorizontalPart:
        case SliderThumbVerticalPart:
        case SearchFieldPart:
        case SearchFieldCancelButtonPart:
        case SearchFieldDecorationPart:
        case SearchFieldResultsDecorationPart:
        case SearchFieldResultsButtonPart:
        default:
            break;
    }

    return false;
}

bool RenderTheme::paintDecorations(RenderObject* o, const RenderObject::PaintInfo& paintInfo, const IntRect& r)
{
    if (paintInfo.context->paintingDisabled())
        return false;

    // Call the appropriate paint method based off the appearance value.
    switch (o->style()->appearance()) {
        case MenulistButtonPart:
            return paintMenuListButton(o, paintInfo, r);
        case TextFieldPart:
        case TextAreaPart:
        case ListboxPart:
        case CheckboxPart:
        case RadioPart:
        case PushButtonPart:
        case SquareButtonPart:
        case DefaultButtonPart:
        case ButtonPart:
        case MenulistPart:
        case SliderHorizontalPart:
        case SliderVerticalPart:
        case SliderThumbHorizontalPart:
        case SliderThumbVerticalPart:
        case SearchFieldPart:
        case SearchFieldCancelButtonPart:
        case SearchFieldDecorationPart:
        case SearchFieldResultsDecorationPart:
        case SearchFieldResultsButtonPart:
        default:
            break;
    }

    return false;
}

Color RenderTheme::activeSelectionBackgroundColor() const
{
    if (!m_activeSelectionColor.isValid())
        m_activeSelectionColor = platformActiveSelectionBackgroundColor().blendWithWhite();
    return m_activeSelectionColor;
}

Color RenderTheme::inactiveSelectionBackgroundColor() const
{
    if (!m_inactiveSelectionColor.isValid())
        m_inactiveSelectionColor = platformInactiveSelectionBackgroundColor().blendWithWhite();
    return m_inactiveSelectionColor;
}

Color RenderTheme::platformActiveSelectionBackgroundColor() const
{
    // Use a blue color by default if the platform theme doesn't define anything.
    return Color(0, 0, 255);
}

Color RenderTheme::platformInactiveSelectionBackgroundColor() const
{
    // Use a grey color by default if the platform theme doesn't define anything.
    return Color(128, 128, 128);
}

Color RenderTheme::platformActiveSelectionForegroundColor() const
{
    return Color();
}

Color RenderTheme::platformInactiveSelectionForegroundColor() const
{
    return Color();
}

Color RenderTheme::activeListBoxSelectionBackgroundColor() const
{
    return activeSelectionBackgroundColor();
}

Color RenderTheme::activeListBoxSelectionForegroundColor() const
{
    // Use a white color by default if the platform theme doesn't define anything.
    return Color(255, 255, 255);
}

Color RenderTheme::inactiveListBoxSelectionBackgroundColor() const
{
    return inactiveSelectionBackgroundColor();
}

Color RenderTheme::inactiveListBoxSelectionForegroundColor() const
{
    // Use a black color by default if the platform theme doesn't define anything.
    return Color(0, 0, 0);
}

int RenderTheme::baselinePosition(const RenderObject* o) const
{
    return o->height() + o->marginTop();
}

bool RenderTheme::isControlContainer(ControlPart appearance) const
{
    // There are more leaves than this, but we'll patch this function as we add support for
    // more controls.
    return appearance != CheckboxPart && appearance != RadioPart;
}

bool RenderTheme::isControlStyled(const RenderStyle* style, const BorderData& border, const FillLayer& background,
                                  const Color& backgroundColor) const
{
    switch (style->appearance()) {
        case PushButtonPart:
        case SquareButtonPart:
        case DefaultButtonPart:
        case ButtonPart:
        case ListboxPart:
        case MenulistPart:
        // FIXME: Uncomment this when making search fields style-able.
        // case SearchFieldPart:
        case TextFieldPart:
        case TextAreaPart:
            // Test the style to see if the UA border and background match.
            return (style->border() != border ||
                    *style->backgroundLayers() != background ||
                    style->backgroundColor() != backgroundColor);
        default:
            return false;
    }
}

bool RenderTheme::supportsFocusRing(const RenderStyle* style) const
{
    return (style->hasAppearance() && style->appearance() != TextFieldPart && style->appearance() != TextAreaPart && style->appearance() != MenulistButtonPart && style->appearance() != ListboxPart);
}

bool RenderTheme::stateChanged(RenderObject* o, ControlState state) const
{
    // Default implementation assumes the controls dont respond to changes in :hover state
    if (state == HoverState && !supportsHover(o->style()))
        return false;

    // Assume pressed state is only responded to if the control is enabled.
    if (state == PressedState && !isEnabled(o))
        return false;

    // Repaint the control.
    o->repaint();
    return true;
}

// FIXME: It would be nice to set this state on the RenderObjects instead of
// having to dig up to the FocusController at paint time.
bool RenderTheme::isActive(const RenderObject* o) const
{
    Node* node = o->element();
    if (!node)
        return false;

    Frame* frame = node->document()->frame();
    if (!frame)
        return false;

    Page* page = frame->page();
    if (!page)
        return false;

    return page->focusController()->isActive();
}

bool RenderTheme::isChecked(const RenderObject* o) const
{
    if (!o->element())
        return false;
    return o->element()->isChecked();
}

bool RenderTheme::isIndeterminate(const RenderObject* o) const
{
    if (!o->element())
        return false;
    return o->element()->isIndeterminate();
}

bool RenderTheme::isEnabled(const RenderObject* o) const
{
    if (!o->element())
        return true;
    return o->element()->isEnabled();
}

bool RenderTheme::isFocused(const RenderObject* o) const
{
    Node* node = o->element();
    if (!node)
        return false;
    Document* document = node->document();
    Frame* frame = document->frame();
    return node == document->focusedNode() && frame && frame->selection()->isFocusedAndActive();
}

bool RenderTheme::isPressed(const RenderObject* o) const
{
    if (!o->element())
        return false;
    return o->element()->active();
}

bool RenderTheme::isReadOnlyControl(const RenderObject* o) const
{
    if (!o->element())
        return false;
    return o->element()->isReadOnlyControl();
}

bool RenderTheme::isHovered(const RenderObject* o) const
{
    if (!o->element())
        return false;
    return o->element()->hovered();
}

bool RenderTheme::isDefault(const RenderObject* o) const
{
    if (!o->document())
        return false;

    Settings* settings = o->document()->settings();
    if (!settings || !settings->inApplicationChromeMode())
        return false;
    
    if (!o->style())
        return false;
    
    return o->style()->appearance() == DefaultButtonPart;
}

void RenderTheme::adjustCheckboxStyle(CSSStyleSelector* selector, RenderStyle* style, Element* e) const
{
    // A summary of the rules for checkbox designed to match WinIE:
    // width/height - honored (WinIE actually scales its control for small widths, but lets it overflow for small heights.)
    // font-size - not honored (control has no text), but we use it to decide which control size to use.
    setCheckboxSize(style);

    // padding - not honored by WinIE, needs to be removed.
    style->resetPadding();

    // border - honored by WinIE, but looks terrible (just paints in the control box and turns off the Windows XP theme)
    // for now, we will not honor it.
    style->resetBorder();

    style->setBoxShadow(0);
}

void RenderTheme::adjustRadioStyle(CSSStyleSelector* selector, RenderStyle* style, Element* e) const
{
    // A summary of the rules for checkbox designed to match WinIE:
    // width/height - honored (WinIE actually scales its control for small widths, but lets it overflow for small heights.)
    // font-size - not honored (control has no text), but we use it to decide which control size to use.
    setRadioSize(style);

    // padding - not honored by WinIE, needs to be removed.
    style->resetPadding();

    // border - honored by WinIE, but looks terrible (just paints in the control box and turns off the Windows XP theme)
    // for now, we will not honor it.
    style->resetBorder();

    style->setBoxShadow(0);
}

void RenderTheme::adjustButtonStyle(CSSStyleSelector* selector, RenderStyle* style, Element* e) const
{
    // Most platforms will completely honor all CSS, and so we have no need to adjust the style
    // at all by default.  We will still allow the theme a crack at setting up a desired vertical size.
    setButtonSize(style);
}

void RenderTheme::adjustTextFieldStyle(CSSStyleSelector* selector, RenderStyle* style, Element* e) const
{
}

void RenderTheme::adjustTextAreaStyle(CSSStyleSelector* selector, RenderStyle* style, Element* e) const
{
}

void RenderTheme::adjustMenuListStyle(CSSStyleSelector* selector, RenderStyle* style, Element* e) const
{
}

void RenderTheme::adjustMenuListButtonStyle(CSSStyleSelector* selector, RenderStyle* style, Element* e) const
{
}

void RenderTheme::adjustSliderTrackStyle(CSSStyleSelector* selector, RenderStyle* style, Element* e) const
{
}

void RenderTheme::adjustSliderThumbStyle(CSSStyleSelector* selector, RenderStyle* style, Element* e) const
{
}

void RenderTheme::adjustSliderThumbSize(RenderObject*) const
{
}

void RenderTheme::adjustSearchFieldStyle(CSSStyleSelector* selector, RenderStyle* style, Element* e) const
{
}

void RenderTheme::adjustSearchFieldCancelButtonStyle(CSSStyleSelector* selector, RenderStyle* style, Element* e) const
{
}

void RenderTheme::adjustSearchFieldDecorationStyle(CSSStyleSelector* selector, RenderStyle* style, Element* e) const
{
}

void RenderTheme::adjustSearchFieldResultsDecorationStyle(CSSStyleSelector* selector, RenderStyle* style, Element* e) const
{
}

void RenderTheme::adjustSearchFieldResultsButtonStyle(CSSStyleSelector* selector, RenderStyle* style, Element* e) const
{
}

void RenderTheme::platformColorsDidChange()
{
    m_activeSelectionColor = Color();
    m_inactiveSelectionColor = Color();
}

Color RenderTheme::systemColor(int cssValueId) const
{
    switch (cssValueId) {
        case CSSValueActiveborder:
            return 0xFFFFFFFF;
        case CSSValueActivecaption:
            return 0xFFCCCCCC;
        case CSSValueAppworkspace:
            return 0xFFFFFFFF;
        case CSSValueBackground:
            return 0xFF6363CE;
        case CSSValueButtonface:
            return 0xFFC0C0C0;
        case CSSValueButtonhighlight:
            return 0xFFDDDDDD;
        case CSSValueButtonshadow:
            return 0xFF888888;
        case CSSValueButtontext:
            return 0xFF000000;
        case CSSValueCaptiontext:
            return 0xFF000000;
        case CSSValueGraytext:
            return 0xFF808080;
        case CSSValueHighlight:
            return 0xFFB5D5FF;
        case CSSValueHighlighttext:
            return 0xFF000000;
        case CSSValueInactiveborder:
            return 0xFFFFFFFF;
        case CSSValueInactivecaption:
            return 0xFFFFFFFF;
        case CSSValueInactivecaptiontext:
            return 0xFF7F7F7F;
        case CSSValueInfobackground:
            return 0xFFFBFCC5;
        case CSSValueInfotext:
            return 0xFF000000;
        case CSSValueMenu:
            return 0xFFC0C0C0;
        case CSSValueMenutext:
            return 0xFF000000;
        case CSSValueScrollbar:
            return 0xFFFFFFFF;
        case CSSValueText:
            return 0xFF000000;
        case CSSValueThreeddarkshadow:
            return 0xFF666666;
        case CSSValueThreedface:
            return 0xFFC0C0C0;
        case CSSValueThreedhighlight:
            return 0xFFDDDDDD;
        case CSSValueThreedlightshadow:
            return 0xFFC0C0C0;
        case CSSValueThreedshadow:
            return 0xFF888888;
        case CSSValueWindow:
            return 0xFFFFFFFF;
        case CSSValueWindowframe:
            return 0xFFCCCCCC;
        case CSSValueWindowtext:
            return 0xFF000000;
    }
    return Color();
}

Color RenderTheme::platformTextSearchHighlightColor() const
{
    return Color(255, 255, 0);
}

} // namespace WebCore
