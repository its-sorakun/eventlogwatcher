// Source - https://stackoverflow.com/q/10834867
// Posted by user1428522, modified by community.

#include <windows.h>
#include <iostream>
using namespace std;

typedef LSTATUS (WINAPI *PRegOpenKeyExW)(HKEY, LPCWSTR, DWORD, REGSAM, PHKEY);
typedef LSTATUS (WINAPI *PRegNotifyChangeKeyValue)(HKEY, BOOL, DWORD, HANDLE, BOOL);
typedef LSTATUS (WINAPI *PRegCloseKey)(HKEY);

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
    PRegOpenKeyExW pRegOpenKeyExW = (PRegOpenKeyExW)GetProcAddress(hAdvApi, "RegOpenKeyExW");
    PRegNotifyChangeKeyValue pRegNotifyChangeKeyValue = (PRegNotifyChangeKeyValue)GetProcAddress(hAdvApi, "RegNotifyChangeKeyValue");
    PRegCloseKey pRegCloseKey = (PRegCloseKey)GetProcAddress(hAdvApi, "RegCloseKey");

    if (!pRegOpenKeyExW || !pRegNotifyChangeKeyValue || !pRegCloseKey)
    {
        cout << "Failed to resolve registry functions" << endl;
        return 1;
    }

    HKEY hkey;
    // using 16bit const char instead of 8bit
    const wchar_t *keyPath = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";

    // open the registry,
    if (pRegOpenKeyExW(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_NOTIFY, &hkey) != ERROR_SUCCESS)
    {
        cout << "Error opening the key" << endl;
        return 1;
    }
    
    HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hEvent == NULL)
    {
        cout << "Error creating event" << endl;
        pRegCloseKey(hkey);
        return 1;
    }

    // cout << "Success" << endl;

    // hook the event to the registry
    while (true)
    {
        // source: https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regnotifychangekeyvalue
        pRegNotifyChangeKeyValue(hkey, TRUE, REG_NOTIFY_CHANGE_LAST_SET, hEvent, TRUE);
        cout << "waiting for the registry modification" << endl;

        // set the trap(thread goes to sleep)
        WaitForSingleObject(hEvent, INFINITE);
        cout << "registry modified" << endl;
    }

    // clean up the handles
    CloseHandle(hEvent);
    pRegCloseKey(hkey);

    // system("PAUSE");
    return 0;
}
