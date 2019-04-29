#ifndef SRC_NODE_CGROUPS_H_
#define SRC_NODE_CGROUPS_H_

#include <string>
#include <fstream>

namespace node {

class Cgroups {
 public:
  void Initialize(std::string cgroups_directory);

  inline bool IsEnabled() { return enabled_; }

  inline uint64_t GetMemoryLimitInBytes(uint64_t default_value) {
    std::string prop = GetProperty("memory", "limit_in_bytes");
    if (prop.size() > 0) {
      return std::stoull(prop);
    } else {
      return default_value;
    }
  }
 private:
  std::string GetProperty(std::string controller, std::string field);

  std::string cgroups_directory_;
  bool enabled_ = false;
};

namespace per_process {
  Cgroups cgroups;
}

void Cgroups::Initialize(std::string cgroups_directory) {
  cgroups_directory_ = cgroups_directory;
  enabled_ = true;
}

std::string Cgroups::GetProperty(std::string controller, std::string field) {
  std::string output;
  std::ifstream file;
  file.open(cgroups_directory_ + "/cgroups/" + controller + "/" + controller + "." + field);
  std::getline(file, output);
  file.close();
  return output;
}

}

#endif