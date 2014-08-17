#ifndef SCRAM_TESTS_TOOLS_H_
#define SCRAM_TESTS_TOOLS_H_

#include "boost/filesystem.hpp"

class FileDeleter {
 public:
  FileDeleter(std::string path) {
    path_ = path;
    if (boost::filesystem::exists(path_))
      remove(path_.c_str());
  }

  ~FileDeleter() {
    if (boost::filesystem::exists(path_))
      remove(path_.c_str());
  }

 private:
  std::string path_;
};

#endif  // SCRAM_TESTS_TOOLS_H_
