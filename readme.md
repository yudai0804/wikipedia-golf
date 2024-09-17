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
success

real    29m40.090s
user    7m16.756s
sys     3m38.827s
```

```
g++ -o main.cpp -I/usr/include/mysql-cppconn/jdbc/ -lmysqlcppconn -O3
./a.out "src" "dst"
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
