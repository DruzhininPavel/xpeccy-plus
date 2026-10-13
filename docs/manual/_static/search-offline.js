// Search shows the text around a hit by fetch()ing the found page, and a page opened from
// disk may not fetch another. The pages come from xp-pages.js (conf.py writes it) instead;
// anything not in there still goes to the real fetch.
(() => {
  const realFetch = window.fetch;
  window.fetch = (url, ...rest) => {
    const root = document.documentElement.dataset.content_root || "";
    const name = String(url).startsWith(root) ? String(url).slice(root.length).replace(/\.html$/, "") : "";
    const page = window.XP_PAGES && window.XP_PAGES[name];
    if (page !== undefined)
      return Promise.resolve(new Response(page, { headers: { "Content-Type": "text/html" } }));
    return realFetch.call(window, url, ...rest);
  };
})();
