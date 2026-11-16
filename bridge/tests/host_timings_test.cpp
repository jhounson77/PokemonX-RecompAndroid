#include "common/px_host_timings.h"
#include "core/hle/service/gsp/gsp_command.h"
#include <algorithm>
#include <atomic>
#include <cassert>
#include <iostream>
#include <thread>
#include <vector>
int main() {
    using namespace PokemonXTiming;
    Take();
    {
        GuestScope scope;
        RecordReadback(50, 1024, true);
    }
    RecordReadback(30, 512, false);
    auto s = Take();
    assert(s.guest_calls == 1 && s.readbacks == 2 && s.sync_ns == 80 && s.inside_guest_ns == 50 &&
           s.staged_bytes == 1536 && s.scaled_readbacks == 1);
    assert(!Take().readbacks);
    std::atomic<unsigned> done{};
    std::vector<std::thread> threads;
    for (unsigned i = 0; i < 4; i++)
        threads.emplace_back([&] {
            for (unsigned j = 0; j < 10000; j++) {
                RecordGuest(100);
                RecordReadback(40, 512, true);
            }
            ++done;
        });
    Sample total;
    auto add = [&](Sample v) {
        total.guest_calls += v.guest_calls;
        total.guest_ns += v.guest_ns;
        total.readbacks += v.readbacks;
        total.sync_ns += v.sync_ns;
        total.staged_bytes += v.staged_bytes;
        total.scaled_readbacks += v.scaled_readbacks;
        total.inside_guest_ns += v.inside_guest_ns;
    };
    while (done < 4) {
        add(Take());
        std::this_thread::yield();
    }
    for (auto& t : threads)
        t.join();
    add(Take());
    assert(total.guest_calls == 40000 && total.guest_ns == 4000000 && total.readbacks == 40000 &&
           total.sync_ns == 1600000 && total.staged_bytes == 40000ULL * 512 &&
           total.scaled_readbacks == 40000 && total.inside_guest_ns == 0);
    Service::GSP::Command command{};
    command.id.Assign(Service::GSP::CommandId::TextureCopy);
    assert(CommandStage(static_cast<std::uint32_t>(command.id.Value())) == Stage::TextureCopy);
    TakeStages();
    assert(CommandStage(0) == Stage::Dma && CommandStage(5) == Stage::CacheFlush &&
           CommandStage(99) == Stage::Other);
    RecordStage(Stage::Submit, 10);
    RecordStage(Stage::Submit, 50);
    RecordStage(Stage::Submit, 5);
    RecordStage(Stage::Count, 999);
    auto stages = TakeStages();
    auto one = stages[static_cast<std::size_t>(Stage::Submit)];
    assert(one.calls == 3 && one.ns == 65 && one.max_ns == 50);
    assert(TakeStages()[static_cast<std::size_t>(Stage::Submit)].calls == 0);
    done = 0;
    threads.clear();
    for (unsigned i = 0; i < 4; i++)
        threads.emplace_back([&] {
            for (unsigned j = 0; j < 10000; j++)
                RecordStage(Stage::PipelineCreate, j % 100 + 1);
            ++done;
        });
    StageSample accumulated;
    auto drain = [&] {
        auto v = TakeStages()[static_cast<std::size_t>(Stage::PipelineCreate)];
        accumulated.calls += v.calls;
        accumulated.ns += v.ns;
        accumulated.max_ns = std::max(accumulated.max_ns, v.max_ns);
    };
    while (done < 4) {
        drain();
        std::this_thread::yield();
    }
    for (auto& t : threads)
        t.join();
    drain();
    assert(accumulated.calls == 40000 && accumulated.ns == 2020000 && accumulated.max_ns == 100);
    std::cout << "PASS nested same-thread scope attribution, reset windows, concurrent "
                 "record/drain totals and TLS isolation\n";
}
