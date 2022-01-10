#ifndef __TRANSLATE_H_
#define __TRANSLATE_H_

#include <algorithm>

#include <linux/limits.h>
#include <stdexcept>
#include <stdlib.h>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <utils/utils.h>

class File2Number {
  std::unordered_map<std::string, int> file2num;
  std::unordered_map<int, std::string> num2file;

  int last_n = 0;

  int add_priv(std::string f) {
    while (num2file.find(last_n) != num2file.end()) {
      last_n++;
    }

    auto n = last_n++;
    file2num[f] = n;
    num2file[n] = f;

    return n;
  }

  auto file_dir_name(std::string &p) {
    auto slash_pos = p.find_last_of('/');
    if (slash_pos == p.npos) {
      return std::make_pair((std::string) ".", p);
    } else {
      return std::make_pair(p.substr(0, slash_pos), p.substr(slash_pos));
    }
  }

  template <bool must_exist = false> std::string simplify_path(std::string &p) {
    if (p.length() && p[0] == '!') {
      return p;
    }
    char simplified[PATH_MAX];
    auto res = realpath(p.c_str(), simplified);
    if (res) {
      return simplified;
    }

    if (!must_exist) {
      auto file_dir = file_dir_name(p);
      return simplify_path<true>(file_dir.first) + file_dir.second;
    }

    ASSERT(false, "Simplifying path failed.", p);
  }

public:
  File2Number() {}

  void add(std::string v, int n) {
    v = simplify_path(v);
    auto fn = num2file.find(n);
    ASSERT(fn == num2file.end(), "Number already mapped", n, "->", fn->second);
    auto fv = file2num.find(v);
    ASSERT(fv == file2num.end(), "Path already mapped", v, "->", fv->second);
    file2num[v] = n;
    num2file[n] = v;
  }

  int get(std::string v) {
    v = simplify_path(v);
    auto f = file2num.find(v);
    if (f != file2num.end()) {
      return f->second;
    } else {
      return add_priv(v);
    }
  }

  auto get(std::vector<std::string> &v) {
    std::vector<int> r(v.size());
    std::transform(v.begin(), v.end(), r.begin(),
                   [=](std::string &vv) { return get(vv); });
    return r;
  }

  std::string get(int n) { return num2file[n]; }

  void clear() {
    file2num.clear();
    num2file.clear();
  }
};

#endif /* __TRANSLATE_H_ */
