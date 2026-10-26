#include "common.h"

#include <algorithm>
#include <cstdlib>
#include <cwchar>

HINSTANCE g_hInst = nullptr;

std::wstring GetModulePath() {
    std::vector<wchar_t> buffer(32768);
    DWORD length = GetModuleFileNameW(g_hInst, buffer.data(), (DWORD)buffer.size());
    if (length == 0) return std::wstring();
    return std::wstring(buffer.data(), length);
}

std::wstring GetFileName(const std::wstring& path) {
    if (path.empty()) return std::wstring();
    const wchar_t* name = PathFindFileNameW(path.c_str());
    return name ? std::wstring(name) : std::wstring();
}

std::wstring GetParentPath(const std::wstring& path) {
    if (path.empty()) return std::wstring();
    std::wstring result = path;
    if (!PathRemoveFileSpecW(&result[0])) return std::wstring();
    result.resize(wcslen(result.c_str()));
    if (result.empty()) return std::wstring();
    if (result.size() > 3 && result.back() == L'\\') result.pop_back();
    return result;
}

std::wstring ToLower(std::wstring value) {
    if (!value.empty()) CharLowerBuffW(&value[0], (DWORD)value.size());
    return value;
}

std::wstring Trim(const std::wstring& value) {
    size_t begin = 0;
    size_t end = value.size();
    while (begin < end && iswspace(value[begin])) ++begin;
    while (end > begin && iswspace(value[end - 1])) --end;
    return value.substr(begin, end - begin);
}

bool EndsWithNoCase(const std::wstring& value, const std::wstring& suffix) {
    if (suffix.size() > value.size()) return false;
    std::wstring tail = value.substr(value.size() - suffix.size());
    return ToLower(tail) == ToLower(suffix);
}

void ReplaceAll(std::wstring& value, const std::wstring& from, const std::wstring& to) {
    if (from.empty()) return;
    size_t pos = 0;
    while ((pos = value.find(from, pos)) != std::wstring::npos) {
        value.replace(pos, from.size(), to);
        pos += to.size();
    }
}

bool FileExists(const std::wstring& path) {
    DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

bool DirExists(const std::wstring& path) {
    DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

std::wstring GetEnvVar(const wchar_t* name) {
    DWORD length = GetEnvironmentVariableW(name, nullptr, 0);
    if (length == 0) return std::wstring();
    std::vector<wchar_t> buffer(length);
    DWORD written = GetEnvironmentVariableW(name, buffer.data(), length);
    if (written == 0 || written >= length) return std::wstring();
    return std::wstring(buffer.data(), written);
}

std::wstring QuoteArg(const std::wstring& arg) {
    if (arg.empty()) return L"\"\"";
    if (arg.find_first_of(L" \t\n\v\"") == std::wstring::npos) return arg;

    std::wstring result;
    result.push_back(L'"');
    size_t backslashes = 0;
    for (wchar_t ch : arg) {
        if (ch == L'\\') {
            ++backslashes;
        } else if (ch == L'"') {
            result.append(backslashes * 2 + 1, L'\\');
            result.push_back(L'"');
            backslashes = 0;
        } else {
            result.append(backslashes, L'\\');
            backslashes = 0;
            result.push_back(ch);
        }
    }
    result.append(backslashes * 2, L'\\');
    result.push_back(L'"');
    return result;
}

std::wstring FormatTemplate(std::wstring tmpl, const std::wstring& value) {
    ReplaceAll(tmpl, L"${str1}", value);
    return tmpl;
}

static std::wstring DecodeMultiByte(const char* data, int size, UINT codePage) {
    if (size <= 0) return std::wstring();
    int length = MultiByteToWideChar(codePage, 0, data, size, nullptr, 0);
    if (length <= 0) return std::wstring();
    std::wstring result((size_t)length, L'\0');
    MultiByteToWideChar(codePage, 0, data, size, &result[0], length);
    return result;
}

std::wstring ReadFileText(const std::wstring& path) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return std::wstring();

    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0 || size.QuadPart > 32 * 1024 * 1024) {
        CloseHandle(file);
        return std::wstring();
    }

    std::vector<char> data((size_t)size.QuadPart);
    DWORD read = 0;
    BOOL ok = ReadFile(file, data.data(), (DWORD)data.size(), &read, nullptr);
    CloseHandle(file);
    if (!ok || read == 0) return std::wstring();

    if (read >= 2 && (unsigned char)data[0] == 0xFF && (unsigned char)data[1] == 0xFE) {
        size_t chars = (read - 2) / 2;
        return std::wstring(reinterpret_cast<const wchar_t*>(data.data() + 2), chars);
    }
    if (read >= 2 && (unsigned char)data[0] == 0xFE && (unsigned char)data[1] == 0xFF) {
        size_t chars = (read - 2) / 2;
        std::wstring result(chars, L'\0');
        const unsigned char* p = reinterpret_cast<const unsigned char*>(data.data() + 2);
        for (size_t i = 0; i < chars; ++i) {
            result[i] = (wchar_t)((p[i * 2] << 8) | p[i * 2 + 1]);
        }
        return result;
    }
    if (read >= 3 && (unsigned char)data[0] == 0xEF && (unsigned char)data[1] == 0xBB &&
        (unsigned char)data[2] == 0xBF) {
        return DecodeMultiByte(data.data() + 3, (int)read - 3, CP_UTF8);
    }

    std::wstring utf8 = DecodeMultiByte(data.data(), (int)read, CP_UTF8);
    if (!utf8.empty()) return utf8;
    return DecodeMultiByte(data.data(), (int)read, CP_ACP);
}

bool WriteAppendText(const std::wstring& path, const std::wstring& text) {
    HANDLE file = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ,
                              nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;

    LARGE_INTEGER size{};
    GetFileSizeEx(file, &size);
    DWORD written = 0;
    if (size.QuadPart == 0) {
        const unsigned char bom[2] = {0xFF, 0xFE};
        WriteFile(file, bom, 2, &written, nullptr);
    }
    if (!text.empty()) {
        WriteFile(file, text.data(), (DWORD)(text.size() * sizeof(wchar_t)), &written, nullptr);
    }
    CloseHandle(file);
    return true;
}

void DebugLog(const std::wstring& message) {
    std::wstring target = GetEnvVar(L"BANDIZIP11_DRY_RUN");
    if (target.empty()) return;

    std::wstring path = target;
    if (target == L"1") {
        wchar_t temp[MAX_PATH] = {};
        DWORD length = GetTempPathW(MAX_PATH, temp);
        if (length == 0) return;
        path = std::wstring(temp, length) + L"BandiZip11-dryrun.log";
    }
    WriteAppendText(path, message + L"\r\n");
}

static std::wstring FormatWin32Error(DWORD code) {
    wchar_t* buffer = nullptr;
    DWORD length = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                      FORMAT_MESSAGE_IGNORE_INSERTS,
                                  nullptr, code, 0, (LPWSTR)&buffer, 0, nullptr);
    std::wstring result;
    if (length && buffer) result.assign(buffer, length);
    if (buffer) LocalFree(buffer);
    while (!result.empty() && (result.back() == L'\r' || result.back() == L'\n')) result.pop_back();
    return result;
}

bool LaunchProcess(const std::wstring& exe,
                   const std::vector<std::wstring>& args,
                   const std::wstring& workingDir,
                   bool hidden,
                   std::wstring* error) {
    std::wstring commandLine = QuoteArg(exe);
    for (const auto& arg : args) {
        commandLine.push_back(L' ');
        commandLine += QuoteArg(arg);
    }

    if (!GetEnvVar(L"BANDIZIP11_DRY_RUN").empty()) {
        DebugLog(commandLine);
        return true;
    }

    std::vector<wchar_t> mutableCommand(commandLine.begin(), commandLine.end());
    mutableCommand.push_back(L'\0');

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    DWORD flags = hidden ? CREATE_NO_WINDOW : 0;

    BOOL ok = CreateProcessW(exe.c_str(), mutableCommand.data(), nullptr, nullptr, FALSE, flags,
                             nullptr, workingDir.empty() ? nullptr : workingDir.c_str(), &startup,
                             &process);
    if (!ok) {
        if (error) *error = FormatWin32Error(GetLastError());
        return false;
    }

    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return true;
}
