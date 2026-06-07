#include <windows.h>
#include <tlhelp32.h>
#include <string.h>

//For Universality, we export our functions with C linkage and ensure they are visible outside the DLL
#define MEM_API extern "C" __declspec(dllexport)


HANDLE hProcess;


MEM_API DWORD GetProcessIdByName(const char* processName) {

    DWORD processId = 0;

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);  //a snapshot of all processes in system, TH32CS_SNAPPROCESS for filtering only active processes

    if (hSnapshot == INVALID_HANDLE_VALUE) {

        return 0;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    wchar_t wideProcessName[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, processName, -1, wideProcessName, MAX_PATH);

    if (Process32FirstW(hSnapshot, &pe32)) {

        do {

            if (_wcsicmp(pe32.szExeFile, wideProcessName) == 0) {

                processId = pe32.th32ProcessID;
                break;
            }
        } while (Process32NextW(hSnapshot, &pe32));

    }

    CloseHandle(hSnapshot);

    return processId;
}

MEM_API bool Attach(DWORD processId) {

     hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, processId); // getting the process handle from the process id

     if (hProcess == NULL) {
        

         return false;
     }
     return true;
}

MEM_API bool AttachByName(const char* processName) {

    DWORD processId = GetProcessIdByName(processName);


    hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, processId); // getting the process handle from the process id

    if (hProcess == NULL) {

        return false;
    }
    return true;
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

