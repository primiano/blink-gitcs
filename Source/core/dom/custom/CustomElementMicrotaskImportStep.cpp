/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Google Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
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

#include "config.h"
#include "core/dom/custom/CustomElementMicrotaskImportStep.h"

#include "core/dom/custom/CustomElementMicrotaskDispatcher.h"
#include "core/dom/custom/CustomElementMicrotaskQueue.h"
#include "core/html/imports/HTMLImportChild.h"
#include "core/html/imports/HTMLImportLoader.h"
#include <stdio.h>

namespace WebCore {

PassOwnPtrWillBeRawPtr<CustomElementMicrotaskImportStep> CustomElementMicrotaskImportStep::create(HTMLImportChild* import)
{
    return adoptPtrWillBeNoop(new CustomElementMicrotaskImportStep(import));
}

CustomElementMicrotaskImportStep::CustomElementMicrotaskImportStep(HTMLImportChild* import)
    : m_import(import->weakPtr())
    , m_queue(import->loader()->microtaskQueue())
    , m_weakFactory(this)
{
}

CustomElementMicrotaskImportStep::~CustomElementMicrotaskImportStep()
{
}

void CustomElementMicrotaskImportStep::parentWasChanged()
{
    m_queue = CustomElementMicrotaskQueue::create();
    m_import.clear();
}

bool CustomElementMicrotaskImportStep::shouldWaitForImport() const
{
    return m_import && !m_import->isLoaded();
}

bool CustomElementMicrotaskImportStep::shouldStopProcessing() const
{
    return m_import && m_import->isSync();
}

void CustomElementMicrotaskImportStep::didUpgradeAllCustomElements()
{
    ASSERT(m_queue);
    if (m_import)
        m_import->didFinishUpgradingCustomElements();
}

CustomElementMicrotaskStep::Result CustomElementMicrotaskImportStep::process()
{
    Result result = m_queue->dispatch();
    if (!(result & ShouldStop) && !shouldWaitForImport())
        didUpgradeAllCustomElements();

    if (shouldWaitForImport())
        result = Result(result | ShouldRemain | ShouldStop);
    if (!shouldStopProcessing())
        result = Result(result & ~ShouldStop);
    return result;
}

bool CustomElementMicrotaskImportStep::needsProcessOrStop() const
{
    return shouldStopProcessing() || m_queue->needsProcessOrStop();
}

void CustomElementMicrotaskImportStep::trace(Visitor* visitor)
{
    visitor->trace(m_queue);
    CustomElementMicrotaskStep::trace(visitor);
}

#if !defined(NDEBUG)
void CustomElementMicrotaskImportStep::show(unsigned indent)
{
    fprintf(stderr, "%*sImport(wait=%d sync=%d, url=%s)\n", indent, "", shouldWaitForImport(), shouldStopProcessing(), m_import ? m_import->url().string().utf8().data() : "null");
    m_queue->show(indent + 1);
}
#endif

} // namespace WebCore
