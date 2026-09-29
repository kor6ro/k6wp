#pragma once
// Single SHA1 implementation shared by compressor and studio (MED-3).
// Consolidates the two byte-identical copies that used to live in
// compressor/src/cache_manager.cpp and studio/src/thumbnailer.cpp into
// one definition in k6wp_shared. Header is windows.h-free.

#include <string>

namespace k6wp {

/// Self-contained SHA1 (FIPS 180-1) of `data`, returned as 40 lowercase
/// hex chars. Never throws.
std::string Sha1Hex(const std::string& data);

}  // namespace k6wp