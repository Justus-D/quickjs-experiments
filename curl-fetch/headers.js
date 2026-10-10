class Headers {
  #map = new Map()

  constructor(init) {
    if (!init) return
    if (init instanceof Headers) {
      init.#map.forEach((value, name) => this.#map.set(name, value))
      return
    }
    // example input: [
    //   ["Host", "example.com"],
    //   ["Content-Type", "text/html"],
    //   ["Accept-Encoding", "gzip"],
    //   ["Accept-Encoding", "br"]
    // ]
    // result: {
    //   "host": "example.com",
    //   "content-type": "text/html",
    //   "accept-encoding", "gzip, br"
    // }
    if (Array.isArray(init)) {
      for (const [name, value] of init) this.append(name, value)
      return
    }
  }

  append(name, value) {
    const k = String(name).toLowerCase().trim()
    const v = String(value).trim()
    const current = this.#map.get(k)
    this.#map.set(k, current ? `${current}, ${v}`: v)
  }

  delete(name) {
    this.#map.delete(String(name).toLowerCase().trim())
  }

  entries() {
    return this.#map.entries()
  }

  [Symbol.iterator]() {
    return this.entries()
  }

  forEach(callbackFn, thisArg) {
    return this.#map.forEach((value, key) => callbackFn.call(thisArg, value, key, this))
  }

  get(name) {
    const result = this.#map.get()
    if (typeof result !== "undefined") {
      return result
    }
    return null
  }

  getSetCookie() {
    throw new Error("Method not implemented")
  }

  has(name) {
    return this.#map.has(String(name).toLowerCase().trim())
  }

  keys() {
    return this.#map.keys()
  }

  set(name, value) {
    this.#map.set(String(name).toLowerCase().trim(), String(value).trim())
  }

  values() {
    this.#map.values()
  }
}

export default Headers
