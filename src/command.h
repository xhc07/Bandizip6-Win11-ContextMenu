#pragma once

#include "bandizip.h"

enum class CommandId : uint32_t {
    Root = 0,
    ExtractHere = 1,
    ExtractSmart = 2,
    ExtractToFolder = 3,
    ExtractDialog = 4,
    OpenArchive = 5,
    TestArchive = 6,
    CompressZip = 7,
    CompressEach = 8,
    CompressDialog = 9,
};

class BandizipCommand final : public IExplorerCommand {
public:
    explicit BandizipCommand(CommandId id);
    ~BandizipCommand();

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;

    HRESULT STDMETHODCALLTYPE GetTitle(IShellItemArray* items, LPWSTR* name) override;
    HRESULT STDMETHODCALLTYPE GetIcon(IShellItemArray* items, LPWSTR* icon) override;
    HRESULT STDMETHODCALLTYPE GetToolTip(IShellItemArray* items, LPWSTR* tooltip) override;
    HRESULT STDMETHODCALLTYPE GetCanonicalName(GUID* guid) override;
    HRESULT STDMETHODCALLTYPE GetState(IShellItemArray* items, BOOL slow, EXPCMDSTATE* state) override;
    HRESULT STDMETHODCALLTYPE Invoke(IShellItemArray* items, IBindCtx* bindContext) override;
    HRESULT STDMETHODCALLTYPE GetFlags(EXPCMDFLAGS* flags) override;
    HRESULT STDMETHODCALLTYPE EnumSubCommands(IEnumExplorerCommand** enumerator) override;

private:
    LONG ref_ = 1;
    CommandId id_;
};
