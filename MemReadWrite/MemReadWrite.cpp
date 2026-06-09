#include <windows.h>
#include <tlhelp32.h>
#include <string.h>
#include <cstdint>

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

MEM_API uintptr_t GetModuleBaseAddress(DWORD processId, const char* moduleName) {
    uintptr_t dwModuleBaseAddress = 0;

 
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);

    if (hSnapshot != INVALID_HANDLE_VALUE) {

        MODULEENTRY32W me32;
        me32.dwSize = sizeof(MODULEENTRY32W);

        wchar_t wideModuleName[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, moduleName, -1, wideModuleName, MAX_PATH);

        if (Module32FirstW(hSnapshot, &me32)) {
            do {

                if (_wcsicmp(me32.szModule, wideModuleName) == 0) {

                    dwModuleBaseAddress = (uintptr_t)me32.modBaseAddr;
                    break;
                }

            } 
            while (Module32NextW(hSnapshot, &me32));
        }

        CloseHandle(hSnapshot);
    }

    return dwModuleBaseAddress;
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




MEM_API uintptr_t ResolvePointerChain(uintptr_t baseAddress, uintptr_t* offsets, int offsetsCount) {

    uintptr_t currentAddress = baseAddress; // starting from the baseAddress

    for (int i = 0; i < offsetsCount; i++) {

        ReadProcessMemory(hProcess, (LPCVOID)currentAddress, &currentAddress, sizeof(currentAddress), NULL);

        if (currentAddress == 0) {

            return 0; // abort if a pointer layer evaluates to null
        }

        currentAddress += offsets[i]; // adding the next iffset to the current pointer 
    }

    return currentAddress;
}




MEM_API int32_t readInt(uintptr_t address) {
    int32_t value = 0;

    ReadBytes(address, (unsigned char*)&value, sizeof(value));
    return value;
}

MEM_API bool writeInt(uintptr_t address, int32_t value) {

    return WriteBytes(address, (unsigned char*)&value, sizeof(value));
}

MEM_API float readFloat(uintptr_t address) {

    float value = 0.0f;

    ReadBytes(address, (unsigned char*)&value, sizeof(value));
    return value;
}

MEM_API bool writeFloat(uintptr_t address, float value) {

    return WriteBytes(address, (unsigned char*)&value, sizeof(value));
}

MEM_API int64_t readLong(uintptr_t address) {

    int64_t value = 0;

    ReadBytes(address, (unsigned char*)&value, sizeof(value));
    return value;
}

MEM_API bool writeLong(uintptr_t address, int64_t value) {

    return WriteBytes(address, (unsigned char*)&value, sizeof(value));
}

MEM_API double readDouble(uintptr_t address) {

    double value = 0.0;

    ReadBytes(address, (unsigned char*)&value, sizeof(value));
    return value;
}

MEM_API bool writeDouble(uintptr_t address, double value) {

    return WriteBytes(address, (unsigned char*)&value, sizeof(value));
}


