#include <gtest/gtest.h>
#include <memory>
#include <vector>
// Demonstrate some basic assertions.
TEST(HelloTest, BasicAssertions) {
  // Expect two strings not to be equal.
  EXPECT_STRNE("hello", "world");
  // Expect equality.
  EXPECT_EQ(7 * 6, 42);
}

static bool is_destruct{false};
class Resource {
public:
  Resource() { std::cout << "Resource Construct" << std::endl; }
  ~Resource() {
    std::cout << "Resource deconstruct" << std::endl;
    is_destruct = true;
  }
};

TEST(CPPBasic, SharedPtr) {
  Resource *r{new Resource()};
  is_destruct = false;
  { std::shared_ptr<Resource> shared{r}; }
  ASSERT_TRUE(is_destruct);
}

TEST(CPPBasic, SharedPtr2) {
  Resource *r{new Resource()};
  is_destruct = false;
  std::vector<std::shared_ptr<Resource>> v;
  {
    std::shared_ptr<Resource> shared{r};
    std::shared_ptr<Resource> &shared_ref{shared};
    v.push_back(shared_ref);
  }
  ASSERT_FALSE(is_destruct);
}

// TEST(CPPBasic, SharedPtr3) {
//   ASSERT_DEATH(
//       {
//         Resource *r{new Resource()};
//         is_destruct = false;
//         std::shared_ptr<Resource> s1{r};
//         { std::shared_ptr<Resource> s2{r}; }
//       },
//       "double free detected");
// }

TEST(CPPBasic, SharedPtr4) {

  Resource *r{new Resource()};
  is_destruct = false;
  std::shared_ptr<Resource> s1{r};
  { std::shared_ptr<const Resource> s2{s1}; }
}

TEST(CPPBasic, UniquePtr) {
  Resource *r{new Resource()};
  is_destruct = false;
  { std::unique_ptr<Resource> unique{r}; }
  ASSERT_TRUE(is_destruct);
}

TEST(CPPBasic, StringToConstChar) {
  std::string a{"123"};
  const char *a_cstr{a.c_str()};
  std::cout << a_cstr << std::endl;
  ASSERT_TRUE(false);
}