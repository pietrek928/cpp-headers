#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "utils/utils.h"

constexpr auto CHANNEL_LOG_LEVEL_aaaa = LogLevel::DEBUG;

class FileStorage {
  int fd = -1;
  std::size_t size = 0;
  void *addr = NULL;

  static constexpr int block_size = 4096;

  void clear() {
    if (size && addr) {
      mem_sync();
      munmap(addr, size);
      size = 0;
      addr = NULL;
    }
    if (fd != -1) {
      data_sync();
      close(fd);
      fd = -1;
    }
  }

public:
  FileStorage(const char *fname, std::size_t size) : size(size) {
    try {
      fd = open(fname, O_RDWR | O_CREAT, 0666);
      ASSERT_SYS(fd, "open failed");
      ASSERT_SYS(fallocate64(fd, 0, 0, size), "fallocate failed");
      addr = mmap64(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
      ASSERT_SYS(addr != MAP_FAILED, "mmap of", size, "bytes failed");
    } catch (...) {
      clear();
      throw;
    }
  }

  ~FileStorage() { clear(); }

  inline auto get_data() { return addr; }

  auto get_size() { return size; }

  void mem_sync() { ASSERT_SYS(msync(addr, size, MS_SYNC), "msync failed"); }

  void data_sync() { ASSERT_SYS(fdatasync(fd), "fdatasync failed"); }

  // preload region from disk, rely on cpu predition to do it simulateusly
  void preload(void *start, std::size_t preload_size) {
    for (std::size_t i = 0; i < preload_size; i += block_size) {
      volatile char a = *((char *)(start) + i);
    }
  }
};

template <class To> class Storage : FileStorage {
public:
  Storage(const char *fname, std::size_t len)
      : FileStorage(fname, len * sizeof(To)) {}
  inline To *get_data() { return (To *)FileStorage::get_data(); }
  To &operator[](std::size_t pos) { return get_data()[pos]; }
};
