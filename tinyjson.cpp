/*
 * @Author: Sugar 45682h@gmail.com
 * @Date: 2022-12-30 11:38:28
 * @Describe:
 */

// windows平台下使用CRT(C Runtime Library)进行内存泄漏检测，Linux/OSx用valgrind工具即可无需添加代码
#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "tinyjson.h"

#include <asm-generic/errno-base.h>
#include <assert.h>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <iostream>

namespace tinyjson {
typedef struct {
    const char* json;
    char* stack;
    size_t size, top;
} context;

inline void EXPECT(context* c, char ch) {
    assert(*c->json == ch);
    ++c->json;
}

inline bool ISDIGIT(char ch) { return ch >= '0' && ch <= '9'; }

inline bool ISDIGIT1TO9(char ch) { return ch >= '1' && ch <= '9'; }

inline bool ISALPHA_ATOF(char ch) { return ch >= 'A' && ch <= 'F'; }

inline bool ISALPHA_aTOf(char ch) { return ch >= 'a' && ch <= 'f'; }

static void* context_push(context* c, size_t size) {
    void* ret;
    assert(size > 0);
    if (c->top + size >= c->size) {
        if (c->size == 0) {
            c->size = PARSE_STACK_INIT_SIZE;
        }
        while (c->top + size >= c->size) {
            c->size += c->size >> 1; // c->size * 1.5
        }
        c->stack = (char*)realloc(c->stack, c->size);
    }
    ret = c->stack + c->top;
    c->top += size;
    return ret;
}

static void* context_pop(context* c, size_t size) {
    assert(c->top >= size);
    return c->stack + (c->top -= size);
}

inline void PUTC(context* c, char ch) { *(char*)context_push(c, sizeof(char)) = ch; }

inline void PUTS(context* c, const char* s, size_t len) { memcpy(context_push(c, len), s, len); }

void copy(value* dst, const value* src) {
    size_t i;
    assert(dst != nullptr && src != nullptr && dst != src);
    switch (src->tiny_type) {
    case STRING:
        set_string(dst, src->u.s.s, src->u.s.len);
        break;
    case ARRAY:
        set_array(dst, src->u.a.size);
        for (i = 0; i < src->u.a.size; ++i) {
            copy(&dst->u.a.e[i], &src->u.a.e[i]);
        }
        dst->u.a.size = src->u.a.size;
        break;
    case OBJECT:
        set_object(dst, src->u.o.size);
        for (i = 0; i < src->u.o.size; ++i) {
            auto v = set_object_value(dst, src->u.o.m[i].k, src->u.o.m[i].klen);
            copy(v, &src->u.o.m[i].v);
        }
        dst->u.o.size = src->u.o.size;
        break;
    default:
        tiny_free(dst);
        memcpy(dst, src, sizeof(value));
        break;
    }
}

void move(value* dst, value* src) {
    assert(dst != nullptr && src != nullptr && dst != src);
    tiny_free(dst);
    memcpy(dst, src, sizeof(value));
    tiny_init(src);
}

void swap(value* lhs, value* rhs) {
    assert(lhs != nullptr && rhs != nullptr);
    if (lhs == rhs) {
        return;
    }
    value tmp;
    memcpy(&tmp, rhs, sizeof(value));
    memcpy(rhs, lhs, sizeof(value));
    memcpy(lhs, &tmp, sizeof(value));
}

/* ws = *(%x20 / %x09 / %x0A / %x0D) */
static void parse_whitespace(context* c) {
    const char* p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
        ++p;
    }
    c->json = p;
}

static int parse_literal(context* c, value* v, const char* literal, type type) {
    EXPECT(c, literal[0]);
    size_t i;
    for (i = 0; literal[i + 1]; ++i) {
        if (c->json[i] != literal[i + 1]) {
            return PARSE_INVALID_VALUE;
        }
    }
    c->json += i;
    v->tiny_type = type;
    return PARSE_OK;
}

static int parse_number(context* c, value* v) {
    const char* p = c->json;
    // 数字合法性校验
    // 负号直接跳过即可
    if (*p == '-') {
        ++p;
    }
    // 校验第一个数字
    if (*p == '0') {
        // 第一个数字为'0'，则这个数字应该是0，跟负号处理相同
        ++p;
    } else {
        // 否则需要保证第一个数字为1-9中的一个
        if (!ISDIGIT1TO9(*p)) {
            return PARSE_INVALID_VALUE;
        }

        // 跳过所有数字即可
        for (++p; ISDIGIT(*p); ++p) {}
    }

    // 出现小数点要跳过
    if (*p == '.') {
        ++p;
        // 小数点后需保证有数字
        if (!ISDIGIT(*p)) {
            return PARSE_INVALID_VALUE;
        }
        for (++p; ISDIGIT(*p); ++p) {}
    }

    // 如果出现大小写E，则表示存在指数部分，跳过E之后可以有一个正或负号，有的话就跳过
    if (*p == 'E' || *p == 'e') {
        ++p;
        if (*p == '+' || *p == '-')
            ++p;
        // E之后必须有数字
        if (!ISDIGIT(*p)) {
            return PARSE_INVALID_VALUE;
        }
        for (++p; ISDIGIT(*p); ++p) {}
    }

    errno = 0;
    v->u.n = strtod(c->json, nullptr);
    if (errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL)) {
        return PARSE_NUMBER_TOO_BIG;
    }

    c->json = p;
    v->tiny_type = NUMBER;
    return PARSE_OK;
}

#define STRING_ERROR(ret)                                                                                              \
    do {                                                                                                               \
        c->top = head;                                                                                                 \
        return ret;                                                                                                    \
    } while (0)

static const char* parse_hex4(const char* p, unsigned int* u) {
    int i;
    *u = 0;
    for (i = 0; i < 4; ++i) {
        char ch = *p++;
        *u <<= 4;
        if (ISDIGIT(ch)) {
            *u |= ch - '0';
        } else if (ISALPHA_ATOF(ch)) {
            *u |= ch - 'A' + 0xa;
        } else if (ISALPHA_aTOf(ch)) {
            *u |= ch - 'a' + 0xa;
        } else {
            return nullptr;
        }
    }
    return p;
}

static void encode_utf8(context* c, unsigned int u) {
    if (u <= 0x7f) {
        // 通过判断可以保证u是小于256的，不会产生截断，为避免编译器警告与上0xff，编译时编译器会进行优化忽略掉这个操作
        PUTC(c, u & 0xff);
    } else if (u <= 0x7ff) {
        PUTC(c, 0xc0 | ((u >> 6) & 0xff));
        PUTC(c, 0x80 | (u & 0x3f));
    } else if (u <= 0xffff) {
        PUTC(c, 0xe0 | ((u >> 12) & 0xff));
        PUTC(c, 0x80 | ((u >> 6) & 0x3f));
        PUTC(c, 0x80 | (u & 0x3f));
    } else if (u <= 0x10ffff) {
        PUTC(c, 0xf0 | ((u >> 18) & 0xff));
        PUTC(c, 0x80 | ((u >> 12) & 0x3f));
        PUTC(c, 0x80 | ((u >> 6) & 0x3f));
        PUTC(c, 0x80 | (u & 0x3f));
    }
}

static int parse_string_raw(context* c, char** str, size_t* len) {
    size_t head = c->top;
    const char* p;
    EXPECT(c, '\"'); // 匹配到开始的双引号
    p = c->json;
    for (;;) {
        unsigned int u, u2;
        char ch = *p++;
        switch (ch) {
        case '\"': // 匹配到结束的双引号
            *len = c->top - head;
            *str = (char*)context_pop(c, *len);
            c->json = p;
            return PARSE_OK;
        case '\\': // 第一个\是转义负号，表示这个字符是'\'
            switch (*p++) {
            case 'u': // 处理UTF-8编码
                if (!(p = parse_hex4(p, &u))) {
                    STRING_ERROR(PARSE_INVALID_UNICODE_HEX);
                }
                if (u >= 0xd800 && u <= 0xdbff) {
                    if (*p++ != '\\') {
                        STRING_ERROR(PARSE_INVALID_UNICODE_SURROGATE);
                    }
                    if (*p++ != 'u') {
                        STRING_ERROR(PARSE_INVALID_UNICODE_SURROGATE);
                    }
                    if (!(p = parse_hex4(p, &u2))) {
                        STRING_ERROR(PARSE_INVALID_UNICODE_HEX);
                    }
                    if (u2 < 0xdc00 || u2 > 0xdfff) {
                        STRING_ERROR(PARSE_INVALID_UNICODE_SURROGATE);
                    }
                    u = 0x10000 + (((u - 0xd800) << 10) | (u2 - 0xdc00));
                }
                encode_utf8(c, u);
                break;
            case '\"':
                PUTC(c, '\"');
                break;
            case '\\':
                PUTC(c, '\\');
                break;
            case '/':
                PUTC(c, '/');
                break;
            case 'b':
                PUTC(c, '\b');
                break;
            case 'f':
                PUTC(c, '\f');
                break;
            case 'n':
                PUTC(c, '\n');
                break;
            case 'r':
                PUTC(c, '\r');
                break;
            case 't':
                PUTC(c, '\t');
                break;
            default:
                STRING_ERROR(PARSE_INVALID_STRING_ESCAPE);
            }
            break;

        case '\0':
            STRING_ERROR(PARSE_MISS_QUOTATION_MARK);
        default:
            if ((unsigned char)ch < 0x20) {
                STRING_ERROR(PARSE_INVALID_STRING_CHAR);
            }
            PUTC(c, ch);
        }
    }
}

static int parse_string(context* c, value* v) {
    char* s;
    int ret;
    size_t len;
    if ((ret = parse_string_raw(c, &s, &len)) == PARSE_OK) {
        set_string(v, s, len);
    }
    return ret;
}

static int parse_value(context* c, value* v); // 前置声明
static int parse_array(context* c, value* v) {
    size_t i, size = 0;
    int ret;
    EXPECT(c, '[');
    // 考虑到数组的空格分割特性，在三个位置添加跳过空格：检测到'['后，检测到','后，parse完元素后
    parse_whitespace(c);
    if (*c->json == ']') {
        c->json++;
        set_array(v, 0);
        return PARSE_OK;
    }
    for (;;) {
        value tmp_v;
        tiny_init(&tmp_v);
        if ((ret = parse_value(c, &tmp_v)) != PARSE_OK) {
            break;
        }
        parse_whitespace(c);
        /* 使用临时value变量存储解析结果再将其内存拷贝到c中而不是直接在c中的内存直接存储变量的原因：
        因为在存储value的过程中会调用contex_push，如果发现堆栈满了会调用realloc函数，而这时如果直接在c中的内存存储变量的话，这个指针此时就已经是悬空指针，
        后面当需要把解析结果存储到对应的内存时因为这个指针已经变成了悬空指针，就会发生错误
        */
        memcpy(context_push(c, sizeof(value)), &tmp_v, sizeof(value));
        ++size;
        if (*c->json == ',') {
            ++c->json;
            parse_whitespace(c);
        } else if (*c->json == ']') {
            ++c->json;
            set_array(v, size);
            v->u.a.size = size;
            size *= sizeof(value);
            memcpy(v->u.a.e = (value*)malloc(size), context_pop(c, size), size);
            return PARSE_OK;
        } else {
            ret = PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
            break;
        }
    }

    // pop堆栈中的内存并释放这块内存
    for (i = 0; i < size; ++i) {
        tiny_free((value*)context_pop(c, sizeof(value)));
    }

    return ret;
}

static int parse_object(context* c, value* v) {
    size_t size;
    member m;
    int ret;
    EXPECT(c, '{');
    parse_whitespace(c);
    if (*c->json == '}') {
        c->json++;
        v->tiny_type = OBJECT;
        v->u.o.size = 0;
        v->u.o.m = nullptr;
        return PARSE_OK;
    }
    m.k = nullptr;
    size = 0;
    for (;;) {
        char* str;
        tiny_init(&m.v);
        // parse key
        if (*c->json != '"') {
            ret = PARSE_MISS_KEY;
            break;
        }
        if ((ret = parse_string_raw(c, &str, &m.klen)) != PARSE_OK) {
            break;
        }
        memcpy(m.k = (char*)malloc(m.klen + 1), str, m.klen);
        m.k[m.klen] = '\0';

        // parse ws colon ws
        parse_whitespace(c);
        if (*c->json != ':') {
            ret = PARSE_MISS_COLON;
            break;
        }
        c->json++;
        parse_whitespace(c);

        // parse value
        if ((ret = parse_value(c, &m.v)) != PARSE_OK) {
            break;
        }
        memcpy(context_push(c, sizeof(member)), &m, sizeof(member));
        ++size;
        m.k = nullptr; // 所有值转移到栈上

        // parse ws [comma | right-curly-brace] ws，逗号或者右花括号
        parse_whitespace(c);
        if (*c->json == ',') {
            c->json++;
            parse_whitespace(c);
        } else if (*c->json == '}') {
            c->json++;
            size_t s = sizeof(member) * size;
            v->tiny_type = OBJECT;
            v->u.o.size = size;
            memcpy(v->u.o.m = (member*)malloc(s), context_pop(c, s), s);
            ret = PARSE_OK;
            break;
        } else {
            ret = PARSE_MISS_COMMA_OR_CURLY_BRACKET;
            break;
        }
    }
    if (ret != PARSE_OK) {
        // pop and free members on the stack
        free(m.k);
        for (int i = 0; i < size; ++i) {
            member* m = (member*)context_pop(c, sizeof(member));
            free(m->k);
            tiny_free(&m->v);
        }
        v->tiny_type = TINYNULL;
    }
    return ret;
}

/* value = null / false / true / number / string /*/
static int parse_value(context* c, value* v) {
    switch (*c->json) {
    case 'n':
        return parse_literal(c, v, "null", TINYNULL);
    case 't':
        return parse_literal(c, v, "true", TRUE);
    case 'f':
        return parse_literal(c, v, "false", FALSE);
    default:
        return parse_number(c, v);
    case '"':
        return parse_string(c, v);
    case '[':
        return parse_array(c, v);
    case '{':
        return parse_object(c, v);
    case '\0':
        return PARSE_EXPECT_VALUE;
    }
}

// JSON-text = ws value ws
int parse(value* v, const char* json) {
    context c;
    assert(v != nullptr);
    c.json = json;
    c.stack = nullptr;
    c.size = c.top = 0;
    tiny_init(v);
    parse_whitespace(&c);

    int ret;
    if ((ret = parse_value(&c, v)) == PARSE_OK) {
        parse_whitespace(&c);
        if (*c.json != '\0') {
            v->tiny_type = TINYNULL;
            ret = PARSE_ROOT_NOT_SINGULAR;
        }
    }
#if 0
    switch (ret) {
    case PARSE_OK:
        printf("PARSE_OK\n");
        break;
    case PARSE_EXPECT_VALUE:
        printf("PARSE_EXPECT_VALUE\n");
        break;
    case PARSE_INVALID_VALUE:
        printf("PARSE_INVALID_VALUE\n");
        break;
    case PARSE_ROOT_NOT_SINGULAR:
        printf("PARSE_ROOT_NOT_SINGULAR\n");
        break;
    case PARSE_NUMBER_TOO_BIG:
        printf("PARSE_NUMBER_TOO_BIG\n");
        break;
    case PARSE_MISS_QUOTATION_MARK:
        printf("PARSE_MISS_QUOTATION_MARK\n");
        break;
    case PARSE_INVALID_STRING_ESCAPE:
        printf("PARSE_INVALID_STRING_ESCAPE\n");
        break;
    case PARSE_INVALID_STRING_CHAR:
        printf("PARSE_INVALID_STRING_CHAR\n");
        break;
    }
#endif
    assert(c.top == 0);
    free(c.stack);

    return ret;
}

static void stringify_string(context* c, const char* s, size_t len) {
    assert(s != nullptr);
    PUTC(c, '"');
    for (size_t i = 0; i < len; ++i) {
        unsigned char ch = (unsigned char)s[i];
        switch (ch) {
        case '\"':
            PUTS(c, "\\\"", 2);
            break;
        case '\\':
            PUTS(c, "\\\\", 2);
            break;
        case '\b':
            PUTS(c, "\\b", 2);
            break;
        case '\t':
            PUTS(c, "\\t", 2);
            break;
        case '\r':
            PUTS(c, "\\r", 2);
            break;
        case '\n':
            PUTS(c, "\\n", 2);
            break;
        case '\f':
            PUTS(c, "\\f", 2);
            break;
        default:
            if (ch < 0x20) {
                char buffer[7];
                sprintf(buffer, "\\u%04X", ch);
                PUTS(c, buffer, 6);
            } else {
                PUTC(c, s[i]);
            }
        }
    }
    PUTC(c, '"');
}

static void stringify_value(context* c, const value* v) {
    switch (v->tiny_type) {
    case TINYNULL:
        PUTS(c, "null", 4);
        break;
    case TRUE:
        PUTS(c, "true", 4);
        break;
    case FALSE:
        PUTS(c, "false", 5);
        break;
    case NUMBER:
        c->top -= 32 - sprintf((char*)context_push(c, 32), "%.17g", v->u.n);
        break;
    case STRING:
        stringify_string(c, v->u.s.s, v->u.s.len);
        break;
    case ARRAY:
        PUTC(c, '[');
        for (size_t i = 0; i < v->u.a.size; ++i) {
            if (i != 0) {
                PUTC(c, ',');
            }
            stringify_value(c, &v->u.a.e[i]);
        }
        PUTC(c, ']');
        break;
    case OBJECT:
        PUTC(c, '{');
        for (size_t i = 0; i < v->u.o.size; ++i) {
            if (i != 0) {
                PUTC(c, ',');
            }
            stringify_string(c, v->u.o.m[i].k, v->u.o.m->klen);
            PUTC(c, ':');
            stringify_value(c, &v->u.o.m[i].v);
        }
        PUTC(c, '}');
        break;
    default:
        break;
    }
}

char* stringify(const value* v, size_t* len) {
    context c;
    assert(v != nullptr);
    c.stack = (char*)malloc(c.size = PARSE_STACK_INIT_SIZE);
    c.top = 0;
    stringify_value(&c, v);
    if (len) {
        *len = c.top;
    }
    PUTC(&c, '\0');
    return c.stack;
}

type get_type(const value* v) {
    assert(v != nullptr);
    return v->tiny_type;
}

double get_number(const value* v) {
    assert(v != nullptr && v->tiny_type == NUMBER);
    return v->u.n;
}

void set_number(value* v, double n) {
    tiny_free(v);
    v->u.n = n;
    v->tiny_type = NUMBER;
}

int get_boolean(const value* v) {
    assert(v != nullptr && (v->tiny_type == TRUE || v->tiny_type == FALSE));
    return v->tiny_type == TRUE;
}

void set_boolean(value* v, int b) {
    // 先释放内存
    tiny_free(v);
    v->tiny_type = b ? TRUE : FALSE;
}

const char* get_string(const value* v) {
    assert(v != nullptr && v->tiny_type == STRING);
    return v->u.s.s;
}

size_t get_string_len(const value* v) {
    assert(v != nullptr && v->tiny_type == STRING);
    return v->u.s.len;
}

void set_string(value* v, const char* s, size_t len) {
    assert(v != nullptr && (s != nullptr || len == 0));
    tiny_free(v);
    v->u.s.s = (char*)malloc(len + 1);
    memcpy(v->u.s.s, s, len);
    v->u.s.s[len] = '\0';
    v->u.s.len = len;
    v->tiny_type = STRING;
}

size_t get_array_size(const value* v) {
    assert(v != nullptr && v->tiny_type == ARRAY);
    return v->u.a.size;
}

size_t get_array_capacity(const value* v) {
    assert(v != nullptr && v->tiny_type == ARRAY);
    return v->u.a.capacity;
}

void array_reserve(value* v, size_t capacity) {
    assert(v != nullptr && v->tiny_type == ARRAY);
    if (v->u.a.capacity >= capacity) {
        return;
    }
    v->u.a.capacity = capacity;
    v->u.a.e = (value*)realloc(v->u.a.e, capacity * sizeof(value));
}

void array_shrink(value* v) {
    assert(v != nullptr && v->tiny_type == ARRAY && v->u.a.capacity >= v->u.a.size);
    if (v->u.a.size == v->u.a.capacity) {
        return;
    }
    v->u.a.capacity = v->u.a.size;
    v->u.a.e = (value*)realloc(v->u.a.e, v->u.a.capacity * sizeof(value));
}

value* array_pushback(value* v) {
    assert(v != nullptr && v->tiny_type == ARRAY);
    if (v->u.a.size == v->u.a.capacity) {
        array_reserve(v, v->u.a.capacity == 0 ? 1 : v->u.a.capacity * EXPAND_COEFFICIENT);
    }
    tiny_init(&v->u.a.e[v->u.a.size]);
    return &v->u.a.e[v->u.a.size++];
}

void array_popback(value* v) {
    assert(v != nullptr && v->tiny_type == ARRAY && v->u.a.size > 0);
    tiny_free(&v->u.a.e[--v->u.a.size]);
}

value* array_insert(value* v, size_t index) {
    assert(v != nullptr && v->tiny_type == ARRAY && index <= v->u.a.capacity);
    if (index == v->u.a.capacity) {
        return array_pushback(v);
    }
    if (v->u.a.capacity < v->u.a.size + 1) {
        array_reserve(v, v->u.a.capacity == 0 ? 1 : v->u.a.capacity * EXPAND_COEFFICIENT);
    }
    for (size_t i = v->u.a.size + 1; i > index; --i) {
        move(&v->u.a.e[i], &v->u.a.e[i - 1]);
    }
    tiny_init(&v->u.a.e[index]);
    ++v->u.a.size;
    return &v->u.a.e[index];
}

void array_erase(value* v, size_t index, size_t count) {
    assert(v != nullptr && v->tiny_type == ARRAY && index < v->u.a.capacity);

    for (size_t i = 0; i < count; ++i) {
        tiny_free(&v->u.a.e[index + i]);
    }

    // for (size_t i = 0; i < count; ++i) {
    //     move(&v->u.a.e[index + i], &v->u.a.e[index + count + i]);
    // }
    memcpy(v->u.a.e + index, v->u.a.e + index + count, (v->u.a.size - index - count) * sizeof(value));

    for (size_t i = v->u.a.size - count; i < v->u.a.size; ++i) {
        tiny_free(&v->u.a.e[i]);
    }
    v->u.a.size -= count;
}

void array_clear(value* v) {
    assert(v != nullptr && v->tiny_type == ARRAY);
    for (size_t i = 0; i < v->u.a.size; ++i) {
        tiny_free(&v->u.a.e[i]);
    }
    v->u.a.size = 0;
}

value* get_array_element(const value* v, size_t index) {
    assert(v != nullptr && v->tiny_type == ARRAY);
    assert(index < v->u.a.size);
    return &v->u.a.e[index];
}

void set_array(value* v, size_t capacity) {
    assert(v != nullptr);
    tiny_free(v);
    v->tiny_type = ARRAY;
    v->u.a.size = 0;
    v->u.a.capacity = capacity;
    v->u.a.e = capacity > 0 ? (value*)malloc(capacity * sizeof(value)) : nullptr;
}

size_t get_object_size(const value* v) {
    assert(v != nullptr && v->tiny_type == OBJECT);
    return v->u.o.size;
}

const char* get_object_key(const value* v, size_t index) {
    assert(v != nullptr && v->tiny_type == OBJECT);
    return v->u.o.m[index].k;
}

size_t get_object_key_length(const value* v, size_t index) {
    assert(v != nullptr && v->tiny_type == OBJECT);
    assert(index < v->u.o.size);
    return v->u.o.m[index].klen;
}

value* get_object_value(const value* v, size_t index) {
    assert(v != nullptr && v->tiny_type == OBJECT);
    return &v->u.o.m[index].v;
}

void set_object(value* v, size_t capacity) {
    assert(v != nullptr);
    tiny_free(v);
    v->tiny_type = OBJECT;
    v->u.o.size = 0;
    v->u.o.capacity = capacity;
    v->u.o.m = capacity > 0 ? (member*)malloc(capacity * sizeof(member)) : nullptr;
}

size_t get_object_capacity(const value* v) {
    assert(v != nullptr && v->tiny_type == OBJECT);
    return v->u.o.capacity;
}

void object_reserve(value* v, size_t capacity) {
    assert(v != nullptr && v->tiny_type == OBJECT);
    if (v->u.o.capacity >= capacity) {
        return;
    }
    v->u.o.capacity = capacity;
    v->u.o.m = (member*)realloc(v->u.o.m, capacity * sizeof(member));
}

void object_shrink(value* v) {
    assert(v != nullptr && v->tiny_type == OBJECT);
    if (v->u.o.size == v->u.o.capacity) {
        return;
    }
    v->u.o.capacity = v->u.o.size;
    v->u.o.m = (member*)realloc(v->u.o.m, v->u.o.capacity * sizeof(member));
}

void object_clear(value* v) {
    assert(v != nullptr && v->tiny_type == OBJECT);
    for (size_t i = 0; i < v->u.o.size; ++i) {
        free(v->u.o.m[i].k);
        v->u.o.m[i].k = nullptr;
        v->u.o.m[i].klen = 0;
        tiny_free(&v->u.o.m[i].v);
    }
    v->u.o.size = 0;
}

value* set_object_value(value* v, char* key, size_t klen) {
    assert(v != nullptr && v->tiny_type == OBJECT && key != nullptr);
    auto index = find_object_index(v, key, klen);
    if (index != KEY_NOT_EXIST) {
        return &v->u.o.m[index].v;
    }
    if (v->u.o.capacity == v->u.o.size) {
        object_reserve(v, v->u.o.capacity == 0 ? 1 : v->u.o.capacity * EXPAND_COEFFICIENT);
    }
    auto size = v->u.o.size++;
    v->u.o.m[size].k = (char*)malloc(klen + 1);
    memcpy(v->u.o.m[size].k, key, klen);
    v->u.o.m[size].k[klen] = '\0';
    v->u.o.m[size].klen = klen;
    tiny_init(&v->u.o.m[size].v);
    return &v->u.o.m[size].v;
}

void remove_object_value(value* v, size_t index) {
    assert(v != nullptr && v->tiny_type == OBJECT && index < v->u.o.size);
    free(v->u.o.m[index].k);
    tiny_free(&v->u.o.m[index].v);
    memcpy(&v->u.o.m[index], &v->u.o.m[index + 1], (v->u.o.size - index - 1) * sizeof(member));
    auto size = v->u.o.size--;
    v->u.o.m[size - 1].k = nullptr;
    v->u.o.m[size - 1].klen = 0;
    tiny_init(&v->u.o.m[size - 1].v);
}

void tiny_free(value* v) {
    assert(v != nullptr);
    size_t i;
    switch (v->tiny_type) {
    case STRING:
        free(v->u.s.s);
        break;
    case ARRAY:
        for (i = 0; i < v->u.a.size; ++i) {
            tiny_free(&v->u.a.e[i]);
        }
        free(v->u.a.e);
        break;
    case OBJECT:
        for (i = 0; i < v->u.o.size; ++i) {
            free(v->u.o.m[i].k);
            tiny_free(&v->u.o.m[i].v);
        }
        free(v->u.o.m);
        break;
    default:
        break;
    }
    v->tiny_type = TINYNULL;
}

size_t find_object_index(const value* v, const char* key, size_t klen) {
    size_t i;
    assert(v != nullptr && v->tiny_type == OBJECT && key != nullptr);
    for (i = 0; i < v->u.o.size; ++i) {
        if (v->u.o.m[i].klen == klen && memcmp(v->u.o.m[i].k, key, klen) == 0) {
            return i;
        }
    }
    return KEY_NOT_EXIST;
}

value* find_object_value(value* v, const char* key, size_t klen) {
    auto index = find_object_index(v, key, klen);
    return index == KEY_NOT_EXIST ? nullptr : &v->u.o.m[index].v;
}

int is_equal(const value* lhs, const value* rhs) {
    size_t i;
    assert(lhs != nullptr && rhs != nullptr);
    if (lhs->tiny_type != rhs->tiny_type) {
        return 0;
    }

    switch (lhs->tiny_type) {
    case STRING:
        return lhs->u.s.len == rhs->u.s.len && memcmp(lhs->u.s.s, rhs->u.s.s, lhs->u.s.len) == 0;
    case NUMBER:
        return lhs->u.n == rhs->u.n;
    case ARRAY:
        if (lhs->u.a.size != rhs->u.a.size) {
            return 0;
        }
        for (i = 0; i < lhs->u.a.size; ++i) {
            if (is_equal(&lhs->u.a.e[i], &rhs->u.a.e[i]) == 0) {
                return 0;
            }
        }
        return 1;
    case OBJECT:
        if (lhs->u.o.size != rhs->u.o.size) {
            return 0;
        }
        for (i = 0; i < lhs->u.o.size; ++i) {
            auto index = find_object_index(rhs, lhs->u.o.m[i].k, lhs->u.o.m[i].klen);
            if (index == KEY_NOT_EXIST) {
                return 0;
            }
            if (is_equal(&lhs->u.o.m[i].v, &rhs->u.o.m[index].v) == 0) {
                return 0;
            }
        }
        return 1;
    default:
        // TINYNULL、TRUE、FALSE这三种类型只需要比较类型相等即可
        return 1;
    }
}
} // namespace tinyjson