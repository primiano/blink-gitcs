/*
 * Copyright (C) 2012, 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "DFGFixupPhase.h"

#if ENABLE(DFG_JIT)

#include "DFGGraph.h"
#include "DFGInsertionSet.h"
#include "DFGPhase.h"
#include "Operations.h"

namespace JSC { namespace DFG {

class FixupPhase : public Phase {
public:
    FixupPhase(Graph& graph)
        : Phase(graph, "fixup")
        , m_insertionSet(graph)
    {
    }
    
    bool run()
    {
        ASSERT(m_graph.m_fixpointState == BeforeFixpoint);
        ASSERT(m_graph.m_form == ThreadedCPS);
        
        for (BlockIndex blockIndex = 0; blockIndex < m_graph.m_blocks.size(); ++blockIndex)
            fixupBlock(m_graph.m_blocks[blockIndex].get());
        return true;
    }

private:
    void fixupBlock(BasicBlock* block)
    {
        if (!block)
            return;
        ASSERT(block->isReachable);
        m_block = block;
        for (m_indexInBlock = 0; m_indexInBlock < block->size(); ++m_indexInBlock) {
            m_currentNode = block->at(m_indexInBlock);
            fixupNode(m_currentNode);
        }
        m_insertionSet.execute(block);
    }
    
    void fixupNode(Node* node)
    {
        if (!node->shouldGenerate())
            return;
        
        NodeType op = node->op();

#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLogF("   %s @%u: ", Graph::opName(op), node->index());
#endif
        
        switch (op) {
        case SetLocal: {
            VariableAccessData* variable = node->variableAccessData();
            
            if (variable->isCaptured()
                || m_graph.isCreatedThisArgument(variable->local()))
                break;
            
            if (variable->shouldUseDoubleFormat()) {
                fixDoubleEdge(node->child1(), NumberUse, ForwardSpeculation);
                break;
            }
            
            SpeculatedType predictedType = variable->argumentAwarePrediction();
            if (isInt32Speculation(predictedType))
                node->child1().setUseKind(Int32Use);
            else if (isCellSpeculation(predictedType))
                node->child1().setUseKind(CellUse);
            else if (isBooleanSpeculation(predictedType))
                node->child1().setUseKind(BooleanUse);
            break;
        }
            
        case BitAnd:
        case BitOr:
        case BitXor:
        case BitRShift:
        case BitLShift:
        case BitURShift: {
            fixIntEdge(node->child1());
            fixIntEdge(node->child2());
            break;
        }
            
        case UInt32ToNumber: {
            node->child1().setUseKind(KnownInt32Use);
            break;
        }
            
        case DoubleAsInt32: {
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
            
        case ValueToInt32: {
            if (node->child1()->shouldSpeculateInteger()) {
                node->child1().setUseKind(Int32Use);
                break;
            }
            
            if (node->child1()->shouldSpeculateNumber()) {
                node->child1().setUseKind(NumberUse);
                break;
            }
            
            if (node->child1()->shouldSpeculateBoolean()) {
                node->child1().setUseKind(BooleanUse);
                break;
            }
            
            node->child1().setUseKind(NotCellUse);
            break;
        }
            
        case Int32ToDouble: {
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
            
        case ValueAdd: {
            if (attemptToMakeIntegerAdd(node))
                break;
            if (Node::shouldSpeculateNumberExpectingDefined(node->child1().node(), node->child2().node())) {
                fixDoubleEdge(node->child1());
                fixDoubleEdge(node->child2());
                break;
            }
            break;
        }
            
        case ArithAdd:
        case ArithSub: {
            if (attemptToMakeIntegerAdd(node))
                break;
            fixDoubleEdge(node->child1());
            fixDoubleEdge(node->child2());
            break;
        }
            
        case ArithNegate: {
            if (m_graph.negateShouldSpeculateInteger(node)) {
                node->child1().setUseKind(Int32Use);
                break;
            }
            fixDoubleEdge(node->child1());
            break;
        }
            
        case ArithMul: {
            if (m_graph.mulShouldSpeculateInteger(node)) {
                node->child1().setUseKind(Int32Use);
                node->child2().setUseKind(Int32Use);
                break;
            }
            fixDoubleEdge(node->child1());
            fixDoubleEdge(node->child2());
            break;
        }

        case ArithDiv: {
            if (Node::shouldSpeculateIntegerForArithmetic(node->child1().node(), node->child2().node())
                && node->canSpeculateInteger()) {
                if (isX86() || isARMv7s()) {
                    node->child1().setUseKind(Int32Use);
                    node->child2().setUseKind(Int32Use);
                    break;
                }
                injectInt32ToDoubleNode(node->child1());
                injectInt32ToDoubleNode(node->child2());

                // We don't need to do ref'ing on the children because we're stealing them from
                // the original division.
                Node* newDivision = m_insertionSet.insertNode(
                    m_indexInBlock, DontRefChildren, RefNode, SpecDouble, *node);
                
                node->setOp(DoubleAsInt32);
                node->children.initialize(Edge(newDivision, KnownNumberUse), Edge(), Edge());
                break;
            }
            fixDoubleEdge(node->child1());
            fixDoubleEdge(node->child2());
            break;
        }
            
        case ArithMin:
        case ArithMax:
        case ArithMod: {
            if (Node::shouldSpeculateIntegerForArithmetic(node->child1().node(), node->child2().node())
                && node->canSpeculateInteger()) {
                node->child1().setUseKind(Int32Use);
                node->child2().setUseKind(Int32Use);
                break;
            }
            fixDoubleEdge(node->child1());
            fixDoubleEdge(node->child2());
            break;
        }
            
        case ArithAbs: {
            if (node->child1()->shouldSpeculateIntegerForArithmetic()
                && node->canSpeculateInteger()) {
                node->child1().setUseKind(Int32Use);
                break;
            }
            fixDoubleEdge(node->child1());
            break;
        }
            
        case ArithSqrt: {
            fixDoubleEdge(node->child1());
            break;
        }
            
        case LogicalNot: {
            if (node->child1()->shouldSpeculateBoolean())
                node->child1().setUseKind(BooleanUse);
            else if (node->child1()->shouldSpeculateObjectOrOther())
                node->child1().setUseKind(ObjectOrOtherUse);
            else if (node->child1()->shouldSpeculateInteger())
                node->child1().setUseKind(Int32Use);
            else if (node->child1()->shouldSpeculateNumber())
                fixDoubleEdge(node->child1());
            break;
        }
            
        case TypeOf: {
            if (node->child1()->shouldSpeculateString())
                node->child1().setUseKind(StringUse);
            else if (node->child1()->shouldSpeculateCell())
                node->child1().setUseKind(CellUse);
            break;
        }
            
        case CompareEq:
        case CompareEqConstant:
        case CompareLess:
        case CompareLessEq:
        case CompareGreater:
        case CompareGreaterEq: {
            if (node->op() == CompareEqConstant)
                break;
            if (Node::shouldSpeculateInteger(node->child1().node(), node->child2().node())) {
                node->child1().setUseKind(Int32Use);
                node->child2().setUseKind(Int32Use);
                break;
            }
            if (Node::shouldSpeculateNumber(node->child1().node(), node->child2().node())) {
                fixDoubleEdge(node->child1());
                fixDoubleEdge(node->child2());
                break;
            }
            if (node->op() != CompareEq)
                break;
            if (node->child1()->shouldSpeculateString() && node->child2()->shouldSpeculateString())
                break;
            if (node->child1()->shouldSpeculateObject() && node->child2()->shouldSpeculateObject()) {
                node->child1().setUseKind(ObjectUse);
                node->child2().setUseKind(ObjectUse);
                break;
            }
            if (node->child1()->shouldSpeculateObject() && node->child2()->shouldSpeculateObjectOrOther()) {
                node->child1().setUseKind(ObjectUse);
                node->child2().setUseKind(ObjectOrOtherUse);
                break;
            }
            if (node->child1()->shouldSpeculateObjectOrOther() && node->child2()->shouldSpeculateObject()) {
                node->child1().setUseKind(ObjectOrOtherUse);
                node->child2().setUseKind(ObjectUse);
                break;
            }
            break;
        }
            
        case CompareStrictEq:
        case CompareStrictEqConstant: {
            if (node->op() == CompareStrictEqConstant)
                break;
            if (Node::shouldSpeculateInteger(node->child1().node(), node->child2().node())) {
                node->child1().setUseKind(Int32Use);
                node->child2().setUseKind(Int32Use);
                break;
            }
            if (Node::shouldSpeculateNumber(node->child1().node(), node->child2().node())) {
                fixDoubleEdge(node->child1());
                fixDoubleEdge(node->child2());
                break;
            }
            if (node->child1()->shouldSpeculateString() && node->child2()->shouldSpeculateString())
                break;
            if (node->child1()->shouldSpeculateObject() && node->child2()->shouldSpeculateObject()) {
                node->child1().setUseKind(ObjectUse);
                node->child2().setUseKind(ObjectUse);
                break;
            }
            break;
        }

        case StringCharAt:
        case StringCharCodeAt: {
            // Currently we have no good way of refining these.
            ASSERT(node->arrayMode() == ArrayMode(Array::String));
            blessArrayOperation(node->child1(), node->child2(), node->child3());
            node->child1().setUseKind(KnownCellUse);
            node->child2().setUseKind(Int32Use);
            break;
        }

        case GetByVal: {
            node->setArrayMode(
                node->arrayMode().refine(
                    node->child1()->prediction(),
                    node->child2()->prediction(),
                    SpecNone, node->flags()));
            
            blessArrayOperation(node->child1(), node->child2(), node->child3());
            
            ArrayMode arrayMode = node->arrayMode();
            if (arrayMode.type() == Array::Double
                && arrayMode.arrayClass() == Array::OriginalArray
                && arrayMode.speculation() == Array::InBounds
                && arrayMode.conversion() == Array::AsIs
                && m_graph.globalObjectFor(node->codeOrigin)->arrayPrototypeChainIsSane()
                && !(node->flags() & NodeUsedAsOther))
                node->setArrayMode(arrayMode.withSpeculation(Array::SaneChain));
            
            switch (node->arrayMode().type()) {
            case Array::SelectUsingPredictions:
            case Array::Unprofiled:
            case Array::Undecided:
                RELEASE_ASSERT_NOT_REACHED();
                break;
            case Array::Generic:
#if USE(JSVALUE32_64)
                node->child1().setUseKind(CellUse); // Speculating cell due to register pressure on 32-bit.
#endif
                break;
            case Array::ForceExit:
                break;
            default:
                node->child1().setUseKind(KnownCellUse);
                node->child2().setUseKind(Int32Use);
                break;
            }
            
            break;
        }
            
        case PutByVal:
        case PutByValAlias: {
            Edge& child1 = m_graph.varArgChild(node, 0);
            Edge& child2 = m_graph.varArgChild(node, 1);
            Edge& child3 = m_graph.varArgChild(node, 2);

            node->setArrayMode(
                node->arrayMode().refine(
                    child1->prediction(),
                    child2->prediction(),
                    child3->prediction()));
            
            blessArrayOperation(child1, child2, m_graph.varArgChild(node, 3));
            
            switch (node->arrayMode().modeForPut().type()) {
            case Array::SelectUsingPredictions:
            case Array::Unprofiled:
            case Array::Undecided:
                RELEASE_ASSERT_NOT_REACHED();
                break;
            case Array::ForceExit:
            case Array::Generic:
#if USE(JSVALUE32_64)
                // Due to register pressure on 32-bit, we speculate cell and
                // ignore the base-is-not-cell case entirely by letting the
                // baseline JIT handle it.
                child1.setUseKind(CellUse);
#endif
                break;
            case Array::Int32:
                child1.setUseKind(KnownCellUse);
                child2.setUseKind(Int32Use);
                child3.setUseKind(Int32Use);
                break;
            case Array::Double:
                child1.setUseKind(KnownCellUse);
                child2.setUseKind(Int32Use);
                fixDoubleEdge(child3, RealNumberUse);
                break;
            case Array::Int8Array:
            case Array::Int16Array:
            case Array::Int32Array:
            case Array::Uint8Array:
            case Array::Uint8ClampedArray:
            case Array::Uint16Array:
            case Array::Uint32Array:
                child1.setUseKind(KnownCellUse);
                child2.setUseKind(Int32Use);
                if (child3->shouldSpeculateInteger())
                    child3.setUseKind(Int32Use);
                else
                    fixDoubleEdge(child3);
                break;
            case Array::Float32Array:
            case Array::Float64Array:
                child1.setUseKind(KnownCellUse);
                child2.setUseKind(Int32Use);
                fixDoubleEdge(child3);
                break;
            default:
                child1.setUseKind(KnownCellUse);
                child2.setUseKind(Int32Use);
                break;
            }
            break;
        }
            
        case ArrayPush: {
            // May need to refine the array mode in case the value prediction contravenes
            // the array prediction. For example, we may have evidence showing that the
            // array is in Int32 mode, but the value we're storing is likely to be a double.
            // Then we should turn this into a conversion to Double array followed by the
            // push. On the other hand, we absolutely don't want to refine based on the
            // base prediction. If it has non-cell garbage in it, then we want that to be
            // ignored. That's because ArrayPush can't handle any array modes that aren't
            // array-related - so if refine() turned this into a "Generic" ArrayPush then
            // that would break things.
            node->setArrayMode(
                node->arrayMode().refine(
                    node->child1()->prediction() & SpecCell,
                    SpecInt32,
                    node->child2()->prediction()));
            blessArrayOperation(node->child1(), Edge(), node->child3());
            node->child1().setUseKind(KnownCellUse);
            
            switch (node->arrayMode().type()) {
            case Array::Int32:
                node->child2().setUseKind(Int32Use);
                break;
            case Array::Double:
                fixDoubleEdge(node->child2());
                break;
            default:
                break;
            }
            break;
        }
            
        case ArrayPop: {
            blessArrayOperation(node->child1(), Edge(), node->child2());
            node->child1().setUseKind(KnownCellUse);
            break;
        }
            
        case RegExpExec:
        case RegExpTest: {
            node->child1().setUseKind(CellUse);
            node->child2().setUseKind(CellUse);
            break;
        }
            
        case Branch: {
            if (node->child1()->shouldSpeculateBoolean())
                node->child1().setUseKind(BooleanUse);
            else if (node->child1()->shouldSpeculateObjectOrOther())
                node->child1().setUseKind(ObjectOrOtherUse);
            else if (node->child1()->shouldSpeculateInteger())
                node->child1().setUseKind(Int32Use);
            else if (node->child1()->shouldSpeculateNumber())
                fixDoubleEdge(node->child1());

            Node* logicalNot = node->child1().node();
            if (logicalNot->op() == LogicalNot
                && logicalNot->adjustedRefCount() == 1) {
                
                // Make sure that OSR exit can't observe the LogicalNot. If it can,
                // then we must compute it and cannot peephole around it.
                bool found = false;
                bool ok = true;
                for (unsigned i = m_indexInBlock; i--;) {
                    Node* candidate = m_block->at(i);
                    if (!candidate->shouldGenerate())
                        continue;
                    if (candidate == logicalNot) {
                        found = true;
                        break;
                    }
                    if (candidate->canExit()) {
                        ok = false;
                        found = true;
                        break;
                    }
                }
                ASSERT_UNUSED(found, found);
                
                if (ok) {
                    Edge newChildEdge = logicalNot->child1();
                    if (newChildEdge->hasBooleanResult()) {
                        m_graph.ref(newChildEdge);
                        m_graph.deref(logicalNot);
                        node->children.setChild1(newChildEdge);
                        
                        BlockIndex toBeTaken = node->notTakenBlockIndex();
                        BlockIndex toBeNotTaken = node->takenBlockIndex();
                        node->setTakenBlockIndex(toBeTaken);
                        node->setNotTakenBlockIndex(toBeNotTaken);
                    }
                }
            }
            break;
        }
            
        case ToPrimitive: {
            if (node->child1()->shouldSpeculateInteger())
                node->child1().setUseKind(Int32Use);
            // FIXME: Add string speculation here.
            // https://bugs.webkit.org/show_bug.cgi?id=110175
            break;
        }
            
        case NewArray: {
            for (unsigned i = m_graph.varArgNumChildren(node); i--;) {
                node->setIndexingType(
                    leastUpperBoundOfIndexingTypeAndType(
                        node->indexingType(), m_graph.varArgChild(node, i)->prediction()));
            }
            switch (node->indexingType()) {
            case ALL_BLANK_INDEXING_TYPES:
                CRASH();
                break;
            case ALL_UNDECIDED_INDEXING_TYPES:
                if (node->numChildren()) {
                    // This will only happen if the children have no type predictions. We
                    // would have already exited by now, but insert a forced exit just to
                    // be safe.
                    m_insertionSet.insertNode(
                        m_indexInBlock, DontRefChildren, DontRefNode, SpecNone, ForceOSRExit,
                        node->codeOrigin);
                }
                break;
            case ALL_INT32_INDEXING_TYPES:
                for (unsigned operandIndex = 0; operandIndex < node->numChildren(); ++operandIndex)
                    m_graph.m_varArgChildren[node->firstChild() + operandIndex].setUseKind(Int32Use);
                break;
            case ALL_DOUBLE_INDEXING_TYPES:
                for (unsigned operandIndex = 0; operandIndex < node->numChildren(); ++operandIndex)
                    m_graph.m_varArgChildren[node->firstChild() + operandIndex].setUseKind(RealNumberUse);
                break;
            case ALL_CONTIGUOUS_INDEXING_TYPES:
            case ALL_ARRAY_STORAGE_INDEXING_TYPES:
                break;
            default:
                CRASH();
                break;
            }
            break;
        }
            
        case NewArrayWithSize: {
            node->child1().setUseKind(Int32Use);
            break;
        }
            
        case ConvertThis: {
            // FIXME: Use Phantom(type check) and Identity instead.
            // https://bugs.webkit.org/show_bug.cgi?id=110395
            if (isOtherSpeculation(node->child1()->prediction()))
                node->child1().setUseKind(OtherUse);
            else if (isObjectSpeculation(node->child1()->prediction()))
                node->child1().setUseKind(ObjectUse);
            break;
        }
            
        case CreateThis: {
            node->child1().setUseKind(CellUse);
            break;
        }
            
        case GetMyArgumentByVal:
        case GetMyArgumentByValSafe: {
            node->child1().setUseKind(Int32Use);
            break;
        }
            
        case GetScopeRegisters:
        case PutScopedVar:
        case SkipTopScope:
        case SkipScope:
        case SetCallee:
        case SetMyScope:
        case PutStructure:
        case AllocatePropertyStorage:
        case ReallocatePropertyStorage:
        case GetScope:
        case GetButterfly: {
            node->child1().setUseKind(KnownCellUse);
            break;
        }
            
        case GetById: {
            if (!node->child1()->shouldSpeculateCell())
                break;
            node->child1().setUseKind(CellUse);
            if (!isInt32Speculation(node->prediction()))
                break;
            if (codeBlock()->identifier(node->identifierNumber()) != globalData().propertyNames->length)
                break;
            ArrayProfile* arrayProfile = 
                m_graph.baselineCodeBlockFor(node->codeOrigin)->getArrayProfile(
                    node->codeOrigin.bytecodeIndex);
            ArrayMode arrayMode = ArrayMode(Array::SelectUsingPredictions);
            if (arrayProfile) {
                arrayProfile->computeUpdatedPrediction(m_graph.baselineCodeBlockFor(node->codeOrigin));
                arrayMode = ArrayMode::fromObserved(arrayProfile, Array::Read, false);
                arrayMode = arrayMode.refine(
                    node->child1()->prediction(), node->prediction());
                if (arrayMode.supportsLength() && arrayProfile->hasDefiniteStructure()) {
                    m_insertionSet.insertNode(
                        m_indexInBlock, RefChildren, DontRefNode, SpecNone, CheckStructure,
                        node->codeOrigin, OpInfo(m_graph.addStructureSet(arrayProfile->expectedStructure())),
                        node->child1());
                }
            } else
                arrayMode = arrayMode.refine(node->child1()->prediction(), node->prediction());
            if (!arrayMode.supportsLength())
                break;
            node->setOp(GetArrayLength);
            ASSERT(node->flags() & NodeMustGenerate);
            node->clearFlags(NodeMustGenerate | NodeClobbersWorld);
            node->child1().setUseKind(KnownCellUse);
            m_graph.deref(node);
            node->setArrayMode(arrayMode);
            
            Node* storage = checkArray(arrayMode, node->codeOrigin, node->child1().node(), 0, lengthNeedsStorage, node->shouldGenerate());
            if (!storage)
                break;
            
            node->child2() = Edge(storage);
            break;
        }
            
        case GetByIdFlush: {
            if (node->child1()->shouldSpeculateCell())
                node->child1().setUseKind(CellUse);
            break;
        }
            
        case CheckExecutable:
        case CheckStructure:
        case ForwardCheckStructure:
        case StructureTransitionWatchpoint:
        case ForwardStructureTransitionWatchpoint:
        case CheckFunction:
        case PutById:
        case PutByIdDirect:
        case CheckHasInstance: {
            node->child1().setUseKind(CellUse);
            break;
        }
            
        case CheckArray: {
            switch (node->arrayMode().type()) {
            case Array::String:
                node->child1().setUseKind(StringUse);
                break;
            default:
                node->child1().setUseKind(CellUse);
                break;
            }
            break;
        }
            
        case Arrayify:
        case ArrayifyToStructure: {
            node->child1().setUseKind(CellUse);
            if (node->child2())
                node->child2().setUseKind(Int32Use);
            break;
        }
            
        case GetByOffset: {
            if (!node->child1()->hasStorageResult())
                node->child1().setUseKind(KnownCellUse);
            break;
        }
            
        case PutByOffset: {
            if (!node->child1()->hasStorageResult())
                node->child1().setUseKind(KnownCellUse);
            node->child2().setUseKind(KnownCellUse);
            break;
        }
            
        case InstanceOf: {
            // FIXME: This appears broken: CheckHasInstance already does an unconditional cell
            // check. https://bugs.webkit.org/show_bug.cgi?id=107479
            if (!(node->child1()->prediction() & ~SpecCell))
                node->child1().setUseKind(CellUse);
            node->child2().setUseKind(CellUse);
            break;
        }
            
        case GetArrayLength:
        case Identity:
        case Nop:
        case Phi:
        case ForwardInt32ToDouble:
        case PhantomPutStructure:
        case GetIndexedPropertyStorage:
        case LastNodeType:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        
#if !ASSERT_DISABLED    
        // Have these no-op cases here to ensure that nobody forgets to add handlers for new opcodes.
        case SetArgument:
        case Phantom:
        case JSConstant:
        case WeakJSConstant:
        case GetLocal:
        case GetCallee:
        case Flush:
        case PhantomLocal:
        case GetLocalUnlinked:
        case InlineStart:
        case GetMyScope:
        case GetScopedVar:
        case GetGlobalVar:
        case PutGlobalVar:
        case GlobalVarWatchpoint:
        case PutGlobalVarCheck:
        case AllocationProfileWatchpoint:
        case Call:
        case Construct:
        case NewObject:
        case NewArrayBuffer:
        case NewRegexp:
        case Resolve:
        case ResolveBase:
        case ResolveBaseStrictPut:
        case ResolveGlobal:
        case Breakpoint:
        case IsUndefined:
        case IsBoolean:
        case IsNumber:
        case IsString:
        case IsObject:
        case IsFunction:
        case StrCat:
        case CreateActivation:
        case TearOffActivation:
        case CreateArguments:
        case PhantomArguments:
        case TearOffArguments:
        case GetMyArgumentsLength:
        case GetMyArgumentsLengthSafe:
        case CheckArgumentsNotCreated:
        case NewFunction:
        case NewFunctionNoCheck:
        case NewFunctionExpression:
        case Jump:
        case Return:
        case Throw:
        case ThrowReferenceError:
        case GarbageValue:
        case CountExecution:
        case ForceOSRExit:
            break;
#else
        default:
            break;
#endif
        }

#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        if (!(node->flags() & NodeHasVarArgs)) {
            dataLogF("new children: ");
            node->dumpChildren(WTF::dataFile());
        }
        dataLogF("\n");
#endif
    }
    
    Node* checkArray(ArrayMode arrayMode, CodeOrigin codeOrigin, Node* array, Node* index, bool (*storageCheck)(const ArrayMode&) = canCSEStorage, bool shouldGenerate = true)
    {
        ASSERT(arrayMode.isSpecific());
        
        Structure* structure = arrayMode.originalArrayStructure(m_graph, codeOrigin);
        
        Edge indexEdge = index ? Edge(index, Int32Use) : Edge();
        
        if (arrayMode.doesConversion()) {
            if (structure) {
                if (m_indexInBlock > 0) {
                    // If the previous node was a CheckStructure inserted because of stuff
                    // that the array profile told us, then remove it.
                    Node* previousNode = m_block->at(m_indexInBlock - 1);
                    if (previousNode->op() == CheckStructure
                        && previousNode->child1() == array
                        && previousNode->codeOrigin == codeOrigin)
                        previousNode->setOpAndDefaultFlags(Phantom);
                }
                
                m_insertionSet.insertNode(
                    m_indexInBlock, RefChildren, DontRefNode, SpecNone, ArrayifyToStructure, codeOrigin,
                    OpInfo(structure), OpInfo(arrayMode.asWord()), Edge(array, CellUse), indexEdge);
            } else {
                m_insertionSet.insertNode(
                    m_indexInBlock, RefChildren, DontRefNode, SpecNone, Arrayify, codeOrigin,
                    OpInfo(arrayMode.asWord()), Edge(array, CellUse), indexEdge);
            }
        } else {
            if (structure) {
                m_insertionSet.insertNode(
                    m_indexInBlock, RefChildren, DontRefNode, SpecNone, CheckStructure, codeOrigin,
                    OpInfo(m_graph.addStructureSet(structure)), Edge(array, CellUse));
            } else {
                m_insertionSet.insertNode(
                    m_indexInBlock, RefChildren, DontRefNode, SpecNone, CheckArray, codeOrigin,
                    OpInfo(arrayMode.asWord()), Edge(array, CellUse));
            }
        }
        
        if (!storageCheck(arrayMode))
            return 0;
        
        if (arrayMode.usesButterfly()) {
            return m_insertionSet.insertNode(
                m_indexInBlock,
                shouldGenerate ? RefChildren : DontRefChildren,
                shouldGenerate ? RefNode : DontRefNode,
                SpecNone, GetButterfly, codeOrigin, Edge(array, KnownCellUse));
        }
        
        return m_insertionSet.insertNode(
            m_indexInBlock,
            shouldGenerate ? RefChildren : DontRefChildren,
            shouldGenerate ? RefNode : DontRefNode,
            SpecNone, GetIndexedPropertyStorage, codeOrigin, OpInfo(arrayMode.asWord()), Edge(array, KnownCellUse));
    }
    
    void blessArrayOperation(Edge base, Edge index, Edge& storageChild)
    {
        Node* node = m_currentNode;
        
        switch (node->arrayMode().type()) {
        case Array::ForceExit: {
            m_insertionSet.insertNode(
                m_indexInBlock, DontRefChildren, DontRefNode, SpecNone, ForceOSRExit,
                node->codeOrigin);
            return;
        }
            
        case Array::SelectUsingPredictions:
        case Array::Unprofiled:
            RELEASE_ASSERT_NOT_REACHED();
            return;
            
        case Array::Generic:
            return;
            
        default: {
            Node* storage = checkArray(node->arrayMode(), node->codeOrigin, base.node(), index.node());
            if (!storage)
                return;
            
            storageChild = Edge(storage);
            return;
        } }
    }
    
    void fixIntEdge(Edge& edge)
    {
        Node* node = edge.node();
        if (node->op() != ValueToInt32) {
            edge.setUseKind(KnownInt32Use);
            return;
        }
        
        Edge oldEdge = edge;
        Edge newEdge = node->child1();
        
        if (newEdge.useKind() != Int32Use) {
            edge.setUseKind(KnownInt32Use);
            return;
        }
        
        ASSERT(newEdge->shouldSpeculateInteger());
        
        m_graph.ref(newEdge);
        m_graph.deref(oldEdge);
        
        edge = newEdge;
    }
    
    void fixDoubleEdge(Edge& edge, UseKind useKind = NumberUse, SpeculationDirection direction = BackwardSpeculation)
    {
        if (edge->prediction() & SpecDouble) {
            edge.setUseKind(useKind);
            return;
        }
        
        injectInt32ToDoubleNode(edge, useKind, direction);
    }

    void injectInt32ToDoubleNode(Edge& edge, UseKind useKind = NumberUse, SpeculationDirection direction = BackwardSpeculation)
    {
        Node* result = m_insertionSet.insertNode(
            m_indexInBlock, DontRefChildren, RefNode, SpecDouble,
            direction == BackwardSpeculation ? Int32ToDouble : ForwardInt32ToDouble,
            m_currentNode->codeOrigin, Edge(edge.node(), NumberUse));
        
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLogF(
            "(replacing @%u->@%u with @%u->@%u) ",
            m_currentNode->index(), edge->index(), m_currentNode->index(), result->index());
#endif

        edge = Edge(result, useKind);
    }
    
    void truncateConstantToInt32(Edge& edge)
    {
        Node* oldNode = edge.node();
        
        ASSERT(oldNode->hasConstant());
        JSValue value = m_graph.valueOfJSConstant(oldNode);
        if (value.isInt32())
            return;
        
        value = jsNumber(JSC::toInt32(value.asNumber()));
        ASSERT(value.isInt32());
        edge.setNode(m_insertionSet.insertNode(
            m_indexInBlock, DontRefChildren, RefNode, SpecInt32, JSConstant,
            m_currentNode->codeOrigin, OpInfo(codeBlock()->addOrFindConstant(value))));
        m_graph.deref(oldNode);
    }
    
    void truncateConstantsIfNecessary(Node* node, AddSpeculationMode mode)
    {
        if (mode != SpeculateIntegerAndTruncateConstants)
            return;
        
        ASSERT(node->child1()->hasConstant() || node->child2()->hasConstant());
        if (node->child1()->hasConstant())
            truncateConstantToInt32(node->child1());
        else
            truncateConstantToInt32(node->child2());
    }
    
    bool attemptToMakeIntegerAdd(Node* node)
    {
        AddSpeculationMode mode = m_graph.addSpeculationMode(node);
        if (mode == DontSpeculateInteger)
            return false;
        
        truncateConstantsIfNecessary(node, mode);
        node->child1().setUseKind(Int32Use);
        node->child2().setUseKind(Int32Use);
        return true;
    }

    BasicBlock* m_block;
    unsigned m_indexInBlock;
    Node* m_currentNode;
    InsertionSet m_insertionSet;
};
    
bool performFixup(Graph& graph)
{
    SamplingRegion samplingRegion("DFG Fixup Phase");
    return runPhase<FixupPhase>(graph);
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

