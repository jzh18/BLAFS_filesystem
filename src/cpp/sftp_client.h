#ifndef SFTP_CLIENT_H
#define SFTP_CLIENT_H

#include <string>

class SftpClient {
  std::string m_server_address;
  std::string m_user_name;
  std::string m_pwd;
  unsigned short m_port;

public:
  SftpClient(std::string server_address, unsigned short port,
             std::string user_name, std::string pwd);

  int retrieveFile(std::string path, std::string dest);
};

#endif