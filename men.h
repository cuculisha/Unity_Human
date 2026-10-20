#pragma once
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <tlhelp32.h>
#include <wchar.h>
#include <string>

#include "Vec.h"
using std::string;

inline HANDLE hProc = NULL;
typedef uint32_t Ptr32;

inline  BOOL rpm(HANDLE hProc, uint32_t addr, void* buf, SIZE_T size)
{
    SIZE_T bytesRead = 0;
    return ReadProcessMemory(hProc, (LPCVOID)(uintptr_t)addr,
        buf, size, &bytesRead)
        && bytesRead == size;
}

template <typename T>
inline T read(uint32_t addr)
{
    T v{};
    rpm(hProc, addr, &v, sizeof(T));
    return v;
}


inline DWORD get_pid_by_name(const wchar_t* processName)
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe; pe.dwSize = sizeof(pe);
    DWORD pid = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, processName) == 0) {
                pid = pe.th32ProcessID; break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

inline uint32_t get_module_base(DWORD pid, const wchar_t* moduleName)
{
    HANDLE snap = CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    MODULEENTRY32W me; me.dwSize = sizeof(me);
    uint32_t base = 0;
    if (Module32FirstW(snap, &me)) {
        do {
            if (_wcsicmp(me.szModule, moduleName) == 0) {
                base = (uint32_t)(uintptr_t)me.modBaseAddr; break;
            }
        } while (Module32NextW(snap, &me));
    }
    CloseHandle(snap);
    return base;
}

inline bool read_str(uint32_t addr, char* out, int maxLen)
{
    if (!addr || !hProc) return false;
    memset(out, 0, maxLen);
    return rpm(hProc, addr, out, maxLen - 1);
}

inline string ReadString(Ptr32 addr) {
    string s;
    char c;
    while ((c = read<char>(addr++)) != '\0') {
        s += c;
    }
    return s;
}

inline Mat4x4 read_mat4x4(Ptr32 addr) {
    return read<Mat4x4>(addr);
}