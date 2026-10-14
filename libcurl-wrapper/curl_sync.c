#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <curl/curl.h>
#include <quickjs.h>

#ifndef JS_SHARED_LIBRARY
#define JS_SHARED_LIBRARY
#endif

#ifndef countof
#define countof(x) (sizeof(x) / sizeof((x)[0]))
#endif

typedef struct generic_list_node generic_list_node_t;
struct generic_list_node {
    generic_list_node_t *next;
    void *ptr;
    size_t len;
};

typedef struct generic_list {
    generic_list_node_t *first;
    generic_list_node_t *last;
    size_t items;
} generic_list_t;

static generic_list_t *generic_list_init(void) {
    return calloc(1, sizeof(generic_list_t));
}

bool generic_list_append(generic_list_t *list, void *ptr, size_t len) {
    if (!list) return false;

    generic_list_node_t *new_node = calloc(1, sizeof(generic_list_node_t));
    if (!new_node) return false;

    new_node->ptr = ptr; // set payload
    new_node->len = len; // set length of payload
    new_node->next = NULL;
    if (!list->first) list->first = new_node; // list empty? set as first node
    if (list->last) list->last->next = new_node;
    list->last = new_node;
    list->items++;

    return true;
}

void generic_list_free_all(generic_list_t *list) {
    if (!list) return;

    generic_list_node_t *node = list->first;
    generic_list_node_t *old_node = NULL;
    while (node) {
        free(node->ptr);
        old_node = node;
        node = old_node->next;
        free(old_node);
    }
    free(list);
}

JSValue generic_list_nstr_to_js_arr(JSContext *ctx, generic_list_t *list) {
    if (!list) return JS_ThrowTypeError(ctx, "invalid list reference");

    JSValue js_arr = JS_NewArray(ctx);
    if (JS_IsException(js_arr)) return JS_EXCEPTION;
    
    generic_list_node_t *node = list->first;
    for (uint32_t i = 0; node; i++, node = node->next) {
        JSValue js_str = JS_NewStringLen(ctx, node->ptr, node->len);
        if (JS_IsException(js_str)) {
            JS_FreeValue(ctx, js_arr);
            return JS_EXCEPTION;
        }
        if (JS_SetPropertyUint32(ctx, js_arr, i, js_str) < 0) {
            JS_FreeValue(ctx, js_arr);
            return JS_EXCEPTION;
        }
    }

    return js_arr;
}

typedef struct js_curl_sync_data {
    CURL *curl;
    char *res_buf;
    size_t res_buf_size;
    struct curl_slist *curl_header_list; // to be freed
    generic_list_t *res_header_list; // to be turned into JS array
} js_curl_sync_data_t;

static JSClassID js_curl_sync_class_id;

static void js_curl_sync_finalizer(JSRuntime *rt, JSValue val) {
    js_curl_sync_data_t *c = JS_GetOpaque(val, js_curl_sync_class_id);
    if (c) {
        if (c->curl) curl_easy_cleanup(c->curl);
        if (c->res_buf) free(c->res_buf);
        if (c->curl_header_list) curl_slist_free_all(c->curl_header_list);
        if (c->res_header_list) generic_list_free_all(c->res_header_list);
        memset(c, 0, sizeof(js_curl_sync_data_t));
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

size_t js_curl_sync_header_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
    size_t realsize = size * nitems;
    js_curl_sync_data_t *c = userdata;

    char *ptr = malloc(realsize);
    if (!ptr) return 0;
    memcpy(ptr, buffer, realsize);
    generic_list_append(c->res_header_list, ptr, realsize);

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

    c->res_header_list = generic_list_init();
    if (!c->res_header_list) {
        free(c);
        JS_FreeValue(ctx, new_target); // is this needed?
        JS_ThrowInternalError(ctx, "CurlSync: generic_list_init failed");
        return JS_EXCEPTION;
    }

    c->curl = curl_easy_init();
    if (!c->curl) {
        generic_list_free_all(c->res_header_list);
        free(c);
        JS_FreeValue(ctx, new_target); // is this needed?
        JS_ThrowInternalError(ctx, "CurlSync: curl_easy_init failed");
        return JS_EXCEPTION;
    }

    // CURL setup
    curl_easy_setopt(c->curl, CURLOPT_WRITEFUNCTION, js_curl_sync_write_callback);
    curl_easy_setopt(c->curl, CURLOPT_WRITEDATA, (void *)c);
    curl_easy_setopt(c->curl, CURLOPT_HEADERFUNCTION, js_curl_sync_header_callback);
    curl_easy_setopt(c->curl, CURLOPT_HEADERDATA, (void *)c);

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
 * JS: CurlSync.prototype.set_http_method(url: string): void
 */
static JSValue js_curl_sync_set_http_method(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    js_curl_sync_data_t *c = JS_GetOpaque2(ctx, this_val, js_curl_sync_class_id);
    if (!c) {
        return JS_EXCEPTION;
    }

    const char *method = JS_ToCString(ctx, argv[0]);
    if (!method) {
        return JS_EXCEPTION;
    }
    curl_easy_setopt(c->curl, CURLOPT_CUSTOMREQUEST, method);
    JS_FreeCString(ctx, method);

    return JS_UNDEFINED;
}

/**
 * JS: CurlSync.prototype.set_http_header_list(url: string): void
 */
static JSValue js_curl_sync_set_http_header_list(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    js_curl_sync_data_t *c = JS_GetOpaque2(ctx, this_val, js_curl_sync_class_id);
    if (!c) {
        return JS_EXCEPTION;
    }

    JSValueConst js_arr = argv[0];

    if (!JS_IsArray(ctx, js_arr)) {
        return JS_ThrowTypeError(ctx, "Header list must be an array of strings");
    }

    // Get length of array by reading the JS .length property
    JSValue js_arr_len = JS_GetPropertyStr(ctx, js_arr, "length");
    if (JS_IsException(js_arr_len)) {
        return JS_EXCEPTION;
    }
    uint32_t len = 0;
    if (JS_ToUint32(ctx, &len, js_arr_len) < 0) {
        JS_FreeValue(ctx, js_arr_len);
        return JS_EXCEPTION;
    }
    JS_FreeValue(ctx, js_arr_len);

    // Discard the old header list
    if (c->curl_header_list) {
        curl_slist_free_all(c->curl_header_list);
        c->curl_header_list = NULL;
    }

    for (uint32_t i = 0; i < len; i++) {
        // Get array item (property) by integer index (uint32)
        JSValue js_str = JS_GetPropertyUint32(ctx, js_arr, i);
        if (!JS_IsString(js_str)) {
            JS_FreeValue(ctx, js_str);
            return JS_ThrowTypeError(ctx, "Array element not a string");
        }
        size_t str_len = 0;
        const char *str = JS_ToCStringLen(ctx, &str_len, js_str);
        JS_FreeValue(ctx, js_str);
        c->curl_header_list = curl_slist_append(c->curl_header_list, str);
        JS_FreeCString(ctx, str);
    }

    curl_easy_setopt(c->curl, CURLOPT_HTTPHEADER, c->curl_header_list);

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

    // clean up previous response data
    if (c->res_buf) {
        free(c->res_buf);
        c->res_buf = NULL;
    }
    c->res_buf_size = 0;
    if (c->res_header_list) {
        generic_list_free_all(c->res_header_list);
        c->res_header_list = generic_list_init();
    }

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

/**
 * JS: CurlSync.prototype.get_response_header_list(): string[]
 */
static JSValue js_curl_sync_get_response_header_list(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    js_curl_sync_data_t *c = JS_GetOpaque2(ctx, this_val, js_curl_sync_class_id);
    if (!c) {
        return JS_EXCEPTION;
    }

    return generic_list_nstr_to_js_arr(ctx, c->res_header_list);
}

static const JSCFunctionListEntry js_curl_sync_proto_funcs[] = {
    JS_CFUNC_DEF("set_url", 1, js_curl_sync_set_url ),
    JS_CFUNC_DEF("set_http_method", 1, js_curl_sync_set_http_method ),
    JS_CFUNC_DEF("set_http_header_list", 1, js_curl_sync_set_http_header_list ),
    JS_CFUNC_DEF("perform", 0, js_curl_sync_perform ),
    JS_CFUNC_DEF("get_response_buffer", 0, js_curl_sync_get_response_buffer ),
    JS_CFUNC_DEF("get_response_text", 0, js_curl_sync_get_response_text ),
    JS_CFUNC_DEF("get_response_header_list", 0, js_curl_sync_get_response_header_list ),
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
