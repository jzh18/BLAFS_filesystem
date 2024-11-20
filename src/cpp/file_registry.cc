#include "file_registry.h"
#include "file_meta.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
constexpr char IFSOCK[] = "S_IFSOCK";
constexpr char IFLNK[] = "S_IFLNK";
constexpr char IFREG[] = "S_IFREG";
constexpr char IFBLK[] = "S_IFBLK";
constexpr char IFDIR[] = "S_IFDIR";
constexpr char IFCHR[] = "S_IFCHR";
constexpr char IFIFO[] = "S_IFIFO";

FileRegistry::FileRegistry() {
  m_root = std::make_shared<FileNode>("/", FileType::IFDIR, nullptr);
}

void FileRegistry::parseRegistry(const std::string &file_path) {

  m_file_path = file_path;
  std::ifstream file(file_path);
  if (file.is_open()) {
    std::string line;
    while (std::getline(file, line)) {
      if (line != "") {
        std::cout << "line: " << line << std::endl;
        parseLine(line);
      }
    }
    file.close();
  } else {
    std::string err{"file not found: " + file_path};
    throw err;
  }
}

void FileRegistry::parseLine(const std::string &line) {
  std::istringstream f(line);
  std::string path, type;
  short count = 0;
  std::getline(f, path, ',');
  std::getline(f, type, ',');
  if (type == "") {
    throw "empty type field.";
  }
  f.clear();
  std::string path_name;
  f.str(path);
  std::shared_ptr<FileNode> cur_filenode = m_root;
  std::string cur_path = "/";
  while (std::getline(f, cur_path, '/')) {
    if (cur_path == "") {
      continue;
    }
    if (!cur_filenode->isSubFile(cur_path)) {
      cur_filenode->addSubFile(
          std::make_shared<FileNode>(cur_path, FileType::IFDIR, cur_filenode));
    }
    cur_filenode = cur_filenode->getSubFile(cur_path);
  }

  if (type == IFDIR) {
    cur_filenode->setFileType(FileType::IFDIR);
  } else if (type == IFREG) {
    cur_filenode->setFileType(FileType::IFREG);
  } else {
    std::string err = "Unsupported FileType: " + type;
    throw err;
  }
}

FileType FileRegistry::getFileType(const std::string &file) const {
  std::istringstream f{file};
  std::string cur_path{"/"};
  std::shared_ptr<FileNode> cur_filenode{m_root};
  while (std::getline(f, cur_path, '/')) {
    if (cur_path == "") {
      continue;
    }
    if (!cur_filenode->isSubFile(cur_path)) {
      std::string err{"file not found: " + file};
      throw err;
    }
    cur_filenode = cur_filenode->getSubFile(cur_path);
  }
  return cur_filenode->getFileType();
}

// file is an absolute path
bool FileRegistry::exists(const std::string &file) const {
  std::istringstream f{file};
  std::string cur_path{"/"};
  std::shared_ptr<FileNode> cur_filenode{m_root};
  while (std::getline(f, cur_path, '/')) {
    if (cur_path == "") {
      continue;
    }
    if (!cur_filenode->isSubFile(cur_path)) {
      return false;
    }
    cur_filenode = cur_filenode->getSubFile(cur_path);
  }
  return true;
}
