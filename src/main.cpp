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
#include <deque>
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

Wikipedia wiki(HOST, USER, PASSWORD);
std::vector<std::vector<int>> graph;

int load() {
  if ((fs::exists(DIRECTORY) && fs::is_directory(DIRECTORY)) == false) {
    std::cerr << "directory error" << std::endl;
    return 1;
  }
  int total_file = 0;
  int max_file_number = -1;
  for (const auto& entry : fs::directory_iterator(DIRECTORY)){
    total_file++;
    max_file_number = std::max(max_file_number, std::stoi(entry.path().filename()));
  } 
  std::cout << "total_file: " << total_file << std::endl;
  graph.resize(max_file_number + 1);

  for (const auto& entry : fs::directory_iterator(DIRECTORY)) {
    std::ifstream file(entry.path(), std::ios::binary);
    if (!file) {
      std::cerr << "Error opening file for reading" << std::endl;
      return 1;
    }

    // ファイルサイズを取得
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    char* buffer = new char[size];

    // ファイルからデータを読み込む
    file.read(buffer, size);
    int index = std::stoi(entry.path().filename());
    graph[index].reserve(size / 4);
    for (int i = 0; i < size; i += 4) {
      int* tmp = (int*)&buffer[i];
      graph[index].push_back(*tmp);
    }
    delete[] buffer;
    file.close();
  }
  return 0;
}

constexpr uint8_t MAX_DEPTH = 6;
int max_ans_cost = 0;
int inf = 1e9;
uint8_t inf_cost = 255;

class Edge {
public:
  uint8_t cost;
  int page_id;
  std::array<int, MAX_DEPTH> path;
  Edge(uint8_t _cost, int _page_id, std::array<int, MAX_DEPTH> _path)
      : cost(_cost), page_id(_page_id), path(_path) {}
};

// カスタムの比較関数
struct Compare {
  bool operator()(const Edge& a, const Edge& b) {
    // 優先度が高い（小さい値）の方が前に来る
    return a.cost > b.cost;
  }
};

int search(std::string start, std::string goal) {
  int start_page_id = wiki.page_title_to_page_id(start);
  int goal_page_id = wiki.page_title_to_page_id(goal);
  if (start_page_id == -1 || goal_page_id == -1) {
    std::cerr << "error" << std::endl;
    return 1;
  }
  std::vector<std::vector<int>> ans;
  std::priority_queue<Edge, std::vector<Edge>, Compare> pq;
  std::vector<uint8_t> visit(graph.size(), inf_cost);
  std::array<int, MAX_DEPTH> _ = {start_page_id};
  pq.emplace(1, start_page_id, _);
  int ok_cost = inf_cost;
  while (pq.empty() == false && ans.size() < (size_t)max_ans_cost) {
    auto [cost, page_id, path] = pq.top();
    pq.pop();
    if (ok_cost != inf_cost && cost > ok_cost) break;
    if (page_id == goal_page_id) {
      ok_cost = cost;
      std::vector<int> tmp(cost);
      for (int i = 0; i < cost; i++) {
        tmp[i] = path[i];
      }
      ans.push_back(tmp);
      continue;
    }
    if (visit[page_id] <= cost) continue;
    visit[page_id] = cost;
    for (auto next : graph[page_id]) {
      uint8_t next_cost = cost + 1;
      if(visit[next] <= next_cost) continue;
      if (next_cost <= MAX_DEPTH && next_cost <= ok_cost) {
        path[cost] = next;
        pq.emplace(next_cost, next, path);
      }
    }
  }
  if (ok_cost == inf_cost) {
    std::cerr << "failed search" << std::endl;
    return 1;
  }

  std::map<int, std::string> cache;

  std::cout << "answer. total:" << ans.size() << std::endl;
  for (size_t i = 0; i < ans.size(); i++) {
    for (size_t j = 0; j < ans[i].size(); j++) {
      if(cache.count(ans[i][j]) == false)
        cache[ans[i][j]] = wiki.page_id_to_page_title(ans[i][j]);
      std::cout << cache[ans[i][j]];
      if (j == ans[i].size() - 1)
        std::cout << std::endl;
      else
        std::cout << "->";
    }
  }
  return 0;
}

int main(int argc, char** argv) {
  if (argc != 4) {
    std::cerr << "input error" << std::endl;
    return 1;
  }

  Timer timer;
  max_ans_cost = atoi(argv[1]);
  if(max_ans_cost <= 0) {
    std::cerr << "max_ans_cost error" << std::endl;
    return 1;
  }
  std::string target = argv[2];
  std::string goal = argv[3];

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
  status = search(target, goal);
  std::cout << timer.get() << "[s]" << std::endl;

  return status;
}
