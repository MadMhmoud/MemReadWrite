#include <windows.h>

//For Universality, we export our functions with C linkage and ensure they are visible outside the DLL
#define MEM_API extern "C" __declspec(dllexport)

HANDLE hProcess;

MEM_API bool Attach(DWORD processId) {

     hProcess = OpenProcess(PROCESS_VM_READ, FALSE, processId); // getting the process handle from the process id

     if (hProcess == NULL) {

         return false;
     }
}

MEM_API void Detach() {

	if (hProcess != NULL) {

		CloseHandle(hProcess);
		hProcess = NULL;
	}
}

MEM_API bool ReadBytes(uintptr_t address, unsigned char* buffer, SIZE_T size) {

    SIZE_T bytesRead = 0;

    // Read the memory from the target process into our local buffer
    BOOL success = ReadProcessMemory(hProcess, (LPCVOID)address, buffer, size, &bytesRead);

    CloseHandle(hProcess);

    // Return true only if the OS reported success AND we read the exact number of expected bytes
    return success && (bytesRead == size);
}


MEM_API bool WriteBytes(uintptr_t address, unsigned char* buffer, SIZE_T size) {

    DWORD oldProtect;

    // Bypass Page Protection
    // Memory might be read-only. We force it to PAGE_EXECUTE_READWRITE so our write doesn't crash.
    if (!VirtualProtectEx(hProcess, (LPVOID)address, size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        CloseHandle(hProcess);
        return false;
    }

    SIZE_T bytesWritten = 0;

    // Write our buffer into the target process
    BOOL success = WriteProcessMemory(hProcess, (LPVOID)address, buffer, size, &bytesWritten);

    // Restore the original page protection
    // This is CRITICAL. Leaving memory unprotected can cause the target application to become unstable.
    VirtualProtectEx(hProcess, (LPVOID)address, size, oldProtect, &oldProtect);

    return success && (bytesWritten == size);
}

