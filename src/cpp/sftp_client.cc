#include "sftp_client.h"
#include <arpa/inet.h>
#include <fstream>
#include <libssh2.h>
#include <libssh2_sftp.h>
#include <netinet/in.h>
#include <spdlog/spdlog.h>
static int waitsocket(int socket_fd, LIBSSH2_SESSION *session) {
  struct timeval timeout;
  int rc;
  fd_set fd;
  fd_set *writefd = NULL;
  fd_set *readfd = NULL;
  int dir;

  timeout.tv_sec = 10;
  timeout.tv_usec = 0;

  FD_ZERO(&fd);

  FD_SET(socket_fd, &fd);

  /* now make sure we wait in the correct direction */
  dir = libssh2_session_block_directions(session);

  if (dir & LIBSSH2_SESSION_BLOCK_INBOUND)
    readfd = &fd;

  if (dir & LIBSSH2_SESSION_BLOCK_OUTBOUND)
    writefd = &fd;

  rc = select(socket_fd + 1, readfd, writefd, NULL, &timeout);

  return rc;
}

static void shutdown(LIBSSH2_SESSION *session) {
  spdlog::debug("libssh2_session_disconnect\n");
  while (libssh2_session_disconnect(session, "Normal Shutdown, Thank you") ==
         LIBSSH2_ERROR_EAGAIN)
    ;
  libssh2_session_free(session);
  spdlog::debug("all done\n");

  libssh2_exit();
}

SftpClient::SftpClient(std::string server_address, unsigned short port,
                       std::string user_name, std::string pwd)
    : m_server_address{server_address}, m_port{port},
      m_user_name{user_name}, m_pwd{pwd} {}

int SftpClient::retrieveFile(std::string path, std::string dest) {

  int rc;
  int total = 0;
  int spin = 0;
  int sock, i, auth_pw = 1;
  struct sockaddr_in sin;
  const char *fingerprint;
  LIBSSH2_SESSION *session;
  LIBSSH2_SFTP *sftp_session;
  LIBSSH2_SFTP_HANDLE *sftp_handle;

  rc = libssh2_init(0);
  if (rc != 0) {
    spdlog::error("libssh2 initialization failed (%d)\n", rc);
    return 1;
  }

  /*
   * The application code is responsible for creating the socket
   * and establishing the connection
   */
  sock = socket(AF_INET, SOCK_STREAM, 0);
  sin.sin_family = AF_INET;
  sin.sin_port = htons(m_port);
  inet_pton(AF_INET, m_server_address.c_str(), &(sin.sin_addr));
  if (connect(sock, (struct sockaddr *)(&sin), sizeof(struct sockaddr_in)) !=
      0) {
    spdlog::error("failed to connect!\n");
    return -1;
  }

  /* Create a session instance */
  session = libssh2_session_init();
  if (!session)
    return -1;

  /* Since we have set non-blocking, tell libssh2 we are non-blocking */
  libssh2_session_set_blocking(session, 0);

  /* ... start it up. This will trade welcome banners, exchange keys,
   * and setup crypto, compression, and MAC layers
   */
  while ((rc = libssh2_session_handshake(session, sock)) ==
         LIBSSH2_ERROR_EAGAIN)
    ;
  if (rc) {
    spdlog::error("Failure establishing SSH session: %d\n", rc);
    return -1;
  }

  /* At this point we havn't yet authenticated.  The first thing to do
   * is check the hostkey's fingerprint against our known hosts Your app
   * may have it hard coded, may go to a file, may present it to the
   * user, that's your call
   */
  const char *username = m_user_name.c_str();
  const char *password = m_pwd.c_str();
  fingerprint = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_SHA1);
  spdlog::debug("Fingerprint: ");
  for (i = 0; i < 20; i++) {
    fprintf(stderr, "%02X ", (unsigned char)fingerprint[i]);
  }
  spdlog::debug("\n");

  if (auth_pw) {
    /* We could authenticate via password */
    while ((rc = libssh2_userauth_password(session, username, password)) ==
           LIBSSH2_ERROR_EAGAIN)
      ;
    if (rc) {
      spdlog::error("Authentication by password failed.\n");
      shutdown(session);
      return 1;
    }
  } else {
    /* Or by public key */
    while ((rc = libssh2_userauth_publickey_fromfile(session, username,
                                                     "/home/username/"
                                                     ".ssh/id_rsa.pub",
                                                     "/home/username/"
                                                     ".ssh/id_rsa",
                                                     password)) ==
           LIBSSH2_ERROR_EAGAIN)
      ;
    if (rc) {
      spdlog::error("\tAuthentication by public key failed\n");
      shutdown(session);
      return 1;
    }
  }
  spdlog::debug("libssh2_sftp_init()!\n");
  do {
    sftp_session = libssh2_sftp_init(session);

    if (!sftp_session) {
      if (libssh2_session_last_errno(session) == LIBSSH2_ERROR_EAGAIN) {
        spdlog::debug("non-blocking init\n");
        waitsocket(sock, session); /* now we wait */
      } else {
        spdlog::error("Unable to init SFTP session\n");
        shutdown(session);
        return 1;
      }
    }
  } while (!sftp_session);
  spdlog::debug("libssh2_sftp_open()!\n");

  /* Request a file via SFTP */
  spdlog::info(path);
  do {
    sftp_handle =
        libssh2_sftp_open(sftp_session, path.c_str(), LIBSSH2_FXF_READ, 0);

    if (!sftp_handle) {
      if (libssh2_session_last_errno(session) != LIBSSH2_ERROR_EAGAIN) {
        spdlog::debug("Unable to open file with SFTP\n");
        shutdown(session);
        return 1;
      } else {
        spdlog::debug("non-blocking open\n");
        waitsocket(sock, session); /* now we wait */
      }
    }
  } while (!sftp_handle);

  spdlog::debug("libssh2_sftp_open() is done, now receive data!\n");
  std::ofstream dest_file(dest, std::ios::binary | std::ios::app);
  if (!(rc = dest_file.is_open())) {
    spdlog::error("Open file error: {}", rc);
    shutdown(session);
    return 1;
  }
  do {
    char mem[1024 * 24];

    /* loop until we fail */
    while ((rc = libssh2_sftp_read(sftp_handle, mem, sizeof(mem))) ==
           LIBSSH2_ERROR_EAGAIN) {
      spin++;
      waitsocket(sock, session); /* now we wait */
    }
    if (rc > 0) {
      total += rc;
      dest_file.write(mem, rc);
    } else {
      break;
    }
  } while (1);

  spdlog::debug("Got %d bytes spin: %d\n", total, spin);
  libssh2_sftp_close(sftp_handle);
  libssh2_sftp_shutdown(sftp_session);

  shutdown(session);
  return 0;
}