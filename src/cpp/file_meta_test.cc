#include "file_meta.h"
#include <gtest/gtest.h>
#include <memory>
#include <spdlog/spdlog.h>
// Demonstrate some basic assertions.
TEST(FileNodeTest, HandleAppendNode) {
  std::shared_ptr<FileNode> node =
      std::make_shared<FileNode>("/", FileType::IFDIR);
  {
    std::shared_ptr<FileNode> home =
        std::make_shared<FileNode>("home", FileType::IFDIR);
    node->addSubFile(home);
  }

  ASSERT_TRUE(node->isSubFile("home"));
  ASSERT_EQ(node->getSubFile("home")->getName(), "home");
}

TEST(FileNodeTest, HandleGetSubs) {
  std::shared_ptr<FileNode> node =
      std::make_shared<FileNode>("/", FileType::IFDIR);
  std::shared_ptr<FileNode> home =
      std::make_shared<FileNode>("home", FileType::IFDIR);
  node->addSubFile(home);
  std::shared_ptr<FileNode> etc =
      std::make_shared<FileNode>("etc", FileType::IFDIR);
  node->addSubFile(etc);

  auto subs{node->listSubFiles()};

  ASSERT_EQ(2, subs.size());
}