#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#ifndef WINVER
#define WINVER 0x0A00
#endif
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <shobjidl.h>
#include <shlwapi.h>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

extern HINSTANCE g_hInst;

std::wstring GetModulePath();
std::wstring GetFileName(const std::wstring& path);
std::wstring GetParentPath(const std::wstring& path);
std::wstring ToLower(std::wstring value);
std::wstring Trim(const std::wstring& value);
bool EndsWithNoCase(const std::wstring& value, const std::wstring& suffix);
void ReplaceAll(std::wstring& value, const std::wstring& from, const std::wstring& to);
bool FileExists(const std::wstring& path);
bool DirExists(const std::wstring& path);
std::wstring GetEnvVar(const wchar_t* name);
std::wstring QuoteArg(const std::wstring& arg);
std::wstring FormatTemplate(std::wstring tmpl, const std::wstring& value);
std::wstring ReadFileText(const std::wstring& path);
bool WriteAppendText(const std::wstring& path, const std::wstring& text);
void DebugLog(const std::wstring& message);
bool LaunchProcess(const std::wstring& exe,
                   const std::vector<std::wstring>& args,
                   const std::wstring& workingDir,
                   bool hidden,
                   std::wstring* error);
