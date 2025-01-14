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

#include <utility>

#include "gtest/gtest-spi.h"
#include "src/program_builder.h"
#include "src/transform/test_helper.h"
#include "src/transform/utils/hoist_to_decl_before.h"

namespace tint::transform {
namespace {

using HoistToDeclBeforeTest = ::testing::Test;

TEST_F(HoistToDeclBeforeTest, VarInit) {
  // fn f() {
  //     var a = 1;
  // }
  ProgramBuilder b;
  auto* expr = b.Expr(1);
  auto* var = b.Decl(b.Var("a", nullptr, expr));
  b.Func("f", {}, b.ty.void_(), {var});

  Program original(std::move(b));
  ProgramBuilder cloned_b;
  CloneContext ctx(&cloned_b, &original);

  HoistToDeclBefore hoistToDeclBefore(ctx);
  auto* sem_expr = ctx.src->Sem().Get(expr);
  hoistToDeclBefore.Add(sem_expr, expr, true);
  hoistToDeclBefore.Apply();

  ctx.Clone();
  Program cloned(std::move(cloned_b));

  auto* expect = R"(
fn f() {
  let tint_symbol = 1;
  var a = tint_symbol;
}
)";

  EXPECT_EQ(expect, str(cloned));
}

TEST_F(HoistToDeclBeforeTest, ForLoopInit) {
  // fn f() {
  //     for(var a = 1; true; ) {
  //     }
  // }
  ProgramBuilder b;
  auto* expr = b.Expr(1);
  auto* s =
      b.For(b.Decl(b.Var("a", nullptr, expr)), b.Expr(true), {}, b.Block());
  b.Func("f", {}, b.ty.void_(), {s});

  Program original(std::move(b));
  ProgramBuilder cloned_b;
  CloneContext ctx(&cloned_b, &original);

  HoistToDeclBefore hoistToDeclBefore(ctx);
  auto* sem_expr = ctx.src->Sem().Get(expr);
  hoistToDeclBefore.Add(sem_expr, expr, true);
  hoistToDeclBefore.Apply();

  ctx.Clone();
  Program cloned(std::move(cloned_b));

  auto* expect = R"(
fn f() {
  let tint_symbol = 1;
  for(var a = tint_symbol; true; ) {
  }
}
)";

  EXPECT_EQ(expect, str(cloned));
}

TEST_F(HoistToDeclBeforeTest, ForLoopCond) {
  // fn f() {
  //     var a : bool;
  //     for(; a; ) {
  //     }
  // }
  ProgramBuilder b;
  auto* var = b.Decl(b.Var("a", b.ty.bool_()));
  auto* expr = b.Expr("a");
  auto* s = b.For({}, expr, {}, b.Block());
  b.Func("f", {}, b.ty.void_(), {var, s});

  Program original(std::move(b));
  ProgramBuilder cloned_b;
  CloneContext ctx(&cloned_b, &original);

  HoistToDeclBefore hoistToDeclBefore(ctx);
  auto* sem_expr = ctx.src->Sem().Get(expr);
  hoistToDeclBefore.Add(sem_expr, expr, true);
  hoistToDeclBefore.Apply();

  ctx.Clone();
  Program cloned(std::move(cloned_b));

  auto* expect = R"(
fn f() {
  var a : bool;
  loop {
    let tint_symbol = a;
    if (!(tint_symbol)) {
      break;
    }
    {
    }
  }
}
)";

  EXPECT_EQ(expect, str(cloned));
}

TEST_F(HoistToDeclBeforeTest, ForLoopCont) {
  // fn f() {
  //     for(; true; var a = 1) {
  //     }
  // }
  ProgramBuilder b;
  auto* expr = b.Expr(1);
  auto* s =
      b.For({}, b.Expr(true), b.Decl(b.Var("a", nullptr, expr)), b.Block());
  b.Func("f", {}, b.ty.void_(), {s});

  Program original(std::move(b));
  ProgramBuilder cloned_b;
  CloneContext ctx(&cloned_b, &original);

  HoistToDeclBefore hoistToDeclBefore(ctx);
  auto* sem_expr = ctx.src->Sem().Get(expr);
  hoistToDeclBefore.Add(sem_expr, expr, true);
  hoistToDeclBefore.Apply();

  ctx.Clone();
  Program cloned(std::move(cloned_b));

  auto* expect = R"(
fn f() {
  loop {
    if (!(true)) {
      break;
    }
    {
    }

    continuing {
      let tint_symbol = 1;
      var a = tint_symbol;
    }
  }
}
)";

  EXPECT_EQ(expect, str(cloned));
}

TEST_F(HoistToDeclBeforeTest, ElseIf) {
  // fn f() {
  //     var a : bool;
  //     if (true) {
  //     } else if (a) {
  //     } else {
  //     }
  // }
  ProgramBuilder b;
  auto* var = b.Decl(b.Var("a", b.ty.bool_()));
  auto* expr = b.Expr("a");
  auto* s = b.If(b.Expr(true), b.Block(),  //
                 b.Else(expr, b.Block()),  //
                 b.Else(b.Block()));
  b.Func("f", {}, b.ty.void_(), {var, s});

  Program original(std::move(b));
  ProgramBuilder cloned_b;
  CloneContext ctx(&cloned_b, &original);

  HoistToDeclBefore hoistToDeclBefore(ctx);
  auto* sem_expr = ctx.src->Sem().Get(expr);
  hoistToDeclBefore.Add(sem_expr, expr, true);
  hoistToDeclBefore.Apply();

  ctx.Clone();
  Program cloned(std::move(cloned_b));

  auto* expect = R"(
fn f() {
  var a : bool;
  if (true) {
  } else {
    let tint_symbol = a;
    if (tint_symbol) {
    } else {
    }
  }
}
)";

  EXPECT_EQ(expect, str(cloned));
}

}  // namespace
}  // namespace tint::transform
