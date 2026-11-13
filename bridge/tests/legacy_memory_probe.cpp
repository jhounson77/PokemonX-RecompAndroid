// Compiled against the previous header to check precompiled-reader ABI.
#include "domain/memory/process_memory.h"
extern "C" bool LegacyRead32(void* memory, std::uint32_t address, std::uint32_t* value) {
 return static_cast<hexlith::domain::memory::ProcessMemory*>(memory)->ReadInto(address,*value);
}
