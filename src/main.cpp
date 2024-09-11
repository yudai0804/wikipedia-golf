#include <iostream>
#include <vector>
#include <memory>
#include <map>
#include <set>
#include <queue>
#include <utility>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <mysql_error.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

// for HOST, USER, PASSWORD
#include "private_config.hpp"

class MySQLController {
private:
  std::string host;
  std::string user;
  std::string password;
  sql::mysql::MySQL_Driver* driver;
  std::shared_ptr<sql::Connection> con;
  std::shared_ptr<sql::Statement> stmt;
public:
  MySQLController(std::string _host, std::string _user, std::string _password) : host(_host), user(_user), password(_password) {
  }
  void init() {
    driver = sql::mysql::get_mysql_driver_instance();
    con = std::shared_ptr<sql::Connection> (driver->connect(host, user, password));
    stmt = std::shared_ptr<sql::Statement> (con->createStatement());
    stmt->execute("USE jawikipedia");
  }
  int page_title_to_page_id(std::string page_title) {
    std::shared_ptr<sql::ResultSet> res;
    res = std::shared_ptr<sql::ResultSet> (stmt->executeQuery("select page_id from page where page_namespace=0 and page_title='" + page_title + "'")); 
    int page_id = -1;
    if (res->next()) {
      page_id = res->getInt("page_id");
    }
    return page_id;
  }
  std::string page_id_to_page_title(int page_id) { 
    std::shared_ptr<sql::ResultSet> res;
    std::string page_title;
    res = std::shared_ptr<sql::ResultSet> (stmt->executeQuery("select page_title from page where page_namespace=0 and page_id=" + std::to_string(page_id))); 
    if (res->next()) {
      page_title = res->getString("page_title");
    }
    return page_title;
  }
  void search(int page_id, std::vector<int> &to) { 
    std::shared_ptr<sql::ResultSet> res;
    // リンクの個数を数える。リンク先のページが存在していないもの(wikipedia上の赤文字のリンク)はカウントしない
    res = std::shared_ptr<sql::ResultSet> (stmt->executeQuery("select page_id from page where page_namespace=0 and page_title in (select lt_title from linktarget where lt_namespace=0 and lt_id in (select pl_target_id from pagelinks where pl_from=" + std::to_string(page_id) + "))"));
    while (res->next()) {
      to.push_back(res->getInt("page_id"));
    }
  }
};

MySQLController mysql(HOST, USER, PASSWORD);
std::map<int, std::pair<std::vector<int>, int>> graph;
int MAX_DEPTH = 3;
int inf = 1e9;

void bfs(std::string start, std::string goal) {
  int start_page_id = mysql.page_title_to_page_id(start);
  int goal_page_id = mysql.page_title_to_page_id(goal);
  // first=page_id, second=cost
  std::queue<std::pair<int,int>> q;
  q.push(std::pair<int,int>(start_page_id, 1));
  bool ok = false;
  int most_low_cost = inf;
  long long cnt = 0;
  while(q.empty() == false) {
    cnt++;
    std::cout << cnt << std::endl;
    auto [page_id, cost] = q.front();
    q.pop();
    if (cost > most_low_cost) continue;
    if (graph.count(page_id)) continue;
    mysql.search(page_id, graph[page_id].first);
    graph[page_id].second = cost;
    for(auto _ : graph[page_id].first) {
      if(_ == goal_page_id) {
        std::cout << "goal" << ',';
        std::cout << "cost:" << cost << mysql.page_id_to_page_title(page_id) << std::endl;
        ok = true;
        if (most_low_cost == inf)
          most_low_cost = cost;
        continue;
      }
      if(graph.count(_))
        continue;
      if(cost + 1 <= MAX_DEPTH && cost + 1 <= most_low_cost) 
        q.push(std::pair<int,int>(_, cost + 1));
    }
  }
  std::cout << most_low_cost << std::endl;
}

int main()
{
  std::string target = "織田信長";
  std::string goal = "東京都立産業技術高等専門学校";
  mysql.init();
  bfs(target, goal);
  return 0;
}
