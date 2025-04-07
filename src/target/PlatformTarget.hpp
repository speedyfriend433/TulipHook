#pragma once

#include "Target.hpp"

// Include your platforms
#include "MacosIntelTarget.hpp"
#include "MacosM1Target.hpp"
#include "PosixArmV7Target.hpp"
#include "PosixArmV8Target.hpp"
#include "Windows32Target.hpp"

// Here you can optionally declare platform detection / switcher if needed
// Do NOT define MacosM1Target here!