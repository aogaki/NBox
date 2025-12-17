// This is a simple macro read files and generate energy distribution
#include <TFile.h>
#include <TH1.h>
#include <TString.h>
#include <TTree.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

std::vector<std::string> GetFileList(std::string dir = "./")
{
  // Get file list of output_*_t*.root
  std::vector<std::string> file_list;
  for (const auto &entry : std::filesystem::directory_iterator(dir)) {
    const auto &path = entry.path();
    if (path.extension() == ".root" &&
        path.filename().string().find("output_") != std::string::npos &&
        path.filename().string().find("_t") != std::string::npos) {
      file_list.push_back(path.string());
    }
  }
  return file_list;
}

// Build DetectorID -> DetectorName mapping from ROOT files
// Returns a vector where index = DetectorID and value = DetectorName
// This guarantees correct mapping regardless of JSON file order
std::vector<std::string> GetDetectorNamesByID(
    const std::vector<std::string> &fileList)
{
  std::map<Int_t, std::string> idToName;

  // Scan all files to build ID->Name mapping
  for (const auto &fileName : fileList) {
    TFile *file = TFile::Open(fileName.c_str());
    if (!file || file->IsZombie()) {
      std::cerr << "Warning: Cannot open file: " << fileName << std::endl;
      continue;
    }

    TTree *hits = (TTree *)file->Get("NBox");
    if (!hits) {
      std::cerr << "Warning: Cannot find 'NBox' tree in file: " << fileName
                << std::endl;
      file->Close();
      delete file;
      continue;
    }
    hits->SetBranchStatus("*", kFALSE);

    // Read both DetectorID and DetectorName
    hits->SetBranchStatus("DetectorID", kTRUE);
    hits->SetBranchStatus("DetectorName", kTRUE);

    Int_t detID;
    char detName[256];
    hits->SetBranchAddress("DetectorID", &detID);
    hits->SetBranchAddress("DetectorName", detName);

    Long64_t nEntries = hits->GetEntries();
    for (Long64_t i = 0; i < nEntries; i++) {
      hits->GetEntry(i);
      idToName[detID] = std::string(detName);
    }

    file->Close();
    delete file;
  }

  // Convert map to vector: index = DetectorID, value = DetectorName
  // Find max ID to size the vector
  Int_t maxID = -1;
  for (const auto &[id, name] : idToName) {
    if (id > maxID) maxID = id;
  }

  std::vector<std::string> detectorNames(maxID + 1);
  for (const auto &[id, name] : idToName) {
    detectorNames[id] = name;
  }

  return detectorNames;
}

std::vector<TH1D *> hists;
void InitHists(const std::vector<std::string> &detList)
{
  // Create histograms indexed by DetectorID
  // detList[i] = name of detector with ID=i
  for (size_t i = 0; i < detList.size(); i++) {
    const auto &detName = detList[i];
    if (detName.empty()) {
      // Skip if this ID has no detector (shouldn't happen)
      hists.push_back(nullptr);
      continue;
    }

    auto histName = "hist" + detName;
    auto histTitle = "Energy distribution: " + detName;
    TH1D *hist =
        new TH1D(histName.c_str(), histTitle.c_str(), 10000, 0.0, 10000.0);
    hists.push_back(hist);
  }
}

void FillHistograms(const std::vector<std::string> &fileList)
{
  std::cout << "\nFilling histograms from " << fileList.size() << " files..."
            << std::endl;

  Long64_t totalEntries = 0;

  for (const auto &fileName : fileList) {
    TFile *file = TFile::Open(fileName.c_str());
    if (!file || file->IsZombie()) {
      std::cerr << "Warning: Cannot open file: " << fileName << std::endl;
      continue;
    }

    TTree *tree = (TTree *)file->Get("NBox");
    if (!tree) {
      std::cerr << "Warning: Cannot find 'NBox' tree in file: " << fileName
                << std::endl;
      file->Close();
      delete file;
      continue;
    }

    // Optimize: only read branches we need
    tree->SetBranchStatus("*", kFALSE);
    tree->SetBranchStatus("DetectorID", kTRUE);
    tree->SetBranchStatus("Edep_keV", kTRUE);

    Int_t detID;
    Double_t edep_keV;
    tree->SetBranchAddress("DetectorID", &detID);
    tree->SetBranchAddress("Edep_keV", &edep_keV);

    Long64_t nEntries = tree->GetEntries();
    totalEntries += nEntries;

    for (Long64_t i = 0; i < nEntries; i++) {
      tree->GetEntry(i);

      // Fill histogram for this detector
      if (detID >= 0 && detID < (Int_t)hists.size() && hists[detID]) {
        hists[detID]->Fill(edep_keV);
      }
    }

    file->Close();
    delete file;
  }

  std::cout << "  Processed " << totalEntries << " entries total" << std::endl;

  // Print statistics
  std::cout << "\nHistogram statistics:" << std::endl;
  for (size_t i = 0; i < hists.size(); i++) {
    if (hists[i] && hists[i]->GetEntries() > 0) {
      std::cout << "  " << hists[i]->GetName() << ": " << hists[i]->GetEntries()
                << " entries, mean = " << hists[i]->GetMean() << " keV"
                << std::endl;
    }
  }
}

void simple_reader()
{
  auto fileList = GetFileList();

  std::cout << "Found " << fileList.size() << " ROOT files:" << std::endl;
  for (const auto &fileName : fileList) {
    std::cout << "  " << fileName << std::endl;
  }
  std::cout << std::endl;

  // Get detector names indexed by DetectorID
  // detectorNames[id] = name of detector with DetectorID=id
  auto detectorNames = GetDetectorNamesByID(fileList);

  std::cout << "Found " << detectorNames.size() << " detectors:" << std::endl;
  for (size_t i = 0; i < detectorNames.size(); i++) {
    if (!detectorNames[i].empty()) {
      std::cout << "  ID " << i << " -> " << detectorNames[i] << std::endl;
    }
  }

  InitHists(detectorNames);

  std::cout << "\nHistograms created:" << std::endl;
  std::cout << "  hists[detectorID] contains histogram for that detector"
            << std::endl;
  std::cout << "  Example: hists[0] is for detector '" << detectorNames[0]
            << "'" << std::endl;

  // Fill histograms from all files
  FillHistograms(fileList);

  // Write out
  auto outputFile = new TFile("energy.root", "RECREATE");
  for (auto &&hist : hists) {
    hist->Write();
  }
  outputFile->Close();
}