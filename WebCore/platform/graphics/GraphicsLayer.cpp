/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#if USE(ACCELERATED_COMPOSITING)

#include "GraphicsLayer.h"

#include "FloatPoint.h"
#include "RotateTransformOperation.h"
#include "TextStream.h"

namespace WebCore {

void KeyframeValueList::insert(const AnimationValue* value)
{
    for (size_t i = 0; i < m_values.size(); ++i) {
        const AnimationValue* curValue = m_values[i];
        if (curValue->keyTime() == value->keyTime()) {
            ASSERT_NOT_REACHED();
            // insert after
            m_values.insert(i + 1, value);
            return;
        }
        if (curValue->keyTime() > value->keyTime()) {
            // insert before
            m_values.insert(i, value);
            return;
        }
    }
    
    m_values.append(value);
}

GraphicsLayer::GraphicsLayer(GraphicsLayerClient* client)
    : m_client(client)
    , m_anchorPoint(0.5f, 0.5f, 0)
    , m_opacity(1)
#ifndef NDEBUG
    , m_zPosition(0)
#endif
    , m_backgroundColorSet(false)
    , m_contentsOpaque(false)
    , m_preserves3D(false)
    , m_backfaceVisibility(true)
    , m_usingTiledLayer(false)
    , m_masksToBounds(false)
    , m_drawsContent(false)
    , m_paintingPhase(GraphicsLayerPaintAll)
    , m_geometryOrientation(CompositingCoordinatesTopDown)
    , m_contentsOrientation(CompositingCoordinatesTopDown)
    , m_parent(0)
    , m_maskLayer(0)
#ifndef NDEBUG
    , m_repaintCount(0)
#endif
{
}

GraphicsLayer::~GraphicsLayer()
{
    removeAllChildren();
    removeFromParent();
}

void GraphicsLayer::addChild(GraphicsLayer* childLayer)
{
    ASSERT(childLayer != this);
    
    if (childLayer->parent())
        childLayer->removeFromParent();

    childLayer->setParent(this);
    m_children.append(childLayer);
}

void GraphicsLayer::addChildAtIndex(GraphicsLayer* childLayer, int index)
{
    ASSERT(childLayer != this);

    if (childLayer->parent())
        childLayer->removeFromParent();

    childLayer->setParent(this);
    m_children.insert(index, childLayer);
}

void GraphicsLayer::addChildBelow(GraphicsLayer* childLayer, GraphicsLayer* sibling)
{
    ASSERT(childLayer != this);
    childLayer->removeFromParent();

    bool found = false;
    for (unsigned i = 0; i < m_children.size(); i++) {
        if (sibling == m_children[i]) {
            m_children.insert(i, childLayer);
            found = true;
            break;
        }
    }

    childLayer->setParent(this);

    if (!found)
        m_children.append(childLayer);
}

void GraphicsLayer::addChildAbove(GraphicsLayer* childLayer, GraphicsLayer* sibling)
{
    childLayer->removeFromParent();
    ASSERT(childLayer != this);

    bool found = false;
    for (unsigned i = 0; i < m_children.size(); i++) {
        if (sibling == m_children[i]) {
            m_children.insert(i+1, childLayer);
            found = true;
            break;
        }
    }

    childLayer->setParent(this);

    if (!found)
        m_children.append(childLayer);
}

bool GraphicsLayer::replaceChild(GraphicsLayer* oldChild, GraphicsLayer* newChild)
{
    ASSERT(!newChild->parent());
    bool found = false;
    for (unsigned i = 0; i < m_children.size(); i++) {
        if (oldChild == m_children[i]) {
            m_children[i] = newChild;
            found = true;
            break;
        }
    }
    if (found) {
        oldChild->setParent(0);

        newChild->removeFromParent();
        newChild->setParent(this);
        return true;
    }
    return false;
}

void GraphicsLayer::removeAllChildren()
{
    while (m_children.size()) {
        GraphicsLayer* curLayer = m_children[0];
        ASSERT(curLayer->parent());
        curLayer->removeFromParent();
    }
}

void GraphicsLayer::removeFromParent()
{
    if (m_parent) {
        unsigned i;
        for (i = 0; i < m_parent->m_children.size(); i++) {
            if (this == m_parent->m_children[i]) {
                m_parent->m_children.remove(i);
                break;
            }
        }

        setParent(0);
    }
}

void GraphicsLayer::setBackgroundColor(const Color& color)
{
    m_backgroundColor = color;
    m_backgroundColorSet = true;
}

void GraphicsLayer::clearBackgroundColor()
{
    m_backgroundColor = Color();
    m_backgroundColorSet = false;
}

void GraphicsLayer::paintGraphicsLayerContents(GraphicsContext& context, const IntRect& clip)
{
    if (m_client)
        m_client->paintContents(this, context, m_paintingPhase, clip);
}

void GraphicsLayer::suspendAnimations(double)
{
}

void GraphicsLayer::resumeAnimations()
{
}

#ifndef NDEBUG
void GraphicsLayer::updateDebugIndicators()
{
    if (GraphicsLayer::showDebugBorders()) {
        if (drawsContent()) {
            if (m_usingTiledLayer)
                setDebugBorder(Color(0, 255, 0, 204), 2.0f);    // tiled layer: green
            else
                setDebugBorder(Color(255, 0, 0, 204), 2.0f);    // normal layer: red
        } else if (masksToBounds()) {
            setDebugBorder(Color(128, 255, 255, 178), 2.0f);    // masking layer: pale blue
            if (GraphicsLayer::showDebugBorders())
                setDebugBackgroundColor(Color(128, 255, 255, 52));
        } else
            setDebugBorder(Color(255, 255, 0, 204), 2.0f);      // container: yellow
    }
}

void GraphicsLayer::setZPosition(float position)
{
    m_zPosition = position;
}
#endif

float GraphicsLayer::accumulatedOpacity() const
{
    if (!preserves3D())
        return 1;
        
    return m_opacity * (parent() ? parent()->accumulatedOpacity() : 1);
}

void GraphicsLayer::distributeOpacity(float accumulatedOpacity)
{
    // If this is a transform layer we need to distribute our opacity to all our children
    
    // Incoming accumulatedOpacity is the contribution from our parent(s). We mutiply this by our own
    // opacity to get the total contribution
    accumulatedOpacity *= m_opacity;
    
    setOpacityInternal(accumulatedOpacity);
    
    if (preserves3D()) {
        size_t numChildren = children().size();
        for (size_t i = 0; i < numChildren; ++i)
            children()[i]->distributeOpacity(accumulatedOpacity);
    }
}

// An "invalid" list is one whose functions don't match, and therefore has to be animated as a Matrix
// The hasBigRotation flag will always return false if isValid is false. Otherwise hasBigRotation is 
// true if the rotation between any two keyframes is >= 180 degrees.

static inline const TransformOperations* operationsAt(const KeyframeValueList& valueList, size_t index)
{
    return static_cast<const TransformAnimationValue*>(valueList.at(index))->value();
}

void GraphicsLayer::fetchTransformOperationList(const KeyframeValueList& valueList, TransformOperationList& list, bool& isValid, bool& hasBigRotation)
{
    ASSERT(valueList.property() == AnimatedPropertyWebkitTransform);

    list.clear();
    isValid = false;
    hasBigRotation = false;
    
    if (valueList.size() < 2)
        return;
    
    // Empty transforms match anything, so find the first non-empty entry as the reference.
    size_t firstIndex = 0;
    for ( ; firstIndex < valueList.size(); ++firstIndex) {
        if (operationsAt(valueList, firstIndex)->operations().size() > 0)
            break;
    }
    
    if (firstIndex >= valueList.size())
        return;
        
    const TransformOperations* firstVal = operationsAt(valueList, firstIndex);
    
    // See if the keyframes are valid.
    for (size_t i = firstIndex + 1; i < valueList.size(); ++i) {
        const TransformOperations* val = operationsAt(valueList, i);
        
        // a null transform matches anything
        if (val->operations().isEmpty())
            continue;
            
        if (firstVal->operations().size() != val->operations().size())
            return;
            
        for (size_t j = 0; j < firstVal->operations().size(); ++j) {
            if (!firstVal->operations().at(j)->isSameType(*val->operations().at(j)))
                return;
        }
    }

    // Keyframes are valid, fill in the list.
    isValid = true;
    
    double lastRotAngle = 0.0;
    double maxRotAngle = -1.0;
        
    list.resize(firstVal->operations().size());
    for (size_t j = 0; j < firstVal->operations().size(); ++j) {
        TransformOperation::OperationType type = firstVal->operations().at(j)->getOperationType();
        list[j] = type;
        
        // if this is a rotation entry, we need to see if any angle differences are >= 180 deg
        if (type == TransformOperation::ROTATE_X ||
            type == TransformOperation::ROTATE_Y ||
            type == TransformOperation::ROTATE_Z ||
            type == TransformOperation::ROTATE_3D) {
            lastRotAngle = static_cast<RotateTransformOperation*>(firstVal->operations().at(j).get())->angle();
            
            if (maxRotAngle < 0)
                maxRotAngle = fabs(lastRotAngle);
            
            for (size_t i = firstIndex + 1; i < valueList.size(); ++i) {
                const TransformOperations* val = operationsAt(valueList, i);
                double rotAngle = val->operations().isEmpty() ? 0 : (static_cast<RotateTransformOperation*>(val->operations().at(j).get())->angle());
                double diffAngle = fabs(rotAngle - lastRotAngle);
                if (diffAngle > maxRotAngle)
                    maxRotAngle = diffAngle;
                lastRotAngle = rotAngle;
            }
        }
    }
    
    hasBigRotation = maxRotAngle >= 180.0;
}


static void writeIndent(TextStream& ts, int indent)
{
    for (int i = 0; i != indent; ++i)
        ts << "  ";
}

void GraphicsLayer::dumpLayer(TextStream& ts, int indent) const
{
    writeIndent(ts, indent);
    ts << "(" << "GraphicsLayer" << " " << static_cast<void*>(const_cast<GraphicsLayer*>(this));
    ts << " \"" << m_name << "\"\n";
    dumpProperties(ts, indent);
    writeIndent(ts, indent);
    ts << ")\n";
}

void GraphicsLayer::dumpProperties(TextStream& ts, int indent) const
{
    writeIndent(ts, indent + 1);
    ts << "(position " << m_position.x() << " " << m_position.y() << ")\n";

    writeIndent(ts, indent + 1);
    ts << "(anchor " << m_anchorPoint.x() << " " << m_anchorPoint.y() << ")\n";

    writeIndent(ts, indent + 1);
    ts << "(bounds " << m_size.width() << " " << m_size.height() << ")\n";

    writeIndent(ts, indent + 1);
    ts << "(opacity " << m_opacity << ")\n";
    
    writeIndent(ts, indent + 1);
    ts << "(usingTiledLayer " << m_usingTiledLayer << ")\n";

    writeIndent(ts, indent + 1);
    ts << "(m_preserves3D " << m_preserves3D << ")\n";

    writeIndent(ts, indent + 1);
    ts << "(drawsContent " << m_drawsContent << ")\n";

    writeIndent(ts, indent + 1);
    ts << "(m_backfaceVisibility " << (m_backfaceVisibility ? "visible" : "hidden") << ")\n";

    writeIndent(ts, indent + 1);
    ts << "(client ";
    if (m_client)
        ts << static_cast<void*>(m_client);
    else
        ts << "none";
    ts << ")\n";

    writeIndent(ts, indent + 1);
    ts << "(backgroundColor ";
    if (!m_backgroundColorSet)
        ts << "none";
    else
        ts << m_backgroundColor.name();
    ts << ")\n";

    writeIndent(ts, indent + 1);
    ts << "(transform ";
    if (m_transform.isIdentity())
        ts << "identity";
    else {
        ts << "[" << m_transform.m11() << " " << m_transform.m12() << " " << m_transform.m13() << " " << m_transform.m14() << "] ";
        ts << "[" << m_transform.m21() << " " << m_transform.m22() << " " << m_transform.m23() << " " << m_transform.m24() << "] ";
        ts << "[" << m_transform.m31() << " " << m_transform.m32() << " " << m_transform.m33() << " " << m_transform.m34() << "] ";
        ts << "[" << m_transform.m41() << " " << m_transform.m42() << " " << m_transform.m43() << " " << m_transform.m44() << "]";
    }
    ts << ")\n";

    writeIndent(ts, indent + 1);
    ts << "(childrenTransform ";
    if (m_childrenTransform.isIdentity())
        ts << "identity";
    else {
        ts << "[" << m_childrenTransform.m11() << " " << m_childrenTransform.m12() << " " << m_childrenTransform.m13() << " " << m_childrenTransform.m14() << "] ";
        ts << "[" << m_childrenTransform.m21() << " " << m_childrenTransform.m22() << " " << m_childrenTransform.m23() << " " << m_childrenTransform.m24() << "] ";
        ts << "[" << m_childrenTransform.m31() << " " << m_childrenTransform.m32() << " " << m_childrenTransform.m33() << " " << m_childrenTransform.m34() << "] ";
        ts << "[" << m_childrenTransform.m41() << " " << m_childrenTransform.m42() << " " << m_childrenTransform.m43() << " " << m_childrenTransform.m44() << "]";
    }
    ts << ")\n";

    writeIndent(ts, indent + 1);
    ts << "(children " << m_children.size() << "\n";
    
    unsigned i;
    for (i = 0; i < m_children.size(); i++)
        m_children[i]->dumpLayer(ts, indent+2);
    writeIndent(ts, indent + 1);
    ts << ")\n";
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)
