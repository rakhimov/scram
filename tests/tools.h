#ifndef SCRAM_TESTS_TOOLS_H_
#define SCRAM_TESTS_TOOLS_H_

#include <string>

#include "boost/filesystem.hpp"

class FileDeleter {
 public:
  explicit FileDeleter(std::string path) {
    path_ = path;
    if (boost::filesystem::exists(path_)) {
      bool done = remove(path_.c_str());
      assert(done);
    }
  }

  ~FileDeleter() {
    if (boost::filesystem::exists(path_)) {
      bool done = remove(path_.c_str());
      assert(done);
    }
  }

 private:
  std::string path_;
};

#endif  // SCRAM_TESTS_TOOLS_H_
