import * as std from "std"
import * as os from "os"

const DEBUG = !!std.getenv("DEBUG")

function curl_worker_main() {
  const parent = os.Worker.parent
  let config = {}
  let response = null

  function writeResponse() {
    parent.postMessage({ type: "response", data: response })
    parent.postMessage({ type: "done" })
    parent.onmessage = null
  }

  function sendRequest() {
    const execArray = ["curl", "-s", "-X", config.method, config.url]
    for (const [name, value] of config.headers) {
      execArray.push("-H", `${name}: ${value}`)
    }
    const fds = os.pipe()
    const pid = os.exec(execArray, {
      stdout: fds[1],
      block: false
    })
    os.close(fds[1])
    const f = std.fdopen(fds[0], "r")
    const [ret, status] = os.waitpid(pid, 0)
    response = f.readAsString()
    f.close()
    if (DEBUG) console.log("[curl-worker.js] waitpid:", ret, status);
    writeResponse()
  }

  parent.onmessage = (msg) => {
    const event = msg.data
    if (DEBUG) console.log("[curl-worker.js] event:", JSON.stringify(event));

    switch (event.type) {
      case "abort":
        parent.postMessage({ type: "done" })
        parent.onmessage = null
        break
      case "ping":
        parent.postMessage({ type: "pong" })
        break
      case "config":
        config = event.data
        break
      case "send":
        sendRequest()
        break
    }
  }

}

curl_worker_main()
