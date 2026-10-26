#include "command.h"

#include <new>

namespace {

const CLSID kCommandClsid = {
    0x4c1e9a76, 0x2b3d, 0x4f80, {0x9e, 0x5a, 0x7d, 0x6c, 0x1b, 0x2a, 0x3f, 0x40}};

LONG g_liveObjects = 0;

bool IsArchiveCommand(CommandId id) {
    switch (id) {
        case CommandId::ExtractHere:
        case CommandId::ExtractSmart:
        case CommandId::ExtractToFolder:
        case CommandId::ExtractDialog:
        case CommandId::OpenArchive:
        case CommandId::TestArchive:
            return true;
        default:
            return false;
    }
}

bool IsCompressCommand(CommandId id) {
    return id == CommandId::CompressZip || id == CommandId::CompressEach ||
           id == CommandId::CompressDialog;
}

bool IsVisible(CommandId id, const Selection& selection, const Settings& settings) {
    if (!selection.valid || selection.count == 0) return false;

    if (IsArchiveCommand(id)) {
        if (!selection.single || !selection.allArchives) return false;
        switch (id) {
            case CommandId::ExtractHere:
                return settings.extractHere;
            case CommandId::ExtractSmart:
                return settings.extractSmart;
            case CommandId::ExtractToFolder:
                return settings.extractToFolder;
            case CommandId::ExtractDialog:
                return settings.extractDialog;
            case CommandId::OpenArchive:
                return settings.openArchive;
            case CommandId::TestArchive:
                return settings.testArchive;
            default:
                return false;
        }
    }

    if (IsCompressCommand(id)) {
        if (!selection.allCompressible) return false;
        switch (id) {
            case CommandId::CompressZip:
                return settings.compressZip;
            case CommandId::CompressEach:
                return settings.compressEach && selection.count > 1;
            case CommandId::CompressDialog:
                return settings.compressDialog;
            default:
                return false;
        }
    }
    return false;
}

std::wstring TitleFor(CommandId id, const Selection* selection) {
    const LocStrings& strings = GetLoc();
    switch (id) {
        case CommandId::Root:
            return strings.root;
        case CommandId::ExtractHere:
            return strings.extractHere;
        case CommandId::ExtractSmart:
            return strings.extractSmart;
        case CommandId::ExtractToFolder: {
            std::wstring name = L"archive";
            if (selection && !selection->paths.empty()) name = ArchiveBaseName(selection->paths[0]);
            return FormatTemplate(strings.extractToFolder, name);
        }
        case CommandId::ExtractDialog:
            return strings.extractDialog;
        case CommandId::OpenArchive:
            return strings.openArchive;
        case CommandId::TestArchive:
            return strings.testArchive;
        case CommandId::CompressZip: {
            std::wstring name = L"archive";
            if (selection) name = CompressBaseName(*selection);
            return FormatTemplate(strings.compressZip, name + L".zip");
        }
        case CommandId::CompressEach:
            return strings.compressEach;
        case CommandId::CompressDialog:
            return strings.compressDialog;
        default:
            return L"Bandizip";
    }
}

void ShowLaunchError(const std::wstring& detail) {
    std::wstring message = L"Could not start Bandizip.\n\n";
    if (!detail.empty()) message += detail;
    MessageBoxW(nullptr, message.c_str(), L"BandiZip11", MB_OK | MB_ICONERROR);
}

void InvokeCommand(CommandId id, const Selection& selection) {
    const BandizipInstall& install = GetBandizip();
    if (!install.found || selection.paths.empty()) return;

    const Settings& settings = GetSettings();
    const std::wstring& parent = selection.parent;
    std::vector<std::wstring> args;
    std::wstring workingDirectory = parent.empty() ? install.dir : parent;

    switch (id) {
        case CommandId::ExtractHere:
            args = {L"x", L"-o:" + parent};
            if (settings.autoClose) args.push_back(L"-y");
            args.push_back(selection.paths[0]);
            break;

        case CommandId::ExtractSmart:
            args = {L"x", L"-target:auto", L"-o:" + parent};
            if (settings.autoClose) args.push_back(L"-y");
            args.push_back(selection.paths[0]);
            break;

        case CommandId::ExtractToFolder:
            args = {L"x", L"-target:name", L"-o:" + parent};
            if (settings.autoClose) args.push_back(L"-y");
            args.push_back(selection.paths[0]);
            break;

        case CommandId::ExtractDialog:
            args = {L"x", L"-target:dlg", L"-o:" + parent, selection.paths[0]};
            break;

        case CommandId::OpenArchive:
            args = {selection.paths[0]};
            break;

        case CommandId::TestArchive:
            args = {L"t"};
            if (settings.autoClose) args.push_back(L"-y");
            args.push_back(selection.paths[0]);
            break;

        case CommandId::CompressZip: {
            std::wstring archive =
                MakeUniqueArchivePath(parent, CompressBaseName(selection), L".zip");
            args = {L"c", L"-fmt:zip", L"-o:" + parent};
            if (settings.autoClose) args.push_back(L"-y");
            args.push_back(archive);
            for (const auto& path : selection.paths) args.push_back(path);
            break;
        }

        case CommandId::CompressEach:
            args = {L"bc", L"-fmt:zip", L"-o:" + parent};
            if (settings.autoClose) args.push_back(L"-y");
            for (const auto& path : selection.paths) args.push_back(path);
            break;

        case CommandId::CompressDialog: {
            std::wstring archive =
                MakeUniqueArchivePath(parent, CompressBaseName(selection), L".zip");
            args = {L"cd", L"-o:" + parent, archive};
            for (const auto& path : selection.paths) args.push_back(path);
            break;
        }

        default:
            return;
    }

    std::wstring error;
    if (!LaunchProcess(install.exe, args, workingDirectory, false, &error)) {
        ShowLaunchError(error);
    }
}

class CommandEnumerator final : public IEnumExplorerCommand {
public:
    CommandEnumerator() { InterlockedIncrement(&g_liveObjects); }
    ~CommandEnumerator() {
        for (IExplorerCommand* command : commands_) {
            if (command) command->Release();
        }
        InterlockedDecrement(&g_liveObjects);
    }

    void Add(IExplorerCommand* command) {
        if (command) commands_.push_back(command);
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) return E_POINTER;
        if (riid == IID_IUnknown || riid == IID_IEnumExplorerCommand) {
            *ppv = static_cast<IEnumExplorerCommand*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        return (ULONG)InterlockedIncrement(&ref_);
    }

    ULONG STDMETHODCALLTYPE Release() override {
        LONG value = InterlockedDecrement(&ref_);
        if (value == 0) delete this;
        return (ULONG)value;
    }

    HRESULT STDMETHODCALLTYPE Next(ULONG count, IExplorerCommand** commands,
                                   ULONG* fetched) override {
        if (!commands) return E_POINTER;
        if (count == 0) {
            if (fetched) *fetched = 0;
            return S_OK;
        }

        ULONG written = 0;
        while (written < count && position_ < commands_.size()) {
            commands[written] = commands_[position_];
            commands[written]->AddRef();
            ++written;
            ++position_;
        }
        if (fetched) *fetched = written;
        return written == count ? S_OK : S_FALSE;
    }

    HRESULT STDMETHODCALLTYPE Skip(ULONG count) override {
        position_ += count;
        if (position_ > commands_.size()) {
            position_ = commands_.size();
            return S_FALSE;
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Reset() override {
        position_ = 0;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Clone(IEnumExplorerCommand** enumerator) override {
        if (!enumerator) return E_POINTER;
        CommandEnumerator* clone = new (std::nothrow) CommandEnumerator();
        if (!clone) return E_OUTOFMEMORY;
        for (IExplorerCommand* command : commands_) {
            command->AddRef();
            clone->commands_.push_back(command);
        }
        *enumerator = clone;
        return S_OK;
    }

private:
    LONG ref_ = 1;
    std::vector<IExplorerCommand*> commands_;
    size_t position_ = 0;
};

class CommandFactory final : public IClassFactory {
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) return E_POINTER;
        if (riid == IID_IUnknown || riid == IID_IClassFactory) {
            *ppv = static_cast<IClassFactory*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override { return 2; }
    ULONG STDMETHODCALLTYPE Release() override { return 1; }

    HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* outer, REFIID riid, void** ppv) override {
        if (ppv) *ppv = nullptr;
        if (outer) return CLASS_E_NOAGGREGATION;

        BandizipCommand* command = new (std::nothrow) BandizipCommand(CommandId::Root);
        if (!command) return E_OUTOFMEMORY;

        HRESULT hr = command->QueryInterface(riid, ppv);
        command->Release();
        return hr;
    }

    HRESULT STDMETHODCALLTYPE LockServer(BOOL) override { return S_OK; }
};

}  // namespace

BandizipCommand::BandizipCommand(CommandId id) : id_(id) {
    InterlockedIncrement(&g_liveObjects);
}

BandizipCommand::~BandizipCommand() {
    InterlockedDecrement(&g_liveObjects);
}

HRESULT STDMETHODCALLTYPE BandizipCommand::QueryInterface(REFIID riid, void** ppv) {
    if (!ppv) return E_POINTER;
    if (riid == IID_IUnknown || riid == IID_IExplorerCommand) {
        *ppv = static_cast<IExplorerCommand*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE BandizipCommand::AddRef() {
    return (ULONG)InterlockedIncrement(&ref_);
}

ULONG STDMETHODCALLTYPE BandizipCommand::Release() {
    LONG value = InterlockedDecrement(&ref_);
    if (value == 0) delete this;
    return (ULONG)value;
}

HRESULT STDMETHODCALLTYPE BandizipCommand::GetTitle(IShellItemArray* items, LPWSTR* name) {
    if (!name) return E_POINTER;
    *name = nullptr;

    std::wstring title;
    if (id_ == CommandId::Root) {
        title = GetLoc().root;
    } else {
        Selection selection;
        bool valid = AnalyzeSelection(items, selection);
        title = TitleFor(id_, valid ? &selection : nullptr);
    }

    if (title.empty()) title = L"Bandizip";
    return SHStrDupW(title.c_str(), name);
}

HRESULT STDMETHODCALLTYPE BandizipCommand::GetIcon(IShellItemArray*, LPWSTR* icon) {
    if (!icon) return E_POINTER;
    *icon = nullptr;
    const BandizipInstall& install = GetBandizip();
    if (!install.found) return E_NOTIMPL;
    return SHStrDupW(install.icon.c_str(), icon);
}

HRESULT STDMETHODCALLTYPE BandizipCommand::GetToolTip(IShellItemArray*, LPWSTR* tooltip) {
    if (tooltip) *tooltip = nullptr;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE BandizipCommand::GetCanonicalName(GUID* guid) {
    if (!guid) return E_POINTER;
    *guid = GUID_NULL;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BandizipCommand::GetState(IShellItemArray* items, BOOL, EXPCMDSTATE* state) {
    if (!state) return E_POINTER;
    *state = ECS_HIDDEN;

    if (!GetBandizip().found) return S_OK;

    Selection selection;
    if (!AnalyzeSelection(items, selection)) return S_OK;

    const Settings& settings = GetSettings();
    if (id_ == CommandId::Root) {
        for (uint32_t value = 1; value <= 9; ++value) {
            if (IsVisible((CommandId)value, selection, settings)) {
                *state = ECS_ENABLED;
                break;
            }
        }
    } else if (IsVisible(id_, selection, settings)) {
        *state = ECS_ENABLED;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BandizipCommand::Invoke(IShellItemArray* items, IBindCtx*) {
    if (id_ == CommandId::Root) return S_OK;

    Selection selection;
    if (AnalyzeSelection(items, selection)) {
        InvokeCommand(id_, selection);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BandizipCommand::GetFlags(EXPCMDFLAGS* flags) {
    if (!flags) return E_POINTER;
    *flags = id_ == CommandId::Root ? ECF_HASSUBCOMMANDS : ECF_DEFAULT;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BandizipCommand::EnumSubCommands(IEnumExplorerCommand** enumerator) {
    if (!enumerator) return E_POINTER;
    *enumerator = nullptr;
    if (id_ != CommandId::Root) return E_NOTIMPL;

    CommandEnumerator* commands = new (std::nothrow) CommandEnumerator();
    if (!commands) return E_OUTOFMEMORY;

    for (uint32_t value = 1; value <= 9; ++value) {
        BandizipCommand* child = new (std::nothrow) BandizipCommand((CommandId)value);
        if (!child) {
            commands->Release();
            return E_OUTOFMEMORY;
        }
        commands->Add(child);
    }

    *enumerator = commands;
    return S_OK;
}

extern "C" __declspec(dllexport) HRESULT __stdcall DllGetClassObject(REFCLSID clsid, REFIID riid,
                                                                     void** ppv) {
    if (!ppv) return E_POINTER;
    *ppv = nullptr;
    if (!IsEqualCLSID(clsid, kCommandClsid)) return CLASS_E_CLASSNOTAVAILABLE;
    static CommandFactory factory;
    return factory.QueryInterface(riid, ppv);
}

extern "C" __declspec(dllexport) HRESULT __stdcall DllCanUnloadNow() {
    return g_liveObjects == 0 ? S_OK : S_FALSE;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_hInst = instance;
        DisableThreadLibraryCalls(instance);
    }
    return TRUE;
}
