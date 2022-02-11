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

#ifndef SRC_TRANSFORM_REMOVE_UNREACHABLE_STATEMENTS_H_
#define SRC_TRANSFORM_REMOVE_UNREACHABLE_STATEMENTS_H_

#include <string>
#include <unordered_map>

#include "src/transform/transform.h"

namespace tint {
namespace transform {

/// RemoveUnreachableStatements is a Transform that removes all statements
/// marked as unreachable.
class RemoveUnreachableStatements
    : public Castable<RemoveUnreachableStatements, Transform> {
 public:
  /// Constructor
  RemoveUnreachableStatements();

  /// Destructor
  ~RemoveUnreachableStatements() override;

  /// @param program the program to inspect
  /// @param data optional extra transform-specific input data
  /// @returns true if this transform should be run for the given program
  bool ShouldRun(const Program* program,
                 const DataMap& data = {}) const override;

 protected:
  /// Runs the transform using the CloneContext built for transforming a
  /// program. Run() is responsible for calling Clone() on the CloneContext.
  /// @param ctx the CloneContext primed with the input program and
  /// ProgramBuilder
  /// @param inputs optional extra transform-specific input data
  /// @param outputs optional extra transform-specific output data
  void Run(CloneContext& ctx,
           const DataMap& inputs,
           DataMap& outputs) const override;
};

}  // namespace transform
}  // namespace tint

#endif  // SRC_TRANSFORM_REMOVE_UNREACHABLE_STATEMENTS_H_
