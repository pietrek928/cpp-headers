#ifndef __CHANGE_MONITOR_H_
#define __CHANGE_MONITOR_H_

#include <map>
#include <string>

#include <fcntl.h> // library for fcntl function
#include <sys/inotify.h>
#include <unistd.h>

#include <mem/lock.h>
#include <utils/utils.h>

class ChangeMonitor : LockObject {
  constexpr static auto EVENT_SIZE =
      (sizeof(struct inotify_event)); /*size of one event*/
  constexpr static auto BUF_LEN = (8 * (EVENT_SIZE + PATH_MAX));

  int fd = -1;
  std::map<int, std::string> watch2path;
  std::map<std::string, int> path2watch;
  std::map<std::string, time_t> last_changed;

public:
  ChangeMonitor() {
    // ASSERT_SYS(fd = inotify_init());
    // fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
    ASSERT_SYS(fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC));
  }

  ~ChangeMonitor() {
    if (fd != -1) {
      close(fd);
    }
  }

  void add(std::string path) {
    auto l = lock();

    auto wd =
        inotify_add_watch(fd, path.c_str(), IN_MODIFY | IN_CREATE | IN_DELETE);
    ASSERT_SYS(wd, path);
    path2watch[path] = wd;
    watch2path[wd] = path;
  }

  void remove(std::string path) {
    auto l = lock();

    auto wd = path2watch[path];
    ASSERT(inotify_rm_watch(fd, wd) != -1, "Removing watch failed", path);
    path2watch.erase(path);
    watch2path.erase(wd);
  }

  time_t get_time() {
    struct timespec tm;
    clock_gettime(CLOCK_MONOTONIC, &tm);
    return ((time_t)tm.tv_sec) * 1000000000 + tm.tv_nsec;
  }

  void fd_wait(int fd, float timeout_s) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(fd, &set);

    struct timeval timeout;
    timeout.tv_sec = timeout_s;
    timeout.tv_usec = 1e6f * (timeout_s - timeout.tv_sec);

    ASSERT_SYS(select(fd + 1, &set, NULL, NULL, &timeout), "Select failed");
  }

  template <class Thandler> void listen(Thandler change_handler) {
    char buffer[BUF_LEN];
    int buffer_len = 0;

    auto cur_time = get_time();
    constexpr time_t debounce_t = 0.4f * 1e9f;

    while (true) {
      /* wait for change timestamp */
      float wait_t = 256.0f;
      for (auto c = last_changed.begin(); c != last_changed.end();) {
        auto diff_t = c->second - cur_time;
        if (diff_t <= 0) {
          change_handler(c->first);
          c = last_changed.erase(c);
        } else {
          wait_t = std::min(wait_t, 1e-9f * (float)diff_t);
          c++;
        }
      }

      fd_wait(fd, std::max(0.0f, wait_t));

      auto length =
          read(fd, buffer + buffer_len, BUF_LEN - buffer_len) + buffer_len;
      cur_time = get_time();

      if (length <= 0) {
        continue;
      }
      int processed = 0;

      while (processed < length) {
        auto l = lock();

        decltype(EVENT_SIZE) data_size = length - processed;
        if (data_size < EVENT_SIZE) {
          break;
        }
        struct inotify_event *event =
            (struct inotify_event *)&buffer[processed];
        auto event_with_name_size = EVENT_SIZE + event->len;
        if (data_size < event_with_name_size) {
          break;
        }
        processed += event_with_name_size;

        if (event->len) {
          std::string full_name = watch2path[event->wd] + "/" + event->name;
          if (event->mask & IN_ISDIR) {
            full_name += "/";
          }

          if (event->mask & IN_CREATE) {
            last_changed[full_name] = cur_time + debounce_t;
          } else if (event->mask & IN_DELETE) {
            // TODO: do something ?
          } else if (event->mask & IN_MODIFY) {
            last_changed[full_name] = cur_time + debounce_t;
          }
        }
      }

      if (processed < length) {
        buffer_len = length - processed;
        memcpy(buffer, buffer + processed, buffer_len);
      }
    }
  }
};

#endif /* __CHANGE_MONITOR_H_ */
