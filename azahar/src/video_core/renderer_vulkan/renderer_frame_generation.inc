// Display-only frame generation. This code never accesses guest memory or clocks.
void RendererVulkan::DestroyFrameHistory() {
    auto device = instance.GetDevice();
    auto destroy = [&](Frame& f) {
        if (f.framebuffer)
            device.destroyFramebuffer(f.framebuffer);
        if (f.image_view)
            device.destroyImageView(f.image_view);
        if (f.image)
            vmaDestroyImage(instance.GetAllocator(), f.image, f.allocation);
        f = {};
    };
    for (auto& f : frame_history)
        destroy(f);
    destroy(flow_frame);
    history_count = 0;
    history_initialized = {};
    flow_initialized = false;
}

void RendererVulkan::CopyFrameHistory(Frame* frame) {
    const unsigned slot = history_count ? history_latest ^ 1 : 0;
    Frame& dst = frame_history[slot];
    const bool initialized = history_initialized[slot];
    const auto source = frame->image, destination = dst.image;
    const u32 width = frame->width, height = frame->height;
    scheduler.Record([source, destination, width, height, initialized](vk::CommandBuffer cmd) {
        FrameGeneration::Copy(cmd, source, destination, width, height, initialized);
    });
    history_initialized[slot] = true;
    history_latest = slot;
    history_count = std::min(history_count + 1, 2U);
}

void RendererVulkan::DrawFramePass(Frame* target, unsigned pipeline,
                                   std::array<vk::ImageView, 3> images, int mode) {
    renderpass_cache.EndRendering();
    const auto set = present_heap.Commit();
    for (unsigned i = 0; i < 3; i++)
        update_queue.AddImageSampler(set, 0, i, images[i], present_samplers[0]);
    const ScreenRectVertex vertices[] = {{0, 0, 0, 0},
                                         {float(target->width), 0, 1, 0},
                                         {0, float(target->height), 0, 1},
                                         {float(target->width), float(target->height), 1, 1}};
    auto [data, offset, invalidate] = vertex_buffer.Map(sizeof(vertices), 16);
    std::memcpy(data, vertices, sizeof(vertices));
    vertex_buffer.Commit(sizeof(vertices));
    FrameGeneration::Uniforms uniforms{};
    uniforms.model = MakeOrthographicMatrix(target->width, target->height);
    uniforms.input = {float(target->width), float(target->height), 1.f / 400.f, 1.f / 240.f};
    uniforms.flags[3] = mode;
    FrameGeneration::Pass params{target->image,
                                 target->framebuffer,
                                 present_pipelines[pipeline],
                                 main_present_window.Renderpass(),
                                 *present_pipeline_layout,
                                 set,
                                 vertex_buffer.Handle(),
                                 offset,
                                 target->width,
                                 target->height,
                                 pipeline == 3,
                                 pipeline == 3 ? flow_initialized : true};
    scheduler.Record([params, uniforms](vk::CommandBuffer cmd) {
        FrameGeneration::Draw(cmd, params, uniforms);
    });
    const bool flow = pipeline == 3;
    if (flow)
        flow_initialized = true;
}

bool RendererVulkan::PresentWithFrameGeneration(PresentWindow& window,
                                                const Layout::FramebufferLayout& layout,
                                                bool flipped) {
    clear_color.float32[0] = Settings::values.bg_red.GetValue();
    clear_color.float32[1] = Settings::values.bg_green.GetValue();
    clear_color.float32[2] = Settings::values.bg_blue.GetValue();
    clear_color.float32[3] = 1.f;
    const bool updated = Core::PerfStats::game_frames_updated;
    Core::PerfStats::game_frames_updated = false;
    const auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(
                         std::chrono::steady_clock::now().time_since_epoch())
                         .count();
    const bool eligible = bool(present_pipelines[3]) && bool(present_pipelines[4]) &&
                          !frame_generation_failed && !PokemonXUI::popup_requested &&
                          !PokemonXUI::battle_menu_full;
    auto decision =
        frame_policy.Step(PokemonXFrames::target.load(), updated, eligible,
                          PokemonXFrames::epoch.load() + PokemonXUI::generation.load(), now);
    if (decision.reset)
        history_count = 0;
    if (decision.kind == PokemonXFrames::Kind::Skip) {
        scheduler.Flush();
        screenRendered = true;
        return true;
    }
    Frame* frame = window.GetRenderFrame();
    if (frame->width != layout.width || frame->height != layout.height) {
        window.WaitPresent();
        scheduler.Finish();
        window.RecreateFrame(frame, layout.width, layout.height);
        frame_policy.Reset();
        history_count = 0;
    }
    try {
        if (decision.capture) {
            if (frame_history[0].width != layout.width ||
                frame_history[0].height != layout.height) {
                scheduler.Finish();
                window.WaitPresent();
                DestroyFrameHistory();
                const auto features = instance.GetPhysicalDevice()
                                          .getFormatProperties(window.GetSurfaceFormat())
                                          .optimalTilingFeatures;
                if (!(features & vk::FormatFeatureFlagBits::eSampledImage))
                    throw std::runtime_error("Surface format cannot be sampled");
                for (auto& f : frame_history)
                    window.RecreateFrame(&f, layout.width, layout.height, true);
                window.RecreateFrame(&flow_frame, 100, 60, true);
            }
            DrawScreens(frame, layout, flipped);
            CopyFrameHistory(frame);
        } else if (updated) {
            history_count = 0;
        }
        if (history_count == 2 && decision.kind != PokemonXFrames::Kind::Original) {
            const std::array views{frame_history[history_latest ^ 1].image_view,
                                   frame_history[history_latest].image_view, flow_frame.image_view};
            const bool middle = decision.kind == PokemonXFrames::Kind::Intermediate;
            if (middle) {
                auto flow_views = views;
                flow_views[2] = views[1];
                DrawFramePass(&flow_frame, 3, flow_views, 1);
            }
            DrawFramePass(frame, 4, views, middle ? 1 : 0);
            if (middle)
                ++PokemonXFrames::generated;
        } else if (!decision.capture) {
            DrawScreens(frame, layout, flipped);
        }
    } catch (const std::exception& e) {
        LOG_WARNING(Render_Vulkan, "Frame interpolation disabled: {}", e.what());
        frame_generation_failed = true;
        history_count = 0;
        DrawScreens(frame, layout, flipped);
    }
    scheduler.Flush(frame->render_ready);
    window.Present(frame);
    screenRendered = true;
    ++PokemonXFrames::submitted;
    return true;
}
