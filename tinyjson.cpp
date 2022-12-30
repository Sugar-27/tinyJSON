/*
 * @Author: Sugar 45682h@gmail.com
 * @Date: 2022-12-30 11:38:28
 * @Describe:
 */

#include "tinyjson.h"

#include <asm-generic/errno-base.h>
#include <assert.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <errno.h>

namespace tinyjson {

#define EXPECT(c, ch)                                                                                                  \
    do {                                                                                                               \
        assert(*c->json == (ch));                                                                                      \
        c->json++;                                                                                                     \
    } while (0)
#define ISDIGIT(ch)     ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')

typedef struct {
    const char* json;
} context;

/* ws = *(%x20 / %x09 / %x0A / %x0D) */
static void parse_whitespace(context* c) {
    const char* p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
        ++p;
    }
    c->json = p;
}

/* null  = "null" */
static int parse_null(context* c, value* v) {
    EXPECT(c, 'n');
    if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l') {
        return PARSE_INVALID_VALUE;
    }
    c->json += 3;
    v->tiny_type = TINYNULL;
    return PARSE_OK;
}

/* true = "true" */
static int parse_true(context* c, value* v) {
    EXPECT(c, 't');
    if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e') {
        return PARSE_INVALID_VALUE;
    }
    c->json += 3;
    v->tiny_type = TRUE;
    return PARSE_OK;
}

/* false = "false" */
static int parse_false(context* c, value* v) {
    EXPECT(c, 'f');
    if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e') {
        return PARSE_INVALID_VALUE;
    }
    c->json += 4;
    v->tiny_type = FALSE;
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
    v->n = strtod(c->json, nullptr);
    if (errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL)) {
        return PARSE_NUMBER_TOO_BIG;
    }

    c->json = p;
    v->tiny_type = NUMBER;
    return PARSE_OK;
}

/* value = null / false / true */
static int parse_value(context* c, value* v) {
    switch (*c->json) {
    case 'n':
        return parse_null(c, v);
    case 't':
        return parse_true(c, v);
    case 'f':
        return parse_false(c, v);
    default:
        return parse_number(c, v);
    case '\0':
        return PARSE_EXPECT_VALUE;
    }
}

// JSON-text = ws value ws
int parse(value* v, const char* json) {
    context c;
    assert(v != nullptr);
    c.json = json;
    v->tiny_type = TINYNULL;
    parse_whitespace(&c);

    int ret;
    if ((ret = parse_value(&c, v)) == PARSE_OK) {
        parse_whitespace(&c);
        if (*c.json != '\0') {
            v->tiny_type = TINYNULL;
            ret = PARSE_ROOT_NOT_SINGULAR;
        }
    }

    return ret;
}

type get_type(const value* v) {
    assert(v != nullptr);
    return v->tiny_type;
}

double get_number(const value* v) {
    assert(v != nullptr && v->tiny_type == NUMBER);
    return v->n;
}
} // namespace tinyjson