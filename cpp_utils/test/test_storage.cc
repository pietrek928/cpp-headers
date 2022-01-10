#include <storage/storage.h>
#include <utils/test.h>

void test() {
  Storage<int> m("a.bin", 1024 * 1024);
  INFO(test) << "yooooooooooo xD " << m[102400] << LOG_ENDL;
  m[102400] = 5;
  INFO(test) << "yooooooooooo xD " << m[102400] << LOG_ENDL;
}

int main() {
  test();
  return 0;
}
