#ifndef FILE_META_H
#define FILE_META_H

#include <map>
#include <memory>
#include <string>
#include <vector>
// in line with linux file type definition:
// https://man7.org/linux/man-pages/man7/inode.7.html
enum class FileType {
  IFSOCK, //   socket
  IFLNK,  // symbolic link
  IFREG,  // regular file
  IFBLK,  // block device
  IFDIR,  //   directory
  IFCHR,  //   character device
  IFIFO,  //  FIFO
};

class FileNode : public std::enable_shared_from_this<FileNode> {
  std::string m_name; // name of this file
  FileType m_type;
  std::weak_ptr<const FileNode> m_parent;
  std::map<const std::string, std::shared_ptr<FileNode>>
      m_subs; // files in this dir if it's a directory

public:
  FileNode(const std::string &name, const FileType &type,
           const std::shared_ptr<FileNode> &parent);
  FileNode(const std::string &name, const FileType &type);
  ~FileNode();
  bool isSubFile(const std::string &name) const;
  void addSubFile(const std::shared_ptr<FileNode> &node);
  std::shared_ptr<FileNode> getSubFile(const std::string &name) const;
  std::string getName() const { return m_name; }
  std::vector<std::shared_ptr<const FileNode>> listSubFiles() const;
  void setFileType(const FileType &file_type) { m_type = file_type; }
  FileType &getFileType() { return m_type; }
};

#endif