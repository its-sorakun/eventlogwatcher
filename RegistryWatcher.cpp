#include <windows.h>
#include <iostream>

using namespace std;

typedef LSTATUS (WINAPI *PRegOpenKeyExW)(HKEY, LPCWSTR, DWORD, REGSAM, PHKEY);
typedef LSTATUS (WINAPI *PRegNotifyChangeKeyValue)(HKEY, BOOL, DWORD, HANDLE, BOOL);
typedef LSTATUS (WINAPI *PRegCloseKey)(HKEY);

int main()
{
    HMODULE hAdvApi = LoadLibrary(L"Advapi32.dll");
    if (hAdvApi == NULL)
    {
        cout << "Advapi32.dll not found" << endl;
        return 1;
    }

    PRegOpenKeyExW pRegOpenKeyExW = (PRegOpenKeyExW)GetProcAddress(hAdvApi, "RegOpenKeyExW");
    PRegNotifyChangeKeyValue pRegNotifyChangeKeyValue = (PRegNotifyChangeKeyValue)GetProcAddress(hAdvApi, "RegNotifyChangeKeyValue");
    PRegCloseKey pRegCloseKey = (PRegCloseKey)GetProcAddress(hAdvApi, "RegCloseKey");

    if (!pRegOpenKeyExW || !pRegNotifyChangeKeyValue || !pRegCloseKey)
    {
        cout << "Failed to resolve registry functions" << endl;
        return 1;
    }

    HKEY hkey;
    const wchar_t *keyPath = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";

    if (pRegOpenKeyExW(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_NOTIFY, &hkey) != ERROR_SUCCESS)
    {
        cout << "Error opening the key" << endl;
        return 1;
    }
    else
    {
        HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        
        while (true)
        {
            pRegNotifyChangeKeyValue(hkey, TRUE, REG_NOTIFY_CHANGE_LAST_SET, hEvent, TRUE);
            cout << "waiting for the registry modification" << endl;

            WaitForSingleObject(hEvent, INFINITE);
            cout << "registry modified" << endl;

            CloseHandle(hEvent);
            pRegCloseKey(hkey);
        }
    }

    return 0;
}
