#!/usr/bin/env -S qjs --std
import cfetch from "./curl-fetch.js"

const result1 = await cfetch("https://ip.0x2d.de/nonewline")
const body1 = await result1.text()
console.log("[test.js] result1.text():", body1)

const result2 = await cfetch("https://0x2d.de/echo-headers-json.php", {
  headers: [
    [ "X-Test-Header-A", "Value A" ],
    [ "X-Test-Header-B", "Value B1" ],
    [ "X-Test-Header-B", "Value B2" ]
  ]
})
const body2 = await result2.json()
console.log("[test.js] result2.json():", body2)

const result3 = await cfetch("https://0x2d.de/echo-headers-json.php", {
  method: "POST"
})
const body3 = await result3.json()
console.log("[test.js] result3.json():", body3)

std.exit(0)
