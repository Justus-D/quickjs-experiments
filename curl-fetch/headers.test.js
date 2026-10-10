#!/usr/bin/env -S qjs --std
import Headers from "./headers.js"

const h1 = new Headers()
h1.set("Test1", "  Test1-Value ")
h1.append("Test2", " Test2v1")
h1.append(" Test2 ", "Test2v2 ")

for (const [name, value] of h1) console.log("h1:", name, value);

console.log("h1", JSON.stringify([...h1]))
