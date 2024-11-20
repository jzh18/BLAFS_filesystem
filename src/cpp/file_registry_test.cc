#include "file_meta.h"
#include "file_registry.h"
#include <gtest/gtest.h>
#include <iostream>

TEST(FileRegistryTest, ParseLine) {
  FileRegistry r{};
  std::string path{"/home/ubuntu,S_IFREG"};

  r.parseLine(path);

  ASSERT_TRUE(r.getRoot()->isSubFile("home"));
  ASSERT_EQ(r.getRoot()->listSubFiles().size(), 1);
  ASSERT_TRUE(r.getRoot()->getSubFile("home")->isSubFile("ubuntu"));
  ASSERT_EQ(
      r.getRoot()->getSubFile("home")->getSubFile("ubuntu")->getFileType(),
      FileType::IFREG);
}

TEST(FileRegistryTest, ParseLines) {
  FileRegistry r{};
  std::string path1{"/home/ubuntu,S_IFREG"};
  r.parseLine(path1);
  std::string path2{"/home/user1,S_IFDIR"};
  r.parseLine(path2);

  auto subs{r.getRoot()->listSubFiles()};

  ASSERT_TRUE(r.getRoot()->isSubFile("home"));
  ASSERT_EQ(r.getRoot()->listSubFiles().size(), 1);
  ASSERT_EQ(
      r.getRoot()->getSubFile("home")->getSubFile("ubuntu")->getFileType(),
      FileType::IFREG);
  ASSERT_EQ(r.getRoot()->getSubFile("home")->getSubFile("user1")->getFileType(),
            FileType::IFDIR);
  ASSERT_EQ(r.getRoot()->getSubFile("home")->listSubFiles().size(), 2);
}

TEST(FileRegistryTest, GetFileType) {
  FileRegistry r{};
  std::string path1{"/home/ubuntu,S_IFREG"};
  r.parseLine(path1);
  std::string path2{"/home/user1,S_IFDIR"};
  r.parseLine(path2);

  auto subs{r.getRoot()->listSubFiles()};

  ASSERT_EQ(r.getFileType("/home/ubuntu"), FileType::IFREG);
  ASSERT_EQ(r.getFileType("/home/user1"), FileType::IFDIR);
}

TEST(FileRegistryTest, Exists) {
  FileRegistry r{};
  std::string path1{"/home/ubuntu,S_IFREG"};
  r.parseLine(path1);
  std::string path2{"/home/user1,S_IFDIR"};
  r.parseLine(path2);

  auto subs{r.getRoot()->listSubFiles()};

  ASSERT_TRUE(r.exists("/home/ubuntu"));
  ASSERT_TRUE(r.exists("/home/user1"));
  ASSERT_FALSE(r.exists("/home/user22"));
}