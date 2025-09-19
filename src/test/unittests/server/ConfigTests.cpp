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
#include "base/XBase.h"

#include <gtest/gtest.h>
#include <sstream>

namespace inputleap {

TEST(ConfigTests, parseMouseScrollDelta_validPositiveFloat_parsedCorrectly)
{
    Config config;
    std::stringstream ss;
    ss << "section: screens\n"
       << "\ttest:\n"
       << "\t\tmouseScrollDelta = 2.5\n"
       << "end\n";
    
    ss >> config;
    
    const Config::ScreenOptions* options = config.getOptions("test");
    ASSERT_NE(nullptr, options);
    
    auto it = options->find(kOptionMouseScrollDelta);
    ASSERT_NE(options->end(), it);
    EXPECT_EQ(2500, it->second); // 2.5 * 1000
}

TEST(ConfigTests, parseMouseScrollDelta_validNegativeFloat_parsedCorrectly)
{
    Config config;
    std::stringstream ss;
    ss << "section: screens\n"
       << "\ttest:\n"
       << "\t\tmouseScrollDelta = -1.5\n"
       << "end\n";
    
    ss >> config;
    
    const Config::ScreenOptions* options = config.getOptions("test");
    ASSERT_NE(nullptr, options);
    
    auto it = options->find(kOptionMouseScrollDelta);
    ASSERT_NE(options->end(), it);
    EXPECT_EQ(-1500, it->second); // -1.5 * 1000
}

TEST(ConfigTests, parseMouseScrollDelta_validInteger_parsedCorrectly)
{
    Config config;
    std::stringstream ss;
    ss << "section: screens\n"
       << "\ttest:\n"
       << "\t\tmouseScrollDelta = 3\n"
       << "end\n";
    
    ss >> config;
    
    const Config::ScreenOptions* options = config.getOptions("test");
    ASSERT_NE(nullptr, options);
    
    auto it = options->find(kOptionMouseScrollDelta);
    ASSERT_NE(options->end(), it);
    EXPECT_EQ(3000, it->second); // 3 * 1000
}

TEST(ConfigTests, parseMouseScrollDelta_zeroValue_parsedCorrectly)
{
    Config config;
    std::stringstream ss;
    ss << "section: screens\n"
       << "\ttest:\n"
       << "\t\tmouseScrollDelta = 0\n"
       << "end\n";
    
    ss >> config;
    
    const Config::ScreenOptions* options = config.getOptions("test");
    ASSERT_NE(nullptr, options);
    
    auto it = options->find(kOptionMouseScrollDelta);
    ASSERT_NE(options->end(), it);
    EXPECT_EQ(0, it->second);
}

TEST(ConfigTests, parseMouseScrollDelta_invalidString_throwsException)
{
    Config config;
    std::stringstream ss;
    ss << "section: screens\n"
       << "\ttest:\n"
       << "\t\tmouseScrollDelta = invalid\n"
       << "end\n";
    
    EXPECT_THROW(ss >> config, XBase);
}

TEST(ConfigTests, parseMouseScrollDelta_partiallyValidString_throwsException)
{
    Config config;
    std::stringstream ss;
    ss << "section: screens\n"
       << "\ttest:\n"
       << "\t\tmouseScrollDelta = 1.5invalid\n"
       << "end\n";
    
    EXPECT_THROW(ss >> config, XBase);
}

// Note: getOptionName and getOptionValue are private methods,
// tested indirectly through config serialization

TEST(ConfigTests, parseMouseScrollDelta_precisionTest_handlesDecimals)
{
    Config config;
    std::stringstream ss;
    ss << "section: screens\n"
       << "\ttest:\n"
       << "\t\tmouseScrollDelta = 1.234\n"
       << "end\n";

    ss >> config;

    const Config::ScreenOptions* options = config.getOptions("test");
    ASSERT_NE(nullptr, options);

    auto it = options->find(kOptionMouseScrollDelta);
    ASSERT_NE(options->end(), it);
    EXPECT_EQ(1234, it->second); // 1.234 * 1000
}

TEST(ConfigTests, parseMouseScrollDelta_verySmallValue_parsedCorrectly)
{
    Config config;
    std::stringstream ss;
    ss << "section: screens\n"
       << "\ttest:\n"
       << "\t\tmouseScrollDelta = 0.001\n"
       << "end\n";

    ss >> config;

    const Config::ScreenOptions* options = config.getOptions("test");
    ASSERT_NE(nullptr, options);

    auto it = options->find(kOptionMouseScrollDelta);
    ASSERT_NE(options->end(), it);
    EXPECT_EQ(1, it->second); // 0.001 * 1000
}

TEST(ConfigTests, parseMouseScrollDelta_largeValue_parsedCorrectly)
{
    Config config;
    std::stringstream ss;
    ss << "section: screens\n"
       << "\ttest:\n"
       << "\t\tmouseScrollDelta = 100.0\n"
       << "end\n";

    ss >> config;

    const Config::ScreenOptions* options = config.getOptions("test");
    ASSERT_NE(nullptr, options);

    auto it = options->find(kOptionMouseScrollDelta);
    ASSERT_NE(options->end(), it);
    EXPECT_EQ(100000, it->second); // 100.0 * 1000
}

TEST(ConfigTests, parseMouseScrollDelta_scientificNotation_parsedCorrectly)
{
    Config config;
    std::stringstream ss;
    ss << "section: screens\n"
       << "\ttest:\n"
       << "\t\tmouseScrollDelta = 1.5e1\n"
       << "end\n";

    ss >> config;

    const Config::ScreenOptions* options = config.getOptions("test");
    ASSERT_NE(nullptr, options);

    auto it = options->find(kOptionMouseScrollDelta);
    ASSERT_NE(options->end(), it);
    EXPECT_EQ(15000, it->second); // 15.0 * 1000
}

TEST(ConfigTests, parseMouseScrollDelta_multipleScreens_eachParsedIndependently)
{
    Config config;
    std::stringstream ss;
    ss << "section: screens\n"
       << "\tscreen1:\n"
       << "\t\tmouseScrollDelta = 2.0\n"
       << "\tscreen2:\n"
       << "\t\tmouseScrollDelta = 0.5\n"
       << "end\n";

    ss >> config;

    const Config::ScreenOptions* options1 = config.getOptions("screen1");
    ASSERT_NE(nullptr, options1);
    auto it1 = options1->find(kOptionMouseScrollDelta);
    ASSERT_NE(options1->end(), it1);
    EXPECT_EQ(2000, it1->second);

    const Config::ScreenOptions* options2 = config.getOptions("screen2");
    ASSERT_NE(nullptr, options2);
    auto it2 = options2->find(kOptionMouseScrollDelta);
    ASSERT_NE(options2->end(), it2);
    EXPECT_EQ(500, it2->second);
}

TEST(ConfigTests, getOptions_nonExistentScreen_returnsNull)
{
    Config config;
    std::stringstream ss;
    ss << "section: screens\n"
       << "\ttest:\n"
       << "\t\tmouseScrollDelta = 1.0\n"
       << "end\n";

    ss >> config;

    const Config::ScreenOptions* options = config.getOptions("nonexistent");
    EXPECT_EQ(nullptr, options);
}

} // namespace inputleap