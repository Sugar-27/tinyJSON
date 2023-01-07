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

// tinyjson支持的数据结构
typedef enum { TINYNULL, FALSE, TRUE, NUMBER, STRING, ARRAY, OBJECT } type;

typedef struct value value;
typedef struct member member;

struct value {
    // 使用union来节省内存空间
    union {
        struct {
            member* m;
            size_t size;
        } o;
        struct {
            value* e;
            size_t size;
        } a; // array
        struct {
            char* s;
            size_t len;
        } s;      // string
        double n; // number
    } u;
    type tiny_type;
};

struct member {
    char* k;
    size_t klen;
    value v;
};

// 返回值定义
enum {
    PARSE_OK = 0,
    PARSE_EXPECT_VALUE,
    PARSE_INVALID_VALUE,
    PARSE_ROOT_NOT_SINGULAR,
    PARSE_NUMBER_TOO_BIG,
    PARSE_MISS_QUOTATION_MARK,
    PARSE_INVALID_STRING_ESCAPE,
    PARSE_INVALID_STRING_CHAR,
    PARSE_INVALID_UNICODE_HEX,
    PARSE_INVALID_UNICODE_SURROGATE,
    PARSE_MISS_COMMA_OR_SQUARE_BRACKET,
    PARSE_MISS_KEY,
    PARSE_MISS_COLON,
    PARSE_MISS_COMMA_OR_CURLY_BRACKET
};

inline void tiny_init(value* v) { v->tiny_type = TINYNULL; }

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
// 获取数组大小
size_t get_array_size(const value* v);
// 获取数组元素
value* get_array_element(const value* v, size_t index);
// 获取json对象大小
size_t get_object_size(const value* v);
// 获取json对象的键
const char* get_object_key(const value* v, size_t index);
// 获取json对象键的长度
size_t get_object_key_length(const value* v, size_t index);
// 获取json对象对应的json_value指针
value* get_object_value(const value* v, size_t index);

// 内存释放函数
void tiny_free(value* v);
inline void set_null(value* v) { tiny_free(v); }

} // namespace tinyjson

#endif