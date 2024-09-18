# wikipedia-golf
wikipedia-golfをするプログラムです。

# 実行
cmake
```
cmake -S . -B build
cmake --build
```

create_graph_file実行

```
mkdir -p graph_bin
# スレッド数は5で設定
time ./build/create_graph_file 5
```

実行結果(参考)
```
$ time ./build/create_graph_file 5
success

real    32m23.187s
user    8m4.320s
sys     4m26.414s

$ time ./build/create_graph_file 1
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

# MySQL
## setup
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

# 使用ライブラリ
[mysql-connector-cpp](https://github.com/mysql/mysql-connector-cpp)
