#include "PlatformTarget.hpp"

#include <sys/mman.h>
#include <unistd.h>

namespace tulip::hook {

Target& Target::get() {
    static Target instance;
    return instance;
}

geode::Result<void*> MacosM1Target::allocateWritableArea(size_t size) {
    size_t pageSize = sysconf(_SC_PAGESIZE);
    size_t allocSize = (size + pageSize - 1) & ~(pageSize - 1);

    void* addr = mmap(nullptr, allocSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (addr == MAP_FAILED) {
        return geode::Err("Failed to allocate writable page");
    }
    return geode::Ok(addr);
}

bool MacosM1Target::makeMemoryExecutable(void*, size_t) {
    size_t pageSize = sysconf(_SC_PAGESIZE);
    uintptr_t pageAddr = reinterpret_cast<uintptr_t>(address) & ~(pageSize - 1);
    size_t allocSize = (size + pageSize - 1) & ~(pageSize - 1);

    int result = mprotect(reinterpret_cast<void*>(pageAddr), allocSize, PROT_READ | PROT_EXEC);
    return (result == 0);
}

}