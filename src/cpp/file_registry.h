#ifndef FILE_REGISTRY_H
#define FILE_REGISTRY_H
#include "file_meta.h"
#include <string>
class FileRegistry {
  std::string m_file_path;
  std::shared_ptr<FileNode> m_root;

public:
  FileRegistry();
  void parseRegistry(const std::string &file_path);
  bool exists(const std::string &file) const;
  void parseLine(const std::string &line);
  FileType getFileType(const std::string& path) const;
  std::shared_ptr<const FileNode> getRoot() const {
    return std::shared_ptr<const FileNode>{m_root};
  }
};

#endif