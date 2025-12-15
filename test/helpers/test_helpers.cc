#include "test_helpers.hh"
#include <fstream>
#include <cmath>
#include <TFile.h>

namespace TestHelpers {

bool FileExists(const std::string& path) {
    std::ifstream f(path.c_str());
    return f.good();
}

std::string GetTestFixturePath(const std::string& filename) {
    return "fixtures/" + filename;
}

bool AlmostEqual(double a, double b, double epsilon) {
    return std::fabs(a - b) < epsilon;
}

bool IsValidROOTFile(const std::string& filepath) {
    TFile* file = TFile::Open(filepath.c_str(), "READ");
    if (!file || file->IsZombie()) {
        if (file) delete file;
        return false;
    }
    bool valid = !file->IsZombie();
    file->Close();
    delete file;
    return valid;
}

bool IsValidJSON(const std::string& filepath) {
    std::ifstream f(filepath);
    if (!f.good()) return false;

    // Simple check: file should contain at least '{' and '}'
    std::string content((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
    return (content.find('{') != std::string::npos &&
            content.find('}') != std::string::npos);
}

void CleanupTestFiles(const std::string& pattern) {
    // Basic cleanup - can be expanded with glob patterns
    // For now, just a placeholder
    // In real implementation, would use filesystem library to match patterns
}

}  // namespace TestHelpers
