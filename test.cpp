/*
 * @Author: Sugar 45682h@gmail.com
 * @Date: 2022-12-30 11:38:48
 * @Describe:tinyJSON的单元测试框架
 */
// windows平台下使用CRT(C Runtime Library)进行内存泄漏检测，Linux/OSx用valgrind工具即可无需添加代码
#include <cstddef>
#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

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

#define EXPECT_EQ_INT(expect, actual)    EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g")
#define EXPECT_EQ_STRING(expect, actual, alength)                                                                      \
    EXPECT_EQ_BASE(sizeof(expect) - 1 == (alength) && memcmp(expect, actual, alength) == 0, expect, actual, "%s")
#define EXPECT_TRUE(actual)  EXPECT_EQ_BASE((actual) != 0, "true", "false", "%s")
#define EXPECT_FALSE(actual) EXPECT_EQ_BASE((actual) == 0, "false", "true", "%s")
#if defined(_MSC_VER)
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%Iu")
#else
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%zu")
#endif

#define TEST_ERROR(error, json)                                                                                        \
    do {                                                                                                               \
        tinyjson::value v;                                                                                             \
        tiny_init(&v);                                                                                                 \
        tinyjson::set_boolean(&v, 0);                                                                                  \
        EXPECT_EQ_INT(error, tinyjson::parse(&v, json));                                                               \
        EXPECT_EQ_INT(tinyjson::TINYNULL, tinyjson::get_type(&v));                                                     \
        tinyjson::tiny_free(&v);                                                                                       \
    } while (0)

#define TEST_NUMBER(expect, json)                                                                                      \
    do {                                                                                                               \
        tinyjson::value v;                                                                                             \
        tiny_init(&v);                                                                                                 \
        auto res = tinyjson::parse(&v, json);                                                                          \
        EXPECT_EQ_INT(tinyjson::PARSE_OK, res);                                                                        \
        EXPECT_EQ_INT(tinyjson::NUMBER, tinyjson::get_type(&v));                                                       \
        EXPECT_EQ_DOUBLE(expect, tinyjson::get_number(&v));                                                            \
        tinyjson::tiny_free(&v);                                                                                       \
    } while (0)

#define TEST_STRING(expect, json)                                                                                      \
    do {                                                                                                               \
        tinyjson::value v;                                                                                             \
        tiny_init(&v);                                                                                                 \
        EXPECT_EQ_INT(tinyjson::PARSE_OK, tinyjson::parse(&v, json));                                                  \
        EXPECT_EQ_INT(tinyjson::STRING, tinyjson::get_type(&v));                                                       \
        EXPECT_EQ_STRING(expect, tinyjson::get_string(&v), tinyjson::get_string_len(&v));                              \
        tinyjson::tiny_free(&v);                                                                                       \
    } while (0)

static void test_parse_null() {
    tinyjson::value v;
    tiny_init(&v);
    tinyjson::set_boolean(&v, 0);
    EXPECT_EQ_INT(tinyjson::PARSE_OK, tinyjson::parse(&v, "null"));
    EXPECT_EQ_INT(tinyjson::TINYNULL, tinyjson::get_type(&v));
    tinyjson::tiny_free(&v);
}

static void test_parse_true() {
    tinyjson::value v;
    tiny_init(&v);
    tinyjson::set_boolean(&v, 0);
    EXPECT_EQ_INT(tinyjson::PARSE_OK, tinyjson::parse(&v, "true"));
    EXPECT_EQ_INT(tinyjson::TRUE, tinyjson::get_type(&v));
    tinyjson::tiny_free(&v);
}

static void test_parse_false() {
    tinyjson::value v;
    tiny_init(&v);
    tinyjson::set_boolean(&v, 1);
    EXPECT_EQ_INT(tinyjson::PARSE_OK, tinyjson::parse(&v, "false"));
    EXPECT_EQ_INT(tinyjson::FALSE, tinyjson::get_type(&v));
    tinyjson::tiny_free(&v);
}

static void test_parse_number() {
    TEST_NUMBER(0.0, "0");
    TEST_NUMBER(0.0, "-0");
    TEST_NUMBER(0.0, "-0.0");
    TEST_NUMBER(1.0, "1");
    TEST_NUMBER(-1.0, "-1");
    TEST_NUMBER(1.5, "1.5");
    TEST_NUMBER(-1.5, "-1.5");
    TEST_NUMBER(3.1416, "3.1416");
    TEST_NUMBER(1E10, "1E10");
    TEST_NUMBER(1e10, "1e10");
    TEST_NUMBER(1E+10, "1E+10");
    TEST_NUMBER(1E-10, "1E-10");
    TEST_NUMBER(-1E10, "-1E10");
    TEST_NUMBER(-1e10, "-1e10");
    TEST_NUMBER(-1E+10, "-1E+10");
    TEST_NUMBER(-1E-10, "-1E-10");
    TEST_NUMBER(1.234E+10, "1.234E+10");
    TEST_NUMBER(1.234E-10, "1.234E-10");
    TEST_NUMBER(0.0, "1e-10000"); /* must underflow */

    TEST_NUMBER(1.0000000000000002, "1.0000000000000002");           /* the smallest number > 1 */
    TEST_NUMBER(4.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denormal */
    TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
    TEST_NUMBER(2.2250738585072009e-308, "2.2250738585072009e-308"); /* Max subnormal double */
    TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
    TEST_NUMBER(2.2250738585072014e-308, "2.2250738585072014e-308"); /* Min normal positive double */
    TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
    TEST_NUMBER(1.7976931348623157e+308, "1.7976931348623157e+308"); /* Max double */
    TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}

static void test_parse_string() {
    TEST_STRING("", "\"\"");
    TEST_STRING("Hello", "\"Hello\"");
    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
    TEST_STRING("Hello\0World", "\"Hello\\u0000World\"");
    TEST_STRING("\x24", "\"\\u0024\"");                    /* Dollar sign U+0024 */
    TEST_STRING("\xC2\xA2", "\"\\u00A2\"");                /* Cents sign U+00A2 */
    TEST_STRING("\xE2\x82\xAC", "\"\\u20AC\"");            /* Euro sign U+20AC */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\uD834\\uDD1E\""); /* G clef sign U+1D11E */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\ud834\\udd1e\""); /* G clef sign U+1D11E */
}

static void test_parse_array() {
    size_t i, j;
    tinyjson::value v;

    tinyjson::tiny_init(&v);
    EXPECT_EQ_INT(tinyjson::PARSE_OK, tinyjson::parse(&v, "[ ]"));
    EXPECT_EQ_INT(tinyjson::ARRAY, tinyjson::get_type(&v));
    EXPECT_EQ_SIZE_T(0, tinyjson::get_array_size(&v));
    tinyjson::tiny_free(&v);

    tinyjson::tiny_init(&v);
    EXPECT_EQ_INT(tinyjson::PARSE_OK, tinyjson::parse(&v, "[ null , false , true , 123 , \"abc\" ]"));
    EXPECT_EQ_INT(tinyjson::ARRAY, tinyjson::get_type(&v));
    EXPECT_EQ_SIZE_T(5, tinyjson::get_array_size(&v));
    EXPECT_EQ_INT(tinyjson::TINYNULL, tinyjson::get_type(tinyjson::get_array_element(&v, 0)));
    EXPECT_EQ_INT(tinyjson::FALSE, tinyjson::get_type(tinyjson::get_array_element(&v, 1)));
    EXPECT_EQ_INT(tinyjson::TRUE, tinyjson::get_type(tinyjson::get_array_element(&v, 2)));
    EXPECT_EQ_INT(tinyjson::NUMBER, tinyjson::get_type(tinyjson::get_array_element(&v, 3)));
    EXPECT_EQ_INT(tinyjson::STRING, tinyjson::get_type(tinyjson::get_array_element(&v, 4)));
    EXPECT_EQ_DOUBLE(123.0, tinyjson::get_number(tinyjson::get_array_element(&v, 3)));
    EXPECT_EQ_STRING("abc",
                     tinyjson::get_string(tinyjson::get_array_element(&v, 4)),
                     tinyjson::get_string_len(tinyjson::get_array_element(&v, 4)));
    tinyjson::tiny_free(&v);

    tinyjson::tiny_init(&v);
    EXPECT_EQ_INT(tinyjson::PARSE_OK, tinyjson::parse(&v, "[ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"));
    EXPECT_EQ_INT(tinyjson::ARRAY, tinyjson::get_type(&v));
    EXPECT_EQ_SIZE_T(4, tinyjson::get_array_size(&v));
    for (i = 0; i < 4; i++) {
        tinyjson::value* a = tinyjson::get_array_element(&v, i);
        EXPECT_EQ_INT(tinyjson::ARRAY, tinyjson::get_type(a));
        EXPECT_EQ_SIZE_T(i, tinyjson::get_array_size(a));
        for (j = 0; j < i; j++) {
            tinyjson::value* e = tinyjson::get_array_element(a, j);
            EXPECT_EQ_INT(tinyjson::NUMBER, tinyjson::get_type(e));
            EXPECT_EQ_DOUBLE((double)j, tinyjson::get_number(e));
        }
    }
    tinyjson::tiny_free(&v);
}

static void test_parse_expect_value() {
    TEST_ERROR(tinyjson::PARSE_EXPECT_VALUE, "");
    TEST_ERROR(tinyjson::PARSE_EXPECT_VALUE, " ");
}

static void test_parse_invalid_value() {
    /* invalid string */
    TEST_ERROR(tinyjson::PARSE_INVALID_VALUE, "nul");
    TEST_ERROR(tinyjson::PARSE_INVALID_VALUE, "?");

    /* invalid number */
    TEST_ERROR(tinyjson::PARSE_INVALID_VALUE, "+0");
    TEST_ERROR(tinyjson::PARSE_INVALID_VALUE, "+1");
    TEST_ERROR(tinyjson::PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
    TEST_ERROR(tinyjson::PARSE_INVALID_VALUE, "1.");   /* at least one digit after '.' */
    TEST_ERROR(tinyjson::PARSE_INVALID_VALUE, "INF");
    TEST_ERROR(tinyjson::PARSE_INVALID_VALUE, "inf");
    TEST_ERROR(tinyjson::PARSE_INVALID_VALUE, "NAN");
    TEST_ERROR(tinyjson::PARSE_INVALID_VALUE, "nan");

    /* invalid value in array */
    TEST_ERROR(tinyjson::PARSE_INVALID_VALUE, "[1,]");
    TEST_ERROR(tinyjson::PARSE_INVALID_VALUE, "[\"a\", nul]");
}

static void test_parse_root_not_singular() {
    TEST_ERROR(tinyjson::PARSE_ROOT_NOT_SINGULAR, "null x");

    /* invalid number */
    TEST_ERROR(tinyjson::PARSE_ROOT_NOT_SINGULAR, "0123"); /* after zero should be '.' or nothing */
    TEST_ERROR(tinyjson::PARSE_ROOT_NOT_SINGULAR, "0x0");
    TEST_ERROR(tinyjson::PARSE_ROOT_NOT_SINGULAR, "0x123");
}

static void test_parse_number_too_big() {
    TEST_ERROR(tinyjson::PARSE_NUMBER_TOO_BIG, "1e309");
    TEST_ERROR(tinyjson::PARSE_NUMBER_TOO_BIG, "-1e309");
}

static void test_parse_missing_quotation_mark() {
    TEST_ERROR(tinyjson::PARSE_MISS_QUOTATION_MARK, "\"");
    TEST_ERROR(tinyjson::PARSE_MISS_QUOTATION_MARK, "\"abc");
}

static void test_parse_invalid_string_escape() {
    TEST_ERROR(tinyjson::PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
    TEST_ERROR(tinyjson::PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
    TEST_ERROR(tinyjson::PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
    TEST_ERROR(tinyjson::PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
}

static void test_parse_invalid_string_char() {
    TEST_ERROR(tinyjson::PARSE_INVALID_STRING_CHAR, "\"\x01\"");
    TEST_ERROR(tinyjson::PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
}

static void test_parse_invalid_unicode_hex() {
    TEST_ERROR(tinyjson::PARSE_INVALID_UNICODE_HEX, "\"\\u\"");
    TEST_ERROR(tinyjson::PARSE_INVALID_UNICODE_HEX, "\"\\u0\"");
    TEST_ERROR(tinyjson::PARSE_INVALID_UNICODE_HEX, "\"\\u01\"");
    TEST_ERROR(tinyjson::PARSE_INVALID_UNICODE_HEX, "\"\\u012\"");
    TEST_ERROR(tinyjson::PARSE_INVALID_UNICODE_HEX, "\"\\u/000\"");
    TEST_ERROR(tinyjson::PARSE_INVALID_UNICODE_HEX, "\"\\uG000\"");
    TEST_ERROR(tinyjson::PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
    TEST_ERROR(tinyjson::PARSE_INVALID_UNICODE_HEX, "\"\\u0G00\"");
    TEST_ERROR(tinyjson::PARSE_INVALID_UNICODE_HEX, "\"\\u00/0\"");
    TEST_ERROR(tinyjson::PARSE_INVALID_UNICODE_HEX, "\"\\u00G0\"");
    TEST_ERROR(tinyjson::PARSE_INVALID_UNICODE_HEX, "\"\\u000/\"");
    TEST_ERROR(tinyjson::PARSE_INVALID_UNICODE_HEX, "\"\\u000G\"");
    TEST_ERROR(tinyjson::PARSE_INVALID_UNICODE_HEX, "\"\\u 123\"");
}

static void test_parse_invalid_unicode_surrogate() {
    TEST_ERROR(tinyjson::PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
    TEST_ERROR(tinyjson::PARSE_INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
    TEST_ERROR(tinyjson::PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
    TEST_ERROR(tinyjson::PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
    TEST_ERROR(tinyjson::PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
}

static void test_parse_miss_comma_or_square_bracket() {
    TEST_ERROR(tinyjson::PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1");
    TEST_ERROR(tinyjson::PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1}");
    TEST_ERROR(tinyjson::PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1 2");
    TEST_ERROR(tinyjson::PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[[]");
}

// 测试非字符串的类型时，先set_string再去设置其他类型，这样可以查看是否调用了free函数
static void test_access_boolean() {
    tinyjson::value v;
    tiny_init(&v);
    tinyjson::set_string(&v, "a", 1);
    tinyjson::set_boolean(&v, 1);
    EXPECT_TRUE(tinyjson::get_boolean(&v));
    tinyjson::set_boolean(&v, 0);
    EXPECT_FALSE(tinyjson::get_boolean(&v));
    tinyjson::tiny_free(&v);
}

static void test_access_number() {
    tinyjson::value v;
    tiny_init(&v);
    tinyjson::set_string(&v, "a", 1);
    tinyjson::set_number(&v, 123.5);
    EXPECT_EQ_DOUBLE(123.5, tinyjson::get_number(&v));
    tinyjson::tiny_free(&v);
}

static void test_access_null() {
    tinyjson::value v;
    tiny_init(&v);
    tinyjson::set_string(&v, "a", 1);
    set_null(&v);
    EXPECT_EQ_INT(tinyjson::TINYNULL, tinyjson::get_type(&v));
    tinyjson::tiny_free(&v);
}

static void test_access_string() {
    tinyjson::value v;
    tiny_init(&v);
    tinyjson::set_string(&v, "", 0);
    EXPECT_EQ_STRING("", tinyjson::get_string(&v), tinyjson::get_string_len(&v));
    tinyjson::set_string(&v, "Hello", 5);
    EXPECT_EQ_STRING("Hello", tinyjson::get_string(&v), tinyjson::get_string_len(&v));
    tinyjson::tiny_free(&v);
}

static void test_parse() {
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_number();
    test_parse_string();
    test_parse_array();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_number_too_big();
    test_parse_missing_quotation_mark();
    test_parse_invalid_string_escape();
    test_parse_invalid_string_char();
    test_parse_invalid_unicode_hex();
    test_parse_invalid_unicode_surrogate();
    test_parse_miss_comma_or_square_bracket();
}

static void test_access() {
    test_access_null();
    test_access_string();
    test_access_boolean();
    test_access_number();
}

int main() {
#ifdef _WINDOWS
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    test_parse();
    test_access();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}