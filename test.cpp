/*
 * @Author: Sugar 45682h@gmail.com
 * @Date: 2022-12-30 11:38:48
 * @Describe:tinyJSON的单元测试框架
 */
#include "tinyjson.h"

#include <iostream>
#include <ostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format)                                                               \
    do {                                                                                                               \
        test_count++;                                                                                                  \
        if (equality)                                                                                                  \
            test_pass++;                                                                                               \
        else {                                                                                                         \
            fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);     \
            main_ret = 1;                                                                                              \
        }                                                                                                              \
    } while (0)

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")

static void test_parse_null() {
    tinyjson::value v;
    v.tiny_type = tinyjson::FALSE;
    EXPECT_EQ_INT(tinyjson::PARSE_OK, tinyjson::parse(&v, "null"));
    EXPECT_EQ_INT(tinyjson::TINYNULL, tinyjson::get_type(&v));
}

static void test_parse_true() {
    tinyjson::value v;
    v.tiny_type = tinyjson::FALSE;
    EXPECT_EQ_INT(tinyjson::PARSE_OK, tinyjson::parse(&v, "true"));
    EXPECT_EQ_INT(tinyjson::TRUE, tinyjson::get_type(&v));
}

static void test_parse_false() {
    tinyjson::value v;
    v.tiny_type = tinyjson::TRUE;
    EXPECT_EQ_INT(tinyjson::PARSE_OK, tinyjson::parse(&v, "false"));
    EXPECT_EQ_INT(tinyjson::FALSE, tinyjson::get_type(&v));
}

static void test_parse_expect_value() {
    tinyjson::value v;

    v.tiny_type = tinyjson::FALSE;
    EXPECT_EQ_INT(tinyjson::PARSE_EXPECT_VALUE, tinyjson::parse(&v, ""));
    EXPECT_EQ_INT(tinyjson::TINYNULL, tinyjson::get_type(&v));

    v.tiny_type = tinyjson::FALSE;
    EXPECT_EQ_INT(tinyjson::PARSE_EXPECT_VALUE, tinyjson::parse(&v, " "));
    EXPECT_EQ_INT(tinyjson::TINYNULL, tinyjson::get_type(&v));
}

static void test_parse_invalid_value() {
    tinyjson::value v;

    v.tiny_type = tinyjson::FALSE;
    EXPECT_EQ_INT(tinyjson::PARSE_INVALID_VALUE, tinyjson::parse(&v, "nul"));
    EXPECT_EQ_INT(tinyjson::TINYNULL, tinyjson::get_type(&v));

    v.tiny_type = tinyjson::FALSE;
    EXPECT_EQ_INT(tinyjson::PARSE_INVALID_VALUE, tinyjson::parse(&v, "?"));
    EXPECT_EQ_INT(tinyjson::TINYNULL, tinyjson::get_type(&v));
}

static void test_parse_root_not_singular() {
    tinyjson::value v;
    v.tiny_type = tinyjson::FALSE;
    EXPECT_EQ_INT(tinyjson::PARSE_ROOT_NOT_SINGULAR, tinyjson::parse(&v, "null x"));
    EXPECT_EQ_INT(tinyjson::TINYNULL, tinyjson::get_type(&v));
}

static void test_parse() {
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
}

int main() {
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}