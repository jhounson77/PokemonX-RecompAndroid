#include "video_core/renderer_vulkan/frame_generation_commands.h"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance, const char*);
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
namespace fg = Vulkan::FrameGeneration;
struct Image {
    vk::Image image;
    vk::DeviceMemory memory;
    vk::ImageView view;
    vk::Framebuffer fb;
    unsigned w, h;
};
struct Buffer {
    vk::Buffer buffer;
    vk::DeviceMemory memory;
    void* data;
};
static unsigned validation_errors = 0;
static VKAPI_ATTR vk::Bool32 VKAPI_CALL Debug(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                              vk::DebugUtilsMessageTypeFlagsEXT,
                                              const vk::DebugUtilsMessengerCallbackDataEXT* data,
                                              void*) {
    if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError) {
        validation_errors++;
        std::cerr << data->pMessage << "\n";
    }
    return VK_FALSE;
}
std::vector<unsigned> Bytes(const char* path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    assert(f);
    auto n = f.tellg();
    std::vector<unsigned> a((size_t(n) + 3) / 4);
    f.seekg(0);
    f.read((char*)a.data(), n);
    return a;
}
int main(int argc, char** argv) {
    std::cout << std::unitbuf;
    assert(argc == 4);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
    vk::ApplicationInfo app{.pApplicationName = "Synthetic frame generation",
                            .apiVersion = VK_API_VERSION_1_1};
    const char* layer = "VK_LAYER_KHRONOS_validation";
    const char* extension = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    vk::ValidationFeatureEnableEXT sync =
        vk::ValidationFeatureEnableEXT::eSynchronizationValidation;
    vk::ValidationFeaturesEXT features{.enabledValidationFeatureCount = 1,
                                       .pEnabledValidationFeatures = &sync};
    auto instance =
        vk::createInstance(vk::InstanceCreateInfo{.pNext = &features,
                                                  .pApplicationInfo = &app,
                                                  .enabledLayerCount = 1,
                                                  .ppEnabledLayerNames = &layer,
                                                  .enabledExtensionCount = 1,
                                                  .ppEnabledExtensionNames = &extension});
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
    auto messenger = instance.createDebugUtilsMessengerEXT(vk::DebugUtilsMessengerCreateInfoEXT{
        .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                       vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                       vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        .pfnUserCallback = Debug});
    auto physical = instance.enumeratePhysicalDevices().at(0);
    std::cout << "GPU " << physical.getProperties().deviceName.data() << '\n';
    auto families = physical.getQueueFamilyProperties();
    unsigned family = 0;
    while (!(families.at(family).queueFlags & vk::QueueFlagBits::eGraphics))
        family++;
    float priority = 1;
    vk::DeviceQueueCreateInfo q{
        .queueFamilyIndex = family, .queueCount = 1, .pQueuePriorities = &priority};
    auto device = physical.createDevice(
        vk::DeviceCreateInfo{.queueCreateInfoCount = 1, .pQueueCreateInfos = &q});
    VULKAN_HPP_DEFAULT_DISPATCHER.init(device);
    auto queue = device.getQueue(family, 0);
    auto types = physical.getMemoryProperties();
    auto type = [&](unsigned bits, vk::MemoryPropertyFlags flags) {
        for (unsigned i = 0; i < types.memoryTypeCount; i++)
            if ((bits & (1U << i)) && (types.memoryTypes[i].propertyFlags & flags) == flags)
                return i;
        throw std::runtime_error("memory type");
    };
    auto buffer = [&](size_t size, vk::BufferUsageFlags usage) {
        Buffer b{};
        b.buffer = device.createBuffer(vk::BufferCreateInfo{.size = size, .usage = usage});
        auto req = device.getBufferMemoryRequirements(b.buffer);
        b.memory = device.allocateMemory(vk::MemoryAllocateInfo{
            .allocationSize = req.size,
            .memoryTypeIndex =
                type(req.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible |
                                             vk::MemoryPropertyFlagBits::eHostCoherent)});
        device.bindBufferMemory(b.buffer, b.memory, 0);
        b.data = device.mapMemory(b.memory, 0, size);
        return b;
    };
    auto pool = device.createCommandPool(vk::CommandPoolCreateInfo{.queueFamilyIndex = family});
    auto cmd = device.allocateCommandBuffers(
        vk::CommandBufferAllocateInfo{.commandPool = pool,
                                      .level = vk::CommandBufferLevel::ePrimary,
                                      .commandBufferCount = 1})[0];
    auto sampler = device.createSampler(
        vk::SamplerCreateInfo{.magFilter = vk::Filter::eLinear,
                              .minFilter = vk::Filter::eLinear,
                              .addressModeU = vk::SamplerAddressMode::eClampToEdge,
                              .addressModeV = vk::SamplerAddressMode::eClampToEdge,
                              .addressModeW = vk::SamplerAddressMode::eClampToEdge});
    vk::DescriptorSetLayoutBinding binding{.binding = 0,
                                           .descriptorType =
                                               vk::DescriptorType::eCombinedImageSampler,
                                           .descriptorCount = 3,
                                           .stageFlags = vk::ShaderStageFlagBits::eFragment};
    auto set_layout = device.createDescriptorSetLayout(
        vk::DescriptorSetLayoutCreateInfo{.bindingCount = 1, .pBindings = &binding});
    vk::PushConstantRange push{.stageFlags = vk::ShaderStageFlagBits::eVertex |
                                             vk::ShaderStageFlagBits::eFragment,
                               .size = 128};
    auto layout =
        device.createPipelineLayout(vk::PipelineLayoutCreateInfo{.setLayoutCount = 1,
                                                                 .pSetLayouts = &set_layout,
                                                                 .pushConstantRangeCount = 1,
                                                                 .pPushConstantRanges = &push});
    vk::DescriptorPoolSize pool_size{vk::DescriptorType::eCombinedImageSampler, 24};
    auto dp = device.createDescriptorPool(
        vk::DescriptorPoolCreateInfo{.maxSets = 8, .poolSizeCount = 1, .pPoolSizes = &pool_size});
    auto module = [&](const char* path) {
        auto code = Bytes(path);
        return device.createShaderModule(
            vk::ShaderModuleCreateInfo{.codeSize = code.size() * 4, .pCode = code.data()});
    };
    auto vertex = module(argv[1]), flow_shader = module(argv[2]), resolve_shader = module(argv[3]);
    Buffer vertices = buffer(128, vk::BufferUsageFlagBits::eVertexBuffer),
           upload = buffer(1600 * 960 * 4, vk::BufferUsageFlagBits::eTransferSrc),
           readback = buffer(1600 * 960 * 4, vk::BufferUsageFlagBits::eTransferDst);
    float quads[] = {0, 0, 0, 0, 400, 0, 1, 0, 0, 240, 0, 1, 400, 240, 1, 1,
                     0, 0, 0, 0, 100, 0, 1, 0, 0, 60,  0, 1, 100, 60,  1, 1};
    unsigned cases = 0;
    for (unsigned scale : {1U, 4U})
        for (auto format : {vk::Format::eR8G8B8A8Unorm, vk::Format::eR8G8B8A8Srgb}) {
            const unsigned W = 400 * scale, H = 240 * scale;
            quads[4] = quads[12] = W;
            quads[9] = quads[13] = H;
            std::memcpy(vertices.data, quads, sizeof(quads));
            vk::AttachmentDescription attachment{.format = format,
                                                 .samples = vk::SampleCountFlagBits::e1,
                                                 .loadOp = vk::AttachmentLoadOp::eClear,
                                                 .storeOp = vk::AttachmentStoreOp::eStore,
                                                 .initialLayout = vk::ImageLayout::eUndefined,
                                                 .finalLayout =
                                                     vk::ImageLayout::eTransferSrcOptimal};
            vk::AttachmentReference reference{0, vk::ImageLayout::eGeneral};
            vk::SubpassDescription subpass{.pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
                                           .colorAttachmentCount = 1,
                                           .pColorAttachments = &reference};
            auto pass =
                device.createRenderPass(vk::RenderPassCreateInfo{.attachmentCount = 1,
                                                                 .pAttachments = &attachment,
                                                                 .subpassCount = 1,
                                                                 .pSubpasses = &subpass});
            auto image = [&](unsigned w, unsigned h) {
                Image i{};
                i.w = w;
                i.h = h;
                i.image = device.createImage(
                    vk::ImageCreateInfo{.imageType = vk::ImageType::e2D,
                                        .format = format,
                                        .extent = {w, h, 1},
                                        .mipLevels = 1,
                                        .arrayLayers = 1,
                                        .samples = vk::SampleCountFlagBits::e1,
                                        .tiling = vk::ImageTiling::eOptimal,
                                        .usage = vk::ImageUsageFlagBits::eTransferSrc |
                                                 vk::ImageUsageFlagBits::eTransferDst |
                                                 vk::ImageUsageFlagBits::eSampled |
                                                 vk::ImageUsageFlagBits::eColorAttachment});
                auto req = device.getImageMemoryRequirements(i.image);
                i.memory = device.allocateMemory(vk::MemoryAllocateInfo{
                    .allocationSize = req.size, .memoryTypeIndex = type(req.memoryTypeBits, {})});
                device.bindImageMemory(i.image, i.memory, 0);
                i.view = device.createImageView(vk::ImageViewCreateInfo{
                    .image = i.image,
                    .viewType = vk::ImageViewType::e2D,
                    .format = format,
                    .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}});
                i.fb = device.createFramebuffer(vk::FramebufferCreateInfo{.renderPass = pass,
                                                                          .attachmentCount = 1,
                                                                          .pAttachments = &i.view,
                                                                          .width = w,
                                                                          .height = h,
                                                                          .layers = 1});
                return i;
            };
            Image input = image(W, H), source = image(W, H), previous = image(W, H),
                  current = image(W, H), flow = image(100, 60), output = image(W, H);
            auto pipeline = [&](vk::ShaderModule fragment) {
                vk::PipelineShaderStageCreateInfo stages[] = {
                    {.stage = vk::ShaderStageFlagBits::eVertex, .module = vertex, .pName = "main"},
                    {.stage = vk::ShaderStageFlagBits::eFragment,
                     .module = fragment,
                     .pName = "main"}};
                vk::VertexInputBindingDescription b{0, 16, vk::VertexInputRate::eVertex};
                vk::VertexInputAttributeDescription attrs[] = {
                    {0, 0, vk::Format::eR32G32Sfloat, 0}, {1, 0, vk::Format::eR32G32Sfloat, 8}};
                vk::PipelineVertexInputStateCreateInfo vi{.vertexBindingDescriptionCount = 1,
                                                          .pVertexBindingDescriptions = &b,
                                                          .vertexAttributeDescriptionCount = 2,
                                                          .pVertexAttributeDescriptions = attrs};
                vk::PipelineInputAssemblyStateCreateInfo ia{
                    .topology = vk::PrimitiveTopology::eTriangleStrip};
                vk::PipelineViewportStateCreateInfo vp{.viewportCount = 1, .scissorCount = 1};
                vk::PipelineRasterizationStateCreateInfo rs{.polygonMode = vk::PolygonMode::eFill,
                                                            .cullMode = vk::CullModeFlagBits::eNone,
                                                            .lineWidth = 1};
                vk::PipelineMultisampleStateCreateInfo ms{.rasterizationSamples =
                                                              vk::SampleCountFlagBits::e1};
                vk::PipelineColorBlendAttachmentState cb{
                    .colorWriteMask =
                        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
                vk::PipelineColorBlendStateCreateInfo blend{.attachmentCount = 1,
                                                            .pAttachments = &cb};
                vk::DynamicState states[] = {vk::DynamicState::eViewport,
                                             vk::DynamicState::eScissor,
                                             vk::DynamicState::eBlendConstants};
                vk::PipelineDynamicStateCreateInfo dynamic{.dynamicStateCount = 3,
                                                           .pDynamicStates = states};
                auto r = device.createGraphicsPipeline(
                    {}, vk::GraphicsPipelineCreateInfo{.stageCount = 2,
                                                       .pStages = stages,
                                                       .pVertexInputState = &vi,
                                                       .pInputAssemblyState = &ia,
                                                       .pViewportState = &vp,
                                                       .pRasterizationState = &rs,
                                                       .pMultisampleState = &ms,
                                                       .pColorBlendState = &blend,
                                                       .pDynamicState = &dynamic,
                                                       .layout = layout,
                                                       .renderPass = pass});
                assert(r.result == vk::Result::eSuccess);
                return r.value;
            };
            auto flow_pipeline = pipeline(flow_shader), resolve_pipeline = pipeline(resolve_shader);
            auto set = [&](Image& a, Image& b, Image& c) {
                auto ds = device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
                    .descriptorPool = dp, .descriptorSetCount = 1, .pSetLayouts = &set_layout})[0];
                vk::DescriptorImageInfo infos[] = {{sampler, a.view, vk::ImageLayout::eGeneral},
                                                   {sampler, b.view, vk::ImageLayout::eGeneral},
                                                   {sampler, c.view, vk::ImageLayout::eGeneral}};
                device.updateDescriptorSets(
                    vk::WriteDescriptorSet{.dstSet = ds,
                                           .dstBinding = 0,
                                           .descriptorCount = 3,
                                           .descriptorType =
                                               vk::DescriptorType::eCombinedImageSampler,
                                           .pImageInfo = infos},
                    {});
                return ds;
            };
            auto input_set = set(input, input, input), flow_set = set(previous, current, current),
                 resolve_set = set(previous, current, flow);
            auto draw = [&](Image& i, vk::Pipeline pipe, vk::DescriptorSet ds, bool sampled,
                            bool initialized, int mode) {
                fg::Uniforms u{};
                u.model = {2.f / i.w, 0, 0, -1, 0, 2.f / i.h, 0, -1, 0, 0, 1, 0, 0, 0, 0, 1};
                u.input = {float(i.w), float(i.h), 1.f / 400, 1.f / 240};
                u.flags[3] = mode;
                fg::Draw(cmd,
                         fg::Pass{i.image, i.fb, pipe, pass, layout, ds, vertices.buffer,
                                  i.w == 100 ? 64U : 0U, i.w, i.h, sampled, initialized},
                         u);
            };
            bool input_initialized = false, source_initialized = false;
            std::vector<std::uint8_t> pictures[2];
            auto submit = [&]() {
                cmd.end();
                queue.submit(vk::SubmitInfo{.commandBufferCount = 1, .pCommandBuffers = &cmd}, {});
                queue.waitIdle();
                device.resetCommandPool(pool);
            };
            for (unsigned n = 0; n < 2; n++) {
                std::vector<std::uint8_t> data(W * H * 4);
                for (unsigned y = 0; y < H; y++)
                    for (unsigned x = 0; x < W; x++) {
                        unsigned at = (y * W + x) * 4;
                        bool square = x >= (140 + n * 8) * scale && x < (164 + n * 8) * scale &&
                                      y >= 100 * scale && y < 124 * scale;
                        unsigned c = ((x / (8 * scale) + y / (8 * scale)) % 2) * 30 + 30;
                        data[at] = square ? 30 : c;
                        data[at + 1] = square ? 255 : c;
                        data[at + 2] = square ? 20 : c;
                        data[at + 3] = 255;
                    }
                pictures[n] = data;
                std::memcpy(upload.data, data.data(), data.size());
                cmd.begin(vk::CommandBufferBeginInfo{});
                vk::ImageMemoryBarrier dest{
                    .srcAccessMask =
                        input_initialized ? vk::AccessFlagBits::eShaderRead : vk::AccessFlags{},
                    .dstAccessMask = vk::AccessFlagBits::eTransferWrite,
                    .oldLayout =
                        input_initialized ? vk::ImageLayout::eGeneral : vk::ImageLayout::eUndefined,
                    .newLayout = vk::ImageLayout::eTransferDstOptimal,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = input.image,
                    .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
                cmd.pipelineBarrier(vk::PipelineStageFlagBits::eFragmentShader,
                                    vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, dest);
                vk::BufferImageCopy region{
                    .imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
                    .imageExtent = {W, H, 1}};
                cmd.copyBufferToImage(upload.buffer, input.image,
                                      vk::ImageLayout::eTransferDstOptimal, region);
                dest.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
                dest.dstAccessMask = vk::AccessFlagBits::eShaderRead;
                dest.oldLayout = vk::ImageLayout::eTransferDstOptimal;
                dest.newLayout = vk::ImageLayout::eGeneral;
                cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                    vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, dest);
                draw(source, resolve_pipeline, input_set, false, source_initialized, 0);
                fg::Copy(cmd, source.image, n ? current.image : previous.image, W, H, false);
                submit();
                input_initialized = source_initialized = true;
            }
            cmd.begin(vk::CommandBufferBeginInfo{});
            draw(flow, flow_pipeline, flow_set, true, false, 1);
            draw(output, resolve_pipeline, resolve_set, false, false, 1);
            vk::ImageMemoryBarrier readable{
                .srcAccessMask = vk::AccessFlagBits::eMemoryWrite,
                .dstAccessMask = vk::AccessFlagBits::eTransferRead,
                .oldLayout = vk::ImageLayout::eTransferSrcOptimal,
                .newLayout = vk::ImageLayout::eTransferSrcOptimal,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = output.image,
                .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
            cmd.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                                vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, readable);
            vk::BufferImageCopy region{
                .imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
                .imageExtent = {W, H, 1}};
            cmd.copyImageToBuffer(output.image, vk::ImageLayout::eTransferSrcOptimal,
                                  readback.buffer, region);
            submit();
            auto bytes = (const std::uint8_t*)readback.data;
            unsigned different_before = 0, different_after = 0;
            double green_sum = 0, green_x = 0;
            for (unsigned y = 0; y < H; y++)
                for (unsigned x = 0; x < W; x++) {
                    unsigned at = (y * W + x) * 4;
                    different_before += std::memcmp(bytes + at, pictures[0].data() + at, 3) != 0;
                    different_after += std::memcmp(bytes + at, pictures[1].data() + at, 3) != 0;
                    if (bytes[at + 1] > 150 && bytes[at + 1] > bytes[at] + 70) {
                        green_sum++;
                        green_x += x;
                    }
                }
            assert(different_before > 20 && different_after > 20 && green_sum > 0);
            std::cout << "motion case scale=" << scale << " format=" << int(format)
                      << " distinct pixels=" << different_before << '/' << different_after
                      << " centroid=" << green_x / green_sum << '\n';
            assert((green_x / green_sum + .5) / scale - .5 > 154 &&
                   (green_x / green_sum + .5) / scale - .5 < 157);
            ++cases;
            // Reuse the same history/flow/output resources. Equal inputs must not shimmer.
            cmd.begin(vk::CommandBufferBeginInfo{});
            draw(source, resolve_pipeline, input_set, false, true, 0);
            fg::Copy(cmd, source.image, previous.image, W, H, true);
            draw(flow, flow_pipeline, flow_set, true, true, 1);
            draw(output, resolve_pipeline, resolve_set, false, true, 1);
            cmd.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                                vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, readable);
            cmd.copyImageToBuffer(output.image, vk::ImageLayout::eTransferSrcOptimal,
                                  readback.buffer, region);
            submit();
            for (unsigned i = 0; i < W * H * 4; i++)
                assert(std::abs(int(((std::uint8_t*)readback.data)[i]) - int(pictures[1][i])) <= 1);
            // An unrelated bright scene should fall back to the current picture rather than
            // crossfade.
            std::memset(upload.data, 255, W * H * 4);
            cmd.begin(vk::CommandBufferBeginInfo{});
            vk::ImageMemoryBarrier upload_barrier{
                .srcAccessMask = vk::AccessFlagBits::eShaderRead,
                .dstAccessMask = vk::AccessFlagBits::eTransferWrite,
                .oldLayout = vk::ImageLayout::eGeneral,
                .newLayout = vk::ImageLayout::eTransferDstOptimal,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = input.image,
                .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
            cmd.pipelineBarrier(vk::PipelineStageFlagBits::eFragmentShader,
                                vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, upload_barrier);
            cmd.copyBufferToImage(upload.buffer, input.image, vk::ImageLayout::eTransferDstOptimal,
                                  region);
            upload_barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            upload_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
            upload_barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
            upload_barrier.newLayout = vk::ImageLayout::eGeneral;
            cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {},
                                upload_barrier);
            draw(source, resolve_pipeline, input_set, false, true, 0);
            fg::Copy(cmd, source.image, current.image, W, H, true);
            draw(flow, flow_pipeline, flow_set, true, true, 1);
            draw(output, resolve_pipeline, resolve_set, false, true, 1);
            cmd.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                                vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, readable);
            cmd.copyImageToBuffer(output.image, vk::ImageLayout::eTransferSrcOptimal,
                                  readback.buffer, region);
            submit();
            for (unsigned i = 0; i < W * H * 4; i++)
                assert(((std::uint8_t*)readback.data)[i] == 255);
            for (auto* i : {&input, &source, &previous, &current, &flow, &output}) {
                device.destroyFramebuffer(i->fb);
                device.destroyImageView(i->view);
                device.destroyImage(i->image);
                device.freeMemory(i->memory);
            }
            device.destroyPipeline(flow_pipeline);
            device.destroyPipeline(resolve_pipeline);
            device.destroyRenderPass(pass);
            device.resetDescriptorPool(dp);
        }
    device.waitIdle();
    for (auto* b : {&vertices, &upload, &readback}) {
        device.unmapMemory(b->memory);
        device.destroyBuffer(b->buffer);
        device.freeMemory(b->memory);
    }
    device.destroyShaderModule(vertex);
    device.destroyShaderModule(flow_shader);
    device.destroyShaderModule(resolve_shader);
    device.destroyDescriptorPool(dp);
    device.destroyPipelineLayout(layout);
    device.destroyDescriptorSetLayout(set_layout);
    device.destroySampler(sampler);
    device.destroyCommandPool(pool);
    device.destroy();
    instance.destroyDebugUtilsMessengerEXT(messenger);
    instance.destroy();
    assert(validation_errors == 0);
    std::cout << "PASS " << cases
              << " real Vulkan synthetic motion/static/cut tests at1x/4x using production commands "
                 "and shaders; zero validation errors\n";
}
