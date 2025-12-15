# NBox Testing Suite

This directory contains the **separate testing framework** for NBox. Tests are **optional** and not required for normal users.

## Why Separate?

- ✅ Main NBox build remains simple (no Google Test dependency)
- ✅ Users building NBox don't need testing infrastructure
- ✅ Developers can easily run tests
- ✅ CI/CD can build tests independently

## Quick Start

### Prerequisites

Same as main NBox:
- Geant4 11.0+
- ROOT 6.x+
- CMake 3.16+
- C++11 compiler

**Note:** Google Test is automatically downloaded by CMake (no manual installation needed)

### Build Tests

```bash
# From NBox root directory
cd test
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
```

### Run Tests

```bash
# Run all tests
./nbox_tests

# Run specific test suite
./nbox_tests --gtest_filter=ConfigManagerTest.*

# Run with verbose output
./nbox_tests --gtest_verbose

# List all tests
./nbox_tests --gtest_list_tests
```

### Using CTest

```bash
# Run all tests via CTest
ctest

# Verbose output
ctest -V

# Parallel execution
ctest -j$(nproc)
```

## Directory Structure

```
test/
├── CMakeLists.txt           # Test build system (separate from main)
├── README.md                # This file
├── unit/                    # Unit tests
│   ├── test_config_manager.cc
│   ├── test_validation.cc
│   └── ...
├── integration/             # Integration tests
│   ├── test_geometry_construction.cc
│   └── ...
├── fixtures/                # Test data files
│   ├── test_detector.json
│   ├── test_geometry.json
│   ├── test_source.root
│   └── invalid/             # Invalid configs for error testing
└── helpers/                 # Test utilities
    ├── test_helpers.hh
    └── test_helpers.cc
```

## Writing Tests

### Example Unit Test

```cpp
// test/unit/test_config_manager.cc
#include <gtest/gtest.h>
#include "ConfigManager.hh"

TEST(ConfigManagerTest, LoadValidDetectorFile) {
    ConfigManager* config = ConfigManager::GetInstance();
    ASSERT_NO_THROW(
        config->LoadDetectorFile("fixtures/test_detector.json")
    );
    EXPECT_GT(config->GetNumDetectorConfigs(), 0);
}

TEST(ConfigManagerTest, LoadInvalidFileThrows) {
    ConfigManager* config = ConfigManager::GetInstance();
    EXPECT_THROW(
        config->LoadDetectorFile("nonexistent.json"),
        std::runtime_error
    );
}
```

### Test Naming Convention

- **Test Suite:** `ClassNameTest`
- **Test Case:** `MethodName_Scenario_ExpectedResult`
- **Example:** `ConfigManager_LoadInvalidFile_ThrowsException`

## Test Coverage

To generate coverage report (requires lcov):

```bash
# Build with coverage
cd test/build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCOVERAGE=ON
cmake --build .
ctest

# Generate report
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
open coverage_html/index.html
```

## CI/CD Integration

Tests run automatically on GitHub Actions:

```yaml
# .github/workflows/test.yml
- name: Build Tests
  run: |
    cd test
    mkdir build && cd build
    cmake ..
    cmake --build .

- name: Run Tests
  run: |
    cd test/build
    ctest --output-on-failure
```

## Test Fixtures

Test data files are in `fixtures/`:

### Valid Configuration Files

- `test_detector.json` - Single detector configuration
- `test_geometry.json` - Simple 2-detector geometry
- `test_source.root` - Thermal neutron source (TH1)
- `test_source_tf1.root` - Cf-252 source (TF1)

### Invalid Configuration Files (for error testing)

- `invalid/missing_detectors.json` - Missing "detectors" array
- `invalid/multiple_sources.root` - Both TH1 and TF1 (should fail)
- `invalid/empty.json` - Empty file

## TDD Workflow

1. **Write failing test** (Red)
   ```cpp
   TEST(NewFeature, DoesNotExistYet) {
       EXPECT_TRUE(false);  // This will fail
   }
   ```

2. **Make it pass** (Green)
   ```cpp
   // Implement minimum code to pass
   ```

3. **Refactor** (Refactor)
   ```cpp
   // Improve code while keeping tests green
   ```

## FAQs

### Q: Do I need to install Google Test?
**A:** No. CMake automatically downloads it via FetchContent.

### Q: Why is testing separate from main build?
**A:** To keep the main NBox build simple for users who don't need tests.

### Q: How do I run tests in CI/CD?
**A:** See `.github/workflows/test.yml` for GitHub Actions example.

### Q: Can I build both main and tests together?
**A:** Yes, but they're intentionally separate. Build them independently.

### Q: Tests are failing, what should I do?
**A:** Run with `-V` flag: `ctest -V` to see detailed output.

## Contributing Tests

When adding new features:

1. Write tests **first** (TDD)
2. Add tests to appropriate directory (`unit/` or `integration/`)
3. Run tests locally before pushing
4. Ensure all tests pass in CI/CD

## Resources

- **Google Test Docs:** https://google.github.io/googletest/
- **CMake Testing:** https://cmake.org/cmake/help/latest/manual/ctest.1.html
- **TDD Guide:** See `TODO/TESTING_STRATEGY.md`
