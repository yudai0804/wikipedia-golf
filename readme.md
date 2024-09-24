# wikipedia-golf
wikipedia golfをするプログラムです。

https://github.com/user-attachments/assets/52ac00e7-4303-4c19-b006-312776250287

## 特徴
- 最短経路を短時間で導出可能
- WikipediaのSQLをそのまま使用せず、SQLの内容をメモリに取り込んでいるため、非常に高速
- SQLの内容をメモリに取り込む際には、高速化のためにマルチスレッドを使用
- C++20

## Wikipedia golfって何？
Wikipediaの単語から単語までを何手で移動できるかを競う遊びです。  
Wikipedia golfは全ての単語において、6手以内で可能と言われています。

## 似たようなサービス
https://www.sixdegreesofwikipedia.com/

# Setup

requirements
- Linux(Debian12)
- gcc(C++20)
- cmake
- mySQL
- [mysql-connector-cpp](https://github.com/mysql/mysql-connector-cpp)

## mySQL

```
wget https://dev.mysql.com/get/mysql-apt-config_0.8.32-1_all.deb
sudo dpkg -i mysql-apt-config_0.8.32-1_all.deb
sudo apt-get update
sudo apt-get install -y mysql-server
# for cpp
sudo apt-get install -y libmysqlcppconn-dev
```
debian12にMySQL入れようとしたら依存関係でうまく行かなかったので次のコマンドを入力した。

MySQLとMariaDBが干渉してたっぽい？
```
dpkg -l | grep -i 'maria\|mysql' | awk '{print $2}' | xargs sudo apt-get purge -y
```

jawikipediaという名前のデータベースを作成
```
mysql -u root -p jawikipedia
```
インポート。何時間もかかるので注意。
```
mysql -u root jawikipedia < jawiki-latest-page.sql
mysql -u root jawikipedia < jawiki-latest-pagelinks.sql
mysql -u root jawikipedia < jawiki-latest-linktarget.sql
```
wikipediaのデータベースの詳細
https://ja.wikipedia.org/wiki/Wikipedia:%E3%83%87%E3%83%BC%E3%82%BF%E3%83%99%E3%83%BC%E3%82%B9%E3%83%80%E3%82%A6%E3%83%B3%E3%83%AD%E3%83%BC%E3%83%89

# C++
## Build
```
cmake -S . -B build
cmake --build build
```

## create_graph_file実行
WikipediaのSQLを扱いやすく加工したバイナリファイルに変換します。

```
mkdir -p graph_bin
# スレッド数は6で設定
time ./build/create_graph_file root tcp://127.0.0.1:3306 --thread_number 6
```

実行結果(CPU: core i5-1235U, RAM: 16GB, NVMe SSD)
```
$ time ./build/create_graph_file root tcp://127.0.0.1:3306 --thread_number 6
success

real    32m23.187s
user    8m4.320s
sys     4m26.414s

$ time ./build/create_graph_file root tcp://127.0.0.1:3306 --thread_number 1
success

real    109m41.642s
user    11m52.537s
sys     8m11.272s
```

生成されたbinの情報
```
$ find graph_bin/ -type f | wc -l
2318462
$ du -sh graph_bin/
9.0G    graph_bin/
```

## wikipedia-golfを実行
Wikipedia golfを対話形式で行います。  
-use_fast_queueのオプションをつけるとメモリの使用量が増える代わりに、動作が速くなります。  
メモリはだいたい5GBくらい使います。オプションをつけない場合、2~3GB程度です。
```
./build/wikipedia-golf --thread_number 6 --max_ans_number 50 -use_fast_queue
```

# Help
```
./wikipedia-golf --help
```
```
Usage: ./wikipedia-golf

option arguments:
-h --help               Show help
-v --version            Show version
--input [PATH]          Input directory path.(defualt: graph_bin)
--thread_number [NUM]   Thread number for loading.(default: 1)
                        Please note that increasing the number of threads will not speed up the search.
--max_ans_number [NUM]  Max answer number.(default: 5)
--allow_similar_path    Allow similar_path.(default: false)
                        Setting it to true will make it very slow.
--use_fast_queue        Using fast queue.
                        Fast queue is using 4GB RAM.
```
```
./create_graph_file --help
```
```
Usage: ./create_graph_file [USER] [HOST]
option arguments:
-h --help               Show help
-v --version            Show version
--output [PATH]         Output directory path.(defualt: graph_bin)
--thread_number [NUM]   Thread number for exporting.(default: 1)
```
