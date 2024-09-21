# wikipedia-golf
wikipedia golfをするプログラムです。


https://github.com/user-attachments/assets/f1785563-a47b-4c56-a71e-191b35d4076c

## example

茶柱->アザラシの経路です。経路の計算は0.5[s]で実行可能です。
```
$ ./build/wikipedia-golf --start "茶柱" --goal "アザラシ" --thread_number 6 --max_ans_number 50
total_file: 2318461
load success
Time: 0h0m19s
total answer:32
茶柱->日本->小田原市->アザラシ
茶柱->日本->地球温暖化->アザラシ
茶柱->日本->大田区->アザラシ
茶柱->日本->昆虫->アザラシ
茶柱->日本->フグ->アザラシ
茶柱->日本->日本料理->アザラシ
茶柱->日本->魚介類->アザラシ
茶柱->日本->縄文時代->アザラシ
茶柱->日本->肉食->アザラシ
茶柱->日本->食肉->アザラシ
茶柱->日本->北方地域->アザラシ
茶柱->日本->アユ->アザラシ
茶柱->日本->キジ->アザラシ
茶柱->日本->コオロギ->アザラシ
茶柱->日本->ツンドラ->アザラシ
茶柱->日本->ニヴフ->アザラシ
茶柱->ビワ->成分本質_(原材料)_では医薬品でないもの->アザラシ
茶柱->茶->毛皮->アザラシ
茶柱->茶->エビ->アザラシ
茶柱->茶->ゾウ->アザラシ
茶柱->茶漬け->ウナギ->アザラシ
茶柱->茶漬け->マグロ->アザラシ
茶柱->ロシアの茶文化->燻製->アザラシ
茶柱->ロシアの茶文化->ラクダ->アザラシ
茶柱->ISO_3103->土器->アザラシ
茶柱->花瓶->バーベキュー->アザラシ
茶柱->タピオカティー->サバヒー->アザラシ
茶柱->チャノキ->積丹半島->アザラシ
茶柱->ティーハウス->メルボルン->アザラシ
茶柱->俗信->カメ->アザラシ
茶柱->俗信->シーサーペント->アザラシ
茶柱->俗信->ステラーカイギュウ->アザラシ
Time: 0.504079[s]
```

# Setup

requirements
- Linux(debian12)
- gcc(for C++20)
- cmake
- mySQL

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
cmake --build
```

## create_graph_file実行
WikipediaのSQLを扱いやすく加工したバイナリファイルに変換します。

```
mkdir -p graph_bin
# スレッド数は6で設定
time ./build/create_graph_file --thread_number 6
```

実行結果(CPU: core i5-1235U, RAM: 16GB, NVMe SSD)
```
$ time ./build/create_graph_file --thread_number 6
success

real    32m23.187s
user    8m4.320s
sys     4m26.414s

$ time ./build/create_graph_file --thread_number 1
success

real    109m41.642s
user    11m52.537s
sys     8m11.272s
```

生成されたbinの情報
```
$ find graph_bin/ -type f | wc -l
2318460
$ du -sh graph_bin/
9.0G    graph_bin/
```

## wikipedia-golfを実行
Wikipedia golfを対話形式で行います。
```
./build/wikipedia-golf --thread_number 6 --max_ans_number 50
```

# Help
```
./wikipedia-golf --help
```
```
Usage: ./wikipedia-golf
If there are spaces included, please enclose the text in single quotes or double quotes.

option arguments:
--input [PATH]          Input directory path.(defualt: graph_bin)
--thread_number [NUM]   Thread number for loading.(default: 1)
                        Please note that increasing the number of threads will not speed up the search.
--max_ans_number [NUM]  Max answer number.(default: 5)
--allow_similar_path    Allow similar_path.(default: false)
                        Setting it to true will make it very slow.
```
```
./create_graph_file --help
```
```
Usage: ./create_graph_file
option arguments:
--output [PATH]         Output directory path.(defualt: graph_bin)
--thread_number [NUM]   Thread number for exporting.(default: 1)
```

# 使用ライブラリ
[mysql-connector-cpp](https://github.com/mysql/mysql-connector-cpp)

# 似たようなサービス
https://www.sixdegreesofwikipedia.com/
