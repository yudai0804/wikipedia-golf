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
std::map<int, std::vector<int>> graph;

int load() {
  if ((fs::exists(DIRECTORY) && fs::is_directory(DIRECTORY)) == false) {
    std::cerr << "directory error" << std::endl;
    return 1;
  }
  int progress = 0;
  int total_file = 0;
  for (const auto& entry : fs::directory_iterator(DIRECTORY)) total_file++;
  std::cout << "total_file: " << total_file << std::endl;
  for (const auto& entry : fs::directory_iterator(DIRECTORY)) {
    progress++;
    // if(progress % 10000 == 0)
    // std::cout  << " [" << (double)progress / total_file * 100 << "%] " <<
    // progress << "/" << total_file << std::endl;
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
    std::vector<int> data(size / 4);
    for (int i = 0; i < size; i += 4) {
      int* tmp = (int*)&buffer[i];
      data[i / 4] = *tmp;
    }
    graph[std::stoi(entry.path().filename())] = data;
    delete[] buffer;
    file.close();
  }
  return 0;
}

constexpr int MAX_DEPTH = 6;
constexpr int MAX_ANS_CNT = 1;
int inf = 1e9;

class Edge {
public:
  int cost;
  int page_id;
  std::array<int, MAX_DEPTH> path;
  Edge(int _cost, int _page_id, std::array<int, MAX_DEPTH> _path)
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
  std::map<int, int> visit;
  std::array<int, MAX_DEPTH> _ = {start_page_id};
  pq.push(Edge(1, start_page_id, _));
  int ok_cost = inf;
  while (pq.empty() == false && ans.size() < MAX_ANS_CNT) {
    auto [cost, page_id, path] = pq.top();
    pq.pop();
    if (ok_cost != inf && cost > ok_cost) break;
    if (page_id == goal_page_id) {
      ok_cost = cost;
      std::vector<int> tmp(cost);
      for (int i = 0; i < cost; i++) {
        tmp[i] = path[i];
      }
      ans.push_back(tmp);
      continue;
    }
    if (visit.count(page_id) && visit[page_id] <= cost) continue;
    visit[page_id] = cost;
    for (auto next : graph[page_id]) {
      int next_cost = cost + 1;
      if (visit.count(next) && visit[next] <= next_cost) continue;
      if (next_cost <= MAX_DEPTH && next_cost <= ok_cost) {
        path[cost] = next;
        pq.push(Edge(next_cost, next, path));
        path[cost] = -1;
      }
    }
  }
  if (ok_cost == inf) {
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
  if (argc != 3) {
    std::cout << "input error" << std::endl;
    return 0;
  }

  Timer timer;
  std::string target = argv[1];
  std::string goal = argv[2];

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
  // timer.print();
  std::cout << timer.get() << "[s]" << std::endl;

  return status;
}
