/**
 * @file create_graph_file.cpp
 * @author Yudai Yamaguchi (yudai.yy0804@gmail.com)
 * @brief SQLのテーブルをバイナリファイルに変換する
 * @version 0.1
 * @date 2024-09-19
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include "timer.hpp"
#include "wikipedia.hpp"
// for HOST, USER, PASSWORD
#include "private_config.hpp"

namespace fs = std::filesystem;

constexpr std::string DIRECTORY = "graph_bin/";
constexpr std::string FILE_TYPE = ".bin";
int PROGRESS_INTERVAL = 100;
bool is_success = true;
std::mutex mtx;

void task(std::shared_ptr<Wikipedia> wiki, int start, int end) {
  auto id = wiki->get_all_page_id();
  for (int i = start; i < end; i++) {
    int target = id[i];
    std::string filename = DIRECTORY + std::to_string(target) + FILE_TYPE;
    if (fs::exists(filename)) continue;

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
  Timer timer;
  int tmp;
  bool parse_ok = true;
  int thread_number = 1;
  // parse
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      std::cout
          << "Usage: ./create_graph_file\n"
          << "option arguments:\n"
          << "--thread_number [NUM]   Thread number for exporting.(default: 1)\n"
          << std::endl;
      return 0;
    } else if (arg == "--thread_number" && i + 1 < argc) {
      tmp = atoi(argv[++i]);
      if (tmp <= 0) parse_ok = false;
      thread_number = tmp;
    } else {
      std::cerr << "input error" << std::endl;
      return 1;
    }
  }
  if (parse_ok == false) {
    std::cerr << "input error" << std::endl;
    return 1;
  }

  if (fs::exists(DIRECTORY) == false) {
    if (fs::is_directory(DIRECTORY) == false) {
      std::cerr << DIRECTORY << "is file" << std::endl;
      return 1;
    } else if (fs::create_directory(DIRECTORY) == false) {
      std::cerr << "create directory failed" << std::endl;
      return 1;
    }
  }

  timer.start();

  std::vector<std::shared_ptr<Wikipedia>> wiki(thread_number);
  for (int i = 0; i < thread_number; i++) {
    wiki[i] = std::make_shared<Wikipedia>(HOST, USER, PASSWORD);
    wiki[i]->init();
  }
  auto id = wiki[0]->get_all_page_id();
  std::vector<std::future<void>> res(thread_number);
  for (int i = 0; i < thread_number; i++) {
    int start = id.size() / thread_number * i;
    int end = start + id.size() / thread_number;
    if (i == thread_number - 1) end = id.size() - 1;
    if (i == 0)
      res[i] = std::async(std::launch::deferred, task, wiki[i], start, end);
    else
      res[i] = std::async(std::launch::async, task, wiki[i], start, end);
  }
  for (int i = 0; i < thread_number; i++) {
    res[i].get();
  }

  if (is_success) {
    std::cout << "success" << std::endl;
  } else {
    std::cout << "failed" << std::endl;
  }
  timer.print();
  return (int)is_success ^ 0x01;
}
