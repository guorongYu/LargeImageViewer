#include <stdio.h>
#include <Windows.h>

__declspec(dllexport) bool __stdcall OpenConsole()
{
    if (AllocConsole())
    {
        freopen("CONIN$", "rb", stdin);
        freopen("CONOUT$", "wb", stdout);
        freopen("CONOUT$", "wb", stderr);
        return true;
    }
    return false;
}

__declspec(dllexport) bool __stdcall CloseConsole()
{
    if (FreeConsole())
        return true;
    return false;
}