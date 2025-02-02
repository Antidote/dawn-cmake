// Copyright 2020 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "dawn/tests/DawnTest.h"

#include "dawn/common/Constants.h"
#include "dawn/common/Log.h"
#include "dawn/utils/GLFWUtils.h"
#include "dawn/utils/WGPUHelpers.h"

#include "GLFW/glfw3.h"

class SwapChainTests : public DawnTest {
  public:
    void SetUp() override {
        DawnTest::SetUp();
        DAWN_TEST_UNSUPPORTED_IF(UsesWire());

        glfwSetErrorCallback([](int code, const char* message) {
            dawn::ErrorLog() << "GLFW error " << code << " " << message;
        });

        // GLFW can fail to start in headless environments, in which SwapChainTests are
        // inapplicable. Skip this cases without producing a test failure.
        if (glfwInit() == GLFW_FALSE) {
            GTEST_SKIP();
        }

        // The SwapChainTests don't create OpenGL contexts so we don't need to call
        // SetupGLFWWindowHintsForBackend. Set GLFW_NO_API anyway to avoid GLFW bringing up a GL
        // context that we won't use.
        ASSERT_TRUE(!IsOpenGL());
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(400, 400, "SwapChainValidationTests window", nullptr, nullptr);

        int width;
        int height;
        glfwGetFramebufferSize(window, &width, &height);

        surface = utils::CreateSurfaceForWindow(GetInstance(), window);
        ASSERT_NE(surface, nullptr);

        baseDescriptor.width = width;
        baseDescriptor.height = height;
        baseDescriptor.usage = wgpu::TextureUsage::RenderAttachment;
        baseDescriptor.format = wgpu::TextureFormat::BGRA8Unorm;
        baseDescriptor.presentMode = wgpu::PresentMode::Mailbox;
    }

    void TearDown() override {
        // Destroy the surface before the window as required by webgpu-native.
        surface = wgpu::Surface();
        if (window != nullptr) {
            glfwDestroyWindow(window);
        }
        DawnTest::TearDown();
    }

    void ClearTexture(wgpu::TextureView view, wgpu::Color color) {
        utils::ComboRenderPassDescriptor desc({view});
        desc.cColorAttachments[0].loadOp = wgpu::LoadOp::Clear;
        desc.cColorAttachments[0].clearColor = color;

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&desc);
        pass.End();

        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);
    }

  protected:
    GLFWwindow* window = nullptr;
    wgpu::Surface surface;

    wgpu::SwapChainDescriptor baseDescriptor;
};

// Basic test for creating a swapchain and presenting one frame.
TEST_P(SwapChainTests, Basic) {
    wgpu::SwapChain swapchain = device.CreateSwapChain(surface, &baseDescriptor);
    ClearTexture(swapchain.GetCurrentTextureView(), {1.0, 0.0, 0.0, 1.0});
    swapchain.Present();
}

// Test replacing the swapchain
TEST_P(SwapChainTests, ReplaceBasic) {
    wgpu::SwapChain swapchain1 = device.CreateSwapChain(surface, &baseDescriptor);
    ClearTexture(swapchain1.GetCurrentTextureView(), {1.0, 0.0, 0.0, 1.0});
    swapchain1.Present();

    wgpu::SwapChain swapchain2 = device.CreateSwapChain(surface, &baseDescriptor);
    ClearTexture(swapchain2.GetCurrentTextureView(), {0.0, 1.0, 0.0, 1.0});
    swapchain2.Present();
}

// Test replacing the swapchain after GetCurrentTextureView
TEST_P(SwapChainTests, ReplaceAfterGet) {
    wgpu::SwapChain swapchain1 = device.CreateSwapChain(surface, &baseDescriptor);
    ClearTexture(swapchain1.GetCurrentTextureView(), {1.0, 0.0, 0.0, 1.0});

    wgpu::SwapChain swapchain2 = device.CreateSwapChain(surface, &baseDescriptor);
    ClearTexture(swapchain2.GetCurrentTextureView(), {0.0, 1.0, 0.0, 1.0});
    swapchain2.Present();
}

// Test destroying the swapchain after GetCurrentTextureView
TEST_P(SwapChainTests, DestroyAfterGet) {
    wgpu::SwapChain swapchain = device.CreateSwapChain(surface, &baseDescriptor);
    ClearTexture(swapchain.GetCurrentTextureView(), {1.0, 0.0, 0.0, 1.0});
}

// Test destroying the surface before the swapchain
TEST_P(SwapChainTests, DestroySurface) {
    wgpu::SwapChain swapchain = device.CreateSwapChain(surface, &baseDescriptor);
    surface = nullptr;
}

// Test destroying the surface before the swapchain but after GetCurrentTextureView
TEST_P(SwapChainTests, DestroySurfaceAfterGet) {
    wgpu::SwapChain swapchain = device.CreateSwapChain(surface, &baseDescriptor);
    ClearTexture(swapchain.GetCurrentTextureView(), {1.0, 0.0, 0.0, 1.0});
    surface = nullptr;
}

// Test switching between present modes.
TEST_P(SwapChainTests, SwitchPresentMode) {
    // Fails with "internal drawable creation failed" on the Windows NVIDIA CQ builders but not
    // locally.
    DAWN_SUPPRESS_TEST_IF(IsWindows() && IsVulkan() && IsNvidia());

    // TODO(jiawei.shao@intel.com): find out why this test sometimes hangs on the latest Linux Intel
    // Vulkan drivers.
    DAWN_SUPPRESS_TEST_IF(IsLinux() && IsVulkan() && IsIntel());

    constexpr wgpu::PresentMode kAllPresentModes[] = {
        wgpu::PresentMode::Immediate,
        wgpu::PresentMode::Fifo,
        wgpu::PresentMode::Mailbox,
    };

    for (wgpu::PresentMode mode1 : kAllPresentModes) {
        for (wgpu::PresentMode mode2 : kAllPresentModes) {
            wgpu::SwapChainDescriptor desc = baseDescriptor;

            desc.presentMode = mode1;
            wgpu::SwapChain swapchain1 = device.CreateSwapChain(surface, &desc);
            ClearTexture(swapchain1.GetCurrentTextureView(), {0.0, 0.0, 0.0, 1.0});
            swapchain1.Present();

            desc.presentMode = mode2;
            wgpu::SwapChain swapchain2 = device.CreateSwapChain(surface, &desc);
            ClearTexture(swapchain2.GetCurrentTextureView(), {0.0, 0.0, 0.0, 1.0});
            swapchain2.Present();
        }
    }
}

// Test resizing the swapchain and without resizing the window.
TEST_P(SwapChainTests, ResizingSwapChainOnly) {
    for (int i = 0; i < 10; i++) {
        wgpu::SwapChainDescriptor desc = baseDescriptor;
        desc.width += i * 10;
        desc.height -= i * 10;

        wgpu::SwapChain swapchain = device.CreateSwapChain(surface, &desc);
        ClearTexture(swapchain.GetCurrentTextureView(), {0.05f * i, 0.0f, 0.0f, 1.0f});
        swapchain.Present();
    }
}

// Test resizing the window but not the swapchain.
TEST_P(SwapChainTests, ResizingWindowOnly) {
    wgpu::SwapChain swapchain = device.CreateSwapChain(surface, &baseDescriptor);

    for (int i = 0; i < 10; i++) {
        glfwSetWindowSize(window, 400 - 10 * i, 400 + 10 * i);
        glfwPollEvents();

        ClearTexture(swapchain.GetCurrentTextureView(), {0.05f * i, 0.0f, 0.0f, 1.0f});
        swapchain.Present();
    }
}

// Test resizing both the window and the swapchain at the same time.
TEST_P(SwapChainTests, ResizingWindowAndSwapChain) {
    // TODO(crbug.com/dawn/1205) Currently failing on new NVIDIA GTX 1660s on Linux/Vulkan.
    DAWN_SUPPRESS_TEST_IF(IsLinux() && IsVulkan() && IsNvidia());
    for (int i = 0; i < 10; i++) {
        glfwSetWindowSize(window, 400 - 10 * i, 400 + 10 * i);
        glfwPollEvents();

        int width;
        int height;
        glfwGetFramebufferSize(window, &width, &height);

        wgpu::SwapChainDescriptor desc = baseDescriptor;
        desc.width = width;
        desc.height = height;

        wgpu::SwapChain swapchain = device.CreateSwapChain(surface, &desc);
        ClearTexture(swapchain.GetCurrentTextureView(), {0.05f * i, 0.0f, 0.0f, 1.0f});
        swapchain.Present();
    }
}

// Test switching devices on the same adapter.
TEST_P(SwapChainTests, SwitchingDevice) {
    // The Vulkan Validation Layers incorrectly disallow gracefully passing a swapchain between two
    // VkDevices using "vkSwapchainCreateInfoKHR::oldSwapchain".
    // See https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/2256
    DAWN_SUPPRESS_TEST_IF(IsVulkan() && IsBackendValidationEnabled());

    wgpu::Device device2 = wgpu::Device::Acquire(GetAdapter().CreateDevice());

    for (int i = 0; i < 3; i++) {
        wgpu::Device deviceToUse;
        if (i % 2 == 0) {
            deviceToUse = device;
        } else {
            deviceToUse = device2;
        }

        wgpu::SwapChain swapchain = deviceToUse.CreateSwapChain(surface, &baseDescriptor);
        swapchain.GetCurrentTextureView();
        swapchain.Present();
    }
}

DAWN_INSTANTIATE_TEST(SwapChainTests, MetalBackend(), VulkanBackend());
