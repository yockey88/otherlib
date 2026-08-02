/**
 * \file tests/core/reflection_tests.cpp
 **/
#include "reflection_tests.hpp"

#include <refl/refl.hpp>

#include "serialization/reflection.hpp"

namespace other {

  struct test_struct {
    int a;
    float b;
    std::string c;

    int not_serialized_but_reflected;
    float not_serialized_or_reflected;
  };

}  // namespace other

OTHER_REFLECT(
  other::test_struct,
  field(a, other::attr::serializable()),
  field(b, other::attr::serializable()),
  field(c, other::attr::serializable()),
  field(not_serialized_but_reflected)
);

namespace other {

  TEST_F(reflection_tests, basic_functionality) {
    test_struct ts{ 42, 3.14f, "Hello, Reflection!" };
    std::string str = type_data_handler<test_struct>::as_string(ts);
    std::println("Reflection Output:\n{}", str);

    ASSERT_NE(str.find("other::test_struct"), std::string::npos);
    ASSERT_NE(str.find("a = 42;"), std::string::npos);
    ASSERT_NE(str.find("b = 3.14;"), std::string::npos);
    ASSERT_NE(str.find("c = \"Hello, Reflection!\";"), std::string::npos);
    ASSERT_EQ(str.find("not_serialized_but_reflected = 0;"), std::string::npos);
    ASSERT_EQ(str.find("not_serialized_or_reflected"), std::string::npos);

    constexpr size_t reflected_fields = 4;
    int32_t found_count = 0;
    refl::util::for_each(refl::reflect(ts).members, [&](const auto& member) {
      std::string str = std::string{ member.name };
      if (str == "a" || str == "b" || str == "c" ||
          str == "not_serialized_but_reflected" || str == "not_serialized_or_reflected") {
        found_count++;
      }
    });
    ASSERT_EQ(found_count, reflected_fields);
  }

  /// regression: the no-arg overload used to insert an empty record and return it
  ///  forever without ever generating the member descriptors
  TEST_F(reflection_tests, get_or_create_populates_member_descriptors) {
    auto* db = subsystem<type_database>::get();
    ASSERT_NE(db, nullptr);

    reflection_data* generated = db->get_reflection_data<test_struct>();
    ASSERT_NE(generated, nullptr);
    ASSERT_FALSE(generated->member_descriptors.empty());
    ASSERT_NE(generated->type_name.find("test_struct"), std::string::npos);

    /// the value-taking overload must resolve to the same cached record
    test_struct ts{};
    reflection_data* by_value = db->get_reflection_data(ts);
    ASSERT_EQ(generated, by_value);
  }

}  // namespace other