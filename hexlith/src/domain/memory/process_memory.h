#ifndef HEXLITH_DOMAIN_MEMORY_PROCESS_MEMORY_H
#define HEXLITH_DOMAIN_MEMORY_PROCESS_MEMORY_H

#include "core/byte_reader.h"
#include "core/result.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace hexlith::domain::memory {

// What a page allows. The ARM11 user mode checks read and write on every
// access; execute is recorded for QueryMemory and for the day lifted code
// cares, but the interpreter fetches through Read and does not check it.
enum class Access : std::uint8_t {
    None = 0,
    Read = 1,
    Write = 2,
    ReadWrite = 3,
    Execute = 4,
    ReadExecute = 5,
    ReadWriteExecute = 7,
};

[[nodiscard]] constexpr bool Allows(Access granted, Access wanted) {
    const auto bits = static_cast<std::uint8_t>(wanted);
    return (static_cast<std::uint8_t>(granted) & bits) == bits;
}

// Host memory standing in for a stretch of the console's physical memory.
//
// Mappings point into a backing rather than owning bytes of their own, which is
// what lets two guest addresses share them. Horizon does this routinely: ldr:ro
// maps the buffer a module was read into at the address the module runs at, and
// MapMemoryBlock maps one shared block into several places. ADR 0012 has why.
//
// The bytes are allocated and zeroed up front. The largest backing is the
// application's FCRAM region, 64 MiB, which is small enough on a PC not to be
// worth allocating lazily.
class Backing {
public:
    // Zero filled. Size is a whole number of pages.
    static std::shared_ptr<Backing> Allocate(std::uint32_t size);

    [[nodiscard]] std::span<std::uint8_t> Bytes() {
        return bytes_;
    }

    [[nodiscard]] std::span<const std::uint8_t> Bytes() const {
        return bytes_;
    }

    [[nodiscard]] std::uint32_t Size() const {
        return static_cast<std::uint32_t>(bytes_.size());
    }

private:
    explicit Backing(std::uint32_t size) : bytes_(size, 0) {}

    std::vector<std::uint8_t> bytes_;
};

// Where one guest page points.
struct PageTarget {
    std::shared_ptr<Backing> backing;
    std::uint32_t offset = 0;
    Access access = Access::None;
};

// A process's 4 GiB address space as a table of 4 KiB pages, each pointing into
// a backing or at nothing.
//
// A read or a write is one table lookup and one host access when it stays inside
// a page, which is every aligned access, so this is what the interpreter runs
// against when it runs the game. The table is a flat array of host pointers,
// one per page, eight megabytes for the whole space: a lookup is an index, not
// a search, which is what ARCHITECTURE.md asks of the path that reads a guest
// word.
//
// It knows nothing of Horizon. Which ranges hold what, and the rules for
// changing them, belong to kernel::AddressSpace.
//
// Satisfies GuestMemory.
// The first pointer and total size preserve the pinned libc++ vector prefix
// used by existing lifted libraries. Ownership is explicit; a borrowed Azahar
// array is never written through this object's mapping API or freed here.
struct PagePointers {
    std::uint8_t** data = nullptr;
    std::uint8_t** owned = nullptr;
    std::size_t count = 0;
    PagePointers(std::size_t size, std::nullptr_t) : owned(new std::uint8_t*[size]{}), count(size) { data=owned; }
    ~PagePointers() { delete[] owned; }
    PagePointers(const PagePointers&)=delete;
    PagePointers& operator=(const PagePointers&)=delete;
    std::uint8_t*& operator[](std::size_t index) const { return data[index]; }
    std::uint8_t*& at(std::size_t index) const {
        if(index>=count)throw std::out_of_range("page pointer index");
        return data[index];
    }
    void Borrow(std::span<std::uint8_t*> pages) {
        if(pages.size()!=count)throw std::invalid_argument("page pointer count");
        delete[] owned;owned=nullptr;data=pages.data();
    }
    void Reset() {
        if(!owned)owned=new std::uint8_t*[count]{};
        data=owned;
    }
};
static_assert(std::is_standard_layout_v<PagePointers>);
static_assert(offsetof(PagePointers,data)==0);
static_assert(sizeof(PagePointers)==sizeof(std::vector<std::uint8_t*>));

// External memory owns mapping and GPU coherence in a hybrid runtime.
class ExternalMemory {
public:
    virtual ~ExternalMemory() = default;
    virtual bool Read(std::uint32_t address, std::span<std::uint8_t> bytes) = 0;
    virtual bool Write(std::uint32_t address, std::span<const std::uint8_t> bytes) = 0;
};

class ProcessMemory {
public:
    // Only an empty address space may delegate; never shadow a mapped page.
    core::Status UseExternal(ExternalMemory& memory);
    core::Status BindExternalPages(std::span<std::uint8_t*> pages);

    // The ARM11's small page, which is also the granularity Horizon maps in.
    static constexpr std::uint32_t PageSize = 0x1000;

    ProcessMemory();

    // Points the pages of a range at consecutive pages of a backing. The range
    // and the offset must be page aligned, the backing must hold the whole of
    // it, and no page of the range may be mapped already.
    core::Status Map(std::uint32_t address,
                     std::uint32_t size,
                     const std::shared_ptr<Backing>& backing,
                     std::uint32_t offset,
                     Access access);

    // Unmaps a range, every page of which must be mapped. The backing lives on
    // while anything else points into it.
    core::Status Unmap(std::uint32_t address, std::uint32_t size);

    // Changes what a mapped range allows.
    core::Status Protect(std::uint32_t address, std::uint32_t size, Access access);

    [[nodiscard]] bool IsMapped(std::uint32_t address) const;

    // The bytes of the page an address is in, whatever access it allows, or an
    // empty span if it is not mapped. For comparing two address spaces, not for
    // the game's accesses, which go through the checks above.
    [[nodiscard]] std::span<const std::uint8_t> PageBytes(std::uint32_t address) const;
    [[nodiscard]] Access AccessAt(std::uint32_t address) const;

    // Where a mapped page points, or nothing. This is how an alias is made: map
    // a second range at the targets of the first.
    [[nodiscard]] std::optional<PageTarget> TargetOf(std::uint32_t address) const;

    // Copies bytes into or out of mapped memory whatever it allows, the way the
    // kernel writes a segment into read-only pages. Fail without copying
    // anything if any byte of the range is unmapped.
    core::Status Load(std::uint32_t address, core::ByteSpan bytes);
    core::Status Copy(std::uint32_t address, std::span<std::uint8_t> bytes) const;

    // The accesses lifted code makes for every load and store, so the common
    // case is here, inline: a whole access inside one mapped page that allows
    // it, which is every aligned one. What straddles pages or fails is in
    // process_memory.cpp.
    [[nodiscard]] std::optional<std::uint8_t> Read8(std::uint32_t address) const {
        return Read<std::uint8_t>(address);
    }

    [[nodiscard]] std::optional<std::uint16_t> Read16(std::uint32_t address) const {
        return Read<std::uint16_t>(address);
    }

    [[nodiscard]] std::optional<std::uint32_t> Read32(std::uint32_t address) const {
        return Read<std::uint32_t>(address);
    }

    [[nodiscard]] std::optional<std::uint64_t> Read64(std::uint32_t address) const {
        return Read<std::uint64_t>(address);
    }

    // Read for lifted code, which hands the value back by reference. A
    // std::optional built on the hot path is kept in memory by GCC at -O1 as four
    // bytes and one byte and read back as eight, and the processor cannot forward
    // two stores to one load, so every guest load stalled for as long as a cache
    // miss. ADR 0034 has the measurements. Always inlined for the same reason the
    // fast path is in this header: the call and the stack slot are the cost.
    template<typename Value>
    [[nodiscard]] [[gnu::always_inline]] bool ReadInto(std::uint32_t address, Value& out) const {
        const std::uint32_t page = address / PageSize;
        const std::uint32_t offset = address % PageSize;
        const std::uint8_t* host = host_[page];
        if (offset + sizeof(Value) <= PageSize && host != nullptr &&
            Allows(access_[page], Access::Read)) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            std::memcpy(&out, host + offset, sizeof(Value));
            return true;
        }
        return ReadIntoSlow(address, out);
    }

    [[gnu::always_inline]] bool Write8(std::uint32_t address, std::uint8_t value) {
        return Write(address, value);
    }

    [[gnu::always_inline]] bool Write16(std::uint32_t address, std::uint16_t value) {
        return Write(address, value);
    }

    [[gnu::always_inline]] bool Write32(std::uint32_t address, std::uint32_t value) {
        return Write(address, value);
    }

    bool Write64(std::uint32_t address, std::uint64_t value) {
        return Write(address, value);
    }

private:
    // What ReadInto does when the access is not whole inside a page that allows
    // it, out of line so that the fast path stays small where it is inlined.
    template<typename Value>
    [[gnu::noinline]] bool ReadIntoSlow(std::uint32_t address, Value& out) const {
        const std::optional<Value> value = ReadValue<Value>(address);
        if (!value.has_value()) {
            return false;
        }
        out = *value;
        return true;
    }

    template<typename Value>
    [[nodiscard]] std::optional<Value> Read(std::uint32_t address) const {
        const std::uint32_t page = address / PageSize;
        const std::uint32_t offset = address % PageSize;
        const std::uint8_t* host = host_[page];
        if (offset + sizeof(Value) <= PageSize && host != nullptr &&
            Allows(access_[page], Access::Read)) {
            Value value{};
            // The hottest path a guest access takes; offset is in the page.
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            std::memcpy(&value, host + offset, sizeof(Value));
            return value;
        }
        return ReadValue<Value>(address);
    }

    template<typename Value>
    [[gnu::always_inline]] bool Write(std::uint32_t address, Value value) {
        const std::uint32_t page = address / PageSize;
        const std::uint32_t offset = address % PageSize;
        std::uint8_t* host = host_[page];
        if (offset + sizeof(Value) <= PageSize && host != nullptr &&
            Allows(access_[page], Access::Write)) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            std::memcpy(host + offset, &value, sizeof(Value));
            return true;
        }
        return WriteValue(address, value);
    }

    template<typename Value>
    [[nodiscard]] std::optional<Value> ReadValue(std::uint32_t address) const;
    template<typename Value>
    bool WriteValue(std::uint32_t address, Value value);

    // The bytes from address to the end of its page, if the page allows the
    // access, or an empty span.
    [[nodiscard]] std::span<std::uint8_t> HostBytes(std::uint32_t address, Access wanted) const;
    [[nodiscard]] bool IsRangeMapped(std::uint32_t address, std::size_t size) const;
    core::Status CheckRange(std::uint32_t address, std::uint32_t size, bool must_be_mapped) const;

    // Indexed by page number: the first host byte of the page, or null for an
    // unmapped page. A bare pointer rather than a span keeps the table at eight
    // bytes a page.
    PagePointers host_;
    std::vector<Access> access_;

    // What keeps each mapped page's backing alive, and where in it the page is.
    // Touched only when the mapping changes, never by a read or a write.
    std::unordered_map<std::uint32_t, PageTarget> targets_;
    ExternalMemory* external_ = nullptr;
};

}  // namespace hexlith::domain::memory

#endif  // HEXLITH_DOMAIN_MEMORY_PROCESS_MEMORY_H
