#include "MacosM1Target.hpp"

#include <Platform.hpp>

using namespace tulip::hook;

#if defined(TULIP_HOOK_MACOS) && defined(TULIP_HOOK_ARMV8)

Target& Target::get() {
	static MacosM1Target ret;
	return ret;
}

geode::Result<csh> MacosM1Target::openCapstone() {
	// cs_err status;

	// status = cs_open(CS_ARCH_X86, CS_MODE_64, &m_capstone);
	// if (status != CS_ERR_OK) {
		return geode::Err("Couldn't open capstone");
	// }

	// return geode::Ok(m_capstone);
}

std::unique_ptr<HandlerGenerator> MacosM1Target::getHandlerGenerator(
	void* address, void* trampoline, void* handler, void* content, HandlerMetadata const& metadata
) {
	return std::make_unique<ArmV8HandlerGenerator>(address, trampoline, handler, content, metadata);
}

std::unique_ptr<WrapperGenerator> MacosM1Target::getWrapperGenerator(void* address, WrapperMetadata const& metadata) {
	return std::make_unique<ArmV8WrapperGenerator>(address, metadata);
}

#endif


#include "MacosM1Target.hpp"

#include <sys/mman.h>
#include <unistd.h>

namespace tulip::hook {

geode::Result<void*> MacosM1Target::allocateWritableArea(size_t size) {
    size_t pageSize = sysconf(_SC_PAGESIZE);
    size_t allocSize = (size + pageSize - 1) & ~(pageSize - 1);

    void* addr = mmap(nullptr, allocSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (addr == MAP_FAILED) {
        return geode::Err("Failed to allocate writable page");
    }
    return geode::Ok(addr);
}

bool MacosM1Target::makeMemoryExecutable(void* address, size_t size) {
    size_t pageSize = sysconf(_SC_PAGESIZE);
    uintptr_t pageAddr = reinterpret_cast<uintptr_t>(address) & ~(pageSize - 1);
    size_t allocSize = (size + pageSize - 1) & ~(pageSize - 1);

    int result = mprotect(reinterpret_cast<void*>(pageAddr), allocSize, PROT_READ | PROT_EXEC);
    return (result == 0);
}

}