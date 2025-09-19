/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "server/Config.h"
#include "inputleap/option_types.h"

#include <gtest/gtest.h>
#include <sstream>

namespace inputleap {

// Test helper function that mimics the Server's scroll delta calculation
std::pair<std::int32_t, std::int32_t> applyScrollDelta(
    std::int32_t xDelta, std::int32_t yDelta,
    const Config::ScreenOptions* options)
{
    std::int32_t adjustedXDelta = xDelta;
    std::int32_t adjustedYDelta = yDelta;

    if (options != nullptr) {
        auto it = options->find(kOptionMouseScrollDelta);
        if (it != options->end()) {
            float multiplier = static_cast<float>(it->second) / 1000.0f;
            adjustedXDelta = static_cast<std::int32_t>(xDelta * multiplier);
            adjustedYDelta = static_cast<std::int32_t>(yDelta * multiplier);
        }
    }

    return {adjustedXDelta, adjustedYDelta};
}

TEST(ServerScrollDeltaTests, applyScrollDelta_noOptions_returnsOriginalValues)
{
    auto result = applyScrollDelta(100, 200, nullptr);
    EXPECT_EQ(100, result.first);
    EXPECT_EQ(200, result.second);
}

TEST(ServerScrollDeltaTests, applyScrollDelta_emptyOptions_returnsOriginalValues)
{
    Config::ScreenOptions emptyOptions;
    auto result = applyScrollDelta(100, 200, &emptyOptions);
    EXPECT_EQ(100, result.first);
    EXPECT_EQ(200, result.second);
}

TEST(ServerScrollDeltaTests, applyScrollDelta_withMultiplier_appliesCorrectScaling)
{
    Config::ScreenOptions options;
    options[kOptionMouseScrollDelta] = 2500; // 2.5 multiplier

    auto result = applyScrollDelta(100, 200, &options);
    EXPECT_EQ(250, result.first);   // 100 * 2.5
    EXPECT_EQ(500, result.second);  // 200 * 2.5
}

TEST(ServerScrollDeltaTests, applyScrollDelta_withNegativeMultiplier_reversesDirection)
{
    Config::ScreenOptions options;
    options[kOptionMouseScrollDelta] = -1500; // -1.5 multiplier

    auto result = applyScrollDelta(100, 200, &options);
    EXPECT_EQ(-150, result.first);  // 100 * -1.5
    EXPECT_EQ(-300, result.second); // 200 * -1.5
}

TEST(ServerScrollDeltaTests, applyScrollDelta_withZeroMultiplier_returnsZero)
{
    Config::ScreenOptions options;
    options[kOptionMouseScrollDelta] = 0; // 0.0 multiplier

    auto result = applyScrollDelta(100, 200, &options);
    EXPECT_EQ(0, result.first);
    EXPECT_EQ(0, result.second);
}

TEST(ServerScrollDeltaTests, applyScrollDelta_withSmallMultiplier_handlesRounding)
{
    Config::ScreenOptions options;
    options[kOptionMouseScrollDelta] = 1; // 0.001 multiplier

    auto result = applyScrollDelta(100, 200, &options);
    EXPECT_EQ(0, result.first);   // 100 * 0.001 = 0.1 -> 0
    EXPECT_EQ(0, result.second);  // 200 * 0.001 = 0.2 -> 0

    // Test that 1000 * 0.001 = 1
    auto result2 = applyScrollDelta(1000, 2000, &options);
    EXPECT_EQ(1, result2.first);  // 1000 * 0.001 = 1
    EXPECT_EQ(2, result2.second); // 2000 * 0.001 = 2
}

TEST(ServerScrollDeltaTests, applyScrollDelta_withLargeMultiplier_handlesCorrectly)
{
    Config::ScreenOptions options;
    options[kOptionMouseScrollDelta] = 10000; // 10.0 multiplier

    auto result = applyScrollDelta(100, 200, &options);
    EXPECT_EQ(1000, result.first);  // 100 * 10.0
    EXPECT_EQ(2000, result.second); // 200 * 10.0
}

TEST(ServerScrollDeltaTests, applyScrollDelta_fractionalMultiplier_truncatesCorrectly)
{
    Config::ScreenOptions options;
    options[kOptionMouseScrollDelta] = 1333; // 1.333 multiplier

    auto result = applyScrollDelta(100, 200, &options);
    EXPECT_EQ(133, result.first);   // 100 * 1.333 = 133.3 -> 133
    EXPECT_EQ(266, result.second);  // 200 * 1.333 = 266.6 -> 266
}

TEST(ServerScrollDeltaTests, applyScrollDelta_negativeInput_handlesCorrectly)
{
    Config::ScreenOptions options;
    options[kOptionMouseScrollDelta] = 2000; // 2.0 multiplier

    auto result = applyScrollDelta(-100, -200, &options);
    EXPECT_EQ(-200, result.first);  // -100 * 2.0
    EXPECT_EQ(-400, result.second); // -200 * 2.0
}

TEST(ServerScrollDeltaTests, applyScrollDelta_mixedSigns_handlesCorrectly)
{
    Config::ScreenOptions options;
    options[kOptionMouseScrollDelta] = -1000; // -1.0 multiplier

    auto result = applyScrollDelta(100, -200, &options);
    EXPECT_EQ(-100, result.first);  // 100 * -1.0
    EXPECT_EQ(200, result.second);  // -200 * -1.0
}

} // namespace inputleap