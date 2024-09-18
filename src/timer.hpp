/**
 * @file timer.hpp
 * @author Yudai Yamaguchi (yudai,yy0804@gmail.com)
 * @brief 経過時間を計測するクラス
 * @version 0.1
 * @date 2024-09-19
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <chrono>
#include <iostream>

class Timer {
private:
  std::chrono::high_resolution_clock::time_point _start, _end;

public:
  void start() { _start = std::chrono::high_resolution_clock::now(); }

  void print() {
    _end = std::chrono::high_resolution_clock::now();
    // 経過時間を計算
    auto seconds =
        std::chrono::duration_cast<std::chrono::seconds>(_end - _start);
    auto hours = std::chrono::duration_cast<std::chrono::hours>(seconds);
    seconds -= hours;  // 残りの秒数を計算
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(seconds);
    seconds -= minutes;  // 残りの秒数を計算
    std::cout << hours.count() << "h" << minutes.count() << "m"
              << seconds.count() << std::endl;
  }

  /**
   * @brief 返り値は秒
   *
   * @return double
   */
  double get() {
    _end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = _end - _start;
    return elapsed.count();
  }
};