#!/usr/bin/env -S qjs --std
import { CurlSync } from "./curl_sync.so"

function printStrArray(arr) {
  if (!Array.isArray(arr)) return
  arr.forEach((str, i) => {
    if (typeof str === "string") {
      console.log(`[${i}] \t"${str.trim()}"`)
    }
  })
}

const curl = new CurlSync()
curl.set_url("https://0x2d.de/echo-headers-json.php")
curl.set_http_header_list(["X-Test-1: test1", "X-Test-2: test2"])
curl.perform()
const text = curl.get_response_text()
console.log("response text:", text)
const response_header_list = curl.get_response_header_list()
console.log("response header list:")
printStrArray(response_header_list)
