// Copyright 2021 The Dawn Authors
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

#ifndef DAWN_NODE_BINDING_GPUTEXTURE_H_
#define DAWN_NODE_BINDING_GPUTEXTURE_H_

#include "dawn/native/DawnNative.h"
#include "dawn/webgpu_cpp.h"
#include "napi.h"
#include "src/dawn/node/interop/WebGPU.h"

namespace wgpu::binding {

    // GPUTexture is an implementation of interop::GPUTexture that wraps a wgpu::Texture.
    class GPUTexture final : public interop::GPUTexture {
      public:
        GPUTexture(wgpu::Texture texture);

        // Implicit cast operator to Dawn GPU object
        inline operator const wgpu::Texture&() const {
            return texture_;
        }

        // interop::GPUTexture interface compliance
        interop::Interface<interop::GPUTextureView> createView(
            Napi::Env,
            interop::GPUTextureViewDescriptor descriptor) override;
        void destroy(Napi::Env) override;
        std::optional<std::string> getLabel(Napi::Env) override;
        void setLabel(Napi::Env, std::optional<std::string> value) override;

      private:
        wgpu::Texture texture_;
    };

}  // namespace wgpu::binding

#endif  // DAWN_NODE_BINDING_GPUTEXTURE_H_
