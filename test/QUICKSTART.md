# Testing Quick Start Guide

**Status:** ✅ All tests passing (15/15)

## Run Tests in 30 Seconds

```bash
# From NBox root directory
cd test
mkdir build && cd build
cmake ..
cmake --build . -j$(sysctl -n hw.ncpu)
./nbox_tests
```

**Expected output:**
```
[==========] 15 tests from 1 test suite ran. (140 ms total)
[  PASSED  ] 15 tests.
```

## What Gets Tested?

✅ **ConfigManager** - 15 unit tests covering:
- Detector configuration loading
- Geometry configuration loading
- Source file loading (ROOT TH1/TF1)
- Error handling (invalid files, missing data)
- Singleton pattern
- State management (Reset() functionality)
- Pre-condition validation

## Test Coverage

- **15/15 tests passing** (100%)
- **100% coverage** of ConfigManager public API
- **~140ms** execution time
- **2 bugs** discovered and fixed through TDD

## Common Commands

```bash
# Run all tests
./nbox_tests

# Run specific test suite
./nbox_tests --gtest_filter=ConfigManagerTest.*

# Run specific test
./nbox_tests --gtest_filter=*LoadValidDetectorFile*

# List all tests
./nbox_tests --gtest_list_tests

# Run with verbose output
./nbox_tests --gtest_verbose

# Run via CTest
ctest
ctest -V  # verbose
ctest --output-on-failure
```

## Test Results Summary

```
Test Project: NBox
Test Suite: ConfigManagerTest
Total Tests: 15
Status: PASSED

Execution Time: ~140 milliseconds
Build Time: ~5 seconds (incremental)

Test Breakdown:
  ✅ Detector loading: 5 tests
  ✅ Geometry loading: 4 tests
  ✅ Source loading: 2 tests
  ✅ State management: 3 tests
  ✅ Utility: 1 test
```

## For Developers

### Adding New Tests

1. Create test file in `test/unit/` or `test/integration/`
2. Follow naming convention: `test_<class_name>.cc`
3. Use Google Test framework
4. Rebuild and run

Example:
```cpp
#include <gtest/gtest.h>
#include "MyClass.hh"

TEST(MyClassTest, DoSomething_ValidInput_Success) {
    MyClass obj;
    EXPECT_TRUE(obj.DoSomething());
}
```

### TDD Workflow

**RED** → Write failing test first
```cpp
TEST(NewFeature, ShouldWork) {
    EXPECT_TRUE(false);  // Will fail
}
```

**GREEN** → Make it pass
```cpp
// Implement minimum code to pass test
```

**REFACTOR** → Improve code while keeping tests green

### CI/CD Integration

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

## Troubleshooting

### Tests Fail After Code Changes
**Expected!** Tests are designed to catch regressions.
1. Review failing test output
2. Fix the code or update the test
3. Re-run tests

### Google Test Not Found
**No problem!** CMake auto-downloads Google Test via FetchContent.
Just run cmake and it will download automatically.

### Fixtures Not Found
```bash
# From test/build directory
cp -r ../fixtures .
```

Or rebuild - CMake copies fixtures automatically.

## Documentation

- [test/README.md](README.md) - Full testing guide
- [test/TDD_RESULTS.md](TDD_RESULTS.md) - Complete TDD implementation results
- [test/CHANGES.md](CHANGES.md) - Code changes from TDD
- [TODO/TESTING_STRATEGY.md](../TODO/TESTING_STRATEGY.md) - Long-term testing roadmap

## Key Benefits

1. ✅ **Fast feedback** - Tests run in 140ms
2. ✅ **Catch bugs early** - Found 2 bugs before production
3. ✅ **Safe refactoring** - Tests prevent regressions
4. ✅ **Documentation** - Tests show how to use API
5. ✅ **No dependencies** - Separate from main build

## Next Steps

### Short Term
- Add more unit tests for other classes
- Write integration tests for full simulation
- Add regression tests for output validation

### Long Term
- Achieve 80%+ overall code coverage
- Set up CI/CD automated testing
- Add performance benchmarks

---

**Questions?** See [test/README.md](README.md) for comprehensive guide.

**Found a bug?** Write a failing test first, then fix it! (TDD approach)
