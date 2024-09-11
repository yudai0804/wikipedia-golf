const { JSDOM } = require('jsdom');
const fs = require('fs');
const jsonfile = require('jsonfile');
const readline = require('readline');

let filePath = 'wiki_00_4'

var unescapeHtml = function (str) {
    if (typeof str !== 'string') return str;

    var patterns = {
        '&lt;': '<',
        '&gt;': '>',
        '&amp;': '&',
        '&quot;': '"',
        '&#x27;': '\'',
        '&#x60;': '`'
    };

    return str.replace(/&(lt|gt|amp|quot|#x27|#x60);/g, function (match) {
        return patterns[match];
    });
};

function isValidURL(str) {
    try {
        new URL(str);
        return true;
    } catch (e) {
        return false;
    }
}

// ファイルストリームを作成
const fileStream = fs.createReadStream(filePath);

// readlineインターフェースを作成
const rl = readline.createInterface({
    input: fileStream,
    crlfDelay: Infinity // 行の終わりの遅延を無視
});

let graph = []

// 各行を処理
rl.on('line', (line) => {
    const json = JSON.parse(line)
    const { window } = new JSDOM(unescapeHtml(json["text"]));
    const { document } = window;
    let a_tag = document.getElementsByTagName('a');
    graph[json["title"]] = ""
    for (let i = 0; i < a_tag.length; i++) {
        let href = decodeURIComponent(a_tag[i].href);
        if (isValidURL(href) == true) continue;
        graph[json["title"]] += href;
        if (i != a_tag.length - 1)
            graph[json["title"]] += ','
    }
});

// エラー処理
rl.on('error', (error) => {
    console.error(`Error reading file: ${error.message}`);
});

// 読み込み完了後の処理
rl.on('close', () => {
    console.log('File reading completed.');
    console.log(graph)
});