#pragma once

#include <Platform.hpp>

#if defined(TULIP_HOOK_MACOS) && defined(TULIP_HOOK_ARMV8)

#include "../generator/ArmV8Generator.hpp"
#include "DarwinTarget.hpp"

namespace tulip::hook {

class MacosM1Target : public Target {
public:
    geode::Result<void*> allocateWritableArea(size_t size) override;
    bool makeMemoryExecutable(void* address, size_t size) override;

    geode::Result<csh> openCapstone() override;
    geode::Result<> allocatePage() override;
    geode::Result<uint32_t> getProtection(void* address) override;
    geode::Result<> protectMemory(void* address, size_t size, uint32_t protection) override;
    geode::Result<> rawWriteMemory(void* destination, void const* source, size_t size) override;
    uint32_t getWritableProtection() override;

    std::unique_ptr<HandlerGenerator> getHandlerGenerator(
        void* address, void* trampoline, void* handler, void* content, HandlerMetadata const& metadata
    ) override;
    std::unique_ptr<WrapperGenerator> getWrapperGenerator(void* address, WrapperMetadata const& metadata) override;
};

} 

#endif
