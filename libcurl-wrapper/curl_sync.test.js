#!/usr/bin/env -S qjs --std
import { CurlSync } from "./curl_sync.so"

const curl = new CurlSync()
curl.set_url("https://ip.0x2d.de/nonewline")
curl.perform()
// const buf = curl.get_response_buffer()
// const str = (new TextDecoder("utf-8")).decode(buf)
const text = curl.get_response_text()
console.log(text)
