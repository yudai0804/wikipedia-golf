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
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include "exception.hpp"
#include "timer.hpp"
#include "wikipedia.hpp"
// for HOST, USER, PASSWORD
#include "private_config.hpp"

namespace fs = std::filesystem;
fs::path directory = "graph_bin/";
std::string filetype = ".bin";

Wikipedia wiki(HOST, USER, PASSWORD);
std::vector<std::vector<int>> graph;
std::mutex graph_mtx;
bool load_success = true;
std::mutex load_success_mtx;

int thread_number = -1;

void load_task(std::vector<int> id, int start, int end) {
  for (int i = start; i < end; i++) {
    fs::path filename = directory / fs::path(std::to_string(id[i]) + filetype);
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
      load_success_mtx.lock();
      load_success = false;
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

void load() {
  if ((fs::exists(directory) && fs::is_directory(directory)) == false) {
    throw EXCEPTION("Directory Error");
  }
  int total_file = 0;
  int max_file_number = -1;
  for (const auto& entry : fs::directory_iterator(directory)) {
    total_file++;
    max_file_number =
        std::max(max_file_number, std::stoi(entry.path().filename()));
  }
  std::cout << "[INFO] total_file: " << total_file << std::endl;
  graph.resize(max_file_number + 1);

  auto id = wiki.get_all_page_id();
  std::vector<std::future<void>> res(thread_number);
  for (int i = 0; i < thread_number; i++) {
    int start = id.size() / thread_number * i;
    int end = start + id.size() / thread_number;
    if (i == thread_number - 1) end = id.size() - 1;
    if (i == 0)
      res[i] = std::async(std::launch::deferred, load_task, id, start, end);
    else
      res[i] = std::async(std::launch::async, load_task, id, start, end);
  }
  for (int i = 0; i < thread_number; i++) {
    res[i].get();
  }

  if (load_success == false) {
    throw EXCEPTION("[ERROR] load failed");
  } else {
    std::cout << "[INFO] load success" << std::endl;
  }
}

constexpr uint8_t MAX_DEPTH = 6;
int inf = 1e9;
uint8_t inf_cost = 255;

int max_ans_number = 5;
bool allow_similar_path = false;

struct Edge {
public:
  uint8_t cost;
  int page_id;
  std::array<int, MAX_DEPTH> path;
};

template <typename T>
class FastQueue {
private:
  std::vector<T> _buffer;
  size_t _l, _r, _size;

public:
  FastQueue() { _l = _r = _size = 0; }
  FastQueue(size_t sz) {
    _size = sz;
    _l = _r = 0;
    _buffer.resize(sz);
  }
  void push(T value) {
    _buffer[_r] = value;
    _r = (_r + 1) % _size;
    if (_r == _l) throw EXCEPTION("buffer error");
  }
  template <class... Args>
  void emplace(Args... args) {
    _buffer[_r] = T(args...);
    _r = (_r + 1) % _size;
    if (_r == _l) throw EXCEPTION("buffer error");
  }
  T front() { return _buffer[_l]; }
  void pop() {
    if (_r == _l) throw EXCEPTION("buffer error");
    _l = (_l + 1) % _size;
  }
  bool empty() { return _r == _l; }
  void clear() { _l = _r = 0; }
};

template <typename Queue>
int search(Queue& q, std::string start, std::string goal) {
  int start_page_id = wiki.page_title_to_page_id(start);
  int goal_page_id = wiki.page_title_to_page_id(goal);
  if (start_page_id == -1 || goal_page_id == -1) {
    std::cerr << "\033[31m[ERROR]\033[0m The entered word does not exist."
              << std::endl;
    return 1;
  }
  std::vector<std::vector<int>> ans_id;
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
    std::cerr << "\033[31m[ERROR]\033[0m failed search" << std::endl;
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
  std::cout << "cost:" << ok_cost - 1 << std::endl;

  for (size_t i = 0; i < ans.size(); i++) {
    std::cout << i << ":";
    for (size_t j = 0; j < ans[i].size(); j++) {
      std::cout << ans[i][j];
      if (j != ans[i].size() - 1)
        std::cout << "->";
      else
        std::cout << std::endl;
    }
  }
  std::cout << "[INFO] search success" << std::endl;

  return 0;
}

int main(int argc, char** argv) {
  try {
    Timer timer;
    std::string start, goal;
    bool parse_ok = true;
    bool use_fast_queue = false;
    int tmp;
    // parse
    for (int i = 1; i < argc; i++) {
      std::string arg = argv[i];
      if (arg == "--help" || arg == "-h") {
        std::cout
            << "Usage: ./wikipedia-golf\n"
            << "If there are spaces included, please enclose the text in single quotes or double quotes.\n\n"
            << "option arguments:\n"
            << "--input [PATH]          Input directory path.(defualt: graph_bin)\n"
            << "--thread_number [NUM]   Thread number for loading.(default: 1)\n"
            << "                        Please note that increasing the number of threads will not speed up the search.\n"
            << "--max_ans_number [NUM]  Max answer number.(default: 5)\n"
            << "--allow_similar_path    Allow similar_path.(default: false)\n"
            << "                        Setting it to true will make it very slow.\n"
            << "--use_fast_queue        Using fast queue.\n"
            << "                        Fast queue is using 4GB RAM.\n"
            << std::endl;
        return 0;
      } else if (arg == "--input" && i + 1 < argc) {
        directory = argv[++i];
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
      } else if (arg == "--use_fast_queue") {
        use_fast_queue = true;
      } else {
        throw EXCEPTION("Parse error");
      }
    }
    if (parse_ok == false) {
      throw EXCEPTION("Parse error");
    }

    std::cout << "[INFO] load start" << std::endl;

    wiki.init();

    timer.start();
    load();
    std::cout << "[INFO] ";
    timer.print();

    std::queue<Edge> std_queue;
    FastQueue<Edge> fast_queue;
    if (use_fast_queue) {
      fast_queue = FastQueue<Edge>(4e9 / sizeof(Edge));
    }
    std::cout << "Please input word" << std::endl;
    while (1) {
      std::cout << "start word:" << std::flush;
      std::getline(std::cin, start);
      if (std::cin.eof()) return 0;
      std::cout << "goal word:" << std::flush;
      std::getline(std::cin, goal);
      if (std::cin.eof()) return 0;
      timer.start();
      if (use_fast_queue) {
        fast_queue.clear();
        search(fast_queue, start, goal);
      } else {
        std_queue = std::queue<Edge>();
        search(std_queue, start, goal);
      }
      std::cout << "[INFO] Time: " << timer.get() << "[s]" << std::endl;
    }

    return 0;
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
}
