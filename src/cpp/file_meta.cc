#include "file_meta.h"
#include <spdlog/spdlog.h>

FileNode::FileNode(const std::string &name, const FileType &type,
                   const std::shared_ptr<FileNode> &parent)
    : m_name{name}, m_type{type}, m_parent{parent} {
  // spdlog::info("FileNode Construct");
}

FileNode::FileNode(const std::string &name, const FileType &type)
    : m_name{name}, m_type{type} {
  // spdlog::info("FileNode Construct");
}

FileNode::~FileNode() {
  // spdlog::info("FileNode destruct");
}

void FileNode::addSubFile(const std::shared_ptr<FileNode> &node) {
  m_subs[node->m_name] = node;
  node->m_parent = shared_from_this();
}

bool FileNode::isSubFile(const std::string &name) const {
  if (m_subs.find(name) == m_subs.end()) {
    return false;
  } else {
    return true;
  }
}

std::shared_ptr<FileNode> FileNode::getSubFile(const std::string &name) const {
  return m_subs.at(name);
}

std::vector<std::shared_ptr<const FileNode>> FileNode::listSubFiles() const {
  std::vector<std::shared_ptr<const FileNode>> subs;
  for (std::map<std::string, std::shared_ptr<FileNode>>::const_iterator it{
           m_subs.begin()};
       it != m_subs.end(); it++) {
    subs.push_back(std::shared_ptr<const FileNode>{it->second});
  }
  return subs;
}