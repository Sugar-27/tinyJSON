/*
 * @Author: Sugar 45682h@gmail.com
 * @Date: 2022-12-30 11:38:35
 * @Describe:
 */
#ifndef __TINYJSON_H
#define __TINYJSON_H

namespace tinyjson {
// tinyjson支持的数据结构
typedef enum { TINYNULL, FALSE, TRUE, NUMBER, STRING, ARRAY, OBJECT } type;

typedef struct {
    double n;
    type tiny_type;
} value;

// 返回值定义
enum { PARSE_OK = 0, PARSE_EXPECT_VALUE, PARSE_INVALID_VALUE, PARSE_ROOT_NOT_SINGULAR, PARSE_NUMBER_TOO_BIG };

// JSON解析函数
int parse(value* v, const char* json);

// 访问结果的函数，用于获取类型
type get_type(const value* v);

// 访问数值，仅当类型为tinyjson::NUMBER时才有效
double get_number(const value* v);

} // namespace tinyjson

#endif