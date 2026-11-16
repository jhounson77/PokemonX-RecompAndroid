#pragma once
#include "video_core/renderer_vulkan/vk_common.h"
#include <array>
#include <cstdint>
#include <span>
namespace Vulkan::FrameGeneration {
struct Uniforms {
    std::array<float, 16> model{};
    std::array<float, 4> input{}, output{};
    std::array<std::int32_t, 4> flags{};
    std::array<float, 4> clip{};
};
static_assert(sizeof(Uniforms) == 128);
struct Pass {
    vk::Image image;
    vk::Framebuffer framebuffer;
    vk::Pipeline pipeline;
    vk::RenderPass renderpass;
    vk::PipelineLayout layout;
    vk::DescriptorSet set;
    vk::Buffer vertices;
    vk::DeviceSize offset;
    std::uint32_t width, height;
    bool sampled, initialized;
};
inline void Copy(vk::CommandBuffer cmd, vk::Image source, vk::Image destination,
                 std::uint32_t width, std::uint32_t height, bool initialized) {
    const vk::ImageSubresourceRange range{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
    std::array<vk::ImageMemoryBarrier, 2> barriers{};
    barriers[0] = vk::ImageMemoryBarrier{.srcAccessMask = vk::AccessFlagBits::eMemoryWrite,
                                         .dstAccessMask = vk::AccessFlagBits::eTransferRead,
                                         .oldLayout = vk::ImageLayout::eTransferSrcOptimal,
                                         .newLayout = vk::ImageLayout::eTransferSrcOptimal,
                                         .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                         .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                         .image = source,
                                         .subresourceRange = range};
    barriers[1] = vk::ImageMemoryBarrier{
        .srcAccessMask = initialized ? vk::AccessFlagBits::eShaderRead : vk::AccessFlags{},
        .dstAccessMask = vk::AccessFlagBits::eTransferWrite,
        .oldLayout = initialized ? vk::ImageLayout::eGeneral : vk::ImageLayout::eUndefined,
        .newLayout = vk::ImageLayout::eTransferDstOptimal,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = destination,
        .subresourceRange = range};
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                        vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barriers);
    const vk::ImageCopy copy{{vk::ImageAspectFlagBits::eColor, 0, 0, 1},
                             {0, 0, 0},
                             {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
                             {0, 0, 0},
                             {width, height, 1}};
    cmd.copyImage(source, vk::ImageLayout::eTransferSrcOptimal, destination,
                  vk::ImageLayout::eTransferDstOptimal, copy);
    const vk::ImageMemoryBarrier readable{.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
                                          .dstAccessMask = vk::AccessFlagBits::eShaderRead,
                                          .oldLayout = vk::ImageLayout::eTransferDstOptimal,
                                          .newLayout = vk::ImageLayout::eGeneral,
                                          .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                          .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                          .image = destination,
                                          .subresourceRange = range};
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                        vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, readable);
}
inline void Draw(vk::CommandBuffer cmd, const Pass& params, const Uniforms& uniforms) {
    const auto image = params.image;
    const auto framebuffer = params.framebuffer;
    const auto width = params.width, height = params.height;
    const auto pipeline_handle = params.pipeline;
    const auto pass = params.renderpass;
    const auto layout = params.layout;
    const auto set = params.set;
    const auto offset = params.offset;
    const bool flow = params.sampled, initialized = params.initialized;
    // Finish prior reads before discarding an output attachment for the next pass.
    if (initialized) {
        const vk::ImageMemoryBarrier writable{
            .srcAccessMask =
                flow ? vk::AccessFlagBits::eShaderRead : vk::AccessFlagBits::eTransferRead,
            .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
            .oldLayout = flow ? vk::ImageLayout::eGeneral : vk::ImageLayout::eTransferSrcOptimal,
            .newLayout = flow ? vk::ImageLayout::eGeneral : vk::ImageLayout::eTransferSrcOptimal,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
        cmd.pipelineBarrier(flow ? vk::PipelineStageFlagBits::eFragmentShader
                                 : vk::PipelineStageFlagBits::eTransfer,
                            vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, {}, {},
                            writable);
    }
    cmd.setViewport(0, vk::Viewport{0, 0, float(width), float(height), 0, 1});
    cmd.setScissor(0, vk::Rect2D{{0, 0}, {width, height}});
    vk::ClearValue clear{};
    clear.color.float32[3] = 1.0f;
    const vk::RenderPassBeginInfo begin{.renderPass = pass,
                                        .framebuffer = framebuffer,
                                        .renderArea = {{0, 0}, {width, height}},
                                        .clearValueCount = 1,
                                        .pClearValues = &clear};
    cmd.beginRenderPass(begin, vk::SubpassContents::eInline);
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_handle);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, 0, set, {});
    cmd.bindVertexBuffers(0, params.vertices, offset);
    const float blend[4] = {1, 1, 1, 1};
    cmd.setBlendConstants(blend);
    cmd.pushConstants(layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                      0, sizeof(uniforms), &uniforms);
    cmd.draw(4, 1, 0, 0);
    cmd.endRenderPass();
    if (flow) {
        const vk::ImageMemoryBarrier readable{
            .srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
            .dstAccessMask = vk::AccessFlagBits::eShaderRead,
            .oldLayout = vk::ImageLayout::eTransferSrcOptimal,
            .newLayout = vk::ImageLayout::eGeneral,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput,
                            vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, readable);
    }
}
} // namespace Vulkan::FrameGeneration
