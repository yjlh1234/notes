/**
 * @file hello.c
 * @brief Hello World 示例程序
 * @author 咯哩
 * @date 2026-03-13
 * 
 * 这是一个简单的 C 语言入门示例，用于演示：
 * - 标准输入输出库的使用
 * - main 函数的基本结构
 * - 编译和运行流程
 */

#include <stdio.h>  // 标准输入输出库，提供 printf 等函数

/**
 * @brief 程序入口函数
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 程序执行状态码，0 表示成功
 */
int main(int argc, const char * argv[]) {
    // 输出问候语
    printf("Hello, World! 🌶️\n");
    
    // 输出来源信息
    printf("From 咯哩's notes repository.\n");
    
    // 返回成功状态码
    return 0;
}
