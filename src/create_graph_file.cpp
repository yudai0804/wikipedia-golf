#include <chrono>
#include <deque>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <utility>
#include <vector>

#include "wikipedia.hpp"
// for HOST, USER, PASSWORD
#include "private_config.hpp"

Wikipedia wiki(HOST, USER, PASSWORD);
std::string DIRECTORY = "graph_bin/";
std::string FILE_TYPE = ".bin";
int PROGRESS_INTERVAL = 100;

int main(int argc, char **argv) {
  wiki.init();
  auto id = wiki.get_all_page_id();
  for (size_t i = 0; i < id.size(); i++) {
    // 進捗を表示
    if (i % PROGRESS_INTERVAL == 0) {
      std::cout << "PROGRESS: " << i << "/" << id.size() << std::endl;
    }

    int target = id[i];
    std::ofstream file(DIRECTORY + std::to_string(target) + FILE_TYPE,
                       std::ios::binary);
    if (!file) {
      std::cerr << "file open failed" << std::endl;
      return 1;
    }
    std::vector<int> res;
    wiki.search(target, res);
    for (size_t j = 0; j < res.size(); j++) {
      file.write(reinterpret_cast<char *>(&res[j]), sizeof(int));
    }
    file.close();
  }
  return 0;
}