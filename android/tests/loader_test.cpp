#include "application/boot.h"
#include "domain/ctr/ncsd.h"
#include "support/synthetic_ctr_image.h"
#include "support/synthetic_romfs.h"
#include <iostream>
#include <stdexcept>
using namespace hexlith;
int main() {
    auto spec = test::DefaultImageSpec();
    std::vector<std::uint8_t> code((spec.text.pages + spec.read_only.pages + spec.data.pages) * domain::kernel::layout::PageSize);
    code[0] = 3; code[3] = 0xef; // ARM SVC ExitProcess, documented by 3dbrew.
    spec.exefs_files = {{.name = ".code", .data = code}};
    spec.romfs = test::BuildSyntheticRomFs(test::DefaultRomFsSpec());
    const auto image = test::BuildSyntheticImage(spec).bytes;
    auto ncsd = domain::ctr::ParseNcsdHeader(image);
    if (!ncsd) throw std::runtime_error(ncsd.Failure().Message());
    auto part = domain::ctr::PartitionContents(image, ncsd.Value(), 0);
    if (!part) throw std::runtime_error(part.Failure().Message());
    auto title = application::BootedTitle::FromNcch(part.Value());
    if (!title) throw std::runtime_error(title.Failure().Message());
    auto stop = title.Value()->Kernel().Run(4096);
    if (stop.reason != domain::kernel::KernelStopReason::ProcessExited) return 1;
    if (title.Value()->Kernel().Executed() != 1) return 2;
    if (application::BootedTitle::FromNcch(part.Value().first(512)).Ok()) return 3;
    if (application::BootedTitle::FromNcch({}).Ok()) return 4;
    std::cout << "Standalone NCCH boots a synthetic program; truncated images rejected\n";
}
