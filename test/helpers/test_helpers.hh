#ifndef TEST_HELPERS_HH
#define TEST_HELPERS_HH

#include <string>

namespace TestHelpers {

// File system helpers
bool FileExists(const std::string& path);
std::string GetTestFixturePath(const std::string& filename);

// Floating point comparison helpers
bool AlmostEqual(double a, double b, double epsilon = 1e-9);

// ROOT file helpers
bool IsValidROOTFile(const std::string& filepath);

// Configuration validation helpers
bool IsValidJSON(const std::string& filepath);

// Cleanup helpers
void CleanupTestFiles(const std::string& pattern);

}  // namespace TestHelpers

#endif  // TEST_HELPERS_HH
