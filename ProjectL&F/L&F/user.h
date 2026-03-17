#ifndef USER_H
#define USER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 用户角色定义
typedef enum {
    ROLE_SUPER_ADMIN = 0,  // 超级管理员
    ROLE_ADMIN,            // 管理员
    ROLE_STUDENT,           // 学生
    ROLE_NONE = -1
} UserRole;

// 用户结构体
typedef struct {
    char username[32];
    char password[64];     // 明文存储（后续可替换为哈希）
    UserRole role;
} User;


// 函数声明（供C++调用的C接口）
#ifdef __cplusplus
extern "C" {
#endif

    //全局变量声明
    extern int user_count;
    extern User users[100];
    extern char current_operator[32]; // 当前操作员用户名

    // 初始化用户系统（可从文件加载）
    int init_user_system(void);

    // 添加用户（仅超级管理员可调用此功能）
    int add_user(const char* username, const char* password, UserRole role, const char* operator_username);

    // 删除用户（仅超级管理员可调用）
    int delete_user(const char* username, const char* operator_username);

    // 用户登录验证
    int authenticate_user(const char* username, const char* password);

    // 查询用户角色
    UserRole get_user_role(const char* username);

    // 保存用户数据（可选：持久化到文件）
    int save_users(void);

    // 清理资源
    void cleanup_user_system(void);

    // 加载用户
    int load_users(void);

#ifdef __cplusplus
};
#endif

#endif // USER_H