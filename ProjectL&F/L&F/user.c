// user.c
#define _CRT_SECURE_NO_WARNINGS
#include "user.h"
#include <stdio.h>

#define MAX_USERS 100
User users[MAX_USERS];
int user_count = 0;

// 模拟当前登录的超级管理员（实际中应由会话管理）
char current_operator[32] = "";

// 辅助函数：查找用户索引
static int find_user_index(const char* username) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            return i;
        }
    }
    return -1;
}

// 从文件加载用户数据
int load_users(void) {
    FILE* fp = fopen("users.dat", "rb");
    if (!fp) {
        return 0; // 文件不存在或无法打开
    }

    // 读取用户数量
    if (fread(&user_count, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return 0;
    }

    if (user_count < 0 || user_count > MAX_USERS) {
        user_count = 0;
        fclose(fp);
        return 0;
    }

    // 读取用户数组
    if (fread(users, sizeof(User), user_count, fp) != user_count) {
        user_count = 0;
        fclose(fp);
        return 0;
    }

    fclose(fp);
    return 1; // 加载成功
}

// 初始化用户系统（可加载默认用户，如超级管理员）
int init_user_system(void) {
    // 尝试从文件加载用户
    if (load_users()) {
        return 1; // 成功加载，无需初始化
    }

    // 文件加载失败：初始化默认超级管理员
    user_count = 0;
    strcpy(users[user_count].username, "sa");
    strcpy(users[user_count].password, "sa123");
    users[user_count].role = ROLE_SUPER_ADMIN;
    user_count++;       

    // 创建后立即保存，确保文件存在
    save_users();
    return 1;
}


// 添加用户：仅当操作者是超级管理员时允许
int add_user(const char* username, const char* password, UserRole role, const char* operator_username) {
    if (!username || !password || !operator_username) return 0;
    if (strlen(username) >= 32 || strlen(password) >= 64) return 0;

    // 检查用户名是否已存在
    if (find_user_index(username) != -1) return 0;

    // 检查操作者是否存在且为超级管理员
    int op_idx = find_user_index(operator_username);
    if (op_idx == -1) return 0;
    if (users[op_idx].role != ROLE_SUPER_ADMIN) return 0;

    // 角色限制：不允许添加超级管理员（防止权限扩散）
    if (role == ROLE_SUPER_ADMIN) return 0;

    // 添加新用户
    if (user_count >= MAX_USERS) return 0;

    strcpy(users[user_count].username, username);
    strcpy(users[user_count].password, password);
    users[user_count].role = role;
    user_count++;

    save_users(); // 可选持久化
    return 1;
}

// 删除用户：仅超级管理员可删除管理员或学生
int delete_user(const char* username, const char* operator_username) {
    if (!username || !operator_username) return 0;

    int op_idx = find_user_index(operator_username);
    if (op_idx == -1 || users[op_idx].role != ROLE_SUPER_ADMIN) return 0;

    int idx = find_user_index(username);
    if (idx == -1) return 0;

    // 不允许删除超级管理员（包括自己）
    if (users[idx].role == ROLE_SUPER_ADMIN) return 0;

    // 移动数组删除
    for (int i = idx; i < user_count - 1; i++) {
        users[i] = users[i + 1];
    }
    user_count--;
    save_users();
    return 1;
}

// 用户登录验证
int authenticate_user(const char* username, const char* password) {
    int idx = find_user_index(username);
    if (idx == -1) return 0;
    return strcmp(users[idx].password, password) == 0;
}

// 获取用户角色
UserRole get_user_role(const char* username) {
    int idx = find_user_index(username);
    if (idx == -1) return -1;
    return users[idx].role;
}

// 保存用户到文件（可选）
int save_users(void) {
    FILE* fp = fopen("users.dat", "wb");
    if (!fp) return 0;
    fwrite(&user_count, sizeof(int), 1, fp);
    fwrite(users, sizeof(User), user_count, fp);
    fclose(fp);
    return 1;
}

// 加载用户（可扩展）
// int load_users(void) { ... }

void cleanup_user_system(void) {
    // 当前无动态内存，可留空
    return;
}