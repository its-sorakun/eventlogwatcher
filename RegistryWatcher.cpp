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

        cout << "Error opening the key" << endl;
    else
        cout << "Success" << endl;
    system("PAUSE");
    return 0;
}
