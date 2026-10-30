#include "hft/data/mapped_file.hpp"

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace fs = std::filesystem;

class MappedFileTest : public ::testing::Test {
protected:
    const std::string sample_file = "./fixtures/sample_ticks.csv";
    const std::string empty_file = "./fixtures/empty.csv";
    const std::string missing_file = "./fixtures/missing.csv";
    const std::string sample_content = "timestamp,symbol,price,size\n1625097600,BTC,35000.5,1.2\n";

    // Setup creates the necessary fixture files before each test runs.
    void SetUp() override {
        fs::create_directories("./fixtures");

        {
            std::ofstream sample(sample_file, std::ios::binary);
            ASSERT_TRUE(sample.is_open());
            sample << sample_content;
            ASSERT_TRUE(sample.good());
        }

        {
            std::ofstream empty(empty_file, std::ios::binary);
            ASSERT_TRUE(empty.is_open());
        }

        fs::remove(missing_file);
    }

    // TearDown cleans up the fixture files after each test.
    void TearDown() override {
        fs::remove(sample_file);
        fs::remove(empty_file);
        fs::remove(missing_file);
        fs::remove("./fixtures");
    }
};

TEST_F(MappedFileTest, OpensValidNonEmptyFile) {
    hft::data::MappedFile file(sample_file.c_str());

    ASSERT_TRUE(file.is_valid());
    EXPECT_EQ(file.size(), sample_content.size());

    std::string_view mapped_data(static_cast<const char*>(file.data()), file.size());
    EXPECT_EQ(mapped_data, sample_content);
}

TEST_F(MappedFileTest, OpensEmptyFile) {
    hft::data::MappedFile file(empty_file.c_str());

    ASSERT_TRUE(file.is_valid());
    EXPECT_EQ(file.size(), 0);
}

TEST_F(MappedFileTest, RejectsMissingFile) {
    hft::data::MappedFile file(missing_file.c_str());
    EXPECT_FALSE(file.is_valid());
}

TEST_F(MappedFileTest, MoveConstructsCorrectly) {
    hft::data::MappedFile original(sample_file.c_str());
    ASSERT_TRUE(original.is_valid());

    hft::data::MappedFile moved(std::move(original));
    
    EXPECT_FALSE(original.is_valid());
    EXPECT_TRUE(moved.is_valid());
    EXPECT_EQ(moved.size(), sample_content.size());
}

TEST_F(MappedFileTest, MoveAssignsCorrectly) {
    hft::data::MappedFile file1(sample_file.c_str());
    hft::data::MappedFile file2(empty_file.c_str());

    ASSERT_TRUE(file1.is_valid());
    ASSERT_TRUE(file2.is_valid());

    file2 = std::move(file1);

    EXPECT_FALSE(file1.is_valid());
    EXPECT_TRUE(file2.is_valid());
    EXPECT_EQ(file2.size(), sample_content.size());
}