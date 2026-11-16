#pragma once

#include "core/arm/arm_interface.h"
#include "core/hle/kernel/svc.h"
#include "domain/lift/engine.h"

namespace PokemonX {

// A resumable Hexlith engine inside Azahar's scheduler. Game code and memory
// are not copied into Azahar or this adapter; Azahar owns mapping/coherence.
class ARMNative final : public Core::ARM_Interface,
                        private hexlith::domain::memory::ExternalMemory {
public:
    ARMNative(Core::System& system, Memory::MemorySystem& memory, u32 id,
              std::shared_ptr<Core::Timing::Timer> timer,
              const hexlith::domain::lift::runtime::EntryTable& entries,
              hexlith::domain::lift::runtime::LoadedModules* modules);
    void Run() override;
    void Step() override;
    void ClearInstructionCache() override {}
    void InvalidateCacheRange(u32 address, std::size_t length) override;
    void ClearExclusiveState() override;
    void SetPageTable(const std::shared_ptr<Memory::PageTable>& table) override;
    void SetPC(u32 value) override;
    u32 GetPC() const override;
    u32 GetReg(int index) const override;
    void SetReg(int index, u32 value) override;
    u32 GetVFPReg(int index) const override;
    void SetVFPReg(int index, u32 value) override;
    u32 GetVFPSystemReg(VFPSystemRegister reg) const override;
    void SetVFPSystemReg(VFPSystemRegister reg, u32 value) override;
    u32 GetCPSR() const override;
    void SetCPSR(u32 value) override;
    u32 GetCP15Register(CP15Register reg) const override;
    void SetCP15Register(CP15Register reg, u32 value) override;
    void SaveContext(ThreadContext& context) override;
    void LoadContext(const ThreadContext& context) override;
    void PrepareReschedule() override { reschedule = true; }
    bool HasSingleInstructionBreakAccuracy() override { return false; }
    const auto& Statistics() const { return engine.Counted(); }
    auto RetiredStatistics() const { return engine.RetiredStatistics(); }
    const auto& FallbackStatistics() const { return engine.FallbackStatistics(); }
    const auto& DecodeStatistics() const { return engine.DecodeStatistics(); }

protected:
    std::shared_ptr<Memory::PageTable> GetPageTable() const override;

private:
    bool Read(u32 address, std::span<u8> bytes) override;
    bool Write(u32 address, std::span<const u8> bytes) override;
    void Execute(u64 limit);
    Memory::MemorySystem& memory;
    Kernel::SVCContext svc;
    hexlith::domain::arm::CpuState state;
    hexlith::domain::memory::ProcessMemory guest_memory;
    hexlith::domain::lift::Engine engine;
    std::shared_ptr<Memory::PageTable> page_table;
    u32 misc_cpsr = 0x10;
    u32 fpexc = 0;
    bool reschedule = false;
};
}
