#define FUSE_USE_VERSION 36

#include "file_registry.h"
#include <boost/filesystem.hpp>
#include <dirent.h>
#include <errno.h>
#include <fuse.h>
#include <fuse_lowlevel.h>
#include <fuse_opt.h>
#include <iostream>
#include <spdlog/spdlog.h>
#include <stdio.h>
#include <sys/stat.h>

#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef __FreeBSD__
#endif
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#endif
#include <cstdlib>
#include <sys/xattr.h>

#include <sys/ioctl.h>

// #include "passthrough_helpers.h"

static char *mount_point;
static const char *real_dir = "/tmp/real";
static const char *lower_dir;

static struct options {
  const char *real_dir;
  const char *file_registry;
  const char *lower_dir;
} options;

#define OPTION(t, p)                                                           \
  { t, offsetof(struct options, p), 1 }

static const struct fuse_opt option_spec[] = {
    OPTION("--realdir=%s", real_dir), OPTION("--lowerdir=%s", lower_dir),
    FUSE_OPT_END};

static const std::string redirect(const char *original_path) {
  spdlog::debug("original path: {0}", original_path);
  boost::filesystem::path lower{lower_dir};
  boost::filesystem::path file_path{original_path};
  boost::filesystem::path full_lower_path = lower / file_path;

  boost::filesystem::path real{real_dir};
  boost::filesystem::path real_file_path = real / file_path;

  boost::filesystem::path path;
  boost::system::error_code ec;

  if (std::string(original_path) == "/") {
    return full_lower_path.string();
  }

  if (boost::filesystem::exists(real_file_path)) {
    return real_file_path.string();
  }

  // return full_lower_path.string();
  // at root directory, this real_file_path always exsits, ending up with zero
  // file list
  // rename will not work becasue real dir is located in the original
  // filesystem; full_lower_path is loated in the overlay filesystem. we can't
  // rename a file to a different filesystem.
  //
  if (boost::filesystem::exists(full_lower_path)) {
    spdlog::debug("full lower path exist: {0}", full_lower_path.string());
    if (boost::filesystem::is_directory(full_lower_path)) {
      spdlog::debug("is dir");
      spdlog::debug("lower file is dir, create a new one: {0}->{1}",
                    full_lower_path.string(), real_file_path.string());
      boost::filesystem::create_directory(real_file_path, ec);
      if (ec.value() != 0) {
        spdlog::error("create dir fail: {}", ec.message());
      }
      path = real_file_path;
    } else {
      spdlog::debug("is file");
      if (!boost::filesystem::exists(real_file_path)) {
        spdlog::debug(
            "fail not exist. lower file is regular file, copy it: {0}->{1}",
            full_lower_path.string(), real_file_path.string());
        // boost::filesystem::copy_file(
        //     full_lower_path, real_file_path,
        //     boost::filesystem::copy_options::skip_existing |
        //         boost::filesystem::copy_options::copy_symlinks |
        //         boost::filesystem::copy_options::recursive,
        //     ec);
        // -r option in here is used to copy symbolinks and reserve the
        // information of symblinks
        // and I don't why the boost copy_file function not work here
        std::string cmd{"cp -r " + full_lower_path.string() + " " +
                        real_file_path.string()};
        system(cmd.c_str());
      }
      path = real_file_path;
    }
  } else {
    path = full_lower_path;
  }
  spdlog::debug("final path: {0}", path.string());
  return path.string();
}

static int mknod_wrapper(int dirfd, const char *_path, const char *link,
                         int mode, dev_t rdev) {

  spdlog::debug("mknode wrapper");
  const char *path{redirect(_path).c_str()};

  int res;

  if (S_ISREG(mode)) {
    res = openat(dirfd, path, O_CREAT | O_EXCL | O_WRONLY, mode);
    if (res >= 0)
      res = close(res);
  } else if (S_ISDIR(mode)) {
    res = mkdirat(dirfd, path, mode);
  } else if (S_ISLNK(mode) && link != NULL) {
    res = symlinkat(link, dirfd, path);
  } else if (S_ISFIFO(mode)) {
    res = mkfifoat(dirfd, path, mode);
  } else {
    res = mknodat(dirfd, path, mode, rdev);
  }

  return res;
}

static void *xmp_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
  (void)conn;
  cfg->use_ino = 1;

  /* Pick up changes from lower filesystem right away. This is
     also necessary for better hardlink support. When the kernel
     calls the unlink() handler, it does not know the inode of
     the to-be-removed entry and can therefore not invalidate
     the cache of the associated inode - resulting in an
     incorrect st_nlink value being reported for any remaining
     hardlinks to this inode. */
  cfg->entry_timeout = 0;
  cfg->attr_timeout = 0;
  cfg->negative_timeout = 0;

  return NULL;
}

static int xmp_getxattr(const char *_path, const char *name, char *value,
                        size_t size) {
  spdlog::debug("getxattr callback");
  std::string redirect_path{redirect(_path)};
  auto path = redirect_path.c_str();
  int res = lgetxattr(path, name, value, size);
  if (res == -1)
    return -errno;
  return res;
}

static int xmp_listxattr(const char *_path, char *list, size_t size) {
  spdlog::debug("listxattr callback");
  std::string redirect_path{redirect(_path)};
  auto path = redirect_path.c_str();
  int res = llistxattr(path, list, size);
  if (res == -1)
    return -errno;
  return res;
}

static int xmp_getattr(const char *_path, struct stat *stbuf,
                       struct fuse_file_info *fi) {
  // @jzh18: must receive the return value of rediect.
  // `auto path=redirect(_path).c_str()` will end up with an empty char*.
  spdlog::debug("getattr callback");
  std::string redirect_path{redirect(_path)};
  auto path = redirect_path.c_str();
  int res;

  (void)path;

  if (fi) {
    spdlog::debug("retrieve from fi");
    res = fstat(fi->fh, stbuf);
  } else {
    spdlog::debug("retrieve from path: {0}", path);
    res = lstat(path, stbuf);
  }

  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_access(const char *_path, int mask) {
  spdlog::debug("access callback");
  std::string redirect_path{redirect(_path)};
  auto path = redirect_path.c_str();
  int res;

  res = access(path, mask);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_readlink(const char *_path, char *buf, size_t size) {
  spdlog::debug("readlink callback");
  std::string redirect_path{redirect(_path)};
  auto path = redirect_path.c_str();
  int res;

  res = readlink(path, buf, size - 1);
  if (res == -1)
    return -errno;

  buf[res] = '\0';
  return 0;
}

struct xmp_dirp {
  DIR *dp;
  struct dirent *entry;
  off_t offset;
};

static int xmp_opendir(const char *_path, struct fuse_file_info *fi) {
  spdlog::debug("opendir callback");
  std::string redirect_path{redirect(_path)};

  auto path = redirect_path.c_str();
  int res;
  struct xmp_dirp *d = static_cast<xmp_dirp *>(malloc(sizeof(struct xmp_dirp)));
  if (d == NULL)
    return -ENOMEM;

  d->dp = opendir(path);
  if (d->dp == NULL) {
    res = -errno;
    free(d);
    return res;
  }
  d->offset = 0;
  d->entry = NULL;

  fi->fh = (unsigned long)d;
  return 0;
}

static inline struct xmp_dirp *get_dirp(struct fuse_file_info *fi) {
  return (struct xmp_dirp *)(uintptr_t)fi->fh;
}

static int xmp_readdir(const char *_path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi,
                       enum fuse_readdir_flags flags) {
  spdlog::debug("readdir callback");
  std::string redirect_path{redirect(_path)};

  auto path = redirect_path.c_str();
  struct xmp_dirp *d = get_dirp(fi);

  (void)path;
  if (offset != d->offset) {
    seekdir(d->dp, offset);

    d->entry = NULL;
    d->offset = offset;
  }
  while (1) {
    struct stat st;
    off_t nextoff;
    enum fuse_fill_dir_flags fill_flags = static_cast<fuse_fill_dir_flags>(0);

    if (!d->entry) {
      d->entry = readdir(d->dp);
      if (!d->entry)
        break;
    }

    if (!(fill_flags & FUSE_FILL_DIR_PLUS)) {
      memset(&st, 0, sizeof(st));
      st.st_ino = d->entry->d_ino;
      st.st_mode = d->entry->d_type << 12;
    }
    nextoff = telldir(d->dp);

    if (filler(buf, d->entry->d_name, &st, nextoff, fill_flags))
      break;

    d->entry = NULL;
    d->offset = nextoff;
  }

  return 0;
}

static int xmp_releasedir(const char *_path, struct fuse_file_info *fi) {

  spdlog::debug("releasedir callback");
  std::string redirect_path{redirect(_path)};
  auto path = redirect_path.c_str();

  struct xmp_dirp *d = get_dirp(fi);
  (void)path;
  closedir(d->dp);
  free(d);
  return 0;
}

static int xmp_mknod(const char *_path, mode_t mode, dev_t rdev) {
  spdlog::debug("mknod callback");
  std::string redirect_path{redirect(_path)};
  auto path = redirect_path.c_str();
  int res;

  if (S_ISFIFO(mode))
    res = mkfifo(path, mode);
  else
    res = mknod(path, mode, rdev);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_mkdir(const char *_path, mode_t mode) {
  spdlog::debug("mkdir callback");
  std::string redirect_path{redirect(_path)};
  auto path = redirect_path.c_str();
  int res;

  res = mkdir(path, mode);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_unlink(const char *_path) {
  spdlog::debug("unlink callback");
  std::string redirect_path{redirect(_path)};
  auto path = redirect_path.c_str();
  int res;

  res = unlink(path);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_rmdir(const char *_path) {
  spdlog::debug("rmdir callback");
  std::string redirect_path{redirect(_path)};
  auto path = redirect_path.c_str();
  int res;

  res = rmdir(path);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_symlink(const char *_from, const char *_to) {
  spdlog::debug("symlink callback");
  std::string redirect_from{redirect(_from)};
  const char *from{redirect_from.c_str()};
  std::string redirect_to{redirect(_to)};
  const char *to{redirect_to.c_str()};
  int res;

  res = symlink(from, to);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_rename(const char *_from, const char *_to, unsigned int flags) {
  spdlog::debug("rename callback");
  std::string redirect_from{redirect(_from)};
  const char *from{redirect_from.c_str()};
  std::string redirect_to{redirect(_to)};
  const char *to{redirect_to.c_str()};
  int res;

  if (flags)
    return -EINVAL;

  res = rename(from, to);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_link(const char *_from, const char *_to) {
  spdlog::debug("link callback");
  std::string redirect_from{redirect(_from)};
  const char *from{redirect_from.c_str()};
  std::string redirect_to{redirect(_to)};
  const char *to{redirect_to.c_str()};
  int res;

  res = link(from, to);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_chmod(const char *_path, mode_t mode,
                     struct fuse_file_info *fi) {
  spdlog::debug("chmod callback");
  std::string redirect_path{redirect(_path)};
  auto path = redirect_path.c_str();
  int res;

  if (fi)
    res = fchmod(fi->fh, mode);
  else
    res = chmod(path, mode);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_chown(const char *_path, uid_t uid, gid_t gid,
                     struct fuse_file_info *fi) {
  spdlog::debug("chown callback");
  std::string redirect_path{redirect(_path)};
  auto path = redirect_path.c_str();
  int res;

  if (fi)
    res = fchown(fi->fh, uid, gid);
  else
    res = lchown(path, uid, gid);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_truncate(const char *_path, off_t size,
                        struct fuse_file_info *fi) {
  spdlog::debug("truncate callback");
  std::string redirect_path{redirect(_path)};
  auto path = redirect_path.c_str();
  int res;

  if (fi != NULL)
    res = ftruncate(fi->fh, size);
  else
    res = truncate(path, size);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_create(const char *_path, mode_t mode,
                      struct fuse_file_info *fi) {
  spdlog::debug("create callback");
  std::string redirect_path{redirect(_path)};
  auto path = redirect_path.c_str();
  int res;

  res = open(path, fi->flags, mode);
  if (res == -1)
    return -errno;

  fi->fh = res;
  return 0;
}

static int xmp_open(const char *_path, struct fuse_file_info *fi) {
  spdlog::debug("open callback");
  std::string redirect_path{redirect(_path)};
  auto path = redirect_path.c_str();
  int res;

  res = open(path, fi->flags);
  if (res == -1)
    return -errno;

  fi->fh = res;
  return 0;
}

static int xmp_read(const char *_path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
  spdlog::debug("read callback");
  std::string redirect_path{redirect(_path)};
  auto path = redirect_path.c_str();
  int fd;
  int res;

  if (fi == NULL)
    fd = open(path, O_RDONLY);
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

static int xmp_write(const char *_path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi) {
  spdlog::debug("write callback");
  std::string redirect_path{redirect(_path)};
  auto path = redirect_path.c_str();
  int fd;
  int res;

  (void)fi;
  if (fi == NULL)
    fd = open(path, O_WRONLY);
  else
    fd = fi->fh;

  if (fd == -1)
    return -errno;

  res = pwrite(fd, buf, size, offset);
  if (res == -1)
    res = -errno;

  if (fi == NULL)
    close(fd);
  return res;
}

static int xmp_statfs(const char *_path, struct statvfs *stbuf) {
  spdlog::debug("statfs callback");
  std::string redirect_path{redirect(_path)};
  auto path = redirect_path.c_str();
  int res;

  res = statvfs(path, stbuf);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_release(const char *_path, struct fuse_file_info *fi) {
  spdlog::debug("release callback");
  std::string redirect_path{redirect(_path)};
  auto path = redirect_path.c_str();
  (void)path;
  close(fi->fh);
  return 0;
}

static int xmp_fsync(const char *_path, int isdatasync,
                     struct fuse_file_info *fi) {
  spdlog::debug("fysnc callback");
  std::string redirect_path{redirect(_path)};
  auto path = redirect_path.c_str();
  int res;
  (void)path;

#ifndef HAVE_FDATASYNC
  (void)isdatasync;
#else
  if (isdatasync)
    res = fdatasync(fi->fh);
  else
#endif
  res = fsync(fi->fh);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_flush(const char *_path, struct fuse_file_info *fi) {
  spdlog::debug("flush callback");
  std::string redirect_path{redirect(_path)};
  auto path = redirect_path.c_str();
  int res;

  (void)path;
  /* This is called from every close on an open file, so call the
     close on the underlying filesystem.	But since flush may be
     called multiple times for an open file, this must not really
     close the file.  This is important if used on a network
     filesystem like NFS which flush the data/metadata on close() */
  res = close(dup(fi->fh));
  if (res == -1)
    return -errno;

  return 0;
}

static off_t xmp_lseek(const char *_path, off_t off, int whence,
                       struct fuse_file_info *fi) {
  spdlog::debug("seek callback");
  std::string redirect_path{redirect(_path)};
  auto path = redirect_path.c_str();
  int fd;
  off_t res;

  if (fi == NULL)
    fd = open(path, O_RDONLY);
  else
    fd = fi->fh;

  if (fd == -1)
    return -errno;

  res = lseek(fd, off, whence);
  if (res == -1)
    res = -errno;

  if (fi == NULL)
    close(fd);
  return res;
}

static int xmp_ioctl(const char *_path, unsigned int cmd, void *arg,
                     struct fuse_file_info *fi, unsigned int flags,
                     void *data) {
  spdlog::debug("ioctl callback");
  std::string redirect_path{redirect(_path)};
  auto path = redirect_path.c_str();
  int res;
  if (boost::filesystem::is_regular_file(path)) {
    int fd = open(path, O_RDWR);
    res = ioctl(fd, cmd, data);
    if (res == -1) {
      perror("ioctl error: ");
      return -errno;
    }
  }
  return -EINVAL;
}

static const struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readlink = xmp_readlink,
    .mknod = xmp_mknod,
    .mkdir = xmp_mkdir,
    .unlink = xmp_unlink,
    .rmdir = xmp_rmdir,
    .symlink = xmp_symlink,
    .rename = xmp_rename,
    .link = xmp_link,
    .chmod = xmp_chmod,
    .chown = xmp_chown,
    .truncate = xmp_truncate,
    .open = xmp_open,
    .read = xmp_read,
    .write = xmp_write,
    .statfs = xmp_statfs,
    .flush = xmp_flush,
    .release = xmp_release,
    .fsync = xmp_fsync,
    .getxattr = xmp_getxattr,
    .listxattr = xmp_listxattr,
    .opendir = xmp_opendir,
    .readdir = xmp_readdir,
    .releasedir = xmp_releasedir,
    .init = xmp_init,
    .access = xmp_access,
    .create = xmp_create,
    .ioctl = xmp_ioctl,
    .lseek = xmp_lseek,
};

int main(int argc, char **argv) {
  spdlog::set_level(spdlog::level::debug);

  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

  options.real_dir = strdup("/mnt/real");
  options.lower_dir = strdup("/mnt/lower");

  if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
    return 1;

  mount_point = argv[argc - 1];
  real_dir = options.real_dir;
  lower_dir = options.lower_dir;

  spdlog::debug("lowerdir={0}, realdir={1}, mount={2}", lower_dir, real_dir,
                mount_point);

  int ret = fuse_main(args.argc, args.argv, &xmp_oper, nullptr);
  fuse_opt_free_args(&args);
  return ret;
}