#include <windows.h>

// Define a macro to make exporting functions cleaner.
// __declspec(dllexport) tells the linker to expose this function to the outside world.
#define MEM_API extern "C" __declspec(dllexport)

// ==========================================
// READ MEMORY FUNCTION
// ==========================================
MEM_API bool ReadBytes(DWORD processId, uintptr_t address, unsigned char* buffer, SIZE_T size) {
    // 1. Request a handle to the target process with read permissions
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, processId);
    if (hProcess == NULL) {
        return false; // Failed to open process (likely missing admin privileges)
    }

    SIZE_T bytesRead = 0;

    // 2. Read the memory from the target process into our local buffer
    BOOL success = ReadProcessMemory(hProcess, (LPCVOID)address, buffer, size, &bytesRead);

    // 3. Always close the handle to prevent memory leaks in the OS
    CloseHandle(hProcess);

    // Return true only if the OS reported success AND we read the exact number of expected bytes
    return success && (bytesRead == size);
}

// ==========================================
// WRITE MEMORY FUNCTION
// ==========================================
MEM_API bool WriteBytes(DWORD processId, uintptr_t address, unsigned char* buffer, SIZE_T size) {
    // 1. Request write AND operation permissions (operation is required to change page protections)
    HANDLE hProcess = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, processId);
    if (hProcess == NULL) {
        return false;
    }

    DWORD oldProtect;

    // 2. Bypass Page Protection
    // Memory might be read-only. We force it to PAGE_EXECUTE_READWRITE so our write doesn't crash.
    if (!VirtualProtectEx(hProcess, (LPVOID)address, size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        CloseHandle(hProcess);
        return false;
    }

    SIZE_T bytesWritten = 0;

    // 3. Write our buffer into the target process
    BOOL success = WriteProcessMemory(hProcess, (LPVOID)address, buffer, size, &bytesWritten);

    // 4. Restore the original page protection
    // This is CRITICAL. Leaving memory unprotected can cause the target application to become unstable.
    VirtualProtectEx(hProcess, (LPVOID)address, size, oldProtect, &oldProtect);

    CloseHandle(hProcess);

    return success && (bytesWritten == size);
}