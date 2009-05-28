/*
 * Copyright (C) 2005 Frerich Raabe <raabe@kde.org>
 * Copyright (C) 2006, 2009 Apple Inc.
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "XPathPath.h"

#if ENABLE(XPATH)

#include "Document.h"
#include "XPathPredicate.h"
#include "XPathStep.h"
#include "XPathValue.h"

namespace WebCore {
namespace XPath {
        
Filter::Filter(Expression* expr, const Vector<Predicate*>& predicates)
    : m_expr(expr), m_predicates(predicates)
{
}

Filter::~Filter()
{
    delete m_expr;
    deleteAllValues(m_predicates);
}

Value Filter::evaluate() const
{
    Value v = m_expr->evaluate();
    
    if (!v.isNodeSet()) 
        return v;

    NodeSet& nodes = v.modifiableNodeSet();
    nodes.sort();

    EvaluationContext& evaluationContext = Expression::evaluationContext();
    for (unsigned i = 0; i < m_predicates.size(); i++) {
        NodeSet newNodes;
        evaluationContext.size = nodes.size();
        evaluationContext.position = 0;
        
        for (unsigned j = 0; j < nodes.size(); j++) {
            Node* node = nodes[j];
            
            evaluationContext.node = node;
            ++evaluationContext.position;
            
            if (m_predicates[i]->evaluate())
                newNodes.append(node);
        }
        nodes.swap(newNodes);
    }

    return v;
}

LocationPath::LocationPath()
    : m_absolute(false)
{
}

LocationPath::~LocationPath()
{
    deleteAllValues(m_steps);
}

Value LocationPath::evaluate() const
{
    EvaluationContext& evaluationContext = Expression::evaluationContext();
    EvaluationContext backupContext = evaluationContext;
    // For absolute location paths, the context node is ignored - the
    // document's root node is used instead.
    Node* context = evaluationContext.node.get();
    if (m_absolute && context->nodeType() != Node::DOCUMENT_NODE) 
        context = context->ownerDocument();

    NodeSet nodes;
    nodes.append(context);
    evaluate(nodes);
    
    evaluationContext = backupContext;
    return Value(nodes, Value::adopt);
}

void LocationPath::evaluate(NodeSet& nodes) const
{
    for (unsigned i = 0; i < m_steps.size(); i++) {
        Step* step = m_steps[i];
        NodeSet newNodes;
        HashSet<Node*> newNodesSet;

        for (unsigned j = 0; j < nodes.size(); j++) {
            NodeSet matches;
            step->evaluate(nodes[j], matches);
            
            for (size_t nodeIndex = 0; nodeIndex < matches.size(); ++nodeIndex) {
                Node* node = matches[nodeIndex];
                if (newNodesSet.add(node).second)
                    newNodes.append(node);
            }
        }
        
        nodes.swap(newNodes);
    }

    nodes.markSorted(false);
}

void LocationPath::appendStep(Step* step)
{
    m_steps.append(step);
}

void LocationPath::insertFirstStep(Step* step)
{
    m_steps.insert(0, step);
}

Path::Path(Filter* filter, LocationPath* path)
    : m_filter(filter)
    , m_path(path)
{
}

Path::~Path()
{
    delete m_filter;
    delete m_path;
}

Value Path::evaluate() const
{
    Value v = m_filter->evaluate();

    NodeSet& nodes = v.modifiableNodeSet();
    m_path->evaluate(nodes);
    
    return v;
}

}
}

#endif // ENABLE(XPATH)
