#ifndef __COMPILE_H_
#define __COMPILE_H_

#include <regex>
#include <string>
#include <vector>

#include <utils/utils.h>

constexpr auto CHANNEL_LOG_LEVEL_compile = LogLevel::DEBUG;

constexpr auto noexisting_sequence = "___a___XxX___a___";

const auto file_regex = std::regex("");
const auto escaped_endl_regex = std::regex("\\\\\n");
const auto escaped_space_regex = std::regex("\\\\ ");
const auto space_regex = std::regex(" ");
const auto double_space_regex = std::regex("  ");
const auto noexisting_sequence_regex = std::regex(noexisting_sequence);

auto parse_compile_deps(std::string result) {
  result = std::regex_replace(result, escaped_endl_regex, "");
  result = std::regex_replace(result, escaped_space_regex, noexisting_sequence);
  result = std::regex_replace(result, double_space_regex, " ");
  result = std::regex_replace(result, space_regex, "\n");
  result = std::regex_replace(result, noexisting_sequence_regex, " ");

  std::vector<std::string> deps;
  decltype(result.length()) last_pos = 0, pos = 0;
  while (pos < result.length()) {
    auto c = result[pos];
    if (c == '\n') {
      auto s = result.substr(last_pos, pos - last_pos);
      if (s.length() && s.back() != ':') {
        deps.push_back(s);
      }
      last_pos = pos + 1;
    }
    pos++;
  }

  return deps;
}

class compile_error : public std::exception {
  const char *what() const throw() { return "Compilation error"; }
};

auto run_compilation(std::vector<std::string> &cmd) {

  std::string cmd_s;
  for (auto &w : cmd) {
    cmd_s += " \"";
    cmd_s += w;
    cmd_s += "\"";
  }
  cmd_s += " 2>&1";
  DBG(compile) << "Running command: " << cmd_s << DBG_ENDL;

  FILE *pipe = popen(cmd_s.c_str(), "r");
  ASSERT(pipe, "Creating process failed");

  std::string stdout_result;
  char buffer[128];
  while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
    // std::cout << "Reading..." << std::endl;
    stdout_result += buffer;
  }

  auto return_code = pclose(pipe);
  if (return_code) {
    ERR(compile) << "Compilation returned " << return_code << DBG_ENDL
                 << stdout_result << DBG_ENDL;
    throw compile_error();
  }

  return stdout_result;
}

#endif /* __COMPILE_H_ */
