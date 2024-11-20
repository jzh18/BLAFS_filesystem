#define FUSE_USE_VERSION 36

#include "file_registry.h"
#include "sftp_client.h"
#include <boost/filesystem.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/cURLpp.hpp>
#include <dirent.h>
#include <errno.h>
#include <fuse.h>
#include <fuse_lowlevel.h>
#include <fuse_opt.h>
#include <iostream>
#include <spdlog/spdlog.h>
#include <stdio.h>
#include <sys/stat.h>

static char *mount_point;
static SftpClient client{"13.48.215.98", 2023, "testuser", "testpassword"};
static FileRegistry file_registry;
static const char *real_dir = "/tmp/real";

static struct options {
  const char *real_dir;
  const char *file_registry;
} options;

#define OPTION(t, p)                                                           \
  { t, offsetof(struct options, p), 1 }

static const struct fuse_opt option_spec[] = {
    OPTION("--realdir=%s", real_dir),
    OPTION("--fileregistry=%s", file_registry), FUSE_OPT_END};

static int getattr_callback(const char *path, struct stat *stbuf,
                            struct fuse_file_info *fi) {

  spdlog::debug("getattr_callback");
  if (!file_registry.exists(path)) {
    spdlog::debug("file expected non-exist");
    return -ENOENT;
  }

  boost::filesystem::path real(real_dir);
  boost::filesystem::path file_path(path);
  boost::filesystem::path full_path = real / file_path;

  bool is_exist = boost::filesystem::exists(full_path);
  if (!is_exist) {
    FileType type = file_registry.getFileType(path);
    if (type == FileType::IFREG) {
      spdlog::debug("Download file");
      client.retrieveFile(path, full_path.c_str());
    } else if (type == FileType::IFDIR) {
      spdlog::debug("Create dir");
      boost::filesystem::create_directories(full_path);
    }
  }
  spdlog::debug("{0} exist: {1}", full_path.string(), is_exist);
  return stat(full_path.c_str(), stbuf);
}

static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler,
                            off_t offset, struct fuse_file_info *fi,
                            enum fuse_readdir_flags flags) {
  spdlog::debug("readdir_callback");
  if (!file_registry.exists(path)) {
    spdlog::debug("file expected non-exist");
    return -ENOENT;
  }
  boost::filesystem::path real(real_dir);
  boost::filesystem::path file_path(path);
  boost::filesystem::path full_path = real / file_path;

  DIR *dp;
  struct dirent *de;

  (void)offset;
  (void)fi;
  (void)flags;

  dp = opendir(full_path.c_str());
  if (dp == NULL)
    return -errno;

  while ((de = readdir(dp)) != NULL) {
    struct stat st;
    memset(&st, 0, sizeof(st));
    st.st_ino = de->d_ino;
    st.st_mode = de->d_type << 12;
    if (filler(buf, de->d_name, &st, 0, FUSE_FILL_DIR_PLUS))
      break;
  }

  closedir(dp);
  return 0;
}

static int open_callback(const char *path, struct fuse_file_info *fi) {
  spdlog::debug("open_callback");
  if (!file_registry.exists(path)) {
    spdlog::debug("file expected non-exist");
    return -ENOENT;
  }
  boost::filesystem::path real(real_dir);
  boost::filesystem::path file_path(path);
  boost::filesystem::path full_path = real / file_path;

  int res;

  res = open(full_path.c_str(), fi->flags);
  if (res == -1)
    return -errno;

  fi->fh = res;
  return 0;
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset,
                         struct fuse_file_info *fi) {
  spdlog::debug("read_callback");
  if (!file_registry.exists(path)) {
    spdlog::debug("file expected non-exist");
    return -ENOENT;
  }
  boost::filesystem::path real(real_dir);
  boost::filesystem::path file_path(path);
  boost::filesystem::path full_path = real / file_path;

  int fd;
  int res;

  if (fi == NULL)
    fd = open(full_path.c_str(), O_RDONLY);
  else
    fd = fi->fh;

  if (fd == -1)
    return -errno;

  res = pread(fd, buf, size, offset);
  if (res == -1)
    res = -errno;

  if (fi == NULL)
    close(fd);
  return res;
}
static struct fuse_operations cached_fs = {.getattr = getattr_callback,
                                           .open = open_callback,
                                           .read = read_callback,
                                           .readdir = readdir_callback};

int main(int argc, char **argv) {
  spdlog::set_level(spdlog::level::debug);

  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

  /* Set defaults -- we have to use strdup so that
          fuse_opt_parse can free the defaults if other
          values are specified */
  options.real_dir = strdup("/mnt/real");

  /* Parse options */
  if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
    return 1;

  mount_point = argv[argc - 1];
  real_dir = options.real_dir;
  file_registry.parseRegistry(options.file_registry);

  int ret = fuse_main(args.argc, args.argv, &cached_fs, nullptr);
  fuse_opt_free_args(&args);
  return ret;
}