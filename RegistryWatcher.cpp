// Source - https://stackoverflow.com/q/10834867
// Posted by user1428522, modified by community.

#include <windows.h>
#include <iostream>
using namespace std;

int main()
{
    // check if Advapi32.dll exists or not(most probably it will if running on Windows 2000 professional or later)
    HMODULE hAdvApi = LoadLibrary(L"Advapi32.dll");
    if (hAdvApi == NULL)
    {
        cout << "Advapi32.dll not found" << endl;
        return 1;
    }
    // GetProcAddress always takes 8-bit ANSI strings for the function name
    void *pRegOpen = GetProcAddress(hAdvApi, "RegOpenKeyExW");

    HKEY hkey;
    // using 16bit const char instead of 8bit
    const wchar_t *keyPath = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";

    // open the registry,
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_NOTIFY, &hkey) != ERROR_SUCCESS)
    {
        cout << "Error opening the key" << endl;
        return 1;
    }
    else
    {
        HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        cout << "Success" << endl;

        // hook the event to the registry
        // source: https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regnotifychangekeyvalue
        RegNotifyChangeKeyValue(hkey, TRUE, REG_NOTIFY_CHANGE_LAST_SET, hEvent, TRUE);
        cout << "waiting for the registry modification" << endl;

        // set the trap(thread goes to sleep)
        WaitForSingleObject(hEvent, INFINITE);
        cout << "registry modified" << endl;

        // clean up the handles
        CloseHandle(hEvent);
        RegCloseKey(hkey);
    }

    // system("PAUSE");
    return 0;
}
