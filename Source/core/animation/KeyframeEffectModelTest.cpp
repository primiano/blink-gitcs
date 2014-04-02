/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
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
#include "core/animation/KeyframeEffectModel.h"

#include "core/animation/AnimatableLength.h"
#include "core/animation/AnimatableUnknown.h"
#include "core/css/CSSPrimitiveValue.h"
#include "core/css/parser/BisonCSSParser.h"
#include "core/css/resolver/CSSToStyleMap.h"
#include <gtest/gtest.h>

using namespace WebCore;

namespace {

const double duration = 1.0;

PassRefPtrWillBeRawPtr<AnimatableValue> unknownAnimatableValue(double n)
{
    return AnimatableUnknown::create(CSSPrimitiveValue::create(n, CSSPrimitiveValue::CSS_UNKNOWN).get());
}

PassRefPtrWillBeRawPtr<AnimatableValue> pixelAnimatableValue(double n)
{
    return AnimatableLength::create(CSSPrimitiveValue::create(n, CSSPrimitiveValue::CSS_PX).get());
}

KeyframeEffectModel::KeyframeVector keyframesAtZeroAndOne(PassRefPtrWillBeRawPtr<AnimatableValue> zeroValue, PassRefPtrWillBeRawPtr<AnimatableValue> oneValue)
{
    KeyframeEffectModel::KeyframeVector keyframes(2);
    keyframes[0] = Keyframe::create();
    keyframes[0]->setOffset(0.0);
    keyframes[0]->setPropertyValue(CSSPropertyLeft, zeroValue.get());
    keyframes[1] = Keyframe::create();
    keyframes[1]->setOffset(1.0);
    keyframes[1]->setPropertyValue(CSSPropertyLeft, oneValue.get());
    return keyframes;
}

void expectProperty(CSSPropertyID property, PassRefPtrWillBeRawPtr<Interpolation> interpolationValue)
{
    LegacyStyleInterpolation* interpolation = toLegacyStyleInterpolation(interpolationValue.get());
    ASSERT_EQ(property, interpolation->id());
}

void expectDoubleValue(double expectedValue, PassRefPtrWillBeRawPtr<Interpolation> interpolationValue)
{
    LegacyStyleInterpolation* interpolation = toLegacyStyleInterpolation(interpolationValue.get());
    RefPtrWillBeRawPtr<AnimatableValue> value = interpolation->currentValue();

    ASSERT_TRUE(value->isLength() || value->isUnknown());

    double actualValue;
    if (value->isLength())
        actualValue = toCSSPrimitiveValue(toAnimatableLength(value.get())->toCSSValue().get())->getDoubleValue();
    else
        actualValue = toCSSPrimitiveValue(toAnimatableUnknown(value.get())->toCSSValue().get())->getDoubleValue();

    EXPECT_FLOAT_EQ(static_cast<float>(expectedValue), actualValue);
}

Interpolation* findValue(WillBeHeapVector<RefPtrWillBeMember<Interpolation> >& values, CSSPropertyID id)
{
    for (size_t i = 0; i < values.size(); ++i) {
        LegacyStyleInterpolation* value = toLegacyStyleInterpolation(values.at(i).get());
        if (value->id() == id)
            return value;
    }
    return 0;
}


TEST(AnimationKeyframeEffectModel, BasicOperation)
{
    KeyframeEffectModel::KeyframeVector keyframes = keyframesAtZeroAndOne(unknownAnimatableValue(3.0), unknownAnimatableValue(5.0));
    RefPtrWillBeRawPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    OwnPtrWillBeRawPtr<WillBeHeapVector<RefPtrWillBeMember<Interpolation> > > values = effect->sample(0, 0.6, duration);
    ASSERT_EQ(1UL, values->size());
    expectProperty(CSSPropertyLeft, values->at(0));
    expectDoubleValue(5.0, values->at(0));
}

TEST(AnimationKeyframeEffectModel, CompositeReplaceNonInterpolable)
{
    KeyframeEffectModel::KeyframeVector keyframes = keyframesAtZeroAndOne(unknownAnimatableValue(3.0), unknownAnimatableValue(5.0));
    keyframes[0]->setComposite(AnimationEffect::CompositeReplace);
    keyframes[1]->setComposite(AnimationEffect::CompositeReplace);
    RefPtrWillBeRawPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    expectDoubleValue(5.0, effect->sample(0, 0.6, duration)->at(0));
}

TEST(AnimationKeyframeEffectModel, CompositeReplace)
{
    KeyframeEffectModel::KeyframeVector keyframes = keyframesAtZeroAndOne(pixelAnimatableValue(3.0), pixelAnimatableValue(5.0));
    keyframes[0]->setComposite(AnimationEffect::CompositeReplace);
    keyframes[1]->setComposite(AnimationEffect::CompositeReplace);
    RefPtrWillBeRawPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    expectDoubleValue(3.0 * 0.4 + 5.0 * 0.6, effect->sample(0, 0.6, duration)->at(0));
}

// FIXME: Re-enable this test once compositing of CompositeAdd is supported.
TEST(AnimationKeyframeEffectModel, DISABLED_CompositeAdd)
{
    KeyframeEffectModel::KeyframeVector keyframes = keyframesAtZeroAndOne(pixelAnimatableValue(3.0), pixelAnimatableValue(5.0));
    keyframes[0]->setComposite(AnimationEffect::CompositeAdd);
    keyframes[1]->setComposite(AnimationEffect::CompositeAdd);
    RefPtrWillBeRawPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    expectDoubleValue((7.0 + 3.0) * 0.4 + (7.0 + 5.0) * 0.6, effect->sample(0, 0.6, duration)->at(0));
}

TEST(AnimationKeyframeEffectModel, CompositeEaseIn)
{
    KeyframeEffectModel::KeyframeVector keyframes = keyframesAtZeroAndOne(pixelAnimatableValue(3.0), pixelAnimatableValue(5.0));
    RefPtrWillBeRawPtr<CSSValue> timingFunction = BisonCSSParser::parseAnimationTimingFunctionValue("ease-in");
    keyframes[0]->setComposite(AnimationEffect::CompositeReplace);
    keyframes[0]->setEasing(CSSToStyleMap::animationTimingFunction(timingFunction.get(), false));
    keyframes[1]->setComposite(AnimationEffect::CompositeReplace);
    RefPtrWillBeRawPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    expectDoubleValue(3.8579516, effect->sample(0, 0.6, duration)->at(0));
    expectDoubleValue(3.8582394, effect->sample(0, 0.6, duration * 100)->at(0));
}

TEST(AnimationKeyframeEffectModel, CompositeCubicBezier)
{
    KeyframeEffectModel::KeyframeVector keyframes = keyframesAtZeroAndOne(pixelAnimatableValue(3.0), pixelAnimatableValue(5.0));
    RefPtrWillBeRawPtr<CSSValue> timingFunction = BisonCSSParser::parseAnimationTimingFunctionValue("cubic-bezier(0.42, 0, 0.58, 1)");
    keyframes[0]->setComposite(AnimationEffect::CompositeReplace);
    keyframes[0]->setEasing(CSSToStyleMap::animationTimingFunction(timingFunction.get(), false));
    keyframes[1]->setComposite(AnimationEffect::CompositeReplace);
    RefPtrWillBeRawPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    expectDoubleValue(4.3363357, effect->sample(0, 0.6, duration)->at(0));
    expectDoubleValue(4.3362322, effect->sample(0, 0.6, duration * 1000)->at(0));
}

TEST(AnimationKeyframeEffectModel, ExtrapolateReplaceNonInterpolable)
{
    KeyframeEffectModel::KeyframeVector keyframes = keyframesAtZeroAndOne(unknownAnimatableValue(3.0), unknownAnimatableValue(5.0));
    keyframes[0]->setComposite(AnimationEffect::CompositeReplace);
    keyframes[1]->setComposite(AnimationEffect::CompositeReplace);
    RefPtrWillBeRawPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    expectDoubleValue(5.0, effect->sample(0, 1.6, duration)->at(0));
}

TEST(AnimationKeyframeEffectModel, ExtrapolateReplace)
{
    KeyframeEffectModel::KeyframeVector keyframes = keyframesAtZeroAndOne(pixelAnimatableValue(3.0), pixelAnimatableValue(5.0));
    RefPtrWillBeRawPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    keyframes[0]->setComposite(AnimationEffect::CompositeReplace);
    keyframes[1]->setComposite(AnimationEffect::CompositeReplace);
    expectDoubleValue(3.0 * -0.6 + 5.0 * 1.6, effect->sample(0, 1.6, duration)->at(0));
}

// FIXME: Re-enable this test once compositing of CompositeAdd is supported.
TEST(AnimationKeyframeEffectModel, DISABLED_ExtrapolateAdd)
{
    KeyframeEffectModel::KeyframeVector keyframes = keyframesAtZeroAndOne(pixelAnimatableValue(3.0), pixelAnimatableValue(5.0));
    keyframes[0]->setComposite(AnimationEffect::CompositeAdd);
    keyframes[1]->setComposite(AnimationEffect::CompositeAdd);
    RefPtrWillBeRawPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    expectDoubleValue((7.0 + 3.0) * -0.6 + (7.0 + 5.0) * 1.6, effect->sample(0, 1.6, duration)->at(0));
}

TEST(AnimationKeyframeEffectModel, ZeroKeyframes)
{
    RefPtrWillBeRawPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(KeyframeEffectModel::KeyframeVector());
    EXPECT_TRUE(effect->sample(0, 0.5, duration)->isEmpty());
}

// FIXME: Re-enable this test once compositing of CompositeAdd is supported.
TEST(AnimationKeyframeEffectModel, DISABLED_SingleKeyframeAtOffsetZero)
{
    KeyframeEffectModel::KeyframeVector keyframes(1);
    keyframes[0] = Keyframe::create();
    keyframes[0]->setOffset(0.0);
    keyframes[0]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(3.0).get());

    RefPtrWillBeRawPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    expectDoubleValue(3.0, effect->sample(0, 0.6, duration)->at(0));
}

// FIXME: Re-enable this test once compositing of CompositeAdd is supported.
TEST(AnimationKeyframeEffectModel, DISABLED_SingleKeyframeAtOffsetOne)
{
    KeyframeEffectModel::KeyframeVector keyframes(1);
    keyframes[0] = Keyframe::create();
    keyframes[0]->setOffset(1.0);
    keyframes[0]->setPropertyValue(CSSPropertyLeft, pixelAnimatableValue(5.0).get());

    RefPtrWillBeRawPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    expectDoubleValue(7.0 * 0.4 + 5.0 * 0.6, effect->sample(0, 0.6, duration)->at(0));
}

TEST(AnimationKeyframeEffectModel, MoreThanTwoKeyframes)
{
    KeyframeEffectModel::KeyframeVector keyframes(3);
    keyframes[0] = Keyframe::create();
    keyframes[0]->setOffset(0.0);
    keyframes[0]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(3.0).get());
    keyframes[1] = Keyframe::create();
    keyframes[1]->setOffset(0.5);
    keyframes[1]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(4.0).get());
    keyframes[2] = Keyframe::create();
    keyframes[2]->setOffset(1.0);
    keyframes[2]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(5.0).get());

    RefPtrWillBeRawPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    expectDoubleValue(4.0, effect->sample(0, 0.3, duration)->at(0));
    expectDoubleValue(5.0, effect->sample(0, 0.8, duration)->at(0));
}

TEST(AnimationKeyframeEffectModel, EndKeyframeOffsetsUnspecified)
{
    KeyframeEffectModel::KeyframeVector keyframes(3);
    keyframes[0] = Keyframe::create();
    keyframes[0]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(3.0).get());
    keyframes[1] = Keyframe::create();
    keyframes[1]->setOffset(0.5);
    keyframes[1]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(4.0).get());
    keyframes[2] = Keyframe::create();
    keyframes[2]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(5.0).get());

    RefPtrWillBeRawPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    expectDoubleValue(3.0, effect->sample(0, 0.1, duration)->at(0));
    expectDoubleValue(4.0, effect->sample(0, 0.6, duration)->at(0));
    expectDoubleValue(5.0, effect->sample(0, 0.9, duration)->at(0));
}

TEST(AnimationKeyframeEffectModel, SampleOnKeyframe)
{
    KeyframeEffectModel::KeyframeVector keyframes(3);
    keyframes[0] = Keyframe::create();
    keyframes[0]->setOffset(0.0);
    keyframes[0]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(3.0).get());
    keyframes[1] = Keyframe::create();
    keyframes[1]->setOffset(0.5);
    keyframes[1]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(4.0).get());
    keyframes[2] = Keyframe::create();
    keyframes[2]->setOffset(1.0);
    keyframes[2]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(5.0).get());

    RefPtrWillBeRawPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    expectDoubleValue(3.0, effect->sample(0, 0.0, duration)->at(0));
    expectDoubleValue(4.0, effect->sample(0, 0.5, duration)->at(0));
    expectDoubleValue(5.0, effect->sample(0, 1.0, duration)->at(0));
}

TEST(AnimationKeyframeEffectModel, MultipleKeyframesWithSameOffset)
{
    KeyframeEffectModel::KeyframeVector keyframes(9);
    keyframes[0] = Keyframe::create();
    keyframes[0]->setOffset(0.0);
    keyframes[0]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(0.0).get());
    keyframes[1] = Keyframe::create();
    keyframes[1]->setOffset(0.1);
    keyframes[1]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(1.0).get());
    keyframes[2] = Keyframe::create();
    keyframes[2]->setOffset(0.1);
    keyframes[2]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(2.0).get());
    keyframes[3] = Keyframe::create();
    keyframes[3]->setOffset(0.5);
    keyframes[3]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(3.0).get());
    keyframes[4] = Keyframe::create();
    keyframes[4]->setOffset(0.5);
    keyframes[4]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(4.0).get());
    keyframes[5] = Keyframe::create();
    keyframes[5]->setOffset(0.5);
    keyframes[5]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(5.0).get());
    keyframes[6] = Keyframe::create();
    keyframes[6]->setOffset(0.9);
    keyframes[6]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(6.0).get());
    keyframes[7] = Keyframe::create();
    keyframes[7]->setOffset(0.9);
    keyframes[7]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(7.0).get());
    keyframes[8] = Keyframe::create();
    keyframes[8]->setOffset(1.0);
    keyframes[8]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(7.0).get());

    RefPtrWillBeRawPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    expectDoubleValue(0.0, effect->sample(0, 0.0, duration)->at(0));
    expectDoubleValue(2.0, effect->sample(0, 0.2, duration)->at(0));
    expectDoubleValue(3.0, effect->sample(0, 0.4, duration)->at(0));
    expectDoubleValue(5.0, effect->sample(0, 0.5, duration)->at(0));
    expectDoubleValue(5.0, effect->sample(0, 0.6, duration)->at(0));
    expectDoubleValue(6.0, effect->sample(0, 0.8, duration)->at(0));
    expectDoubleValue(7.0, effect->sample(0, 1.0, duration)->at(0));
}

// FIXME: Re-enable this test once compositing of CompositeAdd is supported.
TEST(AnimationKeyframeEffectModel, DISABLED_PerKeyframeComposite)
{
    KeyframeEffectModel::KeyframeVector keyframes(2);
    keyframes[0] = Keyframe::create();
    keyframes[0]->setOffset(0.0);
    keyframes[0]->setPropertyValue(CSSPropertyLeft, pixelAnimatableValue(3.0).get());
    keyframes[1] = Keyframe::create();
    keyframes[1]->setOffset(1.0);
    keyframes[1]->setPropertyValue(CSSPropertyLeft, pixelAnimatableValue(5.0).get());
    keyframes[1]->setComposite(AnimationEffect::CompositeAdd);

    RefPtrWillBeRawPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    expectDoubleValue(3.0 * 0.4 + (7.0 + 5.0) * 0.6, effect->sample(0, 0.6, duration)->at(0));
}

TEST(AnimationKeyframeEffectModel, MultipleProperties)
{
    KeyframeEffectModel::KeyframeVector keyframes(2);
    keyframes[0] = Keyframe::create();
    keyframes[0]->setOffset(0.0);
    keyframes[0]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(3.0).get());
    keyframes[0]->setPropertyValue(CSSPropertyRight, unknownAnimatableValue(4.0).get());
    keyframes[1] = Keyframe::create();
    keyframes[1]->setOffset(1.0);
    keyframes[1]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(5.0).get());
    keyframes[1]->setPropertyValue(CSSPropertyRight, unknownAnimatableValue(6.0).get());

    RefPtrWillBeRawPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    OwnPtrWillBeRawPtr<WillBeHeapVector<RefPtrWillBeMember<Interpolation> > > values = effect->sample(0, 0.6, duration);
    EXPECT_EQ(2UL, values->size());
    Interpolation* leftValue = findValue(*values.get(), CSSPropertyLeft);
    ASSERT_TRUE(leftValue);
    expectDoubleValue(5.0, leftValue);
    Interpolation* rightValue = findValue(*values.get(), CSSPropertyRight);
    ASSERT_TRUE(rightValue);
    expectDoubleValue(6.0, rightValue);
}

// FIXME: Re-enable this test once compositing of CompositeAdd is supported.
TEST(AnimationKeyframeEffectModel, DISABLED_RecompositeCompositableValue)
{
    KeyframeEffectModel::KeyframeVector keyframes = keyframesAtZeroAndOne(pixelAnimatableValue(3.0), pixelAnimatableValue(5.0));
    keyframes[0]->setComposite(AnimationEffect::CompositeAdd);
    keyframes[1]->setComposite(AnimationEffect::CompositeAdd);
    RefPtrWillBeRawPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    OwnPtrWillBeRawPtr<WillBeHeapVector<RefPtrWillBeMember<Interpolation> > > values = effect->sample(0, 0.6, duration);
    expectDoubleValue((7.0 + 3.0) * 0.4 + (7.0 + 5.0) * 0.6, values->at(0));
    expectDoubleValue((9.0 + 3.0) * 0.4 + (9.0 + 5.0) * 0.6, values->at(0));
}

TEST(AnimationKeyframeEffectModel, MultipleIterations)
{
    KeyframeEffectModel::KeyframeVector keyframes = keyframesAtZeroAndOne(pixelAnimatableValue(1.0), pixelAnimatableValue(3.0));
    RefPtrWillBeRawPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    expectDoubleValue(2.0, effect->sample(0, 0.5, duration)->at(0));
    expectDoubleValue(2.0, effect->sample(1, 0.5, duration)->at(0));
    expectDoubleValue(2.0, effect->sample(2, 0.5, duration)->at(0));
}

// FIXME: Re-enable this test once compositing of CompositeAdd is supported.
TEST(AnimationKeyframeEffectModel, DISABLED_DependsOnUnderlyingValue)
{
    KeyframeEffectModel::KeyframeVector keyframes(3);
    keyframes[0] = Keyframe::create();
    keyframes[0]->setOffset(0.0);
    keyframes[0]->setPropertyValue(CSSPropertyLeft, pixelAnimatableValue(1.0).get());
    keyframes[0]->setComposite(AnimationEffect::CompositeAdd);
    keyframes[1] = Keyframe::create();
    keyframes[1]->setOffset(0.5);
    keyframes[1]->setPropertyValue(CSSPropertyLeft, pixelAnimatableValue(1.0).get());
    keyframes[2] = Keyframe::create();
    keyframes[2]->setOffset(1.0);
    keyframes[2]->setPropertyValue(CSSPropertyLeft, pixelAnimatableValue(1.0).get());

    RefPtrWillBeRawPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    EXPECT_TRUE(effect->sample(0, 0, duration)->at(0));
    EXPECT_TRUE(effect->sample(0, 0.1, duration)->at(0));
    EXPECT_TRUE(effect->sample(0, 0.25, duration)->at(0));
    EXPECT_TRUE(effect->sample(0, 0.4, duration)->at(0));
    EXPECT_FALSE(effect->sample(0, 0.5, duration)->at(0));
    EXPECT_FALSE(effect->sample(0, 0.6, duration)->at(0));
    EXPECT_FALSE(effect->sample(0, 0.75, duration)->at(0));
    EXPECT_FALSE(effect->sample(0, 0.8, duration)->at(0));
    EXPECT_FALSE(effect->sample(0, 1, duration)->at(0));
}

TEST(AnimationKeyframeEffectModel, AddSyntheticKeyframes)
{
    KeyframeEffectModel::KeyframeVector keyframes(1);
    keyframes[0] = Keyframe::create();
    keyframes[0]->setOffset(0.5);
    keyframes[0]->setPropertyValue(CSSPropertyLeft, unknownAnimatableValue(4.0).get());

    RefPtrWillBeRawPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);
    const KeyframeEffectModel::PropertySpecificKeyframeVector& propertySpecificKeyframes = effect->getPropertySpecificKeyframes(CSSPropertyLeft);
    EXPECT_EQ(3U, propertySpecificKeyframes.size());
    EXPECT_DOUBLE_EQ(0.0, propertySpecificKeyframes[0]->offset());
    EXPECT_DOUBLE_EQ(0.5, propertySpecificKeyframes[1]->offset());
    EXPECT_DOUBLE_EQ(1.0, propertySpecificKeyframes[2]->offset());
}

TEST(AnimationKeyframeEffectModel, ToKeyframeEffectModel)
{
    KeyframeEffectModel::KeyframeVector keyframes(0);
    RefPtrWillBeRawPtr<KeyframeEffectModel> effect = KeyframeEffectModel::create(keyframes);

    AnimationEffect* baseEffect = effect.get();
    EXPECT_TRUE(toKeyframeEffectModel(baseEffect));
}

} // namespace

namespace WebCore {

class KeyframeEffectModelTest : public ::testing::Test {
public:
    typedef KeyframeEffectModel::KeyframeVector KeyframeVector;

    static KeyframeVector normalizedKeyframes(const KeyframeVector& keyframes)
    {
        return KeyframeEffectModel::normalizedKeyframes(keyframes);
    }
};

TEST_F(KeyframeEffectModelTest, NotLooselySorted)
{
    KeyframeEffectModel::KeyframeVector keyframes(4);
    keyframes[0] = Keyframe::create();
    keyframes[1] = Keyframe::create();
    keyframes[1]->setOffset(9);
    keyframes[2] = Keyframe::create();
    keyframes[3] = Keyframe::create();
    keyframes[3]->setOffset(1);

    const KeyframeVector result = normalizedKeyframes(keyframes);
    EXPECT_EQ(0U, result.size());
}

TEST_F(KeyframeEffectModelTest, LastOne)
{
    KeyframeEffectModel::KeyframeVector keyframes(3);
    keyframes[0] = Keyframe::create();
    keyframes[0]->setOffset(-1);
    keyframes[1] = Keyframe::create();
    keyframes[2] = Keyframe::create();
    keyframes[2]->setOffset(2);

    const KeyframeVector result = normalizedKeyframes(keyframes);
    EXPECT_EQ(1U, result.size());
    EXPECT_DOUBLE_EQ(1.0, result[0]->offset());
}

TEST_F(KeyframeEffectModelTest, FirstZero)
{
    KeyframeEffectModel::KeyframeVector keyframes(3);
    keyframes[0] = Keyframe::create();
    keyframes[0]->setOffset(-1);
    keyframes[1] = Keyframe::create();
    keyframes[2] = Keyframe::create();
    keyframes[2]->setOffset(0.25);

    const KeyframeVector result = normalizedKeyframes(keyframes);
    EXPECT_EQ(2U, result.size());
    EXPECT_DOUBLE_EQ(0.0, result[0]->offset());
    EXPECT_DOUBLE_EQ(0.25, result[1]->offset());
}

TEST_F(KeyframeEffectModelTest, EvenlyDistributed1)
{
    KeyframeEffectModel::KeyframeVector keyframes(5);
    keyframes[0] = Keyframe::create();
    keyframes[0]->setOffset(0.125);
    keyframes[1] = Keyframe::create();
    keyframes[2] = Keyframe::create();
    keyframes[3] = Keyframe::create();
    keyframes[4] = Keyframe::create();
    keyframes[4]->setOffset(0.625);

    const KeyframeVector result = normalizedKeyframes(keyframes);
    EXPECT_EQ(5U, result.size());
    EXPECT_DOUBLE_EQ(0.125, result[0]->offset());
    EXPECT_DOUBLE_EQ(0.25, result[1]->offset());
    EXPECT_DOUBLE_EQ(0.375, result[2]->offset());
    EXPECT_DOUBLE_EQ(0.5, result[3]->offset());
    EXPECT_DOUBLE_EQ(0.625, result[4]->offset());
}

TEST_F(KeyframeEffectModelTest, EvenlyDistributed2)
{
    KeyframeEffectModel::KeyframeVector keyframes(8);
    keyframes[0] = Keyframe::create();
    keyframes[0]->setOffset(-0.1);
    keyframes[1] = Keyframe::create();
    keyframes[2] = Keyframe::create();
    keyframes[3] = Keyframe::create();
    keyframes[4] = Keyframe::create();
    keyframes[4]->setOffset(0.75);
    keyframes[5] = Keyframe::create();
    keyframes[6] = Keyframe::create();
    keyframes[7] = Keyframe::create();
    keyframes[7]->setOffset(1.1);

    const KeyframeVector result = normalizedKeyframes(keyframes);
    EXPECT_EQ(6U, result.size());
    EXPECT_DOUBLE_EQ(0.0, result[0]->offset());
    EXPECT_DOUBLE_EQ(0.25, result[1]->offset());
    EXPECT_DOUBLE_EQ(0.5, result[2]->offset());
    EXPECT_DOUBLE_EQ(0.75, result[3]->offset());
    EXPECT_DOUBLE_EQ(0.875, result[4]->offset());
    EXPECT_DOUBLE_EQ(1.0, result[5]->offset());
}

TEST_F(KeyframeEffectModelTest, EvenlyDistributed3)
{
    KeyframeEffectModel::KeyframeVector keyframes(12);
    keyframes[0] = Keyframe::create();
    keyframes[0]->setOffset(0);
    keyframes[1] = Keyframe::create();
    keyframes[2] = Keyframe::create();
    keyframes[3] = Keyframe::create();
    keyframes[4] = Keyframe::create();
    keyframes[4]->setOffset(0.5);
    keyframes[5] = Keyframe::create();
    keyframes[6] = Keyframe::create();
    keyframes[7] = Keyframe::create();
    keyframes[7]->setOffset(0.8);
    keyframes[8] = Keyframe::create();
    keyframes[9] = Keyframe::create();
    keyframes[10] = Keyframe::create();
    keyframes[11] = Keyframe::create();

    const KeyframeVector result = normalizedKeyframes(keyframes);
    EXPECT_EQ(12U, result.size());
    EXPECT_DOUBLE_EQ(0.0, result[0]->offset());
    EXPECT_DOUBLE_EQ(0.125, result[1]->offset());
    EXPECT_DOUBLE_EQ(0.25, result[2]->offset());
    EXPECT_DOUBLE_EQ(0.375, result[3]->offset());
    EXPECT_DOUBLE_EQ(0.5, result[4]->offset());
    EXPECT_DOUBLE_EQ(0.6, result[5]->offset());
    EXPECT_DOUBLE_EQ(0.7, result[6]->offset());
    EXPECT_DOUBLE_EQ(0.8, result[7]->offset());
    EXPECT_DOUBLE_EQ(0.85, result[8]->offset());
    EXPECT_DOUBLE_EQ(0.9, result[9]->offset());
    EXPECT_DOUBLE_EQ(0.95, result[10]->offset());
    EXPECT_DOUBLE_EQ(1.0, result[11]->offset());
}

} // namespace WebCore
