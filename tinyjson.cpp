/*
 * @Author: Sugar 45682h@gmail.com
 * @Date: 2022-12-30 11:38:28
 * @Describe:
 */

#include "tinyjson.h"

#include <assert.h>

namespace tinyjson {

#define EXPECT(c, ch)                                                                                                  \
    do {                                                                                                               \
        assert(*c->json == (ch));                                                                                      \
        c->json++;                                                                                                     \
    } while (0)

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

/* value = null / false / true */
static int parse_value(context* c, value* v) {
    switch (*c->json) {
    case 'n':
        return parse_null(c, v);
    case 't':
        return parse_true(c, v);
    case 'f':
        return parse_false(c, v);
    case '\0':
        return PARSE_EXPECT_VALUE;
    default:
        return PARSE_INVALID_VALUE;
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
            ret = PARSE_ROOT_NOT_SINGULAR;
        }
    }

    return ret;
}

type get_type(const value* v) {
    assert(v != nullptr);
    return v->tiny_type;
}
} // namespace tinyjson