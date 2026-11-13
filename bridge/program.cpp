#include "program.h"
#include "common/px_screen_interface.h"
#include <stdexcept>
#include "core/sha256.h"
#include "domain/ctr/cro.h"
#include "common/logging/log.h"
namespace PokemonX {
namespace { std::shared_ptr<Program> current; }
std::shared_ptr<Program> Program::FromEntries(hexlith::domain::lift::runtime::EntryTable entries) { auto p=std::make_shared<Program>();p->embedded_=std::move(entries);return p; }
void SetProgram(std::shared_ptr<Program> p) { PokemonXUI::battle_loaded=false;PokemonXUI::submenu_loaded=false;PokemonXUI::battle_menu_exit=false;PokemonXUI::scene=0;PokemonXUI::decision_hidden=false;PokemonXUI::popup_requested=false;PokemonXUI::battle_menu_full=false;++PokemonXUI::generation;current = std::move(p); }
std::shared_ptr<Program> GetProgram() { return current; }
std::shared_ptr<Program> Program::Load(const std::string& main, const std::string& modules,
                                     const std::string& digest) {
    auto p = std::make_shared<Program>();
    auto m = hexlith::adapters::lifted::LiftedLibrary::Open(main, digest);
    if (!m) throw std::runtime_error(m.Failure().Message());
    p->main_ = std::move(m.Value());p->expected_digest_=digest;
    auto c = hexlith::adapters::lifted::ModulesLibrary::Open(modules);
    if (!c) throw std::runtime_error(c.Failure().Message());
    p->modules_ = std::move(c.Value()); p->modules_->OfferTo(p->loaded_);
    LOG_INFO(Core_ARM11, "Pokemon X native tables: main and {} modules", p->modules_->Count());
    return p;
}
bool Program::VerifyCode(std::span<const std::uint8_t> bytes) const { return expected_digest_.empty() || hexlith::core::Sha256::Of(bytes).ToHex()==expected_digest_; }
ModuleImage Program::Inspect(std::span<const std::uint8_t> bytes) const {
    const auto header = hexlith::domain::ctr::ParseCroHeader(bytes);
    if (!header) return {};
    for (const auto& s : header.Value().segments) {
        if (s.type == hexlith::domain::ctr::CroSegmentType::Code && s.size != 0 &&
            s.offset <= bytes.size() && s.size <= bytes.size()-s.offset) {
            return {header.Value().name, hexlith::core::Sha256::Of(bytes.subspan(s.offset,s.size)).ToHex(),
                    static_cast<std::uint32_t>(s.offset), static_cast<std::uint32_t>(s.size)};
        }
    }
    return {};
}
void Program::Loaded(const ModuleImage& image, std::uint32_t base) {
    if (image.name.empty()) return;
    if(image.name=="DllBattle"){++PokemonXUI::generation;submenu_bases_.clear();PokemonXUI::submenu_loaded=false;PokemonXUI::decision_hidden=false;PokemonXUI::scene=0;PokemonXUI::battle_loaded=true;}
    if(image.name=="DllField"){++PokemonXUI::generation;if(PokemonXUI::battle_menu_full)PokemonXUI::ClosePopup();submenu_bases_.clear();PokemonXUI::submenu_loaded=false;PokemonXUI::decision_hidden=false;PokemonXUI::battle_loaded=false;PokemonXUI::scene=0;}
    if(PokemonXUI::battle_loaded&&(image.name=="DllBag"||image.name=="DllPokeList"||image.name=="DllPokeViewer"||image.name=="DllBattlePartySelect"||image.name=="DllPoke3Select")){submenu_bases_.insert(base);PokemonXUI::submenu_loaded=true;PokemonXUI::battle_menu_exit=false;}
    const auto result = loaded_.Loaded(image.name,base,image.offset,image.size,image.digest);
    LOG_INFO(Core_ARM11, "CRO {} at {:08X}: native binding outcome {}",image.name,base,result==hexlith::domain::lift::runtime::LoadedModules::Outcome::Lifted?"native AOT":result==hexlith::domain::lift::runtime::LoadedModules::Outcome::DigestDiffers?"interpreter (digest differs)":"interpreter (no table)");
}
void Program::Unloaded(const std::string& name, std::uint32_t base) {
    if (name == "DllBattle") {
        ++PokemonXUI::generation;
        if (PokemonXUI::battle_menu_full) PokemonXUI::ClosePopup();
        submenu_bases_.clear();
        PokemonXUI::battle_loaded = false;
        PokemonXUI::scene = 0;
    }
    const bool tracked_menu = submenu_bases_.erase(base) != 0;
    // The game can reuse a menu loaded before battle, before tracking began.
    const bool reused_menu = PokemonXUI::battle_loaded &&
        (name == "DllBag" || name == "DllPokeList" || name == "DllBattlePartySelect" ||
         name == "DllPoke3Select" || name == "DllPokeViewer");
    const bool was_menu = tracked_menu || reused_menu;
    if (was_menu) {
        PokemonXUI::scene = 0;
        // Residency is not visibility. Bag/party modules can close while their
        // helpers remain loaded. A summary viewer closing alone stays in party.
        if (name != "DllPokeViewer" && PokemonXUI::battle_menu_full) {
            ++PokemonXUI::generation;
            PokemonXUI::battle_menu_exit = true;
            LOG_INFO(Core_ARM11,
                     "Battle menu {} unloaded; scheduling primary screen return despite resident helpers",
                     name);
        }
    }
    PokemonXUI::submenu_loaded = !submenu_bases_.empty();
    PokemonXUI::force_probe = true;
    loaded_.Unloaded(name, base);
}
}
