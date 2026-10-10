import * as std from "std"
import * as os from "os"
import Request from "./request.js"
import Response from "./response.js"

const DEBUG = !!std.getenv("DEBUG")

const cfetch = (resource, options = {}) => new Promise((resolve, reject) => {
  if (DEBUG) console.log("[curl-fetch.js] initializing cfetch");
  const request = new Request(resource, options)
  let responseBody = null

  const worker = new os.Worker("./curl-worker.js")
  worker.onmessage = (msg) => {
    const event = msg.data
    if (DEBUG) console.log("[curl-fetch.js] event:", JSON.stringify(event));

    switch (event.type) {
      case "done":
        worker.onmessage = null
        const response = new Response(responseBody)
        resolve(response)
        break;
      case "response":
        responseBody = event.data
        break;
    }
  }

  worker.postMessage({ type: "config", data: {
    method: request.method,
    url: request.url,
    headers: [...request.headers]
  }})
  worker.postMessage({ type: "send" })
})

export default cfetch
