#include <windows.h>

int _tls_index = 0;

void HostEntry(void) {
    MessageBoxW(NULL,
                L"Bandizip 11 context menu is installed.\n\n"
                L"Right-click a file, folder, or folder background in File Explorer.\n"
                L"Use \"Show more options\" if Windows still displays the classic menu.",
                L"BandiZip11", MB_OK | MB_ICONINFORMATION);
    ExitProcess(0);
}
