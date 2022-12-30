/*
 * @Author: Sugar 45682h@gmail.com
 * @Date: 2022-12-30 11:38:35
 * @Describe:
 */
#ifndef __TINYJSON_H
#define __TINYJSON_H

#include <cassert>
#include <cstddef>

namespace tinyjson {
const int PARSE_STACK_INIT_SIZE = 256;

#define tiny_init(v)                                                                                                   \
    do {                                                                                                               \
        (v)->tiny_type = tinyjson::TINYNULL;                                                                           \
    } while (0)

#define set_null(v) tinyjson::tiny_free(v)

// tinyjson支持的数据结构
typedef enum { TINYNULL, FALSE, TRUE, NUMBER, STRING, ARRAY, OBJECT } type;

typedef struct {
    // 使用union来节省内存空间
    union {
        struct {
            char* s;
            size_t len;
        } s;      // string
        double n; // number
    } u;
    type tiny_type;
} value;

// 返回值定义
enum {
    PARSE_OK = 0,
    PARSE_EXPECT_VALUE,
    PARSE_INVALID_VALUE,
    PARSE_ROOT_NOT_SINGULAR,
    PARSE_NUMBER_TOO_BIG,
    PARSE_MISS_QUOTATION_MARK,
    PARSE_INVALID_STRING_ESCAPE,
    PARSE_INVALID_STRING_CHAR
};

// JSON解析函数
int parse(value* v, const char* json);

// 访问结果的相关函数
// 获取类型
type get_type(const value* v);
// 访问数值，仅当类型为tinyjson::NUMBER时才有效
double get_number(const value* v);
void set_number(value* v, double n);
// 访问布尔属性
int get_boolean(const value* v);
void set_boolean(value* v, int b);
// 访问字符串属性
const char* get_string(const value* v);
size_t get_string_len(const value* v);
// 动态分配内存并将字符串存入到value中
void set_string(value* v, const char* s, size_t len);

// 内存释放函数
void tiny_free(value* v);

} // namespace tinyjson

#endif