/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
              Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include <sstream>
#include <Corrade/TestSuite/Tester.h>

#include "Magnum/Animation/Interpolation.h"
#include "Magnum/Math/Half.h"

namespace Magnum { namespace Animation { namespace Test {

struct InterpolationTest: TestSuite::Tester {
    explicit InterpolationTest();

    void interpolate();
    void interpolateStrict();
    void interpolateSingleKeyframe();
    void interpolateNoKeyframe();

    void interpolateHint();
    void interpolateStrictHint();

    void interpolateDifferentResultType();
    void interpolateDifferentResultTypeStrict();

    void interpolateError();
    void interpolateStrictError();

    void debugExtrapolation();
};

namespace {

const struct {
    const char* name;
    Extrapolation extrapolationBefore;
    Extrapolation extrapolationAfter;
    Float time;
    Float expectedValue, expectedValueStrict;
    std::size_t expectedHint;
} Data[] {
    {"before default-constructed",
        Extrapolation::DefaultConstructed, Extrapolation::Extrapolated,
        -1.0f, 0.0f, 4.0f, 0},
    {"before constant",
        Extrapolation::Constant, Extrapolation::Extrapolated,
        -1.0f, 3.0f, 4.0f, 0},
    {"before extrapolated",
        Extrapolation::Extrapolated, Extrapolation::DefaultConstructed,
        -1.0f, 4.0f, 4.0f, 0},
    {"during first",
        Extrapolation::DefaultConstructed, Extrapolation::DefaultConstructed,
        1.5f, 1.5f, 1.5f, 0},
    {"during second",
        Extrapolation::DefaultConstructed, Extrapolation::DefaultConstructed,
        4.75f, 1.0f, 1.0f, 2},
    {"after default-constructed",
        Extrapolation::Extrapolated, Extrapolation::DefaultConstructed,
        6.0f, 0.0f, -1.5f, 2},
    {"after constant",
        Extrapolation::Extrapolated, Extrapolation::Constant,
        6.0f, 0.5f, -1.5f, 2},
    {"after extrapolated",
        Extrapolation::DefaultConstructed, Extrapolation::Extrapolated,
        6.0f, -1.5f, -1.5f, 2}
};

const struct {
    const char* name;
    Extrapolation extrapolation;
    Float time;
    Float expectedValue;
} SingleKeyframeData[] {
    {"before default-constructed",
        Extrapolation::DefaultConstructed, -1.0f, 0.0f},
    {"before constant",
        Extrapolation::Constant, -1.0f, 3.0f},
    {"before extrapolated",
        Extrapolation::Extrapolated, -1.0f, 3.0f},
    {"at",
        Extrapolation::DefaultConstructed, 0.0f, 3.0f},
    {"after default-constructed",
        Extrapolation::DefaultConstructed, 1.0f, 0.0f},
    {"after constant",
        Extrapolation::Constant, 1.0f, 3.0f},
    {"after extrapolated",
        Extrapolation::Extrapolated, 1.0f, 3.0f}
};

const struct {
    const char* name;
    std::size_t hint;
} HintData[] {
    {"before", 1},
    {"at", 2},
    {"after", 3},
    {"out of bounds", 405780454}
};

}

InterpolationTest::InterpolationTest() {
    addInstancedTests({&InterpolationTest::interpolate,
                       &InterpolationTest::interpolateStrict},
                       Containers::arraySize(Data));

    addInstancedTests({&InterpolationTest::interpolateSingleKeyframe},
                       Containers::arraySize(SingleKeyframeData));

    addTests({&InterpolationTest::interpolateNoKeyframe});

    addInstancedTests({&InterpolationTest::interpolateHint,
                       &InterpolationTest::interpolateStrictHint},
                       Containers::arraySize(HintData));

    addTests({&InterpolationTest::interpolateDifferentResultType,
              &InterpolationTest::interpolateDifferentResultTypeStrict,

              &InterpolationTest::interpolateError,
              &InterpolationTest::interpolateStrictError,

              &InterpolationTest::debugExtrapolation});
}

namespace {
    constexpr Float Keys[]{0.0f, 2.0f, 4.0f, 5.0f};
    constexpr Float Values[]{3.0f, 1.0f, 2.5f, 0.5f};
}

void InterpolationTest::interpolate() {
    const auto& data = Data[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    std::size_t hint{};
    CORRADE_COMPARE((Animation::interpolate<Float, Float>(
        Keys, Values, data.extrapolationBefore, data.extrapolationAfter,
        Math::lerp, data.time, hint)), data.expectedValue);
    CORRADE_COMPARE(hint, data.expectedHint);
}

void InterpolationTest::interpolateStrict() {
    const auto& data = Data[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    std::size_t hint{};
    CORRADE_COMPARE((Animation::interpolateStrict<Float, Float>(
        Keys, Values, Math::lerp, data.time, hint)), data.expectedValueStrict);
    CORRADE_COMPARE(hint, data.expectedHint);
}

void InterpolationTest::interpolateSingleKeyframe() {
    const auto& data = SingleKeyframeData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    std::size_t hint{};
    CORRADE_COMPARE((Animation::interpolate<Float, Float>(
        Containers::arrayView(Keys).prefix(1),
        Containers::arrayView(Values).prefix(1),
        data.extrapolation, data.extrapolation,
        Math::lerp, data.time, hint)), data.expectedValue);
    CORRADE_COMPARE(hint, 0);
}

void InterpolationTest::interpolateNoKeyframe() {
    std::size_t hint{};
    CORRADE_COMPARE((Animation::interpolate<Float, Float>(
        nullptr, nullptr, Extrapolation::Extrapolated,
        Extrapolation::Extrapolated, Math::lerp, 3.5f, hint)), Float{});
    CORRADE_COMPARE(hint, 0);
}

void InterpolationTest::interpolateHint() {
    const auto& data = HintData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    std::size_t hint = data.hint;
    CORRADE_COMPARE((Animation::interpolate<Float, Float>(
        Keys, Values, Extrapolation::Extrapolated, Extrapolation::Extrapolated,
        Math::lerp, 4.75f, hint)), 1.0f);
    CORRADE_COMPARE(hint, 2);
}

void InterpolationTest::interpolateStrictHint() {
    const auto& data = HintData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    std::size_t hint = data.hint;
    CORRADE_COMPARE((Animation::interpolateStrict<Float, Float>(
        Keys, Values, Math::lerp, 4.75f, hint)), 1.0f);
    CORRADE_COMPARE(hint, 2);
}

namespace {
    using namespace Math::Literals;

    const Half HalfValues[]{3.0_h, 1.0_h, 2.5_h, 0.5_h};

    Float lerpHalf(const Half& a, const Half& b, Float t) {
        return Math::lerp(Float(a), Float(b), t);
    }
}

void InterpolationTest::interpolateDifferentResultType() {
    std::size_t hint{};
    CORRADE_COMPARE((Animation::interpolate<Float, Half, Float>(
        Keys, HalfValues, Extrapolation::Extrapolated, Extrapolation::Extrapolated, lerpHalf, 4.75f, hint)), 1.0f);
    CORRADE_COMPARE(hint, 2);
}

void InterpolationTest::interpolateDifferentResultTypeStrict() {
    std::size_t hint{};
    CORRADE_COMPARE((Animation::interpolateStrict<Float, Half, Float>(
        Keys, HalfValues, lerpHalf, 4.75f, hint)), 1.0f);
    CORRADE_COMPARE(hint, 2);
}

void InterpolationTest::interpolateError() {
    std::ostringstream out;
    Error redirectError{&out};

    {
        std::size_t hint{};
        Animation::interpolate<Float, Float>(Keys, nullptr, Extrapolation::Extrapolated, Extrapolation::Extrapolated, Math::lerp, 0.0f, hint);
    }

    CORRADE_COMPARE(out.str(),
        "Animation::interpolate(): keys and values don't have the same size\n");
}

void InterpolationTest::interpolateStrictError() {
    std::ostringstream out;
    Error redirectError{&out};

    {
        std::size_t hint{};
        Animation::interpolateStrict<Float, Float>(
            Containers::arrayView(Keys).prefix(1),
            Containers::arrayView(Values).prefix(1),
            Math::lerp, 0.0f, hint);
    } {
        std::size_t hint{};
        Animation::interpolateStrict<Float, Float>(
            Containers::arrayView(Keys).prefix(3), Values,
            Math::lerp, 0.0f, hint);
    }

    CORRADE_COMPARE(out.str(),
        "Animation::interpolateStrict(): at least two keyframes required\n"
        "Animation::interpolateStrict(): keys and values don't have the same size\n");
}

void InterpolationTest::debugExtrapolation() {
    std::ostringstream out;

    Debug{&out} << Extrapolation::DefaultConstructed << Extrapolation(0xde);
    CORRADE_COMPARE(out.str(), "Animation::Extrapolation::DefaultConstructed Animation::Extrapolation(0xde)\n");
}

}}}

CORRADE_TEST_MAIN(Magnum::Animation::Test::InterpolationTest)
