<!--
 * @Author: Sugar 45682h@gmail.com
 * @Date: 2022-12-30 11:20:09
 * @Describe: 
-->
# 一个简单快捷的JSON库

## 初衷
实习过程中大量使用JSON的相关库进行序列化与反序列化，但大多数时候使用的是第三方库，希望可以做一个自己的JSON库来更深入地学习以及了解相关的JSON原理

## 目标
* 符合标准的 JSON 解析器和生成器
* 手写递归下降解析器（recursive descent parser）
* 使用标准 C 语言（C89）
* 跨平台／编译器（如 Windows／Linux／OS X，vc／gcc／clang）
* 支持 UTF-8 JSON 文本
* 支持以 double 存储 JSON number 类型

## 代码
### tiny_json.h
tinyJSON的头文件，包含对外的类型和 API 函数声明
### tiny_json.c
tinyJSON的实现文件，含有内部的类型声明和函数实现，此文件最终会编译成库
### test.c
使用测试驱动开发（test driven development, TDD），此文件包含测试程序，需要链接 `tinyJSON` 库