#pragma once

// library.json entry codec (REF-2e): EntryToJson / EntryFromJson, moved
// verbatim out of library_manager.cpp (they lived in LibraryManager's
// anonymous namespace) into their own translation unit. Serialization
// contract is unchanged: same keys, same defaults, same exception behaviour.

#include "library_manager.hpp"
#include "thirdparty/json.hpp"

namespace k6wp {

nlohmann::json EntryToJson(const LibraryEntry&);

LibraryEntry EntryFromJson(const nlohmann::json&);

}  // namespace k6wp
