#include <chrono>
#include <deque>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <thread>
#include <utility>
#include <vector>

#include "wikipedia.hpp"
// for HOST, USER, PASSWORD
#include "private_config.hpp"

Wikipedia wiki(HOST, USER, PASSWORD);
constexpr std::string DIRECTORY = "graph_bin/";
constexpr std::string FILE_TYPE = ".bin";
int PROGRESS_INTERVAL = 100;
int progress = 0;
bool is_success = true;
std::mutex mtx;

void task(std::shared_ptr<Wikipedia> wiki, int start, int end) {
  auto id = wiki->get_all_page_id();
  for (int i = start; i < end; i++) {
    progress++;
    int target = id[i];
    std::ofstream file(DIRECTORY + std::to_string(target) + FILE_TYPE,
                       std::ios::binary);
    if (!file) {
      std::lock_guard<std::mutex> lock(mtx);
      is_success = false;
      return;
    }
    std::vector<int> res;
    wiki->search(target, res);
    for (size_t j = 0; j < res.size(); j++) {
      file.write(reinterpret_cast<char *>(&res[j]), sizeof(int));
    }
    file.close();
  }
}

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "input error" << std::endl;
    return 0;
  }
  int thread_number = atoi(argv[1]);
  if (thread_number == 0) {
    std::cerr << "input error" << std::endl;
    return 0;
  }
  std::vector<std::shared_ptr<Wikipedia>> wiki(thread_number);
  std::vector<std::thread> th(thread_number);
  for (int i = 0; i < thread_number; i++) {
    wiki[i] = std::make_shared<Wikipedia>(HOST, USER, PASSWORD);
    wiki[i]->init();
  }
  auto id = wiki[0]->get_all_page_id();
  for (int i = 0; i < thread_number; i++) {
    int start = id.size() / thread_number * i;
    int end = start + id.size() / thread_number;
    if (i == thread_number - 1) end = id.size() - 1;
    th[i] = std::thread(task, wiki[i], start, end);
  }
  for (int i = 0; i < thread_number; i++) {
    th[i].join();
  }
  if (is_success) {
    std::cout << "success" << std::endl;
  } else {
    std::cout << "failed" << std::endl;
  }
  return 0;
}