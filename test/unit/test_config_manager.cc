// Unit tests for ConfigManager
// Following TDD approach: Write failing tests first, then implement fixes

#include <gtest/gtest.h>
#include "ConfigManager.hh"
#include <stdexcept>
#include <fstream>

// Test fixture for ConfigManager tests
class ConfigManagerTest : public ::testing::Test {
protected:
    ConfigManager* config;

    void SetUp() override {
        config = ConfigManager::GetInstance();
        // Reset singleton state for test isolation
        config->Reset();
    }

    void TearDown() override {
        // Clean state is maintained via Reset() in SetUp()
    }

    // Helper to check if file exists
    bool FileExists(const std::string& path) {
        std::ifstream f(path.c_str());
        return f.good();
    }
};

// ============================================================================
// Detector Configuration Loading Tests
// ============================================================================

TEST_F(ConfigManagerTest, LoadValidDetectorFile_Success) {
    // RED phase: This test will fail initially
    ASSERT_TRUE(FileExists("fixtures/test_detector.json"))
        << "Test fixture file not found";

    ASSERT_NO_THROW(config->LoadDetectorFile("fixtures/test_detector.json"));

    EXPECT_TRUE(config->IsDetectorLoaded());
    EXPECT_EQ(config->GetNumDetectorConfigs(), 1);
    EXPECT_TRUE(config->HasDetectorType("TestDetector_Standard"));
}

TEST_F(ConfigManagerTest, LoadDetectorFile_CheckConfigValues) {
    config->LoadDetectorFile("fixtures/test_detector.json");

    const DetectorConfig& det = config->GetDetectorConfig("TestDetector_Standard");

    EXPECT_EQ(det.name, "TestDetector_Standard");
    EXPECT_DOUBLE_EQ(det.diameter, 25.4);
    EXPECT_DOUBLE_EQ(det.length, 1000.0);
    EXPECT_DOUBLE_EQ(det.wallT, 0.8);
    EXPECT_DOUBLE_EQ(det.pressure, 405.3);
}

TEST_F(ConfigManagerTest, LoadNonexistentFile_ThrowsException) {
    EXPECT_THROW(
        config->LoadDetectorFile("nonexistent_file.json"),
        std::runtime_error
    );
}

TEST_F(ConfigManagerTest, LoadEmptyJSON_ThrowsException) {
    EXPECT_THROW(
        config->LoadDetectorFile("fixtures/invalid/empty.json"),
        std::runtime_error
    ) << "Empty JSON should throw exception";
}

TEST_F(ConfigManagerTest, GetNonexistentDetectorType_ThrowsException) {
    config->LoadDetectorFile("fixtures/test_detector.json");

    EXPECT_THROW(
        config->GetDetectorConfig("NonExistentType"),
        std::runtime_error
    );
}

// ============================================================================
// Geometry Configuration Loading Tests
// ============================================================================

TEST_F(ConfigManagerTest, LoadValidGeometryFile_Success) {
    // Load files in any order - both should succeed
    config->LoadGeometryFile("fixtures/test_geometry.json");
    config->LoadDetectorFile("fixtures/test_detector.json");

    EXPECT_TRUE(config->IsGeometryLoaded());
    EXPECT_EQ(config->GetNumPlacements(), 2);

    // Validation should succeed
    EXPECT_NO_THROW(config->ValidateConfiguration());
}

TEST_F(ConfigManagerTest, LoadGeometry_CheckPlacementValues) {
    config->LoadDetectorFile("fixtures/test_detector.json");
    config->LoadGeometryFile("fixtures/test_geometry.json");

    const DetectorPlacement& det0 = config->GetPlacement(0);
    EXPECT_EQ(det0.type, "TestDetector_Standard");
    EXPECT_DOUBLE_EQ(det0.R, 0.0);  // Calculated from x,y
    EXPECT_DOUBLE_EQ(det0.Phi, 0.0);

    const DetectorPlacement& det1 = config->GetPlacement(1);
    EXPECT_EQ(det1.type, "TestDetector_Standard");
    EXPECT_DOUBLE_EQ(det1.R, 100.0);  // x=100, y=0

    // Validation should succeed
    EXPECT_NO_THROW(config->ValidateConfiguration());
}

TEST_F(ConfigManagerTest, LoadGeometryWithoutDetectorConfig_ThrowsException) {
    // Load geometry without detector config (should succeed during loading)
    EXPECT_NO_THROW(
        config->LoadGeometryFile("fixtures/test_geometry.json")
    ) << "Loading geometry without detector should succeed";

    // But validation should fail after all files are loaded
    EXPECT_THROW(
        config->ValidateConfiguration(),
        std::runtime_error
    ) << "ValidateConfiguration() should require detector config when geometry is loaded";
}

TEST_F(ConfigManagerTest, LoadGeometryWithMissingDetector_ThrowsException) {
    // Load files in any order (both should succeed)
    config->LoadGeometryFile("fixtures/invalid/missing_detectors.json");
    config->LoadDetectorFile("fixtures/test_detector.json");

    // But validation should fail because geometry references non-existent detector
    EXPECT_THROW(
        config->ValidateConfiguration(),
        std::runtime_error
    ) << "ValidateConfiguration() should throw when geometry references non-existent detector";
}

// ============================================================================
// Source File Loading Tests
// ============================================================================

TEST_F(ConfigManagerTest, LoadValidSourceFile_Success) {
    ASSERT_TRUE(FileExists("fixtures/test_source.root"))
        << "Source file not found. Run: cd fixtures && root -l -q create_test_source.C";

    ASSERT_NO_THROW(config->LoadSourceFile("fixtures/test_source.root"));

    EXPECT_TRUE(config->IsSourceLoaded());

    // Check that either histogram or function is loaded (not both)
    bool hasHist = (config->GetSourceHistogram() != nullptr);
    bool hasFunc = (config->GetSourceFunction() != nullptr);
    EXPECT_TRUE(hasHist ^ hasFunc) << "Should have exactly one source type";
}

TEST_F(ConfigManagerTest, LoadNonexistentSourceFile_ThrowsException) {
    EXPECT_THROW(
        config->LoadSourceFile("nonexistent.root"),
        std::runtime_error
    );
}

// ============================================================================
// State Validation Tests
// ============================================================================

TEST_F(ConfigManagerTest, InitialState_NothingLoaded) {
    EXPECT_FALSE(config->IsDetectorLoaded());
    EXPECT_FALSE(config->IsGeometryLoaded());
    EXPECT_FALSE(config->IsSourceLoaded());

    EXPECT_EQ(config->GetNumDetectorConfigs(), 0);
    EXPECT_EQ(config->GetNumPlacements(), 0);
    EXPECT_EQ(config->GetSourceHistogram(), nullptr);
    EXPECT_EQ(config->GetSourceFunction(), nullptr);
}

TEST_F(ConfigManagerTest, SingletonPattern_ReturnsSameInstance) {
    ConfigManager* instance1 = ConfigManager::GetInstance();
    ConfigManager* instance2 = ConfigManager::GetInstance();

    EXPECT_EQ(instance1, instance2) << "Singleton should return same instance";
}

// ============================================================================
// Box Geometry Tests
// ============================================================================

TEST_F(ConfigManagerTest, BoxGeometry_DefaultValues) {
    // Box size should be set when geometry is loaded
    EXPECT_DOUBLE_EQ(config->GetBoxX(), 0.0);
    EXPECT_DOUBLE_EQ(config->GetBoxY(), 0.0);
    EXPECT_DOUBLE_EQ(config->GetBoxZ(), 0.0);
}

// ============================================================================
// PrintConfiguration Tests (smoke test)
// ============================================================================

TEST_F(ConfigManagerTest, PrintConfiguration_DoesNotCrash) {
    config->LoadDetectorFile("fixtures/test_detector.json");
    config->LoadGeometryFile("fixtures/test_geometry.json");

    // Should not crash
    ASSERT_NO_THROW(config->PrintConfiguration());
}
