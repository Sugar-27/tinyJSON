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

#define EXPECT(c, ch)                                                                                                  \
    do {                                                                                                               \
        assert(*c->json == (ch));                                                                                      \
        c->json++;                                                                                                     \
    } while (0)
#define ISDIGIT(ch)     ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')
#define PUTC(c, ch)                                                                                                    \
    do {                                                                                                               \
        *(char*)context_push(c, sizeof(char)) = (ch);                                                                  \
    } while (0)

typedef struct {
    const char* json;
    char* stack;
    size_t size, top;
} context;

static void* context_push(context* c, size_t size) {
    void* ret;
    assert(size > 0);
    if (c->top + size >= c->size) {
        if (c->size == 0) {
            c->size = tinyjson::PARSE_STACK_INIT_SIZE;
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

static int parse_string(context* c, value* v) {
    size_t head = c->top, len;
    const char* p;
    EXPECT(c, '\"'); // 匹配到开始的双引号
    p = c->json;
    for (;;) {
        char ch = *p++;
        switch (ch) {
        case '\"': // 匹配到结束的双引号
            len = c->top - head;
            set_string(v, (const char*)context_pop(c, len), len);
            c->json = p;
            return PARSE_OK;
        case '\\': // 第一个\是转义负号，表示这个字符是'\'
            switch (*p++) {
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
                c->top = head;
                return PARSE_INVALID_STRING_ESCAPE;
            }
            break;

        case '\0':
            c->top = head;
            return PARSE_MISS_QUOTATION_MARK;
        default:
            if ((unsigned char)ch < 0x20) {
                c->top = head;
                return PARSE_INVALID_STRING_CHAR;
            }
            PUTC(c, ch);
        }
    }
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

void tiny_free(value* v) {
    assert(v != nullptr);
    if (v->tiny_type == STRING) {
        free(v->u.s.s);
    }
    v->tiny_type = TINYNULL;
}
} // namespace tinyjson