# WinMemAPI

A lightweight, C-linked Windows DLL for external process memory manipulation.

---

## Features

* **Universal Linkage:** `extern "C"` export for seamless use in C++, C#, Python, or Rust.
* **Auto Page Protection:** Automatically bypasses and restores read-only memory protections (`PAGE_EXECUTE_READWRITE`) during writes.
* **Specialized Wrappers:** Built-in functions for primitive types (`int32_t`, `int64_t`, `float`, `double`).

---

## API Reference

### Process Control
* `DWORD GetProcessIdByName(const char* processName)`
* `bool Attach(DWORD processId)`
* `bool AttachByName(const char* processName)`
* `void Detach()`

### Core Functions
```cpp
// Reads target memory into a local buffer
bool ReadBytes(uintptr_t address, unsigned char* buffer, SIZE_T size);

// Overwrites target memory (automatically manages page protections)
bool WriteBytes(uintptr_t address, unsigned char* buffer, SIZE_T size);
```

### Type Wrappers
```cpp
int32_t readInt(uintptr_t address);
bool writeInt(uintptr_t address, int32_t value);

int64_t readLong(uintptr_t address);
bool writeLong(uintptr_t address, int64_t value);

float readFloat(uintptr_t address);
bool writeFloat(uintptr_t address, float value);

double readDouble(uintptr_t address);
bool writeDouble(uintptr_t address, double value);
```

---

## Requirements

* **OS:** Windows 10 / 11 (MSVC Compiler).
* **Architecture:** Crucial! Compile as **x86** for 32-bit targets or **x64** for 64-bit targets.
