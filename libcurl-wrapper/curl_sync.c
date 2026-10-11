#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <quickjs.h>

#ifndef JS_SHARED_LIBRARY
#define JS_SHARED_LIBRARY
#endif

#ifndef countof
#define countof(x) (sizeof(x) / sizeof((x)[0]))
#endif

typedef struct js_curl_sync_data {
    CURL *curl;
    char *res_buf;
    size_t res_buf_size;
} js_curl_sync_data_t;

static JSClassID js_curl_sync_class_id;

static void js_curl_sync_finalizer(JSRuntime *rt, JSValue val) {
    js_curl_sync_data_t *c = JS_GetOpaque(val, js_curl_sync_class_id);
    if (c) {
        if (c->curl) curl_easy_cleanup(c->curl);
        free(c->res_buf);
        free(c);
    }
}

static JSClassDef js_curl_sync_class = {
    .class_name = "CurlSync",
    .finalizer = js_curl_sync_finalizer,
};

size_t js_curl_sync_write_callback(char *buffer, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    js_curl_sync_data_t *c = userp;

    char *ptr = realloc(c->res_buf, c->res_buf_size + realsize);
    if (!ptr) {
        return 0;
    }

    c->res_buf = ptr;
    char *pos = c->res_buf + c->res_buf_size;
    memcpy(pos, buffer, realsize);
    c->res_buf_size += realsize;

    return realsize;
}

/**
 * JS: CurlSync.constructor(): CurlSync
 * (see: quickjs-libc.c:3701)
 */
static JSValue js_curl_sync_constructor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) {
    js_curl_sync_data_t *c = malloc(sizeof(js_curl_sync_data_t));
    if (!c) {
        JS_ThrowOutOfMemory(ctx);
        JS_FreeValue(ctx, new_target);
        return JS_EXCEPTION;
    }
    memset(c, 0, sizeof(*c));

    c->curl = curl_easy_init();
    if (!c->curl) {
        free(c);
        JS_FreeValue(ctx, new_target); // is this needed?
        JS_ThrowInternalError(ctx, "CurlSync: curl_easy_init failed");
        return JS_EXCEPTION;
    }

    // CURL setup
    curl_easy_setopt(c->curl, CURLOPT_WRITEFUNCTION, js_curl_sync_write_callback);
    curl_easy_setopt(c->curl, CURLOPT_WRITEDATA, (void *)c);

    // JS: new CurlSync()
    JSValue js_obj = JS_NewObjectClass(ctx, js_curl_sync_class_id);
    if (JS_IsException(js_obj)) {
        curl_easy_cleanup(c->curl);
        free(c);
        JS_FreeValue(ctx, new_target);
        return JS_EXCEPTION;
    }

    // JS object => C object
    JS_SetOpaque(js_obj, c);
    return js_obj;
}

/**
 * JS: CurlSync.prototype.set_url(url: string): void
 */
static JSValue js_curl_sync_set_url(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    js_curl_sync_data_t *c = JS_GetOpaque2(ctx, this_val, js_curl_sync_class_id);
    if (!c) {
        return JS_EXCEPTION;
    }

    const char *url = JS_ToCString(ctx, argv[0]);
    if (!url) {
        return JS_EXCEPTION;
    }
    curl_easy_setopt(c->curl, CURLOPT_URL, url);
    JS_FreeCString(ctx, url);

    return JS_UNDEFINED;
}

/**
 * JS: CurlSync.prototype.perform(): void
 */
static JSValue js_curl_sync_perform(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    js_curl_sync_data_t *c = JS_GetOpaque2(ctx, this_val, js_curl_sync_class_id);
    if (!c) {
        return JS_EXCEPTION;
    }

    free(c->res_buf);
    c->res_buf = NULL;
    c->res_buf_size = 0;

    CURLcode result = curl_easy_perform(c->curl);

    return JS_UNDEFINED;
}

/**
 * JS: CurlSync.prototype.get_response_buffer(): ArrayBuffer
 */
static JSValue js_curl_sync_get_response_buffer(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    js_curl_sync_data_t *c = JS_GetOpaque2(ctx, this_val, js_curl_sync_class_id);
    if (!c) {
        return JS_EXCEPTION;
    }

    return JS_NewArrayBufferCopy(ctx, (uint8_t *)c->res_buf, c->res_buf_size);
}

/**
 * JS: CurlSync.prototype.get_response_text(): string
 */
static JSValue js_curl_sync_get_response_text(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    js_curl_sync_data_t *c = JS_GetOpaque2(ctx, this_val, js_curl_sync_class_id);
    if (!c) {
        return JS_EXCEPTION;
    }

    return JS_NewStringLen(ctx, c->res_buf, c->res_buf_size);
}

static const JSCFunctionListEntry js_curl_sync_proto_funcs[] = {
    JS_CFUNC_DEF("set_url", 1, js_curl_sync_set_url ),
    JS_CFUNC_DEF("perform", 0, js_curl_sync_perform ),
    JS_CFUNC_DEF("get_response_buffer", 0, js_curl_sync_get_response_buffer ),
    JS_CFUNC_DEF("get_response_text", 0, js_curl_sync_get_response_text ),
};

/**
 * Module init
 * (see: quickjs-libc.c:4012)
 */
static int js_curl_sync_init(JSContext *ctx, JSModuleDef *m) {
    JSRuntime *rt = JS_GetRuntime(ctx);
    JSValue proto, obj;

    // JS: class CurlSync {}
    JS_NewClassID(&js_curl_sync_class_id);
    JS_NewClass(rt, js_curl_sync_class_id, &js_curl_sync_class);

    // JS: proto = {}
    proto = JS_NewObject(ctx);
    // JS: proto = { ...proto_funcs }
    JS_SetPropertyFunctionList(ctx, proto, js_curl_sync_proto_funcs, countof(js_curl_sync_proto_funcs));

    // JS: CurlSync.constructor
    obj = JS_NewCFunction2(ctx, js_curl_sync_constructor, "CurlSync", 0, JS_CFUNC_constructor, 0);
    // JS: proto.constructor = obj
    JS_SetConstructor(ctx, obj, proto);

    // JS: CurlSync.prototype = proto
    JS_SetClassProto(ctx, js_curl_sync_class_id, proto);

    // JS: export default CurlSync
    return JS_SetModuleExport(ctx, m, "CurlSync", obj);
}

#ifdef JS_SHARED_LIBRARY
#define JS_INIT_MODULE js_init_module
#else
#define JS_INIT_MODULE js_init_module_curl_sync
#endif
/**
 * Register the module with QuickJS
 * (see: quickjs-libc.c:4048)
 */
JSModuleDef *JS_INIT_MODULE(JSContext *ctx, const char *module_name) {
    // CURL; only run once per application
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // QuickJS module
    JSModuleDef *m;
    m = JS_NewCModule(ctx, module_name, js_curl_sync_init);
    if (!m) return NULL;
    JS_AddModuleExport(ctx, m, "CurlSync");
    return m;
}
