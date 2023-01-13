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
const size_t KEY_NOT_EXIST = (size_t)-1;
const double EXPAND_COEFFICIENT = 2;

// tinyjson支持的数据结构
typedef enum { TINYNULL, FALSE, TRUE, NUMBER, STRING, ARRAY, OBJECT } type;

typedef struct value value;
typedef struct member member;

struct value {
    // 使用union来节省内存空间
    union {
        struct {
            member* m;
            size_t size, capacity;
        } o;
        struct {
            value* e;
            size_t size, capacity;
        } a; // dynamic array
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

void copy(value* dst, const value* src); // 深度拷贝函数
void move(value* dst, value* src);       // 移动函数
void swap(value* lhs, value* rhs);       // 交换值函数

// JSON解析函数
int parse(value* v, const char* json);
// JSON字符串生成函数
char* stringify(const value* v, size_t* len);

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
// 获取数组容量大小
size_t get_array_capacity(const value* v);
// 获取数组元素
value* get_array_element(const value* v, size_t index);
// 设置数组，提供初始容量
void set_array(value* v, size_t capacity);
// 数组容量拓容
void array_reserve(value* v, size_t capacity);
// 数组收缩容量函数（收缩到恰好能放下所有元素）
void array_shrink(value* v);
// push_back函数，在数组末端压入一个元素并返回该元素指针
value* array_pushback(value* v);
// pop_back函数
void array_popback(value* v);
// insert函数
value* array_insert(value* v, size_t index);
// erase函数，删除从index开始的count个元素
void array_erase(value* v, size_t index, size_t count);
// clear函数，清除所有元素，但不更改容量
void array_clear(value* v);

// 获取object大小
size_t get_object_size(const value* v);
// 获取object的键
const char* get_object_key(const value* v, size_t index);
// 获取object键的长度
size_t get_object_key_length(const value* v, size_t index);
// 获取object对应的json_value指针
value* get_object_value(const value* v, size_t index);
// 设置object，提供初始容量
void set_object(value* v, size_t capacity);
// 获取object的容量
size_t get_object_capacity(const value* v);
// object容量拓容
void object_reserve(value* v, size_t capacity);
// object收缩容量函数（收缩到恰好能放下所有元素）
void object_shrink(value* v);
// clear函数，清除所有元素，但不更改容量
void object_clear(value* v);
// 修改/新增object中的value，返回新增键值对的指针
value* set_object_value(value* v, char* key, size_t klen);
// remove函数，删除object中的value
void remove_object_value(value* v, size_t index);
// 查找key对应的index，线性查找
size_t find_object_index(const value* v, const char* key, size_t klen);
// 辅助函数，查找对应的value
value* find_object_value(value* v, const char* key, size_t klen);

// 内存释放函数
void tiny_free(value* v);
inline void set_null(value* v) { tiny_free(v); }

// 比较函数，比较两个value是否相等
int is_equal(const value* lhs, const value* rhs);

} // namespace tinyjson

#endif