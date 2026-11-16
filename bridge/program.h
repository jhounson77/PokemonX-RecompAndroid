#pragma once
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "adapters/lifted/lifted_library.h"
#include "adapters/lifted/modules_library.h"
#include "domain/lift/runtime.h"
namespace PokemonX {
struct ModuleImage {
    std::string name, digest;
    std::uint32_t offset = 0, size = 0;
};
class Program {
public:
    static std::shared_ptr<Program> Load(const std::string& main, const std::string& modules,
                                         const std::string& verified_digest,
                                         const std::string& supplemental_modules = {},
                                         const std::string& supplemental_main = {});
    static std::shared_ptr<Program> FromEntries(hexlith::domain::lift::runtime::EntryTable entries);
    const auto& Entries() const { return main_ ? main_->Entries() : embedded_; }
    auto& Modules() { return loaded_; }
    bool IsUpdate15() const {return expected_digest_=="7dc08ac5e2a87adabc31c304e1dfa3120a598ef072441983d09c96768cf67e74";}
    bool VerifyCode(std::span<const std::uint8_t> bytes) const;
    ModuleImage Inspect(std::span<const std::uint8_t> bytes) const;
    void Loaded(const ModuleImage& image, std::uint32_t base);
    void Unloaded(const std::string& name, std::uint32_t base);
private:
    std::set<std::uint32_t> submenu_bases_;
    std::string expected_digest_;
    hexlith::domain::lift::runtime::EntryTable embedded_;
    std::unique_ptr<hexlith::adapters::lifted::LiftedLibrary> main_;
    std::unique_ptr<hexlith::adapters::lifted::LiftedLibrary> main_resume_;
    std::unique_ptr<hexlith::adapters::lifted::ModulesLibrary> modules_;
    std::unique_ptr<hexlith::adapters::lifted::ModulesLibrary> supplemental_;
    hexlith::domain::lift::runtime::LoadedModules loaded_;
};
// One running title per process, set before Core::System::Load and cleared
// only after System::Shutdown has destroyed all CPU cores.
void SetProgram(std::shared_ptr<Program> program);
std::shared_ptr<Program> GetProgram();
}
