const { JSDOM } = require('jsdom');

let start_time = new Date().getTime();
let debug_cnt = 0;

async function search(name) {
  debug_cnt++;
  console.log(debug_cnt, new Date().getTime() - start_time)
  let wiki_path = 'https://ja.wikipedia.org/wiki/';

  let dom = await JSDOM.fromURL(wiki_path + encodeURI(name));
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
      to.add(decodeURIComponent(str))
    }
  }

  return to;
}

let graph = new Map();
let visit = new Set();
let loop_cnt = 1

async function make_graph(current, cost) {
  if (cost >= loop_cnt) return;
  if (visit.has(current)) return;
  visit.add(current);
  let res = await search(current);


  if (current in graph == false)
    graph[current] = {};

  for (let next of res) {
    graph[current][next] = cost + 1;

    // if (next in graph == false)
    // graph[next] = [];
    // graph[next][current] = cost + 1;

    if (visit.has(next) == false && cost + 1 < loop_cnt) {
      await make_graph(next, cost + 1);
    }

  }
}

(async () => {

  await make_graph('東京都立産業技術高等専門学校', 0);
  console.log(graph)
  console.log(new Date().getTime() - start_time)
})();