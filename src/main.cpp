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

#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#include <array>
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

#include "log.hpp"
#include "timer.hpp"

namespace fs = std::filesystem;
fs::path directory = "graph_bin/";
std::string filetype = ".bin";

std::vector<std::vector<int>> graph;
std::mutex graph_mtx;
bool load_success = true;
std::mutex load_success_mtx;

std::map<int, std::string> page_id_to_page_title;
std::map<std::string, int> page_title_to_page_id;

std::vector<int> all_page_id;

int thread_number = 1;

size_t LOAD_BUFFER_SIZE = 1e6;
size_t FAST_QUEUE_BUFFER_SIZE = 4e9;

void load_task(std::vector<int> id, int start, int end) {
  char buffer[LOAD_BUFFER_SIZE];
  for (int i = start; i < end; i++) {
    fs::path filename = directory / fs::path(std::to_string(id[i]) + filetype);
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
      load_success_mtx.lock();
      load_success = false;
      load_success_mtx.unlock();
      return;
    }

    // ファイルサイズを取得
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if ((size_t)size > LOAD_BUFFER_SIZE) {
      load_success_mtx.lock();
      load_success = false;
      load_success_mtx.unlock();
      return;
    }

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
    file.close();
  }
}

int load_page_title_to_page_id() {
  fs::path filename = directory / fs::path("0_page_title_to_page_id.txt");
  std::ifstream file(filename);

  if (!file) {
    EXCEPTION("File open failed");
  }

  std::string line;
  int max_page_number = -1;
  while (std::getline(file, line)) {
    for (size_t i = 0; i < line.size(); i++) {
      if (line[i] != ',') continue;
      std::string _num, str;
      int num;
      for (size_t j = 0; j < i; j++) _num += line[j];
      num = std::stoi(_num);
      for (size_t j = i + 1; j < line.size(); j++) {
        str += line[j];
      }
      page_id_to_page_title[num] = str;
      page_title_to_page_id[str] = num;
      if (num > max_page_number) max_page_number = num;
      break;
    }
  }

  file.close();
  return max_page_number;
}

void load() {
  if ((fs::exists(directory) && fs::is_directory(directory)) == false) {
    throw EXCEPTION("Directory error");
  }
  int max_page_number = load_page_title_to_page_id();
  graph.resize(max_page_number + 1);

  for (auto itr = page_id_to_page_title.begin();
       itr != page_id_to_page_title.end(); itr++) {
    all_page_id.push_back(itr->first);
  }
  std::vector<std::future<void>> res(thread_number);
  for (int i = 0; i < thread_number; i++) {
    int start = all_page_id.size() / thread_number * i;
    int end = start + all_page_id.size() / thread_number;
    if (i == thread_number - 1) end = all_page_id.size() - 1;
    if (i == 0)
      res[i] =
          std::async(std::launch::deferred, load_task, all_page_id, start, end);
    else
      res[i] =
          std::async(std::launch::async, load_task, all_page_id, start, end);
  }
  for (int i = 0; i < thread_number; i++) {
    res[i].get();
  }

  if (load_success == false) {
    throw EXCEPTION("Load failed");
  } else {
    LOG_OK << "Load success" << std::endl;
  }
}

constexpr uint8_t MAX_DEPTH = 8;
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
    if (_l == _r) throw EXCEPTION("Buffer error");
  }
  template <class... Args>
  void emplace(Args... args) {
    _buffer[_r] = T(args...);
    _r = (_r + 1) % _size;
    if (_l == _r) throw EXCEPTION("Buffer error");
  }
  T front() { return _buffer[_l]; }
  void pop() {
    if (_l == _r) throw EXCEPTION("Buffer error");
    _l = (_l + 1) % _size;
  }
  bool empty() { return _l == _r; }
  void clear() { _l = _r = 0; }
};

/**
 * @brief 西暦、日付のページを無効化する
 * 無効にするもの
 * - 月
 * - 日
 * - 何月何日
 * - 西暦
 *
 * 西暦参考:https://ja.wikipedia.org/wiki/%E5%B9%B4%E3%81%AE%E4%B8%80%E8%A6%A7
 *
 * @param visit
 */
void ignore_date(std::vector<uint8_t>& visit) {
  auto ignore_task = [&](std::string str) {
    if (page_title_to_page_id.count(str) == 0) return;
    int id = page_title_to_page_id[str];
    visit[id] = 0;
  };
  // 月
  for (int i = 1; i <= 12; i++) {
    ignore_task(std::to_string(i) + "月");
  }
  // 日
  for (int i = 1; i <= 31; i++) {
    ignore_task(std::to_string(i) + "日");
  }
  // 何月何日
  for (int i = 1; i <= 12; i++) {
    for (int j = 1; j <= 31; j++) {
      ignore_task(std::to_string(i) + "月" + std::to_string(j) + "日");
    }
  }
  // 紀元前1年から紀元前2500年
  for (int i = 1; i <= 2500; i++) {
    ignore_task("紀元前" + std::to_string(i) + "年");
  }
  // 1年から2500年
  for (int i = 1; i <= 2500; i++) {
    ignore_task(std::to_string(i) + "年");
  }
}

template <typename Queue>
int search(Queue& q, std::string start, std::string goal, bool is_ignore_date) {
  if (page_title_to_page_id.count(start) == 0 ||
      page_title_to_page_id.count(goal) == 0) {
    LOG_ERROR << "The entered word does not exist." << std::endl;
    return 1;
  }
  int start_page_id = page_title_to_page_id[start];
  int goal_page_id = page_title_to_page_id[goal];
  std::vector<std::vector<int>> ans_id;
  std::vector<uint8_t> visit(graph.size(), inf_cost);
  std::array<int, MAX_DEPTH> _ = {start_page_id};

  if (is_ignore_date) {
    ignore_date(visit);
  }

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
    LOG_ERROR << "Failed search" << std::endl;
    return 1;
  }

  // create ans string data
  std::vector<std::vector<std::string>> ans(ans_id.size());

  for (size_t i = 0; i < ans_id.size(); i++) {
    for (size_t j = 0; j < ans_id[i].size(); j++) {
      std::string str = page_id_to_page_title[ans_id[i][j]];
      ans[i].push_back(str);
    }
  }

  // print
  std::cout
      << "--------------------------------------------------------------------------------"
      << std::endl;
  std::cout << "Total answer: " << ans.size() << std::endl;
  std::cout << "Cost: " << ok_cost - 1 << std::endl;

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
  std::cout
      << "--------------------------------------------------------------------------------"
      << std::endl;

  LOG_OK << "Search success" << std::endl;

  return 0;
}

int main(int argc, char** argv) {
  try {
    Timer timer;
    bool parse_ok = true;
    bool use_fast_queue = false;
    int tmp;
    // parse
    for (int i = 1; i < argc; i++) {
      std::string arg = argv[i];
      if (arg == "--help" || arg == "-h") {
        std::cout
            << "Usage: ./wikipedia-golf\n\n"
            << "option arguments:\n"
            << "-h --help               Show help\n"
            << "-v --version            Show version\n"
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
      } else if (arg == "-v" || arg == "--version") {
        std::cout << "Version " << MAJOR_VERSION << "." << MINOR_VERSION
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
        std::string msg;
        msg = "Parse error.\n\"\033[1m" + arg + "\033[0m\" is unkwnon.";
        throw EXCEPTION(msg);
      }
    }
    if (parse_ok == false) {
      throw EXCEPTION("Parse error");
    }

    LOG_INFO << "Load start" << std::endl;

    timer.start();
    load();
    LOG_INFO;
    timer.print();

    std::queue<Edge> std_queue;
    FastQueue<Edge> fast_queue;
    bool fast_queue_initialize_end = false;
    while (1) {
      std::string start, goal, option;
      bool is_ignore_date = false;
      // 入力してね
      std::cout << "\033[1mPlease input word\033[0m" << std::endl;
      // start
      std::cout << "Start word:" << std::flush;
      // 入力待ちの間にメモリを確保することで、遅延を感じないようにする。
      if (use_fast_queue && fast_queue_initialize_end == false) {
        fast_queue = FastQueue<Edge>(FAST_QUEUE_BUFFER_SIZE / sizeof(Edge));
        fast_queue_initialize_end = true;
      }
      std::getline(std::cin, start);
      if (std::cin.eof()) return 0;

      // goal
      std::cout << "Goal word:" << std::flush;
      std::getline(std::cin, goal);
      if (std::cin.eof()) return 0;

      // option
      std::cout << "Option:" << std::flush;
      std::getline(std::cin, option);
      if (std::cin.eof()) return 0;

      // parse
      if (option == "--ignore_date")
        is_ignore_date = true;
      else if (option != "") {
        LOG_ERROR << "Option error" << std::endl;
        std::cout
            << "If you don't wish to use the options, please press the Enter key.\n"
            << "Available option\n"
            << "--ignore_date" << std::endl;
        continue;
      }

      // search
      timer.start();
      if (use_fast_queue) {
        fast_queue.clear();
        search(fast_queue, start, goal, is_ignore_date);
      } else {
        std_queue = std::queue<Edge>();
        search(std_queue, start, goal, is_ignore_date);
      }
      LOG_INFO << "Time: " << timer.get() << "[s]" << std::endl;
    }

    return 0;
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
}
