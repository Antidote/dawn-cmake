// Copyright 2020 The Tint Authors.
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

#include "src/castable.h"

#include <memory>
#include <string>

#include "gtest/gtest.h"

namespace tint {
namespace {

struct Animal : public tint::Castable<Animal> {
  explicit Animal(std::string n) : name(n) {}
  const std::string name;
};

struct Amphibian : public tint::Castable<Amphibian, Animal> {
  explicit Amphibian(std::string n) : Base(n) {}
};

struct Mammal : public tint::Castable<Mammal, Animal> {
  explicit Mammal(std::string n) : Base(n) {}
};

struct Reptile : public tint::Castable<Reptile, Animal> {
  explicit Reptile(std::string n) : Base(n) {}
};

struct Frog : public tint::Castable<Frog, Amphibian> {
  Frog() : Base("Frog") {}
};

struct Bear : public tint::Castable<Bear, Mammal> {
  Bear() : Base("Bear") {}
};

struct Gecko : public tint::Castable<Gecko, Reptile> {
  Gecko() : Base("Gecko") {}
};

TEST(CastableBase, Is) {
  std::unique_ptr<CastableBase> frog = std::make_unique<Frog>();
  std::unique_ptr<CastableBase> bear = std::make_unique<Bear>();
  std::unique_ptr<CastableBase> gecko = std::make_unique<Gecko>();

  ASSERT_TRUE(frog->Is<Animal>());
  ASSERT_TRUE(bear->Is<Animal>());
  ASSERT_TRUE(gecko->Is<Animal>());

  ASSERT_TRUE(frog->Is<Amphibian>());
  ASSERT_FALSE(bear->Is<Amphibian>());
  ASSERT_FALSE(gecko->Is<Amphibian>());

  ASSERT_FALSE(frog->Is<Mammal>());
  ASSERT_TRUE(bear->Is<Mammal>());
  ASSERT_FALSE(gecko->Is<Mammal>());

  ASSERT_FALSE(frog->Is<Reptile>());
  ASSERT_FALSE(bear->Is<Reptile>());
  ASSERT_TRUE(gecko->Is<Reptile>());
}

TEST(CastableBase, Is_kDontErrorOnImpossibleCast) {
  // Unlike TEST(CastableBase, Is), we're dynamically querying [A -> B] without
  // going via CastableBase.
  auto frog = std::make_unique<Frog>();
  auto bear = std::make_unique<Bear>();
  auto gecko = std::make_unique<Gecko>();

  ASSERT_TRUE((frog->Is<Animal, kDontErrorOnImpossibleCast>()));
  ASSERT_TRUE((bear->Is<Animal, kDontErrorOnImpossibleCast>()));
  ASSERT_TRUE((gecko->Is<Animal, kDontErrorOnImpossibleCast>()));

  ASSERT_TRUE((frog->Is<Amphibian, kDontErrorOnImpossibleCast>()));
  ASSERT_FALSE((bear->Is<Amphibian, kDontErrorOnImpossibleCast>()));
  ASSERT_FALSE((gecko->Is<Amphibian, kDontErrorOnImpossibleCast>()));

  ASSERT_FALSE((frog->Is<Mammal, kDontErrorOnImpossibleCast>()));
  ASSERT_TRUE((bear->Is<Mammal, kDontErrorOnImpossibleCast>()));
  ASSERT_FALSE((gecko->Is<Mammal, kDontErrorOnImpossibleCast>()));

  ASSERT_FALSE((frog->Is<Reptile, kDontErrorOnImpossibleCast>()));
  ASSERT_FALSE((bear->Is<Reptile, kDontErrorOnImpossibleCast>()));
  ASSERT_TRUE((gecko->Is<Reptile, kDontErrorOnImpossibleCast>()));
}

TEST(CastableBase, IsWithPredicate) {
  std::unique_ptr<CastableBase> frog = std::make_unique<Frog>();

  frog->Is<Animal>([&frog](const Animal* a) {
    EXPECT_EQ(a, frog.get());
    return true;
  });

  ASSERT_TRUE((frog->Is<Animal>([](const Animal*) { return true; })));
  ASSERT_FALSE((frog->Is<Animal>([](const Animal*) { return false; })));

  // Predicate not called if cast is invalid
  auto expect_not_called = [] { FAIL() << "Should not be called"; };
  ASSERT_FALSE((frog->Is<Bear>([&](const Animal*) {
    expect_not_called();
    return true;
  })));
}

TEST(CastableBase, IsAnyOf) {
  std::unique_ptr<CastableBase> frog = std::make_unique<Frog>();
  std::unique_ptr<CastableBase> bear = std::make_unique<Bear>();
  std::unique_ptr<CastableBase> gecko = std::make_unique<Gecko>();

  ASSERT_TRUE((frog->IsAnyOf<Animal, Mammal, Amphibian, Reptile>()));
  ASSERT_TRUE((frog->IsAnyOf<Mammal, Amphibian>()));
  ASSERT_TRUE((frog->IsAnyOf<Amphibian, Reptile>()));
  ASSERT_FALSE((frog->IsAnyOf<Mammal, Reptile>()));

  ASSERT_TRUE((bear->IsAnyOf<Animal, Mammal, Amphibian, Reptile>()));
  ASSERT_TRUE((bear->IsAnyOf<Mammal, Amphibian>()));
  ASSERT_TRUE((bear->IsAnyOf<Mammal, Reptile>()));
  ASSERT_FALSE((bear->IsAnyOf<Amphibian, Reptile>()));

  ASSERT_TRUE((gecko->IsAnyOf<Animal, Mammal, Amphibian, Reptile>()));
  ASSERT_TRUE((gecko->IsAnyOf<Mammal, Reptile>()));
  ASSERT_TRUE((gecko->IsAnyOf<Amphibian, Reptile>()));
  ASSERT_FALSE((gecko->IsAnyOf<Mammal, Amphibian>()));
}

TEST(CastableBase, As) {
  std::unique_ptr<CastableBase> frog = std::make_unique<Frog>();
  std::unique_ptr<CastableBase> bear = std::make_unique<Bear>();
  std::unique_ptr<CastableBase> gecko = std::make_unique<Gecko>();

  ASSERT_EQ(frog->As<Animal>(), static_cast<Animal*>(frog.get()));
  ASSERT_EQ(bear->As<Animal>(), static_cast<Animal*>(bear.get()));
  ASSERT_EQ(gecko->As<Animal>(), static_cast<Animal*>(gecko.get()));

  ASSERT_EQ(frog->As<Amphibian>(), static_cast<Amphibian*>(frog.get()));
  ASSERT_EQ(bear->As<Amphibian>(), nullptr);
  ASSERT_EQ(gecko->As<Amphibian>(), nullptr);

  ASSERT_EQ(frog->As<Mammal>(), nullptr);
  ASSERT_EQ(bear->As<Mammal>(), static_cast<Mammal*>(bear.get()));
  ASSERT_EQ(gecko->As<Mammal>(), nullptr);

  ASSERT_EQ(frog->As<Reptile>(), nullptr);
  ASSERT_EQ(bear->As<Reptile>(), nullptr);
  ASSERT_EQ(gecko->As<Reptile>(), static_cast<Reptile*>(gecko.get()));
}

TEST(CastableBase, As_kDontErrorOnImpossibleCast) {
  // Unlike TEST(CastableBase, As), we're dynamically casting [A -> B] without
  // going via CastableBase.
  auto frog = std::make_unique<Frog>();
  auto bear = std::make_unique<Bear>();
  auto gecko = std::make_unique<Gecko>();

  ASSERT_EQ((frog->As<Animal, kDontErrorOnImpossibleCast>()),
            static_cast<Animal*>(frog.get()));
  ASSERT_EQ((bear->As<Animal, kDontErrorOnImpossibleCast>()),
            static_cast<Animal*>(bear.get()));
  ASSERT_EQ((gecko->As<Animal, kDontErrorOnImpossibleCast>()),
            static_cast<Animal*>(gecko.get()));

  ASSERT_EQ((frog->As<Amphibian, kDontErrorOnImpossibleCast>()),
            static_cast<Amphibian*>(frog.get()));
  ASSERT_EQ((bear->As<Amphibian, kDontErrorOnImpossibleCast>()), nullptr);
  ASSERT_EQ((gecko->As<Amphibian, kDontErrorOnImpossibleCast>()), nullptr);

  ASSERT_EQ((frog->As<Mammal, kDontErrorOnImpossibleCast>()), nullptr);
  ASSERT_EQ((bear->As<Mammal, kDontErrorOnImpossibleCast>()),
            static_cast<Mammal*>(bear.get()));
  ASSERT_EQ((gecko->As<Mammal, kDontErrorOnImpossibleCast>()), nullptr);

  ASSERT_EQ((frog->As<Reptile, kDontErrorOnImpossibleCast>()), nullptr);
  ASSERT_EQ((bear->As<Reptile, kDontErrorOnImpossibleCast>()), nullptr);
  ASSERT_EQ((gecko->As<Reptile, kDontErrorOnImpossibleCast>()),
            static_cast<Reptile*>(gecko.get()));
}

TEST(Castable, Is) {
  std::unique_ptr<Animal> frog = std::make_unique<Frog>();
  std::unique_ptr<Animal> bear = std::make_unique<Bear>();
  std::unique_ptr<Animal> gecko = std::make_unique<Gecko>();

  ASSERT_TRUE(frog->Is<Animal>());
  ASSERT_TRUE(bear->Is<Animal>());
  ASSERT_TRUE(gecko->Is<Animal>());

  ASSERT_TRUE(frog->Is<Amphibian>());
  ASSERT_FALSE(bear->Is<Amphibian>());
  ASSERT_FALSE(gecko->Is<Amphibian>());

  ASSERT_FALSE(frog->Is<Mammal>());
  ASSERT_TRUE(bear->Is<Mammal>());
  ASSERT_FALSE(gecko->Is<Mammal>());

  ASSERT_FALSE(frog->Is<Reptile>());
  ASSERT_FALSE(bear->Is<Reptile>());
  ASSERT_TRUE(gecko->Is<Reptile>());
}

TEST(Castable, IsWithPredicate) {
  std::unique_ptr<Animal> frog = std::make_unique<Frog>();

  frog->Is([&frog](const Animal* a) {
    EXPECT_EQ(a, frog.get());
    return true;
  });

  ASSERT_TRUE((frog->Is([](const Animal*) { return true; })));
  ASSERT_FALSE((frog->Is([](const Animal*) { return false; })));

  // Predicate not called if cast is invalid
  auto expect_not_called = [] { FAIL() << "Should not be called"; };
  ASSERT_FALSE((frog->Is([&](const Bear*) {
    expect_not_called();
    return true;
  })));
}

TEST(Castable, As) {
  std::unique_ptr<Animal> frog = std::make_unique<Frog>();
  std::unique_ptr<Animal> bear = std::make_unique<Bear>();
  std::unique_ptr<Animal> gecko = std::make_unique<Gecko>();

  ASSERT_EQ(frog->As<Animal>(), static_cast<Animal*>(frog.get()));
  ASSERT_EQ(bear->As<Animal>(), static_cast<Animal*>(bear.get()));
  ASSERT_EQ(gecko->As<Animal>(), static_cast<Animal*>(gecko.get()));

  ASSERT_EQ(frog->As<Amphibian>(), static_cast<Amphibian*>(frog.get()));
  ASSERT_EQ(bear->As<Amphibian>(), nullptr);
  ASSERT_EQ(gecko->As<Amphibian>(), nullptr);

  ASSERT_EQ(frog->As<Mammal>(), nullptr);
  ASSERT_EQ(bear->As<Mammal>(), static_cast<Mammal*>(bear.get()));
  ASSERT_EQ(gecko->As<Mammal>(), nullptr);

  ASSERT_EQ(frog->As<Reptile>(), nullptr);
  ASSERT_EQ(bear->As<Reptile>(), nullptr);
  ASSERT_EQ(gecko->As<Reptile>(), static_cast<Reptile*>(gecko.get()));
}

TEST(Castable, SwitchNoDefault) {
  std::unique_ptr<Animal> frog = std::make_unique<Frog>();
  std::unique_ptr<Animal> bear = std::make_unique<Bear>();
  std::unique_ptr<Animal> gecko = std::make_unique<Gecko>();
  {
    bool frog_matched_amphibian = false;
    Switch(
        frog.get(),  //
        [&](Reptile*) { FAIL() << "frog is not reptile"; },
        [&](Mammal*) { FAIL() << "frog is not mammal"; },
        [&](Amphibian* amphibian) {
          EXPECT_EQ(amphibian, frog.get());
          frog_matched_amphibian = true;
        });
    EXPECT_TRUE(frog_matched_amphibian);
  }
  {
    bool bear_matched_mammal = false;
    Switch(
        bear.get(),  //
        [&](Reptile*) { FAIL() << "bear is not reptile"; },
        [&](Amphibian*) { FAIL() << "bear is not amphibian"; },
        [&](Mammal* mammal) {
          EXPECT_EQ(mammal, bear.get());
          bear_matched_mammal = true;
        });
    EXPECT_TRUE(bear_matched_mammal);
  }
  {
    bool gecko_matched_reptile = false;
    Switch(
        gecko.get(),  //
        [&](Mammal*) { FAIL() << "gecko is not mammal"; },
        [&](Amphibian*) { FAIL() << "gecko is not amphibian"; },
        [&](Reptile* reptile) {
          EXPECT_EQ(reptile, gecko.get());
          gecko_matched_reptile = true;
        });
    EXPECT_TRUE(gecko_matched_reptile);
  }
}

TEST(Castable, SwitchWithUnusedDefault) {
  std::unique_ptr<Animal> frog = std::make_unique<Frog>();
  std::unique_ptr<Animal> bear = std::make_unique<Bear>();
  std::unique_ptr<Animal> gecko = std::make_unique<Gecko>();
  {
    bool frog_matched_amphibian = false;
    Switch(
        frog.get(),  //
        [&](Reptile*) { FAIL() << "frog is not reptile"; },
        [&](Mammal*) { FAIL() << "frog is not mammal"; },
        [&](Amphibian* amphibian) {
          EXPECT_EQ(amphibian, frog.get());
          frog_matched_amphibian = true;
        },
        [&](Default) { FAIL() << "default should not have been selected"; });
    EXPECT_TRUE(frog_matched_amphibian);
  }
  {
    bool bear_matched_mammal = false;
    Switch(
        bear.get(),  //
        [&](Reptile*) { FAIL() << "bear is not reptile"; },
        [&](Amphibian*) { FAIL() << "bear is not amphibian"; },
        [&](Mammal* mammal) {
          EXPECT_EQ(mammal, bear.get());
          bear_matched_mammal = true;
        },
        [&](Default) { FAIL() << "default should not have been selected"; });
    EXPECT_TRUE(bear_matched_mammal);
  }
  {
    bool gecko_matched_reptile = false;
    Switch(
        gecko.get(),  //
        [&](Mammal*) { FAIL() << "gecko is not mammal"; },
        [&](Amphibian*) { FAIL() << "gecko is not amphibian"; },
        [&](Reptile* reptile) {
          EXPECT_EQ(reptile, gecko.get());
          gecko_matched_reptile = true;
        },
        [&](Default) { FAIL() << "default should not have been selected"; });
    EXPECT_TRUE(gecko_matched_reptile);
  }
}

TEST(Castable, SwitchDefault) {
  std::unique_ptr<Animal> frog = std::make_unique<Frog>();
  std::unique_ptr<Animal> bear = std::make_unique<Bear>();
  std::unique_ptr<Animal> gecko = std::make_unique<Gecko>();
  {
    bool frog_matched_default = false;
    Switch(
        frog.get(),  //
        [&](Reptile*) { FAIL() << "frog is not reptile"; },
        [&](Mammal*) { FAIL() << "frog is not mammal"; },
        [&](Default) { frog_matched_default = true; });
    EXPECT_TRUE(frog_matched_default);
  }
  {
    bool bear_matched_default = false;
    Switch(
        bear.get(),  //
        [&](Reptile*) { FAIL() << "bear is not reptile"; },
        [&](Amphibian*) { FAIL() << "bear is not amphibian"; },
        [&](Default) { bear_matched_default = true; });
    EXPECT_TRUE(bear_matched_default);
  }
  {
    bool gecko_matched_default = false;
    Switch(
        gecko.get(),  //
        [&](Mammal*) { FAIL() << "gecko is not mammal"; },
        [&](Amphibian*) { FAIL() << "gecko is not amphibian"; },
        [&](Default) { gecko_matched_default = true; });
    EXPECT_TRUE(gecko_matched_default);
  }
}

TEST(Castable, SwitchMatchFirst) {
  std::unique_ptr<Animal> frog = std::make_unique<Frog>();
  {
    bool frog_matched_animal = false;
    Switch(
        frog.get(),
        [&](Animal* animal) {
          EXPECT_EQ(animal, frog.get());
          frog_matched_animal = true;
        },
        [&](Amphibian*) { FAIL() << "animal should have been matched first"; });
    EXPECT_TRUE(frog_matched_animal);
  }
  {
    bool frog_matched_amphibian = false;
    Switch(
        frog.get(),
        [&](Amphibian* amphibain) {
          EXPECT_EQ(amphibain, frog.get());
          frog_matched_amphibian = true;
        },
        [&](Animal*) { FAIL() << "amphibian should have been matched first"; });
    EXPECT_TRUE(frog_matched_amphibian);
  }
}

TEST(Castable, SwitchNull) {
  Animal* null = nullptr;
  Switch(
      null,  //
      [&](Amphibian*) { FAIL() << "should not be called"; },
      [&](Animal*) { FAIL() << "should not be called"; });
}

TEST(Castable, SwitchNullNoDefault) {
  Animal* null = nullptr;
  bool default_called = false;
  Switch(
      null,  //
      [&](Amphibian*) { FAIL() << "should not be called"; },
      [&](Animal*) { FAIL() << "should not be called"; },
      [&](Default) { default_called = true; });
  EXPECT_TRUE(default_called);
}

}  // namespace

TINT_INSTANTIATE_TYPEINFO(Animal);
TINT_INSTANTIATE_TYPEINFO(Amphibian);
TINT_INSTANTIATE_TYPEINFO(Mammal);
TINT_INSTANTIATE_TYPEINFO(Reptile);
TINT_INSTANTIATE_TYPEINFO(Frog);
TINT_INSTANTIATE_TYPEINFO(Bear);
TINT_INSTANTIATE_TYPEINFO(Gecko);

}  // namespace tint
