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

## libcurl-wrapper

This is a shared library that can be imported in QuickJS. It is a simple wrapper
for the [libcurl](https://curl.se/libcurl/) library with very basic functionality.
It tries to map libcurl to JavaScript with little abstraction.

You have to build the shared library first. This assumes you have the headers for
QuickJS in `$HOME/git/quickjs`. If not, specify the location in the `make` arguments.
You also need `libcurl-devel` installed on your system.

```bash
make QUICKJS_DIR=/path/to/quickjs
```

Example usage:

```js
// libcurl-wrapper-example.js
import { CurlSync } from "./libcurl-wrapper/curl_sync.so"

const curl = new CurlSync()
curl.set_url("https://ip.0x2d.de/nonewline")
curl.perform()
const data = curl.get_response_text()
console.log(data)
```

Run with:

```bash
qjs --std libcurl-wrapper-example.js
```

This should print your IP address to the console, as in the last example.

---

&copy; 2026 Justus Dietrich
