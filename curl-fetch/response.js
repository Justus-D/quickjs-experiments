import Headers from "./headers.js"

function normalizeResponseInit(options = {}) {
  if (options === null || typeof options !== "object") {
    throw new TypeError("ResponseInit must be an object")
  }

  const init = {}

  if ("status" in options) {
    init.status = Number(options.status)
  }

  if ("statusText" in options) {
    init.statusText = String(options.statusText)
  }

  if ("headers" in options) {
    init.headers = options.headers instanceof Headers ? options.headers : new Headers(options.headers)
  }

  return init
}

class Response {
  constructor(body, options = {}) {
    this.body = body
    const init = normalizeResponseInit(options)
    this.status = init.status
    this.statusText = init.statusText
    this.headers = init.headers
  }

  text() {
    return new Promise((resolve) => {
      resolve(String(this.body))
    })
  }

  async json() {
    return JSON.parse((await this.text()))
  }
}

export default Response
