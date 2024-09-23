/**
 * @file log.hpp
 * @author Yudai Yamaguchi (yudai.yy0804@gmail.com)
 * @brief 例外の定義
 * @version 0.1
 * @date 2024-09-23
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <iostream>
#include <stdexcept>

class _Exception : public std::exception {
private:
  std::string _msg;

public:
  _Exception(std::string msg) : _msg(msg) {}
  ~_Exception() {};

  const char* what(void) const noexcept override { return _msg.c_str(); }
};

/**
 * @brief 例外の箇所のファイル名と行番号を知らせるマクロ
 *
 */
#define EXCEPTION(msg)                                      \
  _Exception("\033[1m" + std::string(__FILE_NAME__) + ":" + \
             std::to_string(__LINE__) + ": \033[31m[ERROR]\033[0m " + msg)

#define LOG_ERROR std::cout << "\033[1;31m[ERROR]\033[0m "
#define LOG_WARN std::cout << "\033[1;33m[ERROR]\033[0m "
#define LOG_OK std::cout << "[\033[1;32mOK\033[0m] "
#define LOG_INFO std::cout << "[INFO] "
#define LOG_DEBUG std::cout << "[DEBUG] "