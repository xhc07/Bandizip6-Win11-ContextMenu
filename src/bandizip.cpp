#include "bandizip.h"

#include <cctype>
#include <cwctype>

namespace {

std::wstring ReadRegistryString(HKEY root, const wchar_t* subkey, const wchar_t* value) {
    wchar_t buffer[32768] = {};
    DWORD size = sizeof(buffer);
    DWORD type = 0;
    if (RegGetValueW(root, subkey, value, RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ, &type, buffer,
                     &size) != ERROR_SUCCESS) {
        return std::wstring();
    }

    std::wstring result(buffer);
    if (result.size() >= 2 && result.front() == L'"' && result.back() == L'"') {
        result = result.substr(1, result.size() - 2);
    }

    wchar_t expanded[32768] = {};
    DWORD expandedLength =
        ExpandEnvironmentStringsW(result.c_str(), expanded, (DWORD)(sizeof(expanded) / sizeof(wchar_t)));
    if (expandedLength > 0 && expandedLength < sizeof(expanded) / sizeof(wchar_t)) {
        result.assign(expanded);
    }
    return result;
}

DWORD ReadRegistryDword(HKEY root, const wchar_t* subkey, const wchar_t* value, DWORD fallback) {
    DWORD data = 0;
    DWORD size = sizeof(data);
    DWORD type = 0;
    if (RegGetValueW(root, subkey, value, RRF_RT_REG_DWORD, &type, &data, &size) == ERROR_SUCCESS) {
        return data;
    }

    std::wstring text = ReadRegistryString(root, subkey, value);
    if (!text.empty()) {
        wchar_t* end = nullptr;
        unsigned long parsed = wcstoul(text.c_str(), &end, 10);
        if (end && *end == L'\0') return (DWORD)parsed;
    }
    return fallback;
}

class IniFile {
public:
    bool Load(const std::wstring& path) {
        std::wstring text = ReadFileText(path);
        if (text.empty()) return false;

        size_t start = 0;
        while (start <= text.size()) {
            size_t end = text.find(L'\n', start);
            if (end == std::wstring::npos) end = text.size();
            std::wstring line = text.substr(start, end - start);
            if (!line.empty() && line.back() == L'\r') line.pop_back();
            line = Trim(line);
            start = end + 1;

            if (line.empty() || line[0] == L';' || line[0] == L'#' || line[0] == L'[') continue;

            size_t equals = line.find(L'=');
            if (equals == std::wstring::npos) continue;

            std::wstring key = Trim(line.substr(0, equals));
            std::wstring value = Trim(line.substr(equals + 1));
            if (key.empty()) continue;
            if (value.size() >= 2 && value.front() == L'"' && value.back() == L'"') {
                value = value.substr(1, value.size() - 2);
            }
            values_[ToUpper(key)] = value;
        }
        return true;
    }

    bool Has(const std::wstring& key) const {
        return values_.find(ToUpper(key)) != values_.end();
    }

    std::wstring Get(const std::wstring& key, const std::wstring& fallback = std::wstring()) const {
        auto it = values_.find(ToUpper(key));
        if (it == values_.end() || it->second.empty()) return fallback;
        return it->second;
    }

private:
    static std::wstring ToUpper(std::wstring value) {
        if (!value.empty()) CharUpperBuffW(&value[0], (DWORD)value.size());
        return value;
    }

    std::map<std::wstring, std::wstring> values_;
};

bool ConfigBool(const IniFile& ini, const wchar_t* key, bool fallback) {
    if (!ini.Has(key)) return fallback;
    std::wstring value = ToLower(Trim(ini.Get(key)));
    return value == L"1" || value == L"true" || value == L"yes" || value == L"on" || value == L"enabled";
}

std::wstring DetectLanguageFile(const BandizipInstall& install) {
    std::wstring configured = ReadRegistryString(HKEY_CURRENT_USER, L"Software\\Bandizip", L"language");
    if (!configured.empty()) {
        if (!EndsWithNoCase(configured, L".ini")) configured += L".ini";
        if (FileExists(install.langDir + L"\\" + configured)) return configured;
    }

    LANGID language = GetUserDefaultUILanguage();
    WORD primary = PRIMARYLANGID(language);
    WORD sub = SUBLANGID(language);

    switch (primary) {
        case LANG_CHINESE:
            if (sub == 1 || sub == 3 || sub == 5) return L"TradChinese.ini";
            return L"SimpChinese.ini";
        case LANG_JAPANESE:
            return L"Japanese.ini";
        case LANG_KOREAN:
            return L"Korean.ini";
        case LANG_GERMAN:
            return L"German.ini";
        case LANG_FRENCH:
            return L"French.ini";
        case LANG_SPANISH:
            return L"Spanish.ini";
        case LANG_ITALIAN:
            return L"Italian.ini";
        case LANG_RUSSIAN:
            return L"Russian.ini";
        case LANG_PORTUGUESE:
            return sub == 2 ? L"PortugueseBR.ini" : L"Portuguese.ini";
        case LANG_DUTCH:
            return L"Dutch.ini";
        case LANG_POLISH:
            return L"Polish.ini";
        case LANG_TURKISH:
            return L"Turkish.ini";
        case LANG_CZECH:
            return L"Czech.ini";
        case LANG_HUNGARIAN:
            return L"Hungarian.ini";
        case LANG_SWEDISH:
            return L"Swedish.ini";
        case LANG_DANISH:
            return L"Danish.ini";
        case LANG_FINNISH:
            return L"Finnish.ini";
        case LANG_NORWEGIAN:
            return L"Norwegian.ini";
        case LANG_UKRAINIAN:
            return L"Ukrainian.ini";
        case LANG_ROMANIAN:
            return L"Romanian.ini";
        case LANG_GREEK:
            return L"Greek.ini";
        case LANG_BULGARIAN:
            return L"Bulgarian.ini";
        case LANG_CROATIAN:
            return (sub == 2 || sub == 3) ? L"Serbian.ini" : L"Croatian.ini";
        case LANG_SLOVAK:
            return L"Slovak.ini";
        case LANG_SLOVENIAN:
            return L"Slovenian.ini";
        case LANG_VIETNAMESE:
            return L"Vietnamese.ini";
        case LANG_THAI:
            return L"Thai.ini";
        case LANG_INDONESIAN:
            return L"Indonesian.ini";
        default:
            break;
    }
    return L"English.ini";
}

BandizipInstall DetectBandizip() {
    BandizipInstall install;
    DebugLog(L"[detect] start");

    std::vector<std::wstring> candidates;
    const wchar_t* appPathKeys[] = {
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\Bandizip.exe",
        L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\App Paths\\Bandizip.exe",
    };
    for (const wchar_t* key : appPathKeys) {
        std::wstring path = ReadRegistryString(HKEY_LOCAL_MACHINE, key, nullptr);
        if (path.empty()) path = ReadRegistryString(HKEY_CURRENT_USER, key, nullptr);
        if (!path.empty()) candidates.push_back(path);
    }

    std::wstring programFiles = GetEnvVar(L"ProgramFiles");
    if (!programFiles.empty()) candidates.push_back(programFiles + L"\\Bandizip\\Bandizip.exe");
    std::wstring programFilesX86 = GetEnvVar(L"ProgramFiles(x86)");
    if (!programFilesX86.empty()) candidates.push_back(programFilesX86 + L"\\Bandizip\\Bandizip.exe");

    for (const auto& candidate : candidates) {
        DebugLog(L"[detect] candidate: " + candidate + (FileExists(candidate) ? L" [exists]" : L" [missing]"));
        if (!FileExists(candidate)) continue;
        std::wstring directory = GetParentPath(candidate);
        std::wstring bc = directory + L"\\bc.exe";
        DebugLog(L"[detect] dir: " + directory + L" bc: " + (FileExists(bc) ? L"yes" : L"no"));
        if (!FileExists(bc)) continue;

        install.found = true;
        install.exe = candidate;
        install.bc = bc;
        install.dir = directory;
        install.langDir = directory + L"\\langs";
        install.icon = directory + L"\\shellicons\\bandizip.ico";
        if (!FileExists(install.icon)) install.icon = candidate + L",0";
        break;
    }
    DebugLog(std::wstring(L"[detect] found=") + (install.found ? L"1" : L"0") + L" exe=" + install.exe);
    return install;
}

void LoadLocalization(const BandizipInstall& install, LocStrings& strings) {
    IniFile ini;
    std::wstring languageFile = DetectLanguageFile(install);
    if (!install.langDir.empty()) {
        std::wstring path = install.langDir + L"\\" + languageFile;
        bool loaded = ini.Load(path);
        DebugLog(L"[loc] " + path + (loaded ? L" [loaded]" : L" [failed]"));
    }

    auto pick = [&](const wchar_t* key, std::wstring& target) {
        std::wstring value = ini.Get(key);
        if (!value.empty()) target = CleanMenuLabel(value);
    };

    pick(L"TEXT_MENU_EXTRACT_HERE", strings.extractHere);
    pick(L"TEXT_MENU_EXTRACT_AUTO", strings.extractSmart);
    pick(L"TEXT_MENU_EXTRACT_ARCHIVENAME", strings.extractToFolder);
    pick(L"TEXT_MENU_EXTRACT_DLG", strings.extractDialog);
    pick(L"TEXT_MENU_OPEN_ARCHIVE", strings.openArchive);
    pick(L"TEXT_MENU_TEST_ARCHIVE", strings.testArchive);
    pick(L"TEXT_MENU_ARCHIVE_ZIP", strings.compressZip);
    pick(L"TEXT_MENU_ARCHIVE_EACHNAME", strings.compressEach);
    pick(L"TEXT_MENU_ARCHIVE_POPUP", strings.compressDialog);
}

Settings LoadSettings(const BandizipInstall& install) {
    Settings settings;
    const wchar_t* key = L"Software\\Bandizip";

    settings.extractHere = ReadRegistryDword(HKEY_CURRENT_USER, key, L"shell_ExtractHere", 1) != 0;
    settings.extractSmart = ReadRegistryDword(HKEY_CURRENT_USER, key, L"shell_ExtractAuto", 1) != 0;
    settings.extractToFolder = ReadRegistryDword(HKEY_CURRENT_USER, key, L"shell_ExtractArchivename", 1) != 0;
    settings.extractDialog = ReadRegistryDword(HKEY_CURRENT_USER, key, L"shell_ExtractDlg", 1) != 0;
    settings.openArchive = ReadRegistryDword(HKEY_CURRENT_USER, key, L"shell_OpenArchive", 1) != 0;
    settings.testArchive = ReadRegistryDword(HKEY_CURRENT_USER, key, L"shell_TestArchive", 1) != 0;
    settings.compressZip =
        ReadRegistryDword(HKEY_CURRENT_USER, key, L"shell_ArchivingAsFileName", 1) != 0;
    settings.compressEach =
        ReadRegistryDword(HKEY_CURRENT_USER, key, L"shell_ArchivingEach", 1) != 0;
    settings.compressDialog =
        ReadRegistryDword(HKEY_CURRENT_USER, key, L"shell_ArchivingPopup", 1) != 0;
    settings.autoClose = true;
    settings.compressionFormat = L"zip";

    std::wstring localAppData = GetEnvVar(L"LOCALAPPDATA");
    if (!localAppData.empty()) {
        IniFile config;
        if (config.Load(localAppData + L"\\BandiZip11\\config.ini")) {
            settings.extractHere = ConfigBool(config, L"ShowExtractHere", settings.extractHere);
            settings.extractSmart = ConfigBool(config, L"ShowExtractSmart", settings.extractSmart);
            settings.extractToFolder =
                ConfigBool(config, L"ShowExtractToFolder", settings.extractToFolder);
            settings.extractDialog = ConfigBool(config, L"ShowExtractDialog", settings.extractDialog);
            settings.openArchive = ConfigBool(config, L"ShowOpen", settings.openArchive);
            settings.testArchive = ConfigBool(config, L"ShowTest", settings.testArchive);
            settings.compressZip = ConfigBool(config, L"ShowCompressZip", settings.compressZip);
            settings.compressEach = ConfigBool(config, L"ShowCompressEach", settings.compressEach);
            settings.compressDialog = ConfigBool(config, L"ShowCompressDialog", settings.compressDialog);
            settings.autoClose = ConfigBool(config, L"AutoClose", settings.autoClose);
        }
    }

    (void)install;
    return settings;
}

}  // namespace

std::wstring CleanMenuLabel(std::wstring value) {
    std::wstring result;
    result.reserve(value.size());
    for (size_t i = 0; i < value.size();) {
        if (value[i] == L'(' && i + 3 < value.size() && value[i + 1] == L'&' && value[i + 3] == L')') {
            i += 4;
            continue;
        }
        if (value[i] == L'&') {
            if (i + 1 < value.size() && value[i + 1] == L'&') {
                result.push_back(L'&');
                i += 2;
            } else {
                ++i;
            }
            continue;
        }
        result.push_back(value[i++]);
    }
    return Trim(result);
}

const BandizipInstall& GetBandizip() {
    static BandizipInstall install = DetectBandizip();
    return install;
}

const Settings& GetSettings() {
    static Settings settings = LoadSettings(GetBandizip());
    return settings;
}

const LocStrings& GetLoc() {
    static LocStrings strings;
    static bool loaded = false;
    if (!loaded) {
        loaded = true;
        if (GetBandizip().found) LoadLocalization(GetBandizip(), strings);
    }
    return strings;
}

bool IsArchivePath(const std::wstring& path) {
    std::wstring name = ToLower(GetFileName(path));
    size_t dot = name.find_last_of(L'.');
    if (dot == std::wstring::npos || dot + 1 >= name.size()) return false;
    std::wstring extension = name.substr(dot + 1);

    static const wchar_t* kExtensions[] = {
        L"zip",  L"zipx", L"rar",  L"7z",   L"alz",  L"egg",  L"cab",  L"bh",   L"001",
        L"arj",  L"lha",  L"lzh",  L"pma",  L"ace",  L"arc",  L"aes",  L"zpaq", L"tar",
        L"gz",   L"tgz",  L"bz",   L"bz2",  L"tbz",  L"tbz2", L"xz",   L"txz",  L"lzma",
        L"tlz",  L"lz",   L"z",    L"iso",  L"img",  L"wim",  L"esd",  L"zst",  L"zstd",
    };
    for (const wchar_t* known : kExtensions) {
        if (extension == known) return true;
    }

    if (extension.size() == 3 && (extension[0] == L'r' || extension[0] == L'z') &&
        iswdigit(extension[1]) && iswdigit(extension[2])) {
        return true;
    }
    if (name.find(L".part") != std::wstring::npos && EndsWithNoCase(name, L".rar")) {
        return true;
    }
    return false;
}

std::wstring ArchiveBaseName(const std::wstring& path) {
    std::wstring name = GetFileName(path);
    static const wchar_t* kCompound[] = {
        L".tar.gz", L".tar.bz2", L".tar.xz", L".tar.zst", L".tar.lzma", L".tar.lz", L".tar.z",
    };
    for (const wchar_t* suffix : kCompound) {
        if (EndsWithNoCase(name, suffix)) {
            name.resize(name.size() - wcslen(suffix));
            if (!name.empty()) return name;
            break;
        }
    }

    size_t dot = name.find_last_of(L'.');
    if (dot != std::wstring::npos && dot > 0) name.resize(dot);
    if (name.empty()) name = L"archive";
    return name;
}

std::wstring CompressBaseName(const Selection& selection) {
    if (selection.count == 1 && !selection.paths.empty()) {
        std::wstring name = GetFileName(selection.paths[0]);
        if (!selection.containsDirectory) {
            size_t dot = name.find_last_of(L'.');
            if (dot != std::wstring::npos && dot > 0) name.resize(dot);
        }
        if (!name.empty()) return name;
    }

    std::wstring parentName = GetFileName(selection.parent);
    if (!parentName.empty()) return parentName;
    return L"archive";
}

std::wstring MakeUniqueArchivePath(const std::wstring& directory,
                                   const std::wstring& baseName,
                                   const std::wstring& extension) {
    std::wstring prefix = directory;
    if (!prefix.empty() && prefix.back() != L'\\') prefix.push_back(L'\\');

    std::wstring candidate = prefix + baseName + extension;
    if (GetFileAttributesW(candidate.c_str()) == INVALID_FILE_ATTRIBUTES) return candidate;
    for (int i = 2; i < 1000; ++i) {
        candidate = prefix + baseName + L" (" + std::to_wstring(i) + L")" + extension;
        if (GetFileAttributesW(candidate.c_str()) == INVALID_FILE_ATTRIBUTES) return candidate;
    }
    return candidate;
}

bool AnalyzeSelection(IShellItemArray* items, Selection& selection) {
    selection = Selection();
    if (!items) return false;

    DWORD count = 0;
    if (FAILED(items->GetCount(&count)) || count == 0) return false;
    DebugLog(L"[sel] count=" + std::to_wstring(count));

    for (DWORD i = 0; i < count; ++i) {
        IShellItem* item = nullptr;
        if (FAILED(items->GetItemAt(i, &item)) || !item) return false;

        LPWSTR path = nullptr;
        HRESULT hr = item->GetDisplayName(SIGDN_FILESYSPATH, &path);
        item->Release();
        if (FAILED(hr) || !path) {
            if (path) CoTaskMemFree(path);
            return false;
        }
        selection.paths.emplace_back(path);
        CoTaskMemFree(path);
    }

    selection.count = count;
    selection.single = count == 1;
    selection.parent = GetParentPath(selection.paths[0]);
    selection.allArchives = true;
    selection.allCompressible = true;

    for (const auto& path : selection.paths) {
        bool directory = DirExists(path);
        bool archive = !directory && IsArchivePath(path);
        if (directory) selection.containsDirectory = true;
        if (!archive) selection.allArchives = false;
        if (archive) selection.allCompressible = false;

        if (path.size() == 3 && path[1] == L':' && path[2] == L'\\') {
            selection.allCompressible = false;
        }
    }

    selection.valid = !selection.paths.empty();
    DebugLog(L"[sel] valid=" + std::wstring(selection.valid ? L"1" : L"0") + L" archives=" +
             (selection.allArchives ? L"1" : L"0") + L" compress=" +
             (selection.allCompressible ? L"1" : L"0"));
    return selection.valid;
}
