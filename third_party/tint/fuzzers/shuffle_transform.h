// Copyright 2022 The Tint Authors.
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

#ifndef FUZZERS_SHUFFLE_TRANSFORM_H_
#define FUZZERS_SHUFFLE_TRANSFORM_H_

#include "src/transform/transform.h"

namespace tint {
namespace fuzzers {

/// ShuffleTransform reorders the module scope declarations into a random order
class ShuffleTransform : public tint::transform::Transform {
 public:
  /// Constructor
  /// @param seed the random seed to use for the shuffling
  explicit ShuffleTransform(size_t seed);

 protected:
  void Run(CloneContext& ctx,
           const tint::transform::DataMap&,
           tint::transform::DataMap&) const override;

 private:
  size_t seed_;
};

}  // namespace fuzzers
}  // namespace tint

#endif  // FUZZERS_SHUFFLE_TRANSFORM_H_
