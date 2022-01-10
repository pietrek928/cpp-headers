#include <stdexcept>
#include <vector>

#include <utils/utils.h>

template <class To, class... Tobjs> class ObjectContainer {
  std::vector<To> cont;
  ObjectContainer<Tobjs...> rest;

public:
  static constexpr bool has_next = true;

  template <typename F> void call_for(unsigned int pos, F &&func) {
    auto s = cont.size();
    if (pos < s) {
      func(cont[pos]);
      return;
    }
    rest.call_for(pos - s, func);
  }

  template <typename F> void for_each(F &&func) {
    for (auto &v : cont) {
      func(v);
    }
    rest.for_each(func);
  }

  template <class Ti> auto insert(Ti &&o, int start_pos = 0) {
    auto s = cont.size();
    if (std::is_same<To, Ti>::value) {
      cont.emplace_back(o);
      return start_pos + s;
    }
    rest.insert(o, start_pos + s);
  }

  template <class Ti> auto insert(const Ti &o, int start_pos = 0) {
    std::cout << "aaaaaaaa" << std::endl;
    auto s = cont.size();
    if (std::is_same<To, Ti>::value) {
      cont.emplace_back(o);
      return start_pos + s;
    }
    rest.insert(o, start_pos + s);
  }

  void clear() {
    cont.clear();
    rest.clear();
  }
};

template <class To> class ObjectContainer<To> {
  std::vector<To> cont;

public:
  static constexpr bool has_next = false;

  template <typename F> void call_for(unsigned int pos, F &&func) {
    auto s = cont.size();
    if (pos < s) {
      func(cont[pos]);
      return;
    }
    THROW(std::out_of_range, "Queried index out of range");
  }

  template <typename F> void for_each(F &&func) {
    for (auto &v : cont) {
      func(v);
    }
  }

  template <class Ti> auto insert(const Ti &o, int start_pos = 0) {
    auto s = cont.size();
    if (same_type<To, Ti>()) {
      cont.emplace_back(o);
      return start_pos + s;
    }
    auto name = typeid(Ti).name();
    THROW(std::invalid_argument, "Passed unsupported class const&", name);
  }

  template <class Ti> auto insert(Ti &&o, int start_pos = 0) {
    auto s = cont.size();
    if (same_type<To, Ti>()) {
      cont.emplace_back(o);
      return start_pos + s;
    }
    auto name = typeid(Ti).name();
    THROW(std::invalid_argument, "Passed unsupported class &&", name);
  }

  void clear() { cont.clear(); }
};
