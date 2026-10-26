#pragma once

#include "common.h"

struct BandizipInstall {
    bool found = false;
    std::wstring exe;
    std::wstring bc;
    std::wstring dir;
    std::wstring langDir;
    std::wstring icon;
};

struct Settings {
    bool extractHere = true;
    bool extractSmart = true;
    bool extractToFolder = true;
    bool extractDialog = true;
    bool openArchive = true;
    bool testArchive = true;
    bool compressZip = true;
    bool compressEach = true;
    bool compressDialog = true;
    bool autoClose = true;
    std::wstring compressionFormat = L"zip";
};

struct LocStrings {
    std::wstring root = L"Bandizip";
    std::wstring extractHere = L"Extract Here";
    std::wstring extractSmart = L"Extract Here (Smart)";
    std::wstring extractToFolder = L"Extract to \"${str1}\\\"";
    std::wstring extractDialog = L"Extract to...";
    std::wstring openArchive = L"Open with Bandizip...";
    std::wstring testArchive = L"Test Archive";
    std::wstring compressZip = L"Compress to \"${str1}\"";
    std::wstring compressEach = L"Compress each to separate archives";
    std::wstring compressDialog = L"Create archive (Bandizip)...";
};

struct Selection {
    bool valid = false;
    DWORD count = 0;
    bool single = false;
    bool allArchives = false;
    bool allCompressible = false;
    bool containsDirectory = false;
    std::wstring parent;
    std::vector<std::wstring> paths;
};

const BandizipInstall& GetBandizip();
const Settings& GetSettings();
const LocStrings& GetLoc();

bool AnalyzeSelection(IShellItemArray* items, Selection& selection);
bool IsArchivePath(const std::wstring& path);
std::wstring ArchiveBaseName(const std::wstring& path);
std::wstring CompressBaseName(const Selection& selection);
std::wstring MakeUniqueArchivePath(const std::wstring& directory,
                                   const std::wstring& baseName,
                                   const std::wstring& extension);
std::wstring CleanMenuLabel(std::wstring value);
