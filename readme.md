# 実行
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

## MySQLをC++で実行
MySQLをC++で実行するためにはMySQL Connector/C++というのが必要
今回は
```
sudo apt-get install -y libmysqlcppconn-dev
```
でインストールした。

ヘッダファイルがどこにあるか確認
```
dpkg -L libmysqlcppconn-dev | grep -e mysql_driver -e mysql_connection
```
debian12での実行結果
```
/usr/include/mysql-cppconn/jdbc/mysql_connection.h
/usr/include/mysql-cppconn/jdbc/mysql_driver.h
```

サンプルコード(C++)
パスワードは自分のパスワードを入力してください。
```
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <iostream>

#define HOST "tcp://127.0.0.1:3306"
// ユーザー名
#define USER "root"
// パスワード
#define PASSWORD "password"

int main()
{
    sql::mysql::MySQL_Driver *driver;
    sql::Connection *con;

    driver = sql::mysql::get_mysql_driver_instance();
    con = driver->connect(HOST, USER, PASSWORD);

    std::cout << "Connected successfully!" << std::endl;

    delete con;
    return 0;
}
```

コンパイル
```
g++ -o test test.cpp -I/usr/include/mysql-cppconn/jdbc/ -lmysqlcppconn
```
実行
```
./test
```
実行結果
```
Connected successfully!
```

参考
https://ja.wikipedia.org/wiki/Wikipedia:%E3%83%87%E3%83%BC%E3%82%BF%E3%83%99%E3%83%BC%E3%82%B9%E3%83%80%E3%82%A6%E3%83%B3%E3%83%AD%E3%83%BC%E3%83%89


メモ
```
set @target='日本';
select lt_id,convert(lt_title using utf8mb4) from linktarget where lt_namespace=0 and lt_id in (select pl_target_id from pagelinks where pl_from in (select page_id from page where page_namespace=0 and page_title=@target));
```

# old

# WikiExtractor
## requirements
- Python3.7
  - 最新のPythonでは動かなかったので3.7を使用

wikipediaの記事データをダウンロード
```
curl -o jawiki-latest-pages-articles.xml.bz2 https://dumps.wikimedia.org/jawiki/latest/jawiki-latest-pages-articles.xml.bz2
```
wikiextractorをclone
```
git clone https://github.com/attardi/wikiextractor.git
```
wikiextractorをinstall
```
python3 setup.py install
```
実行
```
python3 -m wikiextractor.WikiExtractor jawiki-latest-pages-articles.xml.bz2 --links --no-templates --json
```


# setup
```
npm install
```

# run
```
node index.js
```
