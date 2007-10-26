// -*- mode: c++; c-basic-offset: 4 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2007 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "ExecState.h"

#include "JSGlobalObject.h"
#include "function.h"
#include "internal.h"

namespace KJS {


// ECMA 10.2
ExecState::ExecState(Interpreter* interpreter, JSGlobalObject* glob, JSObject* thisV, 
                     FunctionBodyNode* currentBody, CodeType type, ExecState* callingExec, 
                     FunctionImp* func, const List* args)
    : m_interpreter(interpreter)
    , m_exception(0)
    , m_propertyNames(CommonIdentifiers::shared())
    , m_savedExecState(interpreter->currentExec())
    , m_currentBody(currentBody)
    , m_function(func)
    , m_arguments(args)
    , m_iterationDepth(0)
    , m_switchDepth(0) 
{
    m_codeType = type;
    m_callingExecState = callingExec;
    
    // create and initialize activation object (ECMA 10.1.6)
    if (type == FunctionCode) {
        m_activation = new ActivationImp(func, *args);
        m_variable = m_activation;
    } else {
        m_activation = 0;
        m_variable = glob;
    }
    
    // ECMA 10.2
    switch(type) {
    case EvalCode:
        if (m_callingExecState) {
            scope = m_callingExecState->scopeChain();
            m_variable = m_callingExecState->variableObject();
            m_thisVal = m_callingExecState->thisValue();
            break;
        } // else same as GlobalCode
    case GlobalCode:
        scope.clear();
        scope.push(glob);
        m_thisVal = static_cast<JSObject*>(glob);
        break;
    case FunctionCode:
        scope = func->scope();
        scope.push(m_activation);
        m_variable = m_activation; // TODO: DontDelete ? (ECMA 10.2.3)
        m_thisVal = thisV;
        break;
    }

    if (currentBody)
        m_interpreter->setCurrentExec(this);
}

ExecState::~ExecState()
{
    m_interpreter->setCurrentExec(m_savedExecState);

    // The arguments list is only needed to potentially create the  arguments object, 
    // which isn't accessible from nested scopes so we can discard the list as soon 
    // as the function is done running.
    // This prevents lists of Lists from building up, waiting to be garbage collected
    ActivationImp* activation = static_cast<ActivationImp*>(m_activation);
    if (activation)
        activation->releaseArguments();
}

void ExecState::mark()
{
    for (ExecState* exec = this; exec; exec = exec->m_callingExecState)
        exec->scope.mark();
}

Interpreter* ExecState::lexicalInterpreter() const
{
    if (scopeChain().isEmpty())
        return dynamicInterpreter();
    
    JSObject* object = scopeChain().bottom();
    if (object && object->isGlobalObject())
        return static_cast<JSGlobalObject*>(object)->interpreter();

    return dynamicInterpreter();
}


} // namespace KJS
