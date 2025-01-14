// Copyright 2021 The Tint Authors.
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

#include "src/transform/multiplanar_external_texture.h"
#include "src/transform/test_helper.h"

namespace tint {
namespace transform {
namespace {

using MultiplanarExternalTextureTest = TransformTest;

TEST_F(MultiplanarExternalTextureTest, ShouldRunEmptyModule) {
  auto* src = R"()";

  EXPECT_FALSE(ShouldRun<MultiplanarExternalTexture>(src));
}

TEST_F(MultiplanarExternalTextureTest, ShouldRunHasExternalTextureAlias) {
  auto* src = R"(
type ET = texture_external;
)";

  EXPECT_TRUE(ShouldRun<MultiplanarExternalTexture>(src));
}
TEST_F(MultiplanarExternalTextureTest, ShouldRunHasExternalTextureGlobal) {
  auto* src = R"(
[[group(0), binding(0)]] var ext_tex : texture_external;
)";

  EXPECT_TRUE(ShouldRun<MultiplanarExternalTexture>(src));
}

TEST_F(MultiplanarExternalTextureTest, ShouldRunHasExternalTextureParam) {
  auto* src = R"(
fn f(ext_tex : texture_external) {}
)";

  EXPECT_TRUE(ShouldRun<MultiplanarExternalTexture>(src));
}

// Running the transform without passing in data for the new bindings should
// result in an error.
TEST_F(MultiplanarExternalTextureTest, ErrorNoPassedData) {
  auto* src = R"(
@group(0) @binding(0) var s : sampler;
@group(0) @binding(1) var ext_tex : texture_external;

@stage(fragment)
fn main(@builtin(position) coord : vec4<f32>) -> @location(0) vec4<f32> {
  return textureSampleLevel(ext_tex, s, coord.xy);
}
)";
  auto* expect =
      R"(error: missing new binding point data for tint::transform::MultiplanarExternalTexture)";

  auto got = Run<MultiplanarExternalTexture>(src);
  EXPECT_EQ(expect, str(got));
}

// Running the transform with incorrect binding data should result in an error.
TEST_F(MultiplanarExternalTextureTest, ErrorIncorrectBindingPont) {
  auto* src = R"(
@group(0) @binding(0) var s : sampler;
@group(0) @binding(1) var ext_tex : texture_external;

@stage(fragment)
fn main(@builtin(position) coord : vec4<f32>) -> @location(0) vec4<f32> {
  return textureSampleLevel(ext_tex, s, coord.xy);
}
)";

  auto* expect =
      R"(error: missing new binding points for texture_external at binding {0,1})";

  DataMap data;
  // This bindings map specifies 0,0 as the location of the texture_external,
  // which is incorrect.
  data.Add<MultiplanarExternalTexture::NewBindingPoints>(
      MultiplanarExternalTexture::BindingsMap{{{0, 0}, {{0, 1}, {0, 2}}}});
  auto got = Run<MultiplanarExternalTexture>(src, data);
  EXPECT_EQ(expect, str(got));
}

// Tests that the transform works with a textureDimensions call.
TEST_F(MultiplanarExternalTextureTest, Dimensions) {
  auto* src = R"(
@group(0) @binding(0) var ext_tex : texture_external;

@stage(fragment)
fn main(@builtin(position) coord : vec4<f32>) -> @location(0) vec4<f32> {
  var dim : vec2<i32>;
  dim = textureDimensions(ext_tex);
  return vec4<f32>(0.0, 0.0, 0.0, 0.0);
}
)";

  auto* expect = R"(
struct ExternalTextureParams {
  numPlanes : u32;
  vr : f32;
  ug : f32;
  vg : f32;
  ub : f32;
}

@group(0) @binding(1) var ext_tex_plane_1 : texture_2d<f32>;

@group(0) @binding(2) var<uniform> ext_tex_params : ExternalTextureParams;

@group(0) @binding(0) var ext_tex : texture_2d<f32>;

@stage(fragment)
fn main(@builtin(position) coord : vec4<f32>) -> @location(0) vec4<f32> {
  var dim : vec2<i32>;
  dim = textureDimensions(ext_tex);
  return vec4<f32>(0.0, 0.0, 0.0, 0.0);
}
)";

  DataMap data;
  data.Add<MultiplanarExternalTexture::NewBindingPoints>(
      MultiplanarExternalTexture::BindingsMap{{{0, 0}, {{0, 1}, {0, 2}}}});
  auto got = Run<MultiplanarExternalTexture>(src, data);
  EXPECT_EQ(expect, str(got));
}

// Tests that the transform works with a textureDimensions call.
TEST_F(MultiplanarExternalTextureTest, Dimensions_OutOfOrder) {
  auto* src = R"(
@stage(fragment)
fn main(@builtin(position) coord : vec4<f32>) -> @location(0) vec4<f32> {
  var dim : vec2<i32>;
  dim = textureDimensions(ext_tex);
  return vec4<f32>(0.0, 0.0, 0.0, 0.0);
}

@group(0) @binding(0) var ext_tex : texture_external;
)";

  auto* expect = R"(
struct ExternalTextureParams {
  numPlanes : u32;
  vr : f32;
  ug : f32;
  vg : f32;
  ub : f32;
}

@group(0) @binding(1) var ext_tex_plane_1 : texture_2d<f32>;

@group(0) @binding(2) var<uniform> ext_tex_params : ExternalTextureParams;

@stage(fragment)
fn main(@builtin(position) coord : vec4<f32>) -> @location(0) vec4<f32> {
  var dim : vec2<i32>;
  dim = textureDimensions(ext_tex);
  return vec4<f32>(0.0, 0.0, 0.0, 0.0);
}

@group(0) @binding(0) var ext_tex : texture_2d<f32>;
)";

  DataMap data;
  data.Add<MultiplanarExternalTexture::NewBindingPoints>(
      MultiplanarExternalTexture::BindingsMap{{{0, 0}, {{0, 1}, {0, 2}}}});
  auto got = Run<MultiplanarExternalTexture>(src, data);
  EXPECT_EQ(expect, str(got));
}

// Test that the transform works with a textureSampleLevel call.
TEST_F(MultiplanarExternalTextureTest, BasicTextureSampleLevel) {
  auto* src = R"(
@group(0) @binding(0) var s : sampler;
@group(0) @binding(1) var ext_tex : texture_external;

@stage(fragment)
fn main(@builtin(position) coord : vec4<f32>) -> @location(0) vec4<f32> {
  return textureSampleLevel(ext_tex, s, coord.xy);
}
)";

  auto* expect = R"(
struct ExternalTextureParams {
  numPlanes : u32;
  vr : f32;
  ug : f32;
  vg : f32;
  ub : f32;
}

@group(0) @binding(2) var ext_tex_plane_1 : texture_2d<f32>;

@group(0) @binding(3) var<uniform> ext_tex_params : ExternalTextureParams;

@group(0) @binding(0) var s : sampler;

@group(0) @binding(1) var ext_tex : texture_2d<f32>;

fn textureSampleExternal(plane0 : texture_2d<f32>, plane1 : texture_2d<f32>, smp : sampler, coord : vec2<f32>, params : ExternalTextureParams) -> vec4<f32> {
  if ((params.numPlanes == 1u)) {
    return textureSampleLevel(plane0, smp, coord, 0.0);
  }
  let y = (textureSampleLevel(plane0, smp, coord, 0.0).r - 0.0625);
  let uv = (textureSampleLevel(plane1, smp, coord, 0.0).rg - 0.5);
  let u = uv.x;
  let v = uv.y;
  let r = ((1.164000034 * y) + (params.vr * v));
  let g = (((1.164000034 * y) - (params.ug * u)) - (params.vg * v));
  let b = ((1.164000034 * y) + (params.ub * u));
  return vec4<f32>(r, g, b, 1.0);
}

@stage(fragment)
fn main(@builtin(position) coord : vec4<f32>) -> @location(0) vec4<f32> {
  return textureSampleExternal(ext_tex, ext_tex_plane_1, s, coord.xy, ext_tex_params);
}
)";

  DataMap data;
  data.Add<MultiplanarExternalTexture::NewBindingPoints>(
      MultiplanarExternalTexture::BindingsMap{{{0, 1}, {{0, 2}, {0, 3}}}});
  auto got = Run<MultiplanarExternalTexture>(src, data);
  EXPECT_EQ(expect, str(got));
}

// Test that the transform works with a textureSampleLevel call.
TEST_F(MultiplanarExternalTextureTest, BasicTextureSampleLevel_OutOfOrder) {
  auto* src = R"(
@stage(fragment)
fn main(@builtin(position) coord : vec4<f32>) -> @location(0) vec4<f32> {
  return textureSampleLevel(ext_tex, s, coord.xy);
}

@group(0) @binding(1) var ext_tex : texture_external;
@group(0) @binding(0) var s : sampler;
)";

  auto* expect = R"(
struct ExternalTextureParams {
  numPlanes : u32;
  vr : f32;
  ug : f32;
  vg : f32;
  ub : f32;
}

@group(0) @binding(2) var ext_tex_plane_1 : texture_2d<f32>;

@group(0) @binding(3) var<uniform> ext_tex_params : ExternalTextureParams;

fn textureSampleExternal(plane0 : texture_2d<f32>, plane1 : texture_2d<f32>, smp : sampler, coord : vec2<f32>, params : ExternalTextureParams) -> vec4<f32> {
  if ((params.numPlanes == 1u)) {
    return textureSampleLevel(plane0, smp, coord, 0.0);
  }
  let y = (textureSampleLevel(plane0, smp, coord, 0.0).r - 0.0625);
  let uv = (textureSampleLevel(plane1, smp, coord, 0.0).rg - 0.5);
  let u = uv.x;
  let v = uv.y;
  let r = ((1.164000034 * y) + (params.vr * v));
  let g = (((1.164000034 * y) - (params.ug * u)) - (params.vg * v));
  let b = ((1.164000034 * y) + (params.ub * u));
  return vec4<f32>(r, g, b, 1.0);
}

@stage(fragment)
fn main(@builtin(position) coord : vec4<f32>) -> @location(0) vec4<f32> {
  return textureSampleExternal(ext_tex, ext_tex_plane_1, s, coord.xy, ext_tex_params);
}

@group(0) @binding(1) var ext_tex : texture_2d<f32>;

@group(0) @binding(0) var s : sampler;
)";

  DataMap data;
  data.Add<MultiplanarExternalTexture::NewBindingPoints>(
      MultiplanarExternalTexture::BindingsMap{{{0, 1}, {{0, 2}, {0, 3}}}});
  auto got = Run<MultiplanarExternalTexture>(src, data);
  EXPECT_EQ(expect, str(got));
}

// Tests that the transform works with a textureLoad call.
TEST_F(MultiplanarExternalTextureTest, BasicTextureLoad) {
  auto* src = R"(
@group(0) @binding(0) var ext_tex : texture_external;

@stage(fragment)
fn main(@builtin(position) coord : vec4<f32>) -> @location(0) vec4<f32> {
  return textureLoad(ext_tex, vec2<i32>(1, 1));
}
)";

  auto* expect = R"(
struct ExternalTextureParams {
  numPlanes : u32;
  vr : f32;
  ug : f32;
  vg : f32;
  ub : f32;
}

@group(0) @binding(1) var ext_tex_plane_1 : texture_2d<f32>;

@group(0) @binding(2) var<uniform> ext_tex_params : ExternalTextureParams;

@group(0) @binding(0) var ext_tex : texture_2d<f32>;

fn textureLoadExternal(plane0 : texture_2d<f32>, plane1 : texture_2d<f32>, coord : vec2<i32>, params : ExternalTextureParams) -> vec4<f32> {
  if ((params.numPlanes == 1u)) {
    return textureLoad(plane0, coord, 0);
  }
  let y = (textureLoad(plane0, coord, 0).r - 0.0625);
  let uv = (textureLoad(plane1, coord, 0).rg - 0.5);
  let u = uv.x;
  let v = uv.y;
  let r = ((1.164000034 * y) + (params.vr * v));
  let g = (((1.164000034 * y) - (params.ug * u)) - (params.vg * v));
  let b = ((1.164000034 * y) + (params.ub * u));
  return vec4<f32>(r, g, b, 1.0);
}

@stage(fragment)
fn main(@builtin(position) coord : vec4<f32>) -> @location(0) vec4<f32> {
  return textureLoadExternal(ext_tex, ext_tex_plane_1, vec2<i32>(1, 1), ext_tex_params);
}
)";

  DataMap data;
  data.Add<MultiplanarExternalTexture::NewBindingPoints>(
      MultiplanarExternalTexture::BindingsMap{{{0, 0}, {{0, 1}, {0, 2}}}});
  auto got = Run<MultiplanarExternalTexture>(src, data);
  EXPECT_EQ(expect, str(got));
}

// Tests that the transform works with a textureLoad call.
TEST_F(MultiplanarExternalTextureTest, BasicTextureLoad_OutOfOrder) {
  auto* src = R"(
@stage(fragment)
fn main(@builtin(position) coord : vec4<f32>) -> @location(0) vec4<f32> {
  return textureLoad(ext_tex, vec2<i32>(1, 1));
}

@group(0) @binding(0) var ext_tex : texture_external;
)";

  auto* expect = R"(
struct ExternalTextureParams {
  numPlanes : u32;
  vr : f32;
  ug : f32;
  vg : f32;
  ub : f32;
}

@group(0) @binding(1) var ext_tex_plane_1 : texture_2d<f32>;

@group(0) @binding(2) var<uniform> ext_tex_params : ExternalTextureParams;

fn textureLoadExternal(plane0 : texture_2d<f32>, plane1 : texture_2d<f32>, coord : vec2<i32>, params : ExternalTextureParams) -> vec4<f32> {
  if ((params.numPlanes == 1u)) {
    return textureLoad(plane0, coord, 0);
  }
  let y = (textureLoad(plane0, coord, 0).r - 0.0625);
  let uv = (textureLoad(plane1, coord, 0).rg - 0.5);
  let u = uv.x;
  let v = uv.y;
  let r = ((1.164000034 * y) + (params.vr * v));
  let g = (((1.164000034 * y) - (params.ug * u)) - (params.vg * v));
  let b = ((1.164000034 * y) + (params.ub * u));
  return vec4<f32>(r, g, b, 1.0);
}

@stage(fragment)
fn main(@builtin(position) coord : vec4<f32>) -> @location(0) vec4<f32> {
  return textureLoadExternal(ext_tex, ext_tex_plane_1, vec2<i32>(1, 1), ext_tex_params);
}

@group(0) @binding(0) var ext_tex : texture_2d<f32>;
)";

  DataMap data;
  data.Add<MultiplanarExternalTexture::NewBindingPoints>(
      MultiplanarExternalTexture::BindingsMap{{{0, 0}, {{0, 1}, {0, 2}}}});
  auto got = Run<MultiplanarExternalTexture>(src, data);
  EXPECT_EQ(expect, str(got));
}

// Tests that the transform works with both a textureSampleLevel and textureLoad
// call.
TEST_F(MultiplanarExternalTextureTest, TextureSampleAndTextureLoad) {
  auto* src = R"(
@group(0) @binding(0) var s : sampler;
@group(0) @binding(1) var ext_tex : texture_external;

@stage(fragment)
fn main(@builtin(position) coord : vec4<f32>) -> @location(0) vec4<f32> {
  return textureSampleLevel(ext_tex, s, coord.xy) + textureLoad(ext_tex, vec2<i32>(1, 1));
}
)";

  auto* expect = R"(
struct ExternalTextureParams {
  numPlanes : u32;
  vr : f32;
  ug : f32;
  vg : f32;
  ub : f32;
}

@group(0) @binding(2) var ext_tex_plane_1 : texture_2d<f32>;

@group(0) @binding(3) var<uniform> ext_tex_params : ExternalTextureParams;

@group(0) @binding(0) var s : sampler;

@group(0) @binding(1) var ext_tex : texture_2d<f32>;

fn textureSampleExternal(plane0 : texture_2d<f32>, plane1 : texture_2d<f32>, smp : sampler, coord : vec2<f32>, params : ExternalTextureParams) -> vec4<f32> {
  if ((params.numPlanes == 1u)) {
    return textureSampleLevel(plane0, smp, coord, 0.0);
  }
  let y = (textureSampleLevel(plane0, smp, coord, 0.0).r - 0.0625);
  let uv = (textureSampleLevel(plane1, smp, coord, 0.0).rg - 0.5);
  let u = uv.x;
  let v = uv.y;
  let r = ((1.164000034 * y) + (params.vr * v));
  let g = (((1.164000034 * y) - (params.ug * u)) - (params.vg * v));
  let b = ((1.164000034 * y) + (params.ub * u));
  return vec4<f32>(r, g, b, 1.0);
}

fn textureLoadExternal(plane0 : texture_2d<f32>, plane1 : texture_2d<f32>, coord : vec2<i32>, params : ExternalTextureParams) -> vec4<f32> {
  if ((params.numPlanes == 1u)) {
    return textureLoad(plane0, coord, 0);
  }
  let y = (textureLoad(plane0, coord, 0).r - 0.0625);
  let uv = (textureLoad(plane1, coord, 0).rg - 0.5);
  let u = uv.x;
  let v = uv.y;
  let r = ((1.164000034 * y) + (params.vr * v));
  let g = (((1.164000034 * y) - (params.ug * u)) - (params.vg * v));
  let b = ((1.164000034 * y) + (params.ub * u));
  return vec4<f32>(r, g, b, 1.0);
}

@stage(fragment)
fn main(@builtin(position) coord : vec4<f32>) -> @location(0) vec4<f32> {
  return (textureSampleExternal(ext_tex, ext_tex_plane_1, s, coord.xy, ext_tex_params) + textureLoadExternal(ext_tex, ext_tex_plane_1, vec2<i32>(1, 1), ext_tex_params));
}
)";

  DataMap data;
  data.Add<MultiplanarExternalTexture::NewBindingPoints>(
      MultiplanarExternalTexture::BindingsMap{{{0, 1}, {{0, 2}, {0, 3}}}});
  auto got = Run<MultiplanarExternalTexture>(src, data);
  EXPECT_EQ(expect, str(got));
}

// Tests that the transform works with both a textureSampleLevel and textureLoad
// call.
TEST_F(MultiplanarExternalTextureTest, TextureSampleAndTextureLoad_OutOfOrder) {
  auto* src = R"(
@stage(fragment)
fn main(@builtin(position) coord : vec4<f32>) -> @location(0) vec4<f32> {
  return textureSampleLevel(ext_tex, s, coord.xy) + textureLoad(ext_tex, vec2<i32>(1, 1));
}

@group(0) @binding(0) var s : sampler;
@group(0) @binding(1) var ext_tex : texture_external;
)";

  auto* expect = R"(
struct ExternalTextureParams {
  numPlanes : u32;
  vr : f32;
  ug : f32;
  vg : f32;
  ub : f32;
}

@group(0) @binding(2) var ext_tex_plane_1 : texture_2d<f32>;

@group(0) @binding(3) var<uniform> ext_tex_params : ExternalTextureParams;

fn textureSampleExternal(plane0 : texture_2d<f32>, plane1 : texture_2d<f32>, smp : sampler, coord : vec2<f32>, params : ExternalTextureParams) -> vec4<f32> {
  if ((params.numPlanes == 1u)) {
    return textureSampleLevel(plane0, smp, coord, 0.0);
  }
  let y = (textureSampleLevel(plane0, smp, coord, 0.0).r - 0.0625);
  let uv = (textureSampleLevel(plane1, smp, coord, 0.0).rg - 0.5);
  let u = uv.x;
  let v = uv.y;
  let r = ((1.164000034 * y) + (params.vr * v));
  let g = (((1.164000034 * y) - (params.ug * u)) - (params.vg * v));
  let b = ((1.164000034 * y) + (params.ub * u));
  return vec4<f32>(r, g, b, 1.0);
}

fn textureLoadExternal(plane0 : texture_2d<f32>, plane1 : texture_2d<f32>, coord : vec2<i32>, params : ExternalTextureParams) -> vec4<f32> {
  if ((params.numPlanes == 1u)) {
    return textureLoad(plane0, coord, 0);
  }
  let y = (textureLoad(plane0, coord, 0).r - 0.0625);
  let uv = (textureLoad(plane1, coord, 0).rg - 0.5);
  let u = uv.x;
  let v = uv.y;
  let r = ((1.164000034 * y) + (params.vr * v));
  let g = (((1.164000034 * y) - (params.ug * u)) - (params.vg * v));
  let b = ((1.164000034 * y) + (params.ub * u));
  return vec4<f32>(r, g, b, 1.0);
}

@stage(fragment)
fn main(@builtin(position) coord : vec4<f32>) -> @location(0) vec4<f32> {
  return (textureSampleExternal(ext_tex, ext_tex_plane_1, s, coord.xy, ext_tex_params) + textureLoadExternal(ext_tex, ext_tex_plane_1, vec2<i32>(1, 1), ext_tex_params));
}

@group(0) @binding(0) var s : sampler;

@group(0) @binding(1) var ext_tex : texture_2d<f32>;
)";

  DataMap data;
  data.Add<MultiplanarExternalTexture::NewBindingPoints>(
      MultiplanarExternalTexture::BindingsMap{{{0, 1}, {{0, 2}, {0, 3}}}});
  auto got = Run<MultiplanarExternalTexture>(src, data);
  EXPECT_EQ(expect, str(got));
}

// Tests that the transform works with many instances of texture_external.
TEST_F(MultiplanarExternalTextureTest, ManyTextureSampleLevel) {
  auto* src = R"(
@group(0) @binding(0) var s : sampler;
@group(0) @binding(1) var ext_tex : texture_external;
@group(0) @binding(2) var ext_tex_1 : texture_external;
@group(0) @binding(3) var ext_tex_2 : texture_external;
@group(1) @binding(0) var ext_tex_3 : texture_external;

@stage(fragment)
fn main(@builtin(position) coord : vec4<f32>) -> @location(0) vec4<f32> {
  return textureSampleLevel(ext_tex, s, coord.xy) + textureSampleLevel(ext_tex_1, s, coord.xy) + textureSampleLevel(ext_tex_2, s, coord.xy) + textureSampleLevel(ext_tex_3, s, coord.xy);
}
)";

  auto* expect = R"(
struct ExternalTextureParams {
  numPlanes : u32;
  vr : f32;
  ug : f32;
  vg : f32;
  ub : f32;
}

@group(0) @binding(4) var ext_tex_plane_1 : texture_2d<f32>;

@group(0) @binding(5) var<uniform> ext_tex_params : ExternalTextureParams;

@group(0) @binding(6) var ext_tex_plane_1_1 : texture_2d<f32>;

@group(0) @binding(7) var<uniform> ext_tex_params_1 : ExternalTextureParams;

@group(0) @binding(8) var ext_tex_plane_1_2 : texture_2d<f32>;

@group(0) @binding(9) var<uniform> ext_tex_params_2 : ExternalTextureParams;

@group(1) @binding(1) var ext_tex_plane_1_3 : texture_2d<f32>;

@group(1) @binding(2) var<uniform> ext_tex_params_3 : ExternalTextureParams;

@group(0) @binding(0) var s : sampler;

@group(0) @binding(1) var ext_tex : texture_2d<f32>;

@group(0) @binding(2) var ext_tex_1 : texture_2d<f32>;

@group(0) @binding(3) var ext_tex_2 : texture_2d<f32>;

@group(1) @binding(0) var ext_tex_3 : texture_2d<f32>;

fn textureSampleExternal(plane0 : texture_2d<f32>, plane1 : texture_2d<f32>, smp : sampler, coord : vec2<f32>, params : ExternalTextureParams) -> vec4<f32> {
  if ((params.numPlanes == 1u)) {
    return textureSampleLevel(plane0, smp, coord, 0.0);
  }
  let y = (textureSampleLevel(plane0, smp, coord, 0.0).r - 0.0625);
  let uv = (textureSampleLevel(plane1, smp, coord, 0.0).rg - 0.5);
  let u = uv.x;
  let v = uv.y;
  let r = ((1.164000034 * y) + (params.vr * v));
  let g = (((1.164000034 * y) - (params.ug * u)) - (params.vg * v));
  let b = ((1.164000034 * y) + (params.ub * u));
  return vec4<f32>(r, g, b, 1.0);
}

@stage(fragment)
fn main(@builtin(position) coord : vec4<f32>) -> @location(0) vec4<f32> {
  return (((textureSampleExternal(ext_tex, ext_tex_plane_1, s, coord.xy, ext_tex_params) + textureSampleExternal(ext_tex_1, ext_tex_plane_1_1, s, coord.xy, ext_tex_params_1)) + textureSampleExternal(ext_tex_2, ext_tex_plane_1_2, s, coord.xy, ext_tex_params_2)) + textureSampleExternal(ext_tex_3, ext_tex_plane_1_3, s, coord.xy, ext_tex_params_3));
}
)";

  DataMap data;
  data.Add<MultiplanarExternalTexture::NewBindingPoints>(
      MultiplanarExternalTexture::BindingsMap{
          {{0, 1}, {{0, 4}, {0, 5}}},
          {{0, 2}, {{0, 6}, {0, 7}}},
          {{0, 3}, {{0, 8}, {0, 9}}},
          {{1, 0}, {{1, 1}, {1, 2}}},
      });
  auto got = Run<MultiplanarExternalTexture>(src, data);
  EXPECT_EQ(expect, str(got));
}

// Tests that the texture_external passed as a function parameter produces the
// correct output.
TEST_F(MultiplanarExternalTextureTest, ExternalTexturePassedAsParam) {
  auto* src = R"(
fn f(t : texture_external, s : sampler) {
  textureSampleLevel(t, s, vec2<f32>(1.0, 2.0));
}

@group(0) @binding(0) var ext_tex : texture_external;
@group(0) @binding(1) var smp : sampler;

@stage(fragment)
fn main() {
  f(ext_tex, smp);
}
)";

  auto* expect = R"(
struct ExternalTextureParams {
  numPlanes : u32;
  vr : f32;
  ug : f32;
  vg : f32;
  ub : f32;
}

@group(0) @binding(2) var ext_tex_plane_1 : texture_2d<f32>;

@group(0) @binding(3) var<uniform> ext_tex_params : ExternalTextureParams;

fn textureSampleExternal(plane0 : texture_2d<f32>, plane1 : texture_2d<f32>, smp : sampler, coord : vec2<f32>, params : ExternalTextureParams) -> vec4<f32> {
  if ((params.numPlanes == 1u)) {
    return textureSampleLevel(plane0, smp, coord, 0.0);
  }
  let y = (textureSampleLevel(plane0, smp, coord, 0.0).r - 0.0625);
  let uv = (textureSampleLevel(plane1, smp, coord, 0.0).rg - 0.5);
  let u = uv.x;
  let v = uv.y;
  let r = ((1.164000034 * y) + (params.vr * v));
  let g = (((1.164000034 * y) - (params.ug * u)) - (params.vg * v));
  let b = ((1.164000034 * y) + (params.ub * u));
  return vec4<f32>(r, g, b, 1.0);
}

fn f(t : texture_2d<f32>, ext_tex_plane_1_1 : texture_2d<f32>, ext_tex_params_1 : ExternalTextureParams, s : sampler) {
  textureSampleExternal(t, ext_tex_plane_1_1, s, vec2<f32>(1.0, 2.0), ext_tex_params_1);
}

@group(0) @binding(0) var ext_tex : texture_2d<f32>;

@group(0) @binding(1) var smp : sampler;

@stage(fragment)
fn main() {
  f(ext_tex, ext_tex_plane_1, ext_tex_params, smp);
}
)";
  DataMap data;
  data.Add<MultiplanarExternalTexture::NewBindingPoints>(
      MultiplanarExternalTexture::BindingsMap{
          {{0, 0}, {{0, 2}, {0, 3}}},
      });
  auto got = Run<MultiplanarExternalTexture>(src, data);
  EXPECT_EQ(expect, str(got));
}

// Tests that the texture_external passed as a function parameter produces the
// correct output.
TEST_F(MultiplanarExternalTextureTest,
       ExternalTexturePassedAsParam_OutOfOrder) {
  auto* src = R"(
@stage(fragment)
fn main() {
  f(ext_tex, smp);
}

fn f(t : texture_external, s : sampler) {
  textureSampleLevel(t, s, vec2<f32>(1.0, 2.0));
}

@group(0) @binding(0) var ext_tex : texture_external;
@group(0) @binding(1) var smp : sampler;
)";

  auto* expect = R"(
struct ExternalTextureParams {
  numPlanes : u32;
  vr : f32;
  ug : f32;
  vg : f32;
  ub : f32;
}

@group(0) @binding(2) var ext_tex_plane_1 : texture_2d<f32>;

@group(0) @binding(3) var<uniform> ext_tex_params : ExternalTextureParams;

@stage(fragment)
fn main() {
  f(ext_tex, ext_tex_plane_1, ext_tex_params, smp);
}

fn textureSampleExternal(plane0 : texture_2d<f32>, plane1 : texture_2d<f32>, smp : sampler, coord : vec2<f32>, params : ExternalTextureParams) -> vec4<f32> {
  if ((params.numPlanes == 1u)) {
    return textureSampleLevel(plane0, smp, coord, 0.0);
  }
  let y = (textureSampleLevel(plane0, smp, coord, 0.0).r - 0.0625);
  let uv = (textureSampleLevel(plane1, smp, coord, 0.0).rg - 0.5);
  let u = uv.x;
  let v = uv.y;
  let r = ((1.164000034 * y) + (params.vr * v));
  let g = (((1.164000034 * y) - (params.ug * u)) - (params.vg * v));
  let b = ((1.164000034 * y) + (params.ub * u));
  return vec4<f32>(r, g, b, 1.0);
}

fn f(t : texture_2d<f32>, ext_tex_plane_1_1 : texture_2d<f32>, ext_tex_params_1 : ExternalTextureParams, s : sampler) {
  textureSampleExternal(t, ext_tex_plane_1_1, s, vec2<f32>(1.0, 2.0), ext_tex_params_1);
}

@group(0) @binding(0) var ext_tex : texture_2d<f32>;

@group(0) @binding(1) var smp : sampler;
)";
  DataMap data;
  data.Add<MultiplanarExternalTexture::NewBindingPoints>(
      MultiplanarExternalTexture::BindingsMap{
          {{0, 0}, {{0, 2}, {0, 3}}},
      });
  auto got = Run<MultiplanarExternalTexture>(src, data);
  EXPECT_EQ(expect, str(got));
}

// Tests that the texture_external passed as a parameter not in the first
// position produces the correct output.
TEST_F(MultiplanarExternalTextureTest, ExternalTexturePassedAsSecondParam) {
  auto* src = R"(
fn f(s : sampler, t : texture_external) {
  textureSampleLevel(t, s, vec2<f32>(1.0, 2.0));
}

@group(0) @binding(0) var ext_tex : texture_external;
@group(0) @binding(1) var smp : sampler;

@stage(fragment)
fn main() {
  f(smp, ext_tex);
}
)";

  auto* expect = R"(
struct ExternalTextureParams {
  numPlanes : u32;
  vr : f32;
  ug : f32;
  vg : f32;
  ub : f32;
}

@group(0) @binding(2) var ext_tex_plane_1 : texture_2d<f32>;

@group(0) @binding(3) var<uniform> ext_tex_params : ExternalTextureParams;

fn textureSampleExternal(plane0 : texture_2d<f32>, plane1 : texture_2d<f32>, smp : sampler, coord : vec2<f32>, params : ExternalTextureParams) -> vec4<f32> {
  if ((params.numPlanes == 1u)) {
    return textureSampleLevel(plane0, smp, coord, 0.0);
  }
  let y = (textureSampleLevel(plane0, smp, coord, 0.0).r - 0.0625);
  let uv = (textureSampleLevel(plane1, smp, coord, 0.0).rg - 0.5);
  let u = uv.x;
  let v = uv.y;
  let r = ((1.164000034 * y) + (params.vr * v));
  let g = (((1.164000034 * y) - (params.ug * u)) - (params.vg * v));
  let b = ((1.164000034 * y) + (params.ub * u));
  return vec4<f32>(r, g, b, 1.0);
}

fn f(s : sampler, t : texture_2d<f32>, ext_tex_plane_1_1 : texture_2d<f32>, ext_tex_params_1 : ExternalTextureParams) {
  textureSampleExternal(t, ext_tex_plane_1_1, s, vec2<f32>(1.0, 2.0), ext_tex_params_1);
}

@group(0) @binding(0) var ext_tex : texture_2d<f32>;

@group(0) @binding(1) var smp : sampler;

@stage(fragment)
fn main() {
  f(smp, ext_tex, ext_tex_plane_1, ext_tex_params);
}
)";
  DataMap data;
  data.Add<MultiplanarExternalTexture::NewBindingPoints>(
      MultiplanarExternalTexture::BindingsMap{
          {{0, 0}, {{0, 2}, {0, 3}}},
      });
  auto got = Run<MultiplanarExternalTexture>(src, data);
  EXPECT_EQ(expect, str(got));
}

// Tests that multiple texture_external params passed to a function produces the
// correct output.
TEST_F(MultiplanarExternalTextureTest, ExternalTexturePassedAsParamMultiple) {
  auto* src = R"(
fn f(t : texture_external, s : sampler, t2 : texture_external) {
  textureSampleLevel(t, s, vec2<f32>(1.0, 2.0));
  textureSampleLevel(t2, s, vec2<f32>(1.0, 2.0));
}

@group(0) @binding(0) var ext_tex : texture_external;
@group(0) @binding(1) var smp : sampler;
@group(0) @binding(2) var ext_tex2 : texture_external;

@stage(fragment)
fn main() {
  f(ext_tex, smp, ext_tex2);
}
)";

  auto* expect = R"(
struct ExternalTextureParams {
  numPlanes : u32;
  vr : f32;
  ug : f32;
  vg : f32;
  ub : f32;
}

@group(0) @binding(3) var ext_tex_plane_1 : texture_2d<f32>;

@group(0) @binding(4) var<uniform> ext_tex_params : ExternalTextureParams;

@group(0) @binding(5) var ext_tex_plane_1_1 : texture_2d<f32>;

@group(0) @binding(6) var<uniform> ext_tex_params_1 : ExternalTextureParams;

fn textureSampleExternal(plane0 : texture_2d<f32>, plane1 : texture_2d<f32>, smp : sampler, coord : vec2<f32>, params : ExternalTextureParams) -> vec4<f32> {
  if ((params.numPlanes == 1u)) {
    return textureSampleLevel(plane0, smp, coord, 0.0);
  }
  let y = (textureSampleLevel(plane0, smp, coord, 0.0).r - 0.0625);
  let uv = (textureSampleLevel(plane1, smp, coord, 0.0).rg - 0.5);
  let u = uv.x;
  let v = uv.y;
  let r = ((1.164000034 * y) + (params.vr * v));
  let g = (((1.164000034 * y) - (params.ug * u)) - (params.vg * v));
  let b = ((1.164000034 * y) + (params.ub * u));
  return vec4<f32>(r, g, b, 1.0);
}

fn f(t : texture_2d<f32>, ext_tex_plane_1_2 : texture_2d<f32>, ext_tex_params_2 : ExternalTextureParams, s : sampler, t2 : texture_2d<f32>, ext_tex_plane_1_3 : texture_2d<f32>, ext_tex_params_3 : ExternalTextureParams) {
  textureSampleExternal(t, ext_tex_plane_1_2, s, vec2<f32>(1.0, 2.0), ext_tex_params_2);
  textureSampleExternal(t2, ext_tex_plane_1_3, s, vec2<f32>(1.0, 2.0), ext_tex_params_3);
}

@group(0) @binding(0) var ext_tex : texture_2d<f32>;

@group(0) @binding(1) var smp : sampler;

@group(0) @binding(2) var ext_tex2 : texture_2d<f32>;

@stage(fragment)
fn main() {
  f(ext_tex, ext_tex_plane_1, ext_tex_params, smp, ext_tex2, ext_tex_plane_1_1, ext_tex_params_1);
}
)";
  DataMap data;
  data.Add<MultiplanarExternalTexture::NewBindingPoints>(
      MultiplanarExternalTexture::BindingsMap{
          {{0, 0}, {{0, 3}, {0, 4}}},
          {{0, 2}, {{0, 5}, {0, 6}}},
      });
  auto got = Run<MultiplanarExternalTexture>(src, data);
  EXPECT_EQ(expect, str(got));
}

// Tests that multiple texture_external params passed to a function produces the
// correct output.
TEST_F(MultiplanarExternalTextureTest,
       ExternalTexturePassedAsParamMultiple_OutOfOrder) {
  auto* src = R"(
@stage(fragment)
fn main() {
  f(ext_tex, smp, ext_tex2);
}

fn f(t : texture_external, s : sampler, t2 : texture_external) {
  textureSampleLevel(t, s, vec2<f32>(1.0, 2.0));
  textureSampleLevel(t2, s, vec2<f32>(1.0, 2.0));
}

@group(0) @binding(0) var ext_tex : texture_external;
@group(0) @binding(1) var smp : sampler;
@group(0) @binding(2) var ext_tex2 : texture_external;

)";

  auto* expect = R"(
struct ExternalTextureParams {
  numPlanes : u32;
  vr : f32;
  ug : f32;
  vg : f32;
  ub : f32;
}

@group(0) @binding(3) var ext_tex_plane_1 : texture_2d<f32>;

@group(0) @binding(4) var<uniform> ext_tex_params : ExternalTextureParams;

@group(0) @binding(5) var ext_tex_plane_1_1 : texture_2d<f32>;

@group(0) @binding(6) var<uniform> ext_tex_params_1 : ExternalTextureParams;

@stage(fragment)
fn main() {
  f(ext_tex, ext_tex_plane_1, ext_tex_params, smp, ext_tex2, ext_tex_plane_1_1, ext_tex_params_1);
}

fn textureSampleExternal(plane0 : texture_2d<f32>, plane1 : texture_2d<f32>, smp : sampler, coord : vec2<f32>, params : ExternalTextureParams) -> vec4<f32> {
  if ((params.numPlanes == 1u)) {
    return textureSampleLevel(plane0, smp, coord, 0.0);
  }
  let y = (textureSampleLevel(plane0, smp, coord, 0.0).r - 0.0625);
  let uv = (textureSampleLevel(plane1, smp, coord, 0.0).rg - 0.5);
  let u = uv.x;
  let v = uv.y;
  let r = ((1.164000034 * y) + (params.vr * v));
  let g = (((1.164000034 * y) - (params.ug * u)) - (params.vg * v));
  let b = ((1.164000034 * y) + (params.ub * u));
  return vec4<f32>(r, g, b, 1.0);
}

fn f(t : texture_2d<f32>, ext_tex_plane_1_2 : texture_2d<f32>, ext_tex_params_2 : ExternalTextureParams, s : sampler, t2 : texture_2d<f32>, ext_tex_plane_1_3 : texture_2d<f32>, ext_tex_params_3 : ExternalTextureParams) {
  textureSampleExternal(t, ext_tex_plane_1_2, s, vec2<f32>(1.0, 2.0), ext_tex_params_2);
  textureSampleExternal(t2, ext_tex_plane_1_3, s, vec2<f32>(1.0, 2.0), ext_tex_params_3);
}

@group(0) @binding(0) var ext_tex : texture_2d<f32>;

@group(0) @binding(1) var smp : sampler;

@group(0) @binding(2) var ext_tex2 : texture_2d<f32>;
)";
  DataMap data;
  data.Add<MultiplanarExternalTexture::NewBindingPoints>(
      MultiplanarExternalTexture::BindingsMap{
          {{0, 0}, {{0, 3}, {0, 4}}},
          {{0, 2}, {{0, 5}, {0, 6}}},
      });
  auto got = Run<MultiplanarExternalTexture>(src, data);
  EXPECT_EQ(expect, str(got));
}

// Tests that the texture_external passed to as a parameter to multiple
// functions produces the correct output.
TEST_F(MultiplanarExternalTextureTest, ExternalTexturePassedAsParamNested) {
  auto* src = R"(
fn nested(t : texture_external, s : sampler) {
  textureSampleLevel(t, s, vec2<f32>(1.0, 2.0));
}

fn f(t : texture_external, s : sampler) {
  nested(t, s);
}

@group(0) @binding(0) var ext_tex : texture_external;
@group(0) @binding(1) var smp : sampler;

@stage(fragment)
fn main() {
  f(ext_tex, smp);
}
)";

  auto* expect = R"(
struct ExternalTextureParams {
  numPlanes : u32;
  vr : f32;
  ug : f32;
  vg : f32;
  ub : f32;
}

@group(0) @binding(2) var ext_tex_plane_1 : texture_2d<f32>;

@group(0) @binding(3) var<uniform> ext_tex_params : ExternalTextureParams;

fn textureSampleExternal(plane0 : texture_2d<f32>, plane1 : texture_2d<f32>, smp : sampler, coord : vec2<f32>, params : ExternalTextureParams) -> vec4<f32> {
  if ((params.numPlanes == 1u)) {
    return textureSampleLevel(plane0, smp, coord, 0.0);
  }
  let y = (textureSampleLevel(plane0, smp, coord, 0.0).r - 0.0625);
  let uv = (textureSampleLevel(plane1, smp, coord, 0.0).rg - 0.5);
  let u = uv.x;
  let v = uv.y;
  let r = ((1.164000034 * y) + (params.vr * v));
  let g = (((1.164000034 * y) - (params.ug * u)) - (params.vg * v));
  let b = ((1.164000034 * y) + (params.ub * u));
  return vec4<f32>(r, g, b, 1.0);
}

fn nested(t : texture_2d<f32>, ext_tex_plane_1_1 : texture_2d<f32>, ext_tex_params_1 : ExternalTextureParams, s : sampler) {
  textureSampleExternal(t, ext_tex_plane_1_1, s, vec2<f32>(1.0, 2.0), ext_tex_params_1);
}

fn f(t : texture_2d<f32>, ext_tex_plane_1_2 : texture_2d<f32>, ext_tex_params_2 : ExternalTextureParams, s : sampler) {
  nested(t, ext_tex_plane_1_2, ext_tex_params_2, s);
}

@group(0) @binding(0) var ext_tex : texture_2d<f32>;

@group(0) @binding(1) var smp : sampler;

@stage(fragment)
fn main() {
  f(ext_tex, ext_tex_plane_1, ext_tex_params, smp);
}
)";
  DataMap data;
  data.Add<MultiplanarExternalTexture::NewBindingPoints>(
      MultiplanarExternalTexture::BindingsMap{
          {{0, 0}, {{0, 2}, {0, 3}}},
      });
  auto got = Run<MultiplanarExternalTexture>(src, data);
  EXPECT_EQ(expect, str(got));
}

// Tests that the texture_external passed to as a parameter to multiple
// functions produces the correct output.
TEST_F(MultiplanarExternalTextureTest,
       ExternalTexturePassedAsParamNested_OutOfOrder) {
  auto* src = R"(
fn nested(t : texture_external, s : sampler) {
  textureSampleLevel(t, s, vec2<f32>(1.0, 2.0));
}

fn f(t : texture_external, s : sampler) {
  nested(t, s);
}

@group(0) @binding(0) var ext_tex : texture_external;
@group(0) @binding(1) var smp : sampler;

@stage(fragment)
fn main() {
  f(ext_tex, smp);
}
)";

  auto* expect = R"(
struct ExternalTextureParams {
  numPlanes : u32;
  vr : f32;
  ug : f32;
  vg : f32;
  ub : f32;
}

@group(0) @binding(2) var ext_tex_plane_1 : texture_2d<f32>;

@group(0) @binding(3) var<uniform> ext_tex_params : ExternalTextureParams;

fn textureSampleExternal(plane0 : texture_2d<f32>, plane1 : texture_2d<f32>, smp : sampler, coord : vec2<f32>, params : ExternalTextureParams) -> vec4<f32> {
  if ((params.numPlanes == 1u)) {
    return textureSampleLevel(plane0, smp, coord, 0.0);
  }
  let y = (textureSampleLevel(plane0, smp, coord, 0.0).r - 0.0625);
  let uv = (textureSampleLevel(plane1, smp, coord, 0.0).rg - 0.5);
  let u = uv.x;
  let v = uv.y;
  let r = ((1.164000034 * y) + (params.vr * v));
  let g = (((1.164000034 * y) - (params.ug * u)) - (params.vg * v));
  let b = ((1.164000034 * y) + (params.ub * u));
  return vec4<f32>(r, g, b, 1.0);
}

fn nested(t : texture_2d<f32>, ext_tex_plane_1_1 : texture_2d<f32>, ext_tex_params_1 : ExternalTextureParams, s : sampler) {
  textureSampleExternal(t, ext_tex_plane_1_1, s, vec2<f32>(1.0, 2.0), ext_tex_params_1);
}

fn f(t : texture_2d<f32>, ext_tex_plane_1_2 : texture_2d<f32>, ext_tex_params_2 : ExternalTextureParams, s : sampler) {
  nested(t, ext_tex_plane_1_2, ext_tex_params_2, s);
}

@group(0) @binding(0) var ext_tex : texture_2d<f32>;

@group(0) @binding(1) var smp : sampler;

@stage(fragment)
fn main() {
  f(ext_tex, ext_tex_plane_1, ext_tex_params, smp);
}
)";
  DataMap data;
  data.Add<MultiplanarExternalTexture::NewBindingPoints>(
      MultiplanarExternalTexture::BindingsMap{
          {{0, 0}, {{0, 2}, {0, 3}}},
      });
  auto got = Run<MultiplanarExternalTexture>(src, data);
  EXPECT_EQ(expect, str(got));
}

// Tests that the transform works with a function using an external texture,
// even if there's no external texture declared at module scope.
TEST_F(MultiplanarExternalTextureTest,
       ExternalTexturePassedAsParamWithoutGlobalDecl) {
  auto* src = R"(
fn f(ext_tex : texture_external) -> vec2<i32> {
  return textureDimensions(ext_tex);
}
)";

  auto* expect = R"(
struct ExternalTextureParams {
  numPlanes : u32;
  vr : f32;
  ug : f32;
  vg : f32;
  ub : f32;
}

fn f(ext_tex : texture_2d<f32>, ext_tex_plane_1 : texture_2d<f32>, ext_tex_params : ExternalTextureParams) -> vec2<i32> {
  return textureDimensions(ext_tex);
}
)";

  DataMap data;
  data.Add<MultiplanarExternalTexture::NewBindingPoints>(
      MultiplanarExternalTexture::BindingsMap{{{0, 0}, {{0, 1}, {0, 2}}}});
  auto got = Run<MultiplanarExternalTexture>(src, data);
  EXPECT_EQ(expect, str(got));
}

// Tests that the the transform handles aliases to external textures
TEST_F(MultiplanarExternalTextureTest, ExternalTextureAlias) {
  auto* src = R"(
type ET = texture_external;

fn f(t : ET, s : sampler) {
  textureSampleLevel(t, s, vec2<f32>(1.0, 2.0));
}

[[group(0), binding(0)]] var ext_tex : ET;
[[group(0), binding(1)]] var smp : sampler;

[[stage(fragment)]]
fn main() {
  f(ext_tex, smp);
}
)";

  auto* expect = R"(
struct ExternalTextureParams {
  numPlanes : u32;
  vr : f32;
  ug : f32;
  vg : f32;
  ub : f32;
}

@group(0) @binding(2) var ext_tex_plane_1 : texture_2d<f32>;

@group(0) @binding(3) var<uniform> ext_tex_params : ExternalTextureParams;

type ET = texture_external;

fn textureSampleExternal(plane0 : texture_2d<f32>, plane1 : texture_2d<f32>, smp : sampler, coord : vec2<f32>, params : ExternalTextureParams) -> vec4<f32> {
  if ((params.numPlanes == 1u)) {
    return textureSampleLevel(plane0, smp, coord, 0.0);
  }
  let y = (textureSampleLevel(plane0, smp, coord, 0.0).r - 0.0625);
  let uv = (textureSampleLevel(plane1, smp, coord, 0.0).rg - 0.5);
  let u = uv.x;
  let v = uv.y;
  let r = ((1.164000034 * y) + (params.vr * v));
  let g = (((1.164000034 * y) - (params.ug * u)) - (params.vg * v));
  let b = ((1.164000034 * y) + (params.ub * u));
  return vec4<f32>(r, g, b, 1.0);
}

fn f(t : texture_2d<f32>, ext_tex_plane_1_1 : texture_2d<f32>, ext_tex_params_1 : ExternalTextureParams, s : sampler) {
  textureSampleExternal(t, ext_tex_plane_1_1, s, vec2<f32>(1.0, 2.0), ext_tex_params_1);
}

@group(0) @binding(0) var ext_tex : texture_2d<f32>;

@group(0) @binding(1) var smp : sampler;

@stage(fragment)
fn main() {
  f(ext_tex, ext_tex_plane_1, ext_tex_params, smp);
}
)";
  DataMap data;
  data.Add<MultiplanarExternalTexture::NewBindingPoints>(
      MultiplanarExternalTexture::BindingsMap{
          {{0, 0}, {{0, 2}, {0, 3}}},
      });
  auto got = Run<MultiplanarExternalTexture>(src, data);
  EXPECT_EQ(expect, str(got));
}

// Tests that the the transform handles aliases to external textures
TEST_F(MultiplanarExternalTextureTest, ExternalTextureAlias_OutOfOrder) {
  auto* src = R"(
[[stage(fragment)]]
fn main() {
  f(ext_tex, smp);
}

fn f(t : ET, s : sampler) {
  textureSampleLevel(t, s, vec2<f32>(1.0, 2.0));
}

[[group(0), binding(0)]] var ext_tex : ET;
[[group(0), binding(1)]] var smp : sampler;

type ET = texture_external;
)";

  auto* expect = R"(
struct ExternalTextureParams {
  numPlanes : u32;
  vr : f32;
  ug : f32;
  vg : f32;
  ub : f32;
}

@group(0) @binding(2) var ext_tex_plane_1 : texture_2d<f32>;

@group(0) @binding(3) var<uniform> ext_tex_params : ExternalTextureParams;

@stage(fragment)
fn main() {
  f(ext_tex, ext_tex_plane_1, ext_tex_params, smp);
}

fn textureSampleExternal(plane0 : texture_2d<f32>, plane1 : texture_2d<f32>, smp : sampler, coord : vec2<f32>, params : ExternalTextureParams) -> vec4<f32> {
  if ((params.numPlanes == 1u)) {
    return textureSampleLevel(plane0, smp, coord, 0.0);
  }
  let y = (textureSampleLevel(plane0, smp, coord, 0.0).r - 0.0625);
  let uv = (textureSampleLevel(plane1, smp, coord, 0.0).rg - 0.5);
  let u = uv.x;
  let v = uv.y;
  let r = ((1.164000034 * y) + (params.vr * v));
  let g = (((1.164000034 * y) - (params.ug * u)) - (params.vg * v));
  let b = ((1.164000034 * y) + (params.ub * u));
  return vec4<f32>(r, g, b, 1.0);
}

fn f(t : texture_2d<f32>, ext_tex_plane_1_1 : texture_2d<f32>, ext_tex_params_1 : ExternalTextureParams, s : sampler) {
  textureSampleExternal(t, ext_tex_plane_1_1, s, vec2<f32>(1.0, 2.0), ext_tex_params_1);
}

@group(0) @binding(0) var ext_tex : texture_2d<f32>;

@group(0) @binding(1) var smp : sampler;

type ET = texture_external;
)";
  DataMap data;
  data.Add<MultiplanarExternalTexture::NewBindingPoints>(
      MultiplanarExternalTexture::BindingsMap{
          {{0, 0}, {{0, 2}, {0, 3}}},
      });
  auto got = Run<MultiplanarExternalTexture>(src, data);
  EXPECT_EQ(expect, str(got));
}

}  // namespace
}  // namespace transform
}  // namespace tint
