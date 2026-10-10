# QuickJS Experiments

A small collection of hand-crafted experiments using the
[QuickJS JavaScript Engine](https://bellard.org/quickjs/)
by Fabrice Bellard and Charlie Gordon.

## curl-fetch

This is my attempt at creating an implementation of the
[Fetch API](https://developer.mozilla.org/en-US/docs/Web/API/Fetch_API)
for QuickJS using curl.

Example usage:

```js
// curl-fetch-example.js
import cfetch from "./curl-fetch/curl-fetch.js"

const response = await cfetch("https://ip.0x2d.de/nonewline")
const data = await response.text()
console.log(data)
```

Run with:

```bash
qjs --std curl-fetch-example.js
```

This should print your IP address to the console.

---

&copy; 2026 Justus Dietrich
