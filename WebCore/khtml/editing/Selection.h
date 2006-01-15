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

#ifndef KHTML_EDITING_SELECTION_H
#define KHTML_EDITING_SELECTION_H

#include <kxmlcore/PassRefPtr.h>
#include "text_granularity.h"
#include "dom_position.h"

namespace WebCore {

class Position;

class Selection
{
public:
    enum EState { NONE, CARET, RANGE };
    enum EDirection { FORWARD, BACKWARD, RIGHT, LEFT };
#define SEL_DEFAULT_AFFINITY DOWNSTREAM

    Selection();
    Selection(const Position &, const Position &, EAffinity);

    EState state() const { return m_state; }

    void setAffinity(EAffinity affinity) { m_affinity = affinity; }
    EAffinity affinity() const { return m_affinity; }

    void clear();

    void setBase(Position base) { m_base = base; }
    void setExtent(Position extent) { m_extent = extent; }
    void setStart(Position start) { m_start = start; }
    void setEnd(Position end) { m_end = end; }
    Position base() const { return m_base; }
    Position extent() const { return m_extent; }
    Position start() const { return m_start; }
    Position end() const { return m_end; }

    bool isNone() const { return state() == NONE; }
    bool isCaret() const { return state() == CARET; }
    bool isRange() const { return state() == RANGE; }
    bool isCaretOrRange() const { return state() != NONE; }

    bool isBaseFirst() const { return m_baseIsFirst; }

    PassRefPtr<RangeImpl> toRange() const;

    void debugPosition() const;

private:
    friend class SelectionController; // for access to validate; should be fixed later
    void validate(ETextGranularity granularity = CHARACTER);
    void adjustForEditableContent();

    Position m_base;              // base position for the selection
    Position m_extent;            // extent position for the selection
    Position m_start;             // start position for the selection
    Position m_end;               // end position for the selection

    EAffinity m_affinity;         // the upstream/downstream affinity of the caret

    // these are cached, can be recalculated by validate()
    EState m_state;               // the state of the selection
    bool m_baseIsFirst;           // true if base is before the extent
};

inline bool operator==(const Selection &a, const Selection &b)
{
    return a.start() == b.start() && a.end() == b.end() && a.affinity() == b.affinity();
}

inline bool operator!=(const Selection &a, const Selection &b)
{
    return !(a == b);
}

} // namespace khtml

#endif // KHTML_EDITING_SELECTION_H
