const { JSDOM } = require('jsdom');

class Queue {
  constructor() {
    this.items = [];
  }

  push(element) {
    this.items.push(element);
  }

  pop() {
    return this.items.shift();
  }

  top() {
    return this.items[0];
  }

  empty() {
    return this.items.length === 0;
  }

  size() {
    return this.items.length;
  }
}

let start_time = new Date().getTime();
let debug_cnt = 0;

let graph = [];
let cost_map = [];
let promise = [];
let loop_depth = 1;
let promise_cnt = 0;
let max_promise = 100;
let queue = new Queue();

async function search(from, cost) {
  debug_cnt++;
  console.log(debug_cnt, new Date().getTime() - start_time)
  let wiki_path = 'https://ja.wikipedia.org/wiki/';

  let dom = await JSDOM.fromURL(wiki_path + encodeURI(from));
  let document = dom.window.document;

  let ignore = [
    '特別:',
    'Help:',
    'Portal:',
    'プロジェクト:',
    'Template:',
    'Category:',
    'Wikipedia:',
    'ノート:',
    'ファイル:',
    'メインページ'];
  let to = new Set();

  let a_tag = document.getElementsByTagName('a');

  for (let i = 0; i < a_tag.length; i++) {
    href = a_tag[i].href;
    if (href.includes(wiki_path) == false)
      continue;

    let ok = true;

    for (let j = 0; j < ignore.length; j++) {
      let _ignore = encodeURI(wiki_path + ignore[j]);
      if (href.includes(_ignore) == true)
        ok = false;
    }
    if (href.includes('action=edit') == true)
      ok = false;

    if (ok) {
      let str = href.split('#')[0].replace(wiki_path, '');
      if (str != from)
        to.add(decodeURIComponent(str))
    }
  }

  if (from in graph == false)
    graph[from] = [];

  for (let next of to) {
    graph[from].push(next);
    if (cost + 1 >= loop_depth)
      continue;

    if (cost_map[next] == undefined || cost + 1 < cost_map[next])
      queue.push({ next: next, from: from, cost: cost + 1 })
  }
  promise_cnt--;
}

// 幅優先探索でグラフを作成する
async function make_graph(start, cost) {
  if (cost >= loop_depth) return;
  queue.push({ next: start, from: '', cost: cost });

  let activePromises = new Set();

  while (true) {
    if (!queue.empty()) {
      let { next, from, cost } = queue.pop();
      if (cost_map[next] != undefined && cost_map[next] <= cost) continue;

      let promise = search(next, cost);
      activePromises.add(promise);
      promise.finally(() => activePromises.delete(promise));
    } else if (activePromises.size == 0) {
      break;
    }

    // 少し待機して、キューが空でない場合の無限ループ防止
    await new Promise(resolve => setTimeout(resolve, 20));
  }
}

(async () => {
  await make_graph('東京都立産業技術高等専門学校', 0);
  console.log(graph)
  // TODO: 次は結果をちゃんと表示できるようにする
})();