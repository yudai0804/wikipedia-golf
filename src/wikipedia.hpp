/**
 * @file wikipedia.hpp
 * @author Yudai Yamaguchi (yudai.yy0804@gmail.com)
 * @brief wikipediaのSQLを制御するクラス
 * @version 0.1
 * @date 2024-09-17
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <mysql_connection.h>
#include <mysql_driver.h>
#include <mysql_error.h>

#include <chrono>
#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <utility>
#include <vector>

class Wikipedia {
private:
  std::string host;
  std::string user;
  std::string password;
  sql::mysql::MySQL_Driver *driver;
  std::shared_ptr<sql::Connection> con;
  std::shared_ptr<sql::Statement> stmt;
  std::map<std::string, int> mp;

public:
  Wikipedia(std::string _host, std::string _user, std::string _password)
      : host(_host), user(_user), password(_password) {}
  void init() {
    std::shared_ptr<sql::ResultSet> res;
    driver = sql::mysql::get_mysql_driver_instance();
    con =
        std::shared_ptr<sql::Connection>(driver->connect(host, user, password));
    stmt = std::shared_ptr<sql::Statement>(con->createStatement());
    stmt->execute("USE jawikipedia");

    res = std::shared_ptr<sql::ResultSet>(stmt->executeQuery(
        "SELECT page_title,page_id FROM page WHERE page_namespace=0"));
    while (res->next()) {
      mp[res->getString("page_title")] = res->getInt("page_id");
    }
  }
  int page_title_to_page_id(std::string page_title) {
    std::shared_ptr<sql::ResultSet> res;
    res = std::shared_ptr<sql::ResultSet>(stmt->executeQuery(
        "SELECT page_id FROM page WHERE page_namespace=0 AND page_title='" +
        page_title + "'"));
    int page_id = -1;
    if (res->next()) {
      page_id = res->getInt("page_id");
    }
    return page_id;
  }
  std::string page_id_to_page_title(int page_id) {
    std::shared_ptr<sql::ResultSet> res;
    std::string page_title;
    res = std::shared_ptr<sql::ResultSet>(stmt->executeQuery(
        "SELECT page_title FROM page WHERE page_namespace=0 AND page_id=" +
        std::to_string(page_id)));
    if (res->next()) {
      page_title = res->getString("page_title");
    }
    return page_title;
  }
  void search(int page_id, std::vector<int> &to) {
    std::shared_ptr<sql::ResultSet> res;
    // リンクの個数を数える。リンク先のページが存在していないもの(wikipedia上の赤文字のリンク)はカウントしない
    res = std::shared_ptr<sql::ResultSet>(stmt->executeQuery(
        "SELECT lt_title FROM linktarget WHERE lt_namespace=0 AND lt_id IN (SELECT pl_target_id FROM pagelinks WHERE pl_from=" +
        std::to_string(page_id) + ")"));
    while (res->next()) {
      auto page_id = mp[res->getString("lt_title")];
      if (page_id != 0) to.push_back(page_id);
    }
  }

  std::vector<int> get_all_page_id(void) {
    std::vector<int> res;
    for (auto itr = mp.begin(); itr != mp.end(); itr++) {
      res.push_back(itr->second);
    }
    return res;
  }
};
