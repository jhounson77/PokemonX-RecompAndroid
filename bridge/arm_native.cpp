#include "arm_native.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include "domain/arm/interpreter/interpreter.h"

namespace PokemonX {
namespace arm = hexlith::domain::arm;

ARMNative::ARMNative(Core::System& system, Memory::MemorySystem& memory_, u32 id,
                     std::shared_ptr<Core::Timing::Timer> timer,
                     const hexlith::domain::lift::runtime::EntryTable& entries,
                     hexlith::domain::lift::runtime::LoadedModules* modules)
    : ARM_Interface(id, std::move(timer)), memory(memory_), svc(system), engine(entries, modules) {
    if (!guest_memory.UseExternal(*this)) throw std::runtime_error("could not attach Azahar memory");
    SetPageTable(memory.GetCurrentPageTable());
}
void ARMNative::Run() { Execute(static_cast<u64>(std::max<s64>(GetTimer().GetDowncount(), 0))); }
void ARMNative::Step() { Execute(1); }
void ARMNative::Execute(u64 limit) {
    if (break_flag || limit == 0) return;
    reschedule = false;
    // Another core may have written while this core was not running. ARM
    // permits a reservation to be lost; never allow a stale STREX to succeed.
    ClearExclusiveState();
    while (limit != 0 && !reschedule && !break_flag) {
        // Bound return latency without changing emulated time or game speed.
        const auto budget = std::min<u64>(limit, 8192);
        u64 executed = 0;
        const auto stop = engine.Run(state, guest_memory, budget, executed);
        GetTimer().AddTicks(executed);
        limit -= std::min(limit, executed);
        if (stop.reason == arm::StopReason::SupervisorCall) {
            // The engine already advanced PC. Azahar owns IPC, scheduling,
            // handles, GPU synchronization and the registers returned by SVC.
            svc.CallSVC(stop.detail);
            if(stop.detail == 3 || stop.detail == 9) reschedule = true;
            limit = std::min<u64>(limit, static_cast<u64>(std::max<s64>(GetTimer().GetDowncount(), 0)));
        } else if (stop.reason != arm::StopReason::StepLimit) {
            throw std::runtime_error("native CPU stopped at PC " + std::to_string(GetPC()) + ": " + std::string(arm::Describe(stop.reason)) + ", detail=" + std::to_string(stop.detail));
        }
        if (executed == 0) break;
    }
}
bool ARMNative::Read(u32 address, std::span<u8> bytes) {
    const auto offset = address & (Memory::CITRA_PAGE_SIZE-1);
    if (page_table && bytes.size() <= Memory::CITRA_PAGE_SIZE-offset) {
        if (const auto* page = page_table->GetPointerArray()[address / Memory::CITRA_PAGE_SIZE]) {
            std::memcpy(bytes.data(), page+offset, bytes.size());
            return true;
        }
    }
    memory.ReadBlock(address, bytes.data(), bytes.size());
    return true;
}
bool ARMNative::Write(u32 address, std::span<const u8> bytes) {
    const auto offset = address & (Memory::CITRA_PAGE_SIZE-1);
    if (page_table && bytes.size() <= Memory::CITRA_PAGE_SIZE-offset) {
        if (auto* page = page_table->GetPointerArray()[address / Memory::CITRA_PAGE_SIZE]) {
            std::memcpy(page+offset, bytes.data(), bytes.size());
            return true;
        }
    }
    memory.WriteBlock(address, bytes.data(), bytes.size());
    return true;
}
void ARMNative::SetPageTable(const std::shared_ptr<Memory::PageTable>& table) {
    page_table = table;
    const auto pages=table?std::span<u8*>(table->GetPointerArray()):std::span<u8*>{};
    if(!guest_memory.BindExternalPages(pages))throw std::runtime_error("cannot bind live Azahar pages");
    // Look up each pointer afresh. Rasterizer-cached pages become null and
    // must go through MemorySystem to synchronize CPU and GPU contents.
    ClearExclusiveState();
}
std::shared_ptr<Memory::PageTable> ARMNative::GetPageTable() const { return page_table; }
void ARMNative::InvalidateCacheRange(u32, std::size_t) {
    // Generated entry tables are immutable. Until code-write tracking exists,
    // frontend must reject executable patches rather than use stale AOT code.
    ClearExclusiveState();
}
void ARMNative::ClearExclusiveState() { state.exclusive_open = false; }
void ARMNative::SetPC(u32 value) { state.registers.at(15) = value; }
u32 ARMNative::GetPC() const { return state.registers.at(15); }
u32 ARMNative::GetReg(int index) const { return state.registers.at(index); }
void ARMNative::SetReg(int index, u32 value) { state.registers.at(index) = value; }
u32 ARMNative::GetVFPReg(int index) const { return index < 32 ? state.float_registers.at(index) : 0; }
void ARMNative::SetVFPReg(int index, u32 value) { if (index < 32) state.float_registers.at(index) = value; }
u32 ARMNative::GetVFPSystemReg(VFPSystemRegister reg) const {
    return reg == VFP_FPSCR ? state.fpscr : reg == VFP_FPEXC ? fpexc : 0;
}
void ARMNative::SetVFPSystemReg(VFPSystemRegister reg, u32 value) {
    if (reg == VFP_FPSCR) state.fpscr = value;
    else if (reg == VFP_FPEXC) fpexc = value;
}
u32 ARMNative::GetCPSR() const {
    constexpr u32 native_bits = 0xF80F0020;
    return (misc_cpsr & ~native_bits) | (arm::PackCpsr(state) & native_bits);
}
void ARMNative::SetCPSR(u32 value) {
    misc_cpsr = value;
    arm::WriteCpsrFromUserMode(state, value, 12);
    state.thumb = (value & (1u << 5)) != 0;
}
u32 ARMNative::GetCP15Register(CP15Register reg) const {
    return reg == CP15_THREAD_UPRW ? state.thread_id_read_write : reg == CP15_THREAD_URO ? state.thread_id_read_only : 0;
}
void ARMNative::SetCP15Register(CP15Register reg, u32 value) {
    if (reg == CP15_THREAD_UPRW) state.thread_id_read_write = value;
    else if (reg == CP15_THREAD_URO) state.thread_id_read_only = value;
}
void ARMNative::SaveContext(ThreadContext& context) {
    context.cpu_registers = state.registers;
    std::copy(state.float_registers.begin(), state.float_registers.end(), context.fpu_registers.begin());
    context.cpsr = GetCPSR(); context.fpscr = state.fpscr; context.fpexc = fpexc;
}
void ARMNative::LoadContext(const ThreadContext& context) {
    state.registers = context.cpu_registers;
    std::copy_n(context.fpu_registers.begin(), 32, state.float_registers.begin());
    SetCPSR(context.cpsr); state.fpscr = context.fpscr; fpexc = context.fpexc;
    ClearExclusiveState();
}
}
