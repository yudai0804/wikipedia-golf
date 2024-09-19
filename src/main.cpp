/**
 * @file main.cpp
 * @author Yudai Yamaguchi (yudai.yy0804@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-19
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <chrono>
#include <filesystem>
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

#include "timer.hpp"
#include "wikipedia.hpp"
// for HOST, USER, PASSWORD
#include "private_config.hpp"

namespace fs = std::filesystem;
constexpr std::string DIRECTORY = "graph_bin/";
constexpr std::string FILE_TYPE = ".bin";

Wikipedia wiki(HOST, USER, PASSWORD);
std::vector<std::vector<int>> graph;
std::mutex graph_mtx;
bool load_success = true;
std::mutex load_success_mtx;

int thread_number = -1;

void load_task(std::vector<int> id, int start, int end) {
  for (int i = start; i < end; i++) {
    std::string filename = DIRECTORY + std::to_string(id[i]) + FILE_TYPE;
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
      load_success_mtx.lock();
      load_success = false;
      std::cerr << "Error opening file for reading" << std::endl;
      load_success_mtx.unlock();
    }

    // ファイルサイズを取得
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    char* buffer = new char[size];

    // ファイルからデータを読み込む
    file.read(buffer, size);
    int index = id[i];
    graph_mtx.lock();
    graph[index].reserve(size / 4);
    for (int i = 0; i < size; i += 4) {
      int* tmp = (int*)&buffer[i];
      graph[index].push_back(*tmp);
    }
    graph_mtx.unlock();
    delete[] buffer;
    file.close();
  }
}

int load() {
  if ((fs::exists(DIRECTORY) && fs::is_directory(DIRECTORY)) == false) {
    std::cerr << "directory error" << std::endl;
    return 1;
  }
  int total_file = 0;
  int max_file_number = -1;
  for (const auto& entry : fs::directory_iterator(DIRECTORY)) {
    total_file++;
    max_file_number =
        std::max(max_file_number, std::stoi(entry.path().filename()));
  }
  std::cout << "total_file: " << total_file << std::endl;
  graph.resize(max_file_number + 1);

  auto id = wiki.get_all_page_id();
  int load_thread_number = thread_number - 1;
  if (thread_number == 1) {
    load_task(id, 0, id.size() - 1);
  } else {
    std::vector<std::thread> th(load_thread_number);
    for (int i = 0; i < load_thread_number; i++) {
      int start = id.size() / load_thread_number * i;
      int end = start + id.size() / load_thread_number;
      if (i == load_thread_number - 1) end = id.size() - 1;
      th[i] = std::thread(load_task, id, start, end);
    }
    for (int i = 0; i < load_thread_number; i++) {
      th[i].join();
    }
  }

  return (int)load_success ^ 0x01;
}

constexpr uint8_t MAX_DEPTH = 6;
int inf = 1e9;
uint8_t inf_cost = 255;

int max_ans_number = 5;
bool allow_similar_path = false;

class Edge {
public:
  uint8_t cost;
  int page_id;
  std::array<int, MAX_DEPTH> path;
  Edge(uint8_t _cost, int _page_id, std::array<int, MAX_DEPTH> _path)
      : cost(_cost), page_id(_page_id), path(_path) {}
};

int search(std::string start, std::string goal) {
  int start_page_id = wiki.page_title_to_page_id(start);
  int goal_page_id = wiki.page_title_to_page_id(goal);
  if (start_page_id == -1 || goal_page_id == -1) {
    std::cerr << "The entered word does not exist." << std::endl;
    return 1;
  }
  std::vector<std::vector<int>> ans_id;
  std::queue<Edge> q;
  std::vector<uint8_t> visit(graph.size(), inf_cost);
  std::array<int, MAX_DEPTH> _ = {start_page_id};
  q.emplace(1, start_page_id, _);
  int ok_cost = inf_cost;
  while (q.empty() == false && ans_id.size() < (size_t)max_ans_number) {
    auto [cost, page_id, path] = q.front();
    q.pop();
    if (ok_cost != inf_cost && cost > ok_cost) break;
    if (page_id == goal_page_id) {
      ok_cost = cost;
      std::vector<int> tmp(cost);
      for (int i = 0; i < cost; i++) {
        tmp[i] = path[i];
      }
      ans_id.push_back(tmp);
      continue;
    }
    if (allow_similar_path && visit[page_id] < cost) continue;
    if (allow_similar_path == false && visit[page_id] <= cost) continue;
    visit[page_id] = cost;
    for (auto next : graph[page_id]) {
      uint8_t next_cost = cost + 1;
      if (allow_similar_path && visit[next] < next_cost) continue;
      if (allow_similar_path == false && visit[next] <= next_cost) continue;
      if (next_cost <= MAX_DEPTH && next_cost <= ok_cost) {
        path[cost] = next;
        q.emplace(next_cost, next, path);
      }
    }
  }
  if (ok_cost == inf_cost) {
    std::cerr << "failed search" << std::endl;
    return 1;
  }

  // create ans string data

  std::map<int, std::string> cache;
  std::vector<std::vector<std::string>> ans(ans_id.size());

  for (size_t i = 0; i < ans_id.size(); i++) {
    for (size_t j = 0; j < ans_id[i].size(); j++) {
      if (cache.count(ans_id[i][j]) == false)
        cache[ans_id[i][j]] = wiki.page_id_to_page_title(ans_id[i][j]);
      ans[i].push_back(cache[ans_id[i][j]]);
    }
  }

  // print
  std::cout << "total answer:" << ans.size() << std::endl;

  for (size_t i = 0; i < ans.size(); i++) {
    for (size_t j = 0; j < ans[i].size(); j++) {
      std::cout << ans[i][j];
      if (j != ans[i].size() - 1)
        std::cout << "->";
      else
        std::cout << std::endl;
    }
  }

  return 0;
}

int main(int argc, char** argv) {
  Timer timer;
  std::string start, goal;
  bool parse_ok = true;
  int tmp;

  // parse
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      std::cout
          << "Usage: ./wikipedia-golf [--start WORD] [--goal WORD]\n"
          << "If there are spaces included, please enclose the text in single quotes or double quotes.\n\n"
          << "option arguments:\n"
          << "--thread_number [NUM]   Thread number for loading.(default: 1)\n"
          << "                        Please note that increasing the number of threads will not speed up the search.\n"
          << "--max_ans_number [NUM]  Max answer number.(default: 5)\n"
          << "--allow_similar_path    Allow similar_path.(default: false)\n"
          << "                        Setting it to true will make it very slow.\n"
          << std::endl;
      return 0;
    } else if (arg == "--start" && i + 1 < argc) {
      start = argv[++i];
    } else if (arg == "--goal" && i + 1 < argc) {
      goal = argv[++i];
    } else if (arg == "--thread_number" && i + 1 < argc) {
      tmp = atoi(argv[++i]);
      if (tmp <= 0) parse_ok = false;
      thread_number = tmp;
    } else if (arg == "--max_ans_number" && i + 1 < argc) {
      tmp = atoi(argv[++i]);
      if (tmp <= 0) parse_ok = false;
      max_ans_number = tmp;
    } else if (arg == "--allow_similar_path") {
      allow_similar_path = true;
    } else {
      std::cerr << "input error" << std::endl;
      return 1;
    }
  }
  if (parse_ok == false) {
    std::cerr << "input error" << std::endl;
    return 1;
  }

  wiki.init();

  timer.start();
  int status = load();
  if (status == 0) {
    std::cout << "load success" << std::endl;
  } else {
    std::cout << "load failed" << std::endl;
    return status;
  }
  timer.print();

  timer.start();
  status = search(start, goal);
  std::cout << "Time: " << timer.get() << "[s]" << std::endl;

  return status;
}
