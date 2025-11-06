// Source - https://stackoverflow.com/q/10834867
// Posted by user1428522, modified by community.

#include <windows.h>
#include <iostream>
using namespace std;

int main()
{
    HKEY hkey;
    // using 16bit const char instead of 8bit
    const wchar_t *keyPath = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";

    // open the registry,
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_NOTIFY, &hkey) != ERROR_SUCCESS)
    {
        cout << "Error opening the key" << endl;
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

    system("PAUSE");
    return 0;
}
