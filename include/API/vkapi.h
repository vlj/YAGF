// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan\vulkan.h>
#include <vulkan\vk_platform.h>
#include <memory>
#include <vector>

namespace vulkan_wrapper
{
    struct command_pool
    {
        VkCommandPool object;
        VkCommandPoolCreateInfo info;

        command_pool()
        {

        }

        ~command_pool()
        {

        }

        command_pool(command_pool&&) = delete;
        command_pool(const command_pool&) = delete;
    private:
        VkDevice m_device;
    };

    struct command_buffer
    {
        VkCommandBuffer object;
        VkCommandBufferAllocateInfo info;

        command_buffer()
        {}

        ~command_buffer()
        {}

        command_buffer(command_buffer&&) = delete;
        command_buffer(const command_buffer&) = delete;
    private:
        VkDevice m_device;
    };

    struct queue
    {
        VkQueue object;
        struct {
            uint32_t queue_family;
            uint32_t queue_index;
        } info;

        queue()
        {}

        ~queue()
        {}

        queue(queue&&) = delete;
        queue(const queue&) = delete;
    private:
        VkDevice m_device;
    };

    struct buffer
    {
        VkBuffer object;
        VkBufferCreateInfo info;

        buffer()
        {}

        ~buffer()
        {}

        buffer(buffer&&) = delete;
        buffer(const buffer&) = delete;
    private:
        VkDevice m_device;
    };

    struct image
    {
        VkImage object;
        VkImageCreateInfo info;

        image()
        {}

        ~image()
        {}

        image(image&&) = delete;
        image(const image&) = delete;
    private:
        VkDevice m_device;
    };

    struct descriptor_pool
    {
        VkDescriptorPool object;
        VkDescriptorPoolCreateInfo info;

        descriptor_pool()
        {}

        ~descriptor_pool()
        {}

        descriptor_pool(descriptor_pool&&) = delete;
        descriptor_pool(const descriptor_pool&) = delete;
    private:
        VkDevice m_device;
    };

    struct pipeline
    {
        VkPipeline object;
        VkPipelineCreateFlags info;

        pipeline()
        {}

        ~pipeline()
        {}

        pipeline(pipeline&&) = delete;
        pipeline(const pipeline&) = delete;
    private:
        VkDevice m_device;
    };

    struct pipeline_layout
    {
        VkPipelineLayout object;
        VkPipelineLayoutCreateInfo info;

        pipeline_layout()
        {}

        ~pipeline_layout()
        {}

        pipeline_layout(pipeline_layout&&) = delete;
        pipeline_layout(const pipeline_layout&) = delete;
    private:
        VkDevice m_device;
    };

    struct swapchain
    {
        VkSwapchainKHR object;
        VkSwapchainCreateInfoKHR info;

        swapchain()
        {}

        ~swapchain()
        {}

        swapchain(swapchain&&) = delete;
        swapchain(const swapchain&) = delete;
    private:
        VkDevice m_device;
    };

    struct framebuffer
    {
        VkFramebuffer object;
        VkFramebufferCreateInfo info;

        framebuffer()
        {}

        ~framebuffer()
        {}

        framebuffer(framebuffer&&) = delete;
        framebuffer(const framebuffer&) = delete;
    private:
        VkDevice m_device;
    };
}

using command_list_storage_t = std::shared_ptr<vulkan_wrapper::command_pool>;
using command_list_t = std::shared_ptr<vulkan_wrapper::command_buffer>;
using device_t = void*;//Microsoft::WRL::ComPtr<ID3D12Device>;
using command_queue_t = std::shared_ptr<vulkan_wrapper::queue>;
using buffer_t = std::shared_ptr<vulkan_wrapper::buffer>;
using image_t = std::shared_ptr<vulkan_wrapper::image>;
using descriptor_storage_t = std::shared_ptr<vulkan_wrapper::descriptor_pool>;
using pipeline_state_t = std::shared_ptr<vulkan_wrapper::pipeline>;
using pipeline_layout_t = std::shared_ptr<vulkan_wrapper::pipeline_layout>;
using swap_chain_t = std::shared_ptr<vulkan_wrapper::swapchain>;
using framebuffer_t = std::shared_ptr<vulkan_wrapper::framebuffer>;

/*struct root_signature_builder
{
private:
    std::vector<std::vector<D3D12_DESCRIPTOR_RANGE > > all_ranges;
    std::vector<D3D12_ROOT_PARAMETER> root_parameters;
    D3D12_ROOT_SIGNATURE_DESC desc = {};

    void build_root_parameter(std::vector<D3D12_DESCRIPTOR_RANGE > &&ranges, D3D12_SHADER_VISIBILITY visibility);
public:
    root_signature_builder(std::vector<std::tuple<std::vector<D3D12_DESCRIPTOR_RANGE >, D3D12_SHADER_VISIBILITY> > &&parameters, D3D12_ROOT_SIGNATURE_FLAGS flags);
    pipeline_layout_t get(device_t dev);
};*/

device_t create_device();
command_queue_t create_graphic_command_queue(device_t dev);
swap_chain_t create_swap_chain(device_t dev, command_queue_t queue, HWND window);
std::vector<image_t> get_image_view_from_swap_chain(device_t dev, swap_chain_t chain);

#include "GfxApi.h"