#include "adapters/vulkan/vulkan_context.h"
#include "adapters/vulkan/pipeline_cache_store.h"
#include <filesystem>
#include <fstream>

#include <array>
#include <format>
#include <optional>
#include <string_view>

namespace hexlith::adapters::vulkan {
namespace {

constexpr const char* ValidationLayer = "VK_LAYER_KHRONOS_validation";

VKAPI_ATTR VkBool32 VKAPI_CALL
OnValidationMessage(VkDebugUtilsMessageSeverityFlagBitsEXT /*severity*/,
                    VkDebugUtilsMessageTypeFlagsEXT /*type*/,
                    const VkDebugUtilsMessengerCallbackDataEXT* data,
                    void* context) {
    static_cast<VulkanContext*>(context)->NoteValidationMessage(data->pMessage);
    return VK_FALSE;
}

bool LayerAvailable(const char* name) {
    std::uint32_t count = 0;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    std::vector<VkLayerProperties> layers(count);
    vkEnumerateInstanceLayerProperties(&count, layers.data());
    for (const VkLayerProperties& layer : layers) {
        if (std::string_view{static_cast<const char*>(layer.layerName)} == name) {
            return true;
        }
    }
    return false;
}

}  // namespace

core::Status Check(VkResult result, const char* call) {
    if (result != VK_SUCCESS) {
        return core::Error{
            std::format("{} failed with VkResult {}", call, static_cast<int>(result))};
    }
    return {};
}

core::Result<std::unique_ptr<VulkanContext>> VulkanContext::Create(const VulkanOptions& options) {
    std::unique_ptr<VulkanContext> context{new VulkanContext()};
    if (const core::Status instance = context->CreateInstance(options); !instance) {
        return instance.Failure();
    }
    if (const core::Status chosen = context->ChooseDevice(options); !chosen) {
        return chosen.Failure();
    }
    if (const core::Status created = context->CreateDevice(); !created) {
        return created.Failure();
    }
    context->CreatePipelineCache(options.pipeline_cache_directory);
    return context;
}

bool VulkanContext::SupportsRgbaBlit()const{VkFormatProperties p{};vkGetPhysicalDeviceFormatProperties(physical_,VK_FORMAT_R8G8B8A8_UNORM,&p);constexpr auto flags=VK_FORMAT_FEATURE_BLIT_SRC_BIT|VK_FORMAT_FEATURE_BLIT_DST_BIT;return (p.optimalTilingFeatures&flags)==flags;}

void VulkanContext::CreatePipelineCache(const std::string& directory) {
    VkPhysicalDeviceProperties properties{};vkGetPhysicalDeviceProperties(physical_,&properties);
    std::array<std::uint8_t,16> uuid{};std::copy_n(properties.pipelineCacheUUID,16,uuid.begin());
    std::vector<std::uint8_t> initial;
    if(!directory.empty()){
        std::error_code error;std::filesystem::create_directories(directory,error);
        if(!error){
            std::string id;for(auto byte:uuid)id+=std::format("{:02x}",byte);
            pipeline_cache_path_=directory+std::format("/pipeline-v1-{}-{}-{}-{}.bin",properties.vendorID,properties.deviceID,properties.driverVersion,id);
            auto length=std::filesystem::file_size(pipeline_cache_path_,error);
            if(!error&&length>=56&&length<=cache_store::MaxBytes+24){
                std::vector<std::uint8_t> packed(length);std::ifstream in(pipeline_cache_path_,std::ios::binary);
                if(in.read(reinterpret_cast<char*>(packed.data()),static_cast<std::streamsize>(packed.size())))initial=cache_store::Unpack(packed,properties.vendorID,properties.deviceID,uuid);
            }
        }
    }
    // No EXTERNALLY_SYNCHRONIZED flag: concurrent pipeline creation uses the
    // driver's default cache synchronization, including specialization workers.
    VkPipelineCacheCreateInfo info{.sType=VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,.pNext=nullptr,.flags=0,.initialDataSize=initial.size(),.pInitialData=initial.empty()?nullptr:initial.data()};
    auto made=vkCreatePipelineCache(device_,&info,nullptr,&pipeline_cache_);
    if(made!=VK_SUCCESS&&!initial.empty()){info.initialDataSize=0;info.pInitialData=nullptr;pipeline_cache_=VK_NULL_HANDLE;made=vkCreatePipelineCache(device_,&info,nullptr,&pipeline_cache_);initial.clear();}
    if(made==VK_SUCCESS)loaded_cache_bytes_=initial.size();else pipeline_cache_=VK_NULL_HANDLE;
}

// Called after backend workers have joined, when nothing can mutate the cache.
void VulkanContext::SavePipelineCache(){
    if(pipeline_cache_==VK_NULL_HANDLE||pipeline_cache_path_.empty())return;
    std::size_t size=0;if(vkGetPipelineCacheData(device_,pipeline_cache_,&size,nullptr)!=VK_SUCCESS||size<32||size>cache_store::MaxBytes)return;
    std::vector<std::uint8_t> bytes(size);if(vkGetPipelineCacheData(device_,pipeline_cache_,&size,bytes.data())!=VK_SUCCESS)return;bytes.resize(size);
    auto packed=cache_store::Pack(bytes);const auto temporary=pipeline_cache_path_+".tmp";
    {std::ofstream out(temporary,std::ios::binary|std::ios::trunc);if(!out.write(reinterpret_cast<const char*>(packed.data()),static_cast<std::streamsize>(packed.size())))return;out.flush();if(!out)return;}
    std::error_code error;std::filesystem::rename(temporary,pipeline_cache_path_,error);
}

core::Status VulkanContext::CreateInstance(const VulkanOptions& options) {
    const bool validation = options.validation && LayerAvailable(ValidationLayer);
    VkApplicationInfo application{};
    application.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application.pApplicationName = "Hexlith";
    application.apiVersion = VK_API_VERSION_1_2;

    VkDebugUtilsMessengerCreateInfoEXT messenger{};
    messenger.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    messenger.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    // The loader's own messages, such as a driver it skips, are not about
    // what Hexlith asks of Vulkan.
    messenger.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    messenger.pfnUserCallback = OnValidationMessage;
    messenger.pUserData = this;

    const std::array<const char*, 1> layers = {ValidationLayer};
    const std::array<const char*, 1> extensions = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
    VkInstanceCreateInfo create{};
    create.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create.pApplicationInfo = &application;
    if (validation) {
        create.pNext = &messenger;
        create.enabledLayerCount = 1;
        create.ppEnabledLayerNames = layers.data();
        create.enabledExtensionCount = 1;
        create.ppEnabledExtensionNames = extensions.data();
    }
    if (const core::Status created =
            Check(vkCreateInstance(&create, nullptr, &instance_), "vkCreateInstance");
        !created) {
        return created;
    }
#ifdef PX_CUSTOM_VULKAN
    volkLoadInstance(instance_);
#endif
    if (validation) {
        const auto create_messenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance_, "vkCreateDebugUtilsMessengerEXT"));
        if (create_messenger != nullptr) {
            return Check(create_messenger(instance_, &messenger, nullptr, &messenger_),
                         "vkCreateDebugUtilsMessengerEXT");
        }
    }
    return {};
}

core::Status VulkanContext::ChooseDevice(const VulkanOptions& options) {
    std::uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance_, &count, nullptr);
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(instance_, &count, devices.data());
    for (VkPhysicalDevice device : devices) {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(device, &properties);
        const std::string name{static_cast<const char*>(properties.deviceName)};
        if ((!options.device.empty() && name.find(options.device) == std::string::npos) ||
            properties.apiVersion < VK_API_VERSION_1_2) {
            continue;
        }
        std::uint32_t families = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &families, nullptr);
        std::vector<VkQueueFamilyProperties> family_properties(families);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &families, family_properties.data());
        for (std::uint32_t family = 0; family < families; ++family) {
            const VkQueueFlags wanted = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
            if ((family_properties[family].queueFlags & wanted) == wanted) {
                physical_ = device;
                queue_family_ = family;
                device_name_ = name;
                VkPhysicalDeviceFloatControlsProperties floats{};
                floats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES;
                VkPhysicalDeviceProperties2 all{};
                all.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
                all.pNext = &floats;
                // Vulkan 1.1 entry points are resolved through the instance so
                // Android builds can keep their older loader link API.
                const auto get_properties = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2>(
                    vkGetInstanceProcAddr(instance_, "vkGetPhysicalDeviceProperties2"));
                if (get_properties == nullptr) {
                    return core::Error{"Vulkan loader exposes no physical device properties2"};
                }
                get_properties(device, &all);
                floats_.keeps_special_values =
                    floats.shaderSignedZeroInfNanPreserveFloat32 == VK_TRUE;
                floats_.keeps_denormals = floats.shaderDenormPreserveFloat32 == VK_TRUE;
                return {};
            }
        }
    }
    return core::Error{options.device.empty()
                           ? std::string{"no Vulkan 1.2 device has a graphics queue"}
                           : std::format("no Vulkan device named like '{}'", options.device)};
}

core::Status VulkanContext::CreateDevice() {
    const float priority = 1.0F;
    VkDeviceQueueCreateInfo queue{};
    queue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue.queueFamilyIndex = queue_family_;
    queue.queueCount = 1;
    queue.pQueuePriorities = &priority;
    // The output merger's logic operations and the custom clip plane need
    // these; a device without them still checks shaders.
    VkPhysicalDeviceFeatures available{};
    vkGetPhysicalDeviceFeatures(physical_, &available);
    VkPhysicalDeviceFeatures enabled{};
    enabled.logicOp = available.logicOp;
    enabled.shaderClipDistance = available.shaderClipDistance;
    can_draw_ = available.logicOp == VK_TRUE && available.shaderClipDistance == VK_TRUE;
    VkDeviceCreateInfo create{};
    create.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create.queueCreateInfoCount = 1;
    create.pQueueCreateInfos = &queue;
    create.pEnabledFeatures = &enabled;
    if (const core::Status created =
            Check(vkCreateDevice(physical_, &create, nullptr, &device_), "vkCreateDevice");
        !created) {
        return created;
    }
#ifdef PX_CUSTOM_VULKAN
    volkLoadDevice(device_);
#endif
    vkGetDeviceQueue(device_, queue_family_, 0, &queue_);
    VkCommandPoolCreateInfo pool{};
    pool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool.queueFamilyIndex = queue_family_;
    return Check(vkCreateCommandPool(device_, &pool, nullptr, &pool_), "vkCreateCommandPool");
}

VulkanContext::~VulkanContext() {
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
        static_cast<void>(Retire(last_ticket_, true));
        vkDestroyCommandPool(device_, pool_, nullptr);
        SavePipelineCache();
        vkDestroyPipelineCache(device_,pipeline_cache_,nullptr);
        vkDestroyDevice(device_, nullptr);
    }
    if (messenger_ != VK_NULL_HANDLE) {
        const auto destroy = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance_, "vkDestroyDebugUtilsMessengerEXT"));
        if (destroy != nullptr) {
            destroy(instance_, messenger_, nullptr);
        }
    }
    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
    }
}

void VulkanContext::NoteValidationMessage(const char* message) {
    if (validation_messages_++ == 0) {
        first_validation_message_ = message;
    }
}

core::Result<Ticket>
VulkanContext::SubmitWithoutWaiting(const std::function<void(VkCommandBuffer)>& record) {
    VkCommandBufferAllocateInfo allocate{};
    allocate.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate.commandPool = pool_;
    allocate.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate.commandBufferCount = 1;
    VkCommandBuffer commands = VK_NULL_HANDLE;
    if (const core::Status allocated = Check(
            vkAllocateCommandBuffers(device_, &allocate, &commands), "vkAllocateCommandBuffers");
        !allocated) {
        return allocated.Failure();
    }
    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commands, &begin);
    record(commands);
    vkEndCommandBuffer(commands);
    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence fence = VK_NULL_HANDLE;
    core::Status status =
        Check(vkCreateFence(device_, &fence_info, nullptr, &fence), "vkCreateFence");
    if (status) {
        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &commands;
        status = Check(vkQueueSubmit(queue_, 1, &submit, fence), "vkQueueSubmit");
    }
    if (!status) {
        vkDestroyFence(device_, fence, nullptr);
        vkFreeCommandBuffers(device_, pool_, 1, &commands);
        return status.Failure();
    }
    in_flight_.push_back({.ticket = ++last_ticket_, .fence = fence, .commands = commands});
    return last_ticket_;
}

core::Status VulkanContext::Retire(Ticket upto, bool wait) {
    core::Status status;
    while (!in_flight_.empty() && in_flight_.front().ticket <= upto) {
        const InFlight& front = in_flight_.front();
        if (wait) {
            if (const core::Status waited =
                    Check(vkWaitForFences(device_, 1, &front.fence, VK_TRUE, UINT64_MAX),
                          "vkWaitForFences");
                !waited) {
                return waited;
            }
        } else if (vkGetFenceStatus(device_, front.fence) != VK_SUCCESS) {
            break;
        }
        vkDestroyFence(device_, front.fence, nullptr);
        vkFreeCommandBuffers(device_, pool_, 1, &front.commands);
        finished_ = front.ticket;
        in_flight_.pop_front();
    }
    return status;
}

core::Status VulkanContext::Wait(Ticket ticket) {
    return Retire(ticket, true);
}

bool VulkanContext::Finished(Ticket ticket) {
    static_cast<void>(Retire(ticket, false));
    return ticket <= finished_;
}

core::Status VulkanContext::Submit(const std::function<void(VkCommandBuffer)>& record) {
    const core::Result<Ticket> ticket = SubmitWithoutWaiting(record);
    if (!ticket) {
        return ticket.Failure();
    }
    return Wait(ticket.Value());
}

std::optional<std::uint32_t> VulkanContext::MemoryType(std::uint32_t allowed,
                                                       VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties memory{};
    vkGetPhysicalDeviceMemoryProperties(physical_, &memory);
    for (std::uint32_t type = 0; type < memory.memoryTypeCount; ++type) {
        if ((allowed & (1U << type)) != 0 &&
            (memory.memoryTypes[type].propertyFlags & properties) == properties) {
            return type;
        }
    }
    return std::nullopt;
}

core::Result<std::unique_ptr<HostBuffer>> HostBuffer::Create(VulkanContext& context,
                                                             VkDeviceSize size,
                                                             VkBufferUsageFlags usage,
                                                             HostUse use) {
    std::unique_ptr<HostBuffer> buffer{new HostBuffer(context)};
    buffer->size_ = size;
    VkBufferCreateInfo create{};
    create.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create.size = size;
    create.usage = usage;
    create.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkDevice device = context.Device();
    if (const core::Status created =
            Check(vkCreateBuffer(device, &create, nullptr, &buffer->buffer_), "vkCreateBuffer");
        !created) {
        return created.Failure();
    }
    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(device, buffer->buffer_, &requirements);
    constexpr VkMemoryPropertyFlags Visible =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    std::optional<std::uint32_t> type;
    if (use == HostUse::ReadBack) {
        type = context.MemoryType(requirements.memoryTypeBits,
                                  Visible | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
    }
    if (!type) {
        type = context.MemoryType(requirements.memoryTypeBits, Visible);
    }
    if (!type) {
        return core::Error{"no memory the host can see for a buffer"};
    }
    VkMemoryAllocateInfo allocate{};
    allocate.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate.allocationSize = requirements.size;
    allocate.memoryTypeIndex = *type;
    if (const core::Status allocated = Check(
            vkAllocateMemory(device, &allocate, nullptr, &buffer->memory_), "vkAllocateMemory");
        !allocated) {
        return allocated.Failure();
    }
    vkBindBufferMemory(device, buffer->buffer_, buffer->memory_, 0);
    if (const core::Status mapped = Check(
            vkMapMemory(device, buffer->memory_, 0, size, 0, &buffer->mapped_), "vkMapMemory");
        !mapped) {
        return mapped.Failure();
    }
    return buffer;
}

HostBuffer::~HostBuffer() {
    VkDevice device = context_->Device();
    if (mapped_ != nullptr) {
        vkUnmapMemory(device, memory_);
    }
    vkDestroyBuffer(device, buffer_, nullptr);
    vkFreeMemory(device, memory_, nullptr);
}

}  // namespace hexlith::adapters::vulkan
