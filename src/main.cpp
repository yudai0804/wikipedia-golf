#include <chrono>
#include <deque>
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

std::map<int, std::pair<std::vector<int>, int>> graph;
int MAX_DEPTH = 3;
int inf = 1e9;

void search(std::string start, std::string goal) {
  int start_page_id = wiki.page_title_to_page_id(start);
  int goal_page_id = wiki.page_title_to_page_id(goal);
  if (start_page_id == -1 || goal_page_id == -1) {
    std::cout << "error" << std::endl;
    return;
  }
  std::queue<std::pair<int, int>> q;
  std::set<int> visit;
  q.push(std::pair<int, int>(start_page_id, 1));
  bool ok = false;
  while (q.empty() == false && ok == false) {
    auto [page_id, cost] = q.front();
    q.pop();
    if (visit.count(page_id)) continue;
    visit.insert(page_id);
    wiki.search(page_id, graph[page_id].first);
    graph[page_id].second = cost;
    for (auto _ : graph[page_id].first) {
      if (visit.count(_)) continue;
      graph[_].first.push_back(page_id);
      graph[_].second = cost;
      if (_ == goal_page_id) {
        ok = true;
        break;
      }
      if (cost + 1 <= MAX_DEPTH) q.push(std::pair<int, int>(_, cost + 1));
    }
  }
  if (ok == false) {
    std::cout << "failed" << std::endl;
    return;
  }
  std::deque<int> dq;
  visit.clear();
  std::vector<int> ans;
  auto dfs = [&](auto dfs, int page_id, int cost) {
    if (visit.count(page_id) || ans.size() > 0) return;
    dq.push_back(page_id);
    visit.insert(page_id);
    if (page_id == start_page_id) {
      for (auto it = dq.rbegin(); it != dq.rend(); it++) ans.push_back(*it);
      dq.pop_back();
      return;
    }
    for (auto next : graph[page_id].first) {
      if (graph[next].second == cost - 1) dfs(dfs, next, cost - 1);
    }
    dq.pop_back();
  };
  dfs(dfs, goal_page_id, graph[goal_page_id].second + 1);
  std::cout << "answer" << std::endl;
  for (int i = 0; i < ans.size(); i++)
    std::cout << i << ":" << wiki.page_id_to_page_title(ans[i]) << std::endl;
}

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cout << "input error" << std::endl;
    return 0;
  }
  std::string target = argv[1];
  std::string goal = argv[2];
  wiki.init();
  auto start_page_id = wiki.page_title_to_page_id(target);
  // 開始時刻を記録
  auto start = std::chrono::high_resolution_clock::now();

  search(target, goal);

  // 終了時刻を記録
  auto end = std::chrono::high_resolution_clock::now();

  // 経過時間を計算
  std::chrono::duration<double> elapsed = end - start;

  // 結果を表示
  std::cout << "Elapsed time: " << elapsed.count() << " seconds\n";

  return 0;
}
