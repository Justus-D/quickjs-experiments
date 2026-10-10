import Headers from "./headers.js"

function normalizeRequestInit(options = {}) {
  if (options === null || typeof options !== "object") {
    throw new TypeError("RequestInit must be an object")
  }

  const init = {}

  if ("method" in options) {
    init.method = String(options.method).toUpperCase()
  }

  if ("headers" in options) {
    init.headers = options.headers instanceof Headers ? options.headers : new Headers(options.headers)
  }

  return init
}

class Request {
  constructor(input, options = {}) {
    const init = normalizeRequestInit(options)
    if (input instanceof Request) {
      this.url = input.url
      this.method = (init.method || input.method).toUpperCase()
      this.headers = input.headers
    } else {
      this.url = String(input)
      this.method = (init.method || "GET").toUpperCase()
      this.headers = new Headers(init.headers)
    }
  }
}

export default Request
