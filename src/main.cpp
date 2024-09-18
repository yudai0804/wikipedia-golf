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

#include "wikipedia.hpp"
// for HOST, USER, PASSWORD
#include "private_config.hpp"

namespace fs = std::filesystem;
constexpr std::string DIRECTORY = "graph_bin/";

Wikipedia wiki(HOST, USER, PASSWORD);
std::map<int, std::vector<int>> graph;
std::vector<int8_t> visit;

int load() {
  if ((fs::exists(DIRECTORY) && fs::is_directory(DIRECTORY)) == false) {
    std::cerr << "directory error" << std::endl;
    return 1;
  }
  int progress = 0;
  int total_file = 0;
  for (const auto& entry : fs::directory_iterator(DIRECTORY)) 
    total_file++;
  std::cout << "total_file: " << total_file << std::endl;
  for (const auto& entry : fs::directory_iterator(DIRECTORY)) {
    progress++;
    // if(progress % 10000 == 0)
      // std::cout  << " [" << (double)progress / total_file * 100 << "%] " << progress << "/" << total_file << std::endl;
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

int MAX_DEPTH = 3;
int inf = 1e9;

int search(std::string start, std::string goal) {
  int start_page_id = wiki.page_title_to_page_id(start);
  int goal_page_id = wiki.page_title_to_page_id(goal);
  if (start_page_id == -1 || goal_page_id == -1) {
    std::cerr << "error" << std::endl;
    return 1;
  }
  std::queue<std::pair<int, int>> q;
  std::map<int, int> visit;
  q.push(std::pair<int, int>(start_page_id, 1));
  bool ok = false;
  while (q.empty() == false && ok == false) {
    auto [page_id, cost] = q.front();
    q.pop();
    if (visit.count(page_id)) continue;
    visit[page_id] = cost;
    if (page_id == goal_page_id) {
      ok = true;
      break;
    }
    for (auto _ : graph[page_id]) {
      if (visit.count(_)) continue;
      if (cost + 1 <= MAX_DEPTH) q.push(std::pair<int, int>(_, cost + 1));
    }
  }
  if (ok == false) {
    std::cerr << "failed search" << std::endl;
    return 1;
  }
  std::deque<int> dq;
  std::set<int> visit2;
  // ややこしいので別名をつける
  auto &cost = visit;
  std::vector<int> ans;
  auto dfs = [&](auto dfs, int page_id, int _cost) {
    if (visit2.count(page_id) || ans.size() > 0) return;
    dq.push_back(page_id);
    visit2.insert(page_id);
    if (page_id == start_page_id) {
      for (auto it = dq.rbegin(); it != dq.rend(); it++) ans.push_back(*it);
      dq.pop_back();
      return;
    }
    for (auto next : graph[page_id]) {
      if (cost[next] == _cost - 1) dfs(dfs, next, _cost - 1);
    }
    dq.pop_back();
  };
  dfs(dfs, goal_page_id, cost[goal_page_id] + 1);
  std::cout << "answer" << std::endl;
  for (int i = 0; i < ans.size(); i++)
    std::cout << i << ":" << wiki.page_id_to_page_title(ans[i]) << std::endl;
  return 0;
}

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cout << "input error" << std::endl;
    return 0;
  }

  std::string target = argv[1];
  std::string goal = argv[2];

  wiki.init();

  int status = load();
  if(status == 0) {
    std::cout << "load success" << std::endl;
  } else {
    std::cout << "load failed" << std::endl;
    return status;
  }
  
  status = search(target, goal);
  
  return status;
}
