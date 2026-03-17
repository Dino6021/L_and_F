#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <direct.h>
#include <math.h>

// 定义文件夹名称
#define RECORDS_FOLDER "lost_and_found_records"  // 失物招领记录文件夹
#define SUMMARY_FOLDER "weekly_summaries"    // 每周总结文件夹
#define AUCTION_FOLDER "auction_preparation"  // 拍卖准备文件夹
#define USERS_FILE "users.dat"  // 用户数据文件

// 定义用户类型
#define SUPER_ADMIN 0   // 超级管理员
#define ADMIN 1         // 普通管理员
#define STUDENT 2       // 学生用户

// 物品结构体
typedef struct {
    char category[30];       // 手动输入的物品类别
    char name[50];           // 物品名称
    int id;                  // 物品ID
    char model[50];          // 型号/特征
    char description[200];   // 物品描述
    char location[100];      // 发现地点
    char lost_date[20];      // 遗失日期时间(YYYY-MM-DD HH:MM)
    char finder[20];         // 登记人
    int confirmed;           // 0-未确认，1-已确认

    // 认领信息
    int claimed;             // 0-未认领，1-已认领
    char claimant_id[20];    // 认领人学号
    char claimant_phone[20]; // 认领人电话
    char claim_date[20];     // 认领日期时间(YYYY-MM-DD HH:MM)
} LostItem;

// 用户结构体
typedef struct {
    char username[20];
    char password[20];
    int type;  // 0-超级管理员，1-普通管理员，2-学生
} User;

// 全局变量
LostItem items[1000];  // 最多存储1000个物品
int itemCount = 0;
User users[50] = {     // 预设用户
    {"superadmin", "super123", SUPER_ADMIN}
};
int userCount = 1;

// 函数声明
void initialInterface();   // 初始界面
User* login();// 登录
void createNecessaryFolders();// 创建必要文件夹
void loadAllData();// 加载所有数据
void saveItemToFile(LostItem* item);// 保存物品到文件（核心修改）
void deleteItemFile(const char* dateStr, int id);// 删除物品文件（核心修改）
void registerItem(User* user);// 登记物品
void confirmItem(User* admin);// 确认物品
void deleteItem(User* admin);// 删除物品
void queryItems();// 查询物品
void sortItems();// 物品排序
void claimItem(User* user);// 物品认领
void getCurrentDateTime(char* datetime);// 获取当前日期时间
void trimWhitespace(char* str);// 去除字符串空格
void buildFilePath(const char* folder, const char* filename, char* fullPath);// 构建文件路径
void manageUsers(User* superAdmin);// 管理用户
int isUsernameExists(const char* username);// 检查用户名是否存在
void mainMenu(User* currentUser);// 主菜单
int daysBetweenDates(const char* date1, const char* date2);// 计算日期差
void generateWeeklySummary();// 生成每周汇总
void prepareAuctionItems();// 准备拍卖物品
int parseDateTime(const char* dateStr, struct tm* tm);// 解析日期时间
void saveUsersData();// 保存用户数据
void loadUsersData();// 加载用户数据
int getNextItemId();// 获取下一个物品ID
void saveAllItems();// 保存所有物品
void extractDatePart(const char* datetime, char* datePart);// 提取日期部分（新增辅助函数）

int main() {
    createNecessaryFolders();
    loadUsersData();  // 加载用户数据
    loadAllData();
    initialInterface();
    return 0;
}

// 初始界面
void initialInterface() {
    int choice;
    while (1) {
        printf("\n===== 机房失物招领系统 =====\n");
        printf("1. 登录账号\n");
        printf("2. 退出程序\n");
        printf("请选择操作: ");

        if (scanf("%d", &choice) != 1) {
            printf("输入错误，请重新输入数字！\n");
            while (getchar() != '\n');
            continue;
        }
        getchar();

        switch (choice) {
        case 1: {
            while (1) {
                User* currentUser = login();
                if (currentUser != NULL) {
                    mainMenu(currentUser);
                    break;
                }
                else {
                    printf("是否重新登录？(1-是 0-否): ");
                    int retry;
                    if (scanf("%d", &retry) == 1 && retry == 0) break;
                    getchar();
                }
            }
            break;
        }
        case 2:
            saveUsersData();  // 退出前保存用户数据
            saveAllItems();   // 退出前保存所有物品数据
            printf("谢谢使用，再见！\n");
            exit(0);
        default:
            printf("无效的选择，请重新输入！\n");
        }
    }
}

// 日期时间解析函数（YYYY-MM-DD HH:MM）
int parseDateTime(const char* dateStr, struct tm* tm) {
    if (strlen(dateStr) != 16) return 0; // 格式必须是YYYY-MM-DD HH:MM
    if (dateStr[4] != '-' || dateStr[7] != '-' || dateStr[10] != ' ' || dateStr[13] != ':') return 0;

    // 提取年、月、日、时、分
    char yearStr[5], monthStr[3], dayStr[3];
    char hourStr[3], minStr[3];

    strncpy(yearStr, dateStr, 4);
    yearStr[4] = '\0';
    strncpy(monthStr, dateStr + 5, 2);
    monthStr[2] = '\0';
    strncpy(dayStr, dateStr + 8, 2);
    dayStr[2] = '\0';
    strncpy(hourStr, dateStr + 11, 2);
    hourStr[2] = '\0';
    strncpy(minStr, dateStr + 14, 2);
    minStr[2] = '\0';

    // 转换为数字
    int year = atoi(yearStr);
    int month = atoi(monthStr);
    int day = atoi(dayStr);
    int hour = atoi(hourStr);
    int min = atoi(minStr);

    // 简单验证
    if (year < 2000 || year > 2100 || month < 1 || month > 12 || day < 1 || day > 31 ||
        hour < 0 || hour > 23 || min < 0 || min > 59) {
        return 0;
    }

    // 填充tm结构体
    tm->tm_year = year - 1900; // tm_year是从1900开始的年数
    tm->tm_mon = month - 1;    // tm_mon是0-11
    tm->tm_mday = day;
    tm->tm_hour = hour;
    tm->tm_min = min;
    tm->tm_sec = 0;
    tm->tm_isdst = -1; // 自动处理夏令时

    return 1;
}

// 计算两个日期之间的天数差（date2 - date1）
int daysBetweenDates(const char* date1, const char* date2) {
    struct tm tm1 = { 0 }, tm2 = { 0 };

    // 只取日期部分进行比较
    char date1Short[11], date2Short[11];
    strncpy(date1Short, date1, 10);
    date1Short[10] = '\0';
    strncpy(date2Short, date2, 10);
    date2Short[10] = '\0';

    // 构建临时的日期时间字符串用于解析
    char date1Full[17], date2Full[17];
    sprintf(date1Full, "%s 00:00", date1Short);
    sprintf(date2Full, "%s 00:00", date2Short);

    if (!parseDateTime(date1Full, &tm1) || !parseDateTime(date2Full, &tm2)) {
        return 0; // 日期解析错误
    }

    time_t t1 = mktime(&tm1);
    time_t t2 = mktime(&tm2);

    if (t1 == -1 || t2 == -1) {
        return 0;
    }

    double diff = difftime(t2, t1);
    return (int)(diff / (60 * 60 * 24));
}

// 计算两个日期时间之间的分钟差（date2 - date1）
int minutesBetweenDatetimes(const char* datetime1, const char* datetime2) {
    struct tm tm1 = { 0 }, tm2 = { 0 };

    if (!parseDateTime(datetime1, &tm1) || !parseDateTime(datetime2, &tm2)) {
        return 0; // 日期解析错误
    }

    time_t t1 = mktime(&tm1);
    time_t t2 = mktime(&tm2);

    if (t1 == -1 || t2 == -1) {
        return 0;
    }

    double diff = difftime(t2, t1);
    return (int)(diff / 60);
}

// 创建所有必要的文件夹
void createNecessaryFolders() {
#ifdef _WIN32
    _mkdir(RECORDS_FOLDER);
    _mkdir(SUMMARY_FOLDER);
    _mkdir(AUCTION_FOLDER);
#else
    mkdir(RECORDS_FOLDER, 0755);
    mkdir(SUMMARY_FOLDER, 0755);
    mkdir(AUCTION_FOLDER, 0755);
#endif
}

// 构建完整文件路径
void buildFilePath(const char* folder, const char* filename, char* fullPath) {
#ifdef _WIN32
    sprintf(fullPath, "%s\\%s", folder, filename);
#else
    sprintf(fullPath, "%s/%s", folder, filename);
#endif
}

// 生成上周物品汇总
void generateWeeklySummary() {
    char today[20], startDate[20];
    time_t now;
    struct tm* timeinfo;

    getCurrentDateTime(today);
    time(&now);
    timeinfo = localtime(&now);
    timeinfo->tm_mday -= 7;
    mktime(timeinfo);
    strftime(startDate, 20, "%Y-%m-%d %H:%M", timeinfo);

    printf("\n===== 生成上周物品汇总 =====\n");
    printf("汇总日期范围: %s 至 %s\n", startDate, today);

    int count = 0;
    LostItem weeklyItems[1000];

    for (int i = 0; i < itemCount; i++) {
        if (items[i].confirmed &&
            daysBetweenDates(items[i].lost_date, startDate) <= 0 &&
            daysBetweenDates(items[i].lost_date, today) >= 0) {
            weeklyItems[count++] = items[i];
        }
    }

    if (count == 0) {
        printf("上周没有记录在案的物品！\n");
        return;
    }

    char filename[30], fullPath[100];
    timeinfo = localtime(&now);
    timeinfo->tm_mday -= (timeinfo->tm_wday == 0 ? 0 : timeinfo->tm_wday);
    strftime(filename, 30, "summary_%Y-%m-%d.txt", timeinfo);
    buildFilePath(SUMMARY_FOLDER, filename, fullPath);

    FILE* file = fopen(fullPath, "w");
    if (file == NULL) {
        printf("创建汇总文件失败！路径: %s\n", fullPath);
        return;
    }

    fprintf(file, "===== 机房失物招领上周汇总 %s 至 %s =====\n\n", startDate, today);
    fprintf(file, "共 %d 件物品\n\n", count);

    for (int i = 0; i < count; i++) {
        fprintf(file, "物品ID: %d\n", weeklyItems[i].id);
        fprintf(file, "名称: %s\n", weeklyItems[i].name);
        fprintf(file, "类别: %s\n", weeklyItems[i].category);
        fprintf(file, "特征: %s\n", weeklyItems[i].model);
        fprintf(file, "发现地点: %s\n", weeklyItems[i].location);
        fprintf(file, "发现日期时间: %s\n", weeklyItems[i].lost_date);
        fprintf(file, "状态: %s\n", weeklyItems[i].claimed ? "已认领" : "未认领");
        fprintf(file, "----------------------------------------\n");
    }

    fclose(file);
    printf("上周物品汇总已生成: %s\n", fullPath);
}

// 准备拍卖物品清单
void prepareAuctionItems() {
    char today[20], oneYearAgo[20];
    time_t now;
    struct tm* timeinfo;

    getCurrentDateTime(today);
    time(&now);
    timeinfo = localtime(&now);
    timeinfo->tm_year -= 1;
    mktime(timeinfo);
    strftime(oneYearAgo, 20, "%Y-%m-%d %H:%M", timeinfo);

    printf("\n===== 生成待拍卖物品清单 =====\n");
    printf("筛选条件: %s 之前丢失且未认领的物品\n", oneYearAgo);

    int count = 0;
    LostItem auctionItems[1000];

    for (int i = 0; i < itemCount; i++) {
        if (items[i].confirmed &&
            items[i].claimed == 0 &&
            daysBetweenDates(items[i].lost_date, oneYearAgo) >= 0) {
            auctionItems[count++] = items[i];
        }
    }

    if (count == 0) {
        printf("没有符合拍卖条件的物品！\n");
        return;
    }

    char filename[30], fullPath[100];
    timeinfo = localtime(&now);
    strftime(filename, 30, "auction_%Y-%m.txt", timeinfo);
    buildFilePath(AUCTION_FOLDER, filename, fullPath);

    FILE* file = fopen(fullPath, "w");
    if (file == NULL) {
        printf("创建拍卖准备文件失败！路径: %s\n", fullPath);
        return;
    }

    fprintf(file, "===== 机房失物招领待拍卖物品清单 %s =====\n\n", today);
    fprintf(file, "以下物品已超过一年无人认领，准备进行网络拍卖:\n");
    fprintf(file, "共 %d 件物品\n\n", count);

    for (int i = 0; i < count; i++) {
        fprintf(file, "物品ID: %d\n", auctionItems[i].id);
        fprintf(file, "名称: %s\n", auctionItems[i].name);
        fprintf(file, "类别: %s\n", auctionItems[i].category);
        fprintf(file, "特征: %s\n", auctionItems[i].model);
        fprintf(file, "发现地点: %s\n", auctionItems[i].location);
        fprintf(file, "发现日期时间: %s\n", auctionItems[i].lost_date);
        fprintf(file, "存放时间: %d 天\n", daysBetweenDates(today, auctionItems[i].lost_date));
        fprintf(file, "----------------------------------------\n");
    }

    fprintf(file, "\n拍卖所得将捐赠给希望工程\n");
    fclose(file);
    printf("待拍卖物品清单已生成: %s\n", fullPath);
}

// 用户登录
User* login() {
    char username[20], password[20];
    int i;

    printf("\n===== 用户登录 =====\n");
    printf("请输入用户名: ");
    if (fgets(username, 20, stdin) != NULL) {
        trimWhitespace(username);
    }
    else {
        return NULL;
    }

    printf("请输入密码: ");
    if (fgets(password, 20, stdin) != NULL) {
        trimWhitespace(password);
    }
    else {
        return NULL;
    }

    for (i = 0; i < userCount; i++) {
        if (strcmp(users[i].username, username) == 0 &&
            strcmp(users[i].password, password) == 0) {
            printf("登录成功！欢迎您，");
            if (users[i].type == SUPER_ADMIN) printf("超级管理员");
            else if (users[i].type == ADMIN) printf("管理员");
            else printf("值日同学");
            printf("\n");
            return &users[i];
        }
    }

    printf("用户名或密码错误！\n");
    return NULL;
}

void mainMenu(User* currentUser) {
    int choice;

    while (1) {
        printf("\n===== 机房失物招领系统 =====\n");
        int base = 1;

        printf("%d. 登记物品\n", base++);

        if (currentUser->type == SUPER_ADMIN || currentUser->type == ADMIN) {
            printf("%d. 确认物品\n", base++);
            printf("%d. 删除物品\n", base++);
            printf("%d. 生成上周物品汇总\n", base++);
            printf("%d. 生成待拍卖物品清单\n", base++);
        }

        if (currentUser->type == SUPER_ADMIN) {
            printf("%d. 用户管理\n", base++);
        }

        printf("%d. 查询物品\n", base++);
        printf("%d. 物品排序\n", base++);
        printf("%d. 物品认领\n", base++);
        printf("%d. 退出登录\n", base++);
        printf("0. 退出程序\n");

        printf("请选择操作: ");

        if (scanf("%d", &choice) != 1) {
            printf("输入错误，请重新输入数字！\n");
            while (getchar() != '\n');
            continue;
        }
        getchar();

        if (currentUser->type == SUPER_ADMIN) {
            switch (choice) {
            case 1: registerItem(currentUser); break;
            case 2: confirmItem(currentUser); break;
            case 3: deleteItem(currentUser); break;
            case 4: generateWeeklySummary(); break;
            case 5: prepareAuctionItems(); break;
            case 6: manageUsers(currentUser); break;
            case 7: queryItems(); break;
            case 8: sortItems(); break;
            case 9: claimItem(currentUser); break;
            case 10:
                printf("已退出登录！\n");
                return;
            case 0:
                saveUsersData();  // 退出前保存用户数据
                saveAllItems();   // 退出前保存所有物品数据
                printf("谢谢使用，再见！\n");
                exit(0);
            default:
                printf("无效的选择，请重新输入！\n");
            }
        }
        else if (currentUser->type == ADMIN) {
            switch (choice) {
            case 1: registerItem(currentUser); break;
            case 2: confirmItem(currentUser); break;
            case 3: deleteItem(currentUser); break;
            case 4: generateWeeklySummary(); break;
            case 5: prepareAuctionItems(); break;
            case 6: queryItems(); break;
            case 7: sortItems(); break;
            case 8: claimItem(currentUser); break;
            case 9:
                printf("已退出登录！\n");
                return;
            case 0:
                saveUsersData();  // 退出前保存用户数据
                saveAllItems();   // 退出前保存所有物品数据
                printf("谢谢使用，再见！\n");
                exit(0);
            default:
                printf("无效的选择，请重新输入！\n");
            }
        }
        else {  // 学生用户
            switch (choice) {
            case 1: registerItem(currentUser); break;
            case 2: queryItems(); break;
            case 3: sortItems(); break;
            case 4: claimItem(currentUser); break;
            case 5:
                printf("已退出登录！\n");
                return;
            case 0:
                saveUsersData();  // 退出前保存用户数据
                saveAllItems();   // 退出前保存所有物品数据
                printf("谢谢使用，再见！\n");
                exit(0);
            default:
                printf("无效的选择，请重新输入！\n");
            }
        }
    }
}

void trimWhitespace(char* str) {
    if (str == NULL) return;

    int start = 0;
    while (isspace((unsigned char)str[start])) start++;

    int end = strlen(str) - 1;
    while (end >= start && isspace((unsigned char)str[end])) end--;

    if (start > 0) memmove(str, str + start, end - start + 1);
    str[end - start + 1] = '\0';
}

// 获取当前日期时间（精确到分钟）
void getCurrentDateTime(char* datetime) {
    time_t now;
    struct tm* timeinfo;

    time(&now);
    timeinfo = localtime(&now);
    strftime(datetime, 20, "%Y-%m-%d %H:%M", timeinfo);
}

// 保存用户数据到文件
void saveUsersData() {
    FILE* file = fopen(USERS_FILE, "wb");
    if (file == NULL) {
        printf("保存用户数据失败！\n");
        return;
    }

    fwrite(&userCount, sizeof(int), 1, file);
    fwrite(users, sizeof(User), userCount, file);
    fclose(file);
}

// 从文件加载用户数据
void loadUsersData() {
    FILE* file = fopen(USERS_FILE, "rb");
    if (file == NULL) {
        // 如果文件不存在，使用默认用户
        printf("未找到用户数据，使用默认用户\n");
        return;
    }

    fread(&userCount, sizeof(int), 1, file);
    if (userCount > 50) userCount = 50; // 防止数据错误
    fread(users, sizeof(User), userCount, file);
    fclose(file);
    printf("成功加载 %d 个用户数据\n", userCount);
}

void manageUsers(User* superAdmin) {
    if (superAdmin->type != SUPER_ADMIN) {
        printf("权限不足，只有超级管理员可以管理用户！\n");
        return;
    }

    int choice;
    while (1) {
        printf("\n===== 用户管理 =====\n");
        printf("1. 查看所有用户\n");
        printf("2. 添加新管理员\n");
        printf("3. 添加新学生\n");
        printf("4. 删除用户\n");
        printf("0. 返回主菜单\n");
        printf("请选择操作: ");

        if (scanf("%d", &choice) != 1) {
            printf("输入错误，请重新输入数字！\n");
            while (getchar() != '\n');
            continue;
        }
        getchar();

        switch (choice) {
        case 1:
            printf("\n===== 所有用户列表 =====\n");
            printf("序号\t用户名\t\t用户类型\n");
            printf("--------------------------------\n");
            for (int i = 0; i < userCount; i++) {
                printf("%d\t%s\t\t", i + 1, users[i].username);
                if (users[i].type == SUPER_ADMIN) printf("超级管理员\n");
                else if (users[i].type == ADMIN) printf("普通管理员\n");
                else printf("学生\n");
            }
            break;

        case 2:
            if (userCount >= 50) {
                printf("用户数量已达上限！\n");
                break;
            }

            {
                char username[20], password[20];
                printf("\n===== 添加新管理员 =====\n");
                printf("请输入用户名: ");
                if (fgets(username, 20, stdin) != NULL) trimWhitespace(username);
                else { printf("输入错误！\n"); break; }

                if (isUsernameExists(username)) {
                    printf("用户名已存在！\n");
                    break;
                }

                printf("请输入密码: ");
                if (fgets(password, 20, stdin) != NULL) trimWhitespace(password);
                else { printf("输入错误！\n"); break; }

                strcpy(users[userCount].username, username);
                strcpy(users[userCount].password, password);
                users[userCount].type = ADMIN;
                userCount++;
                saveUsersData();
                printf("新管理员添加成功！\n");
            }
            break;

        case 3:
            if (userCount >= 50) {
                printf("用户数量已达上限！\n");
                break;
            }

            {
                char username[20], password[20];
                printf("\n===== 添加新学生 =====\n");
                printf("请输入用户名: ");
                if (fgets(username, 20, stdin) != NULL) trimWhitespace(username);
                else { printf("输入错误！\n"); break; }

                if (isUsernameExists(username)) {
                    printf("用户名已存在！\n");
                    break;
                }

                printf("请输入密码: ");
                if (fgets(password, 20, stdin) != NULL) trimWhitespace(password);
                else { printf("输入错误！\n"); break; }

                strcpy(users[userCount].username, username);
                strcpy(users[userCount].password, password);
                users[userCount].type = STUDENT;
                userCount++;
                saveUsersData();
                printf("新学生添加成功！\n");
            }
            break;

        case 4:
        {
            int idx;
            printf("\n===== 删除用户 =====\n");
            printf("请输入要删除的用户序号: ");
            if (scanf("%d", &idx) != 1) {
                printf("输入错误！\n");
                while (getchar() != '\n');
                break;
            }
            getchar();

            if (idx < 1 || idx > userCount) {
                printf("无效的用户序号！\n");
                break;
            }

            // 不能删除超级管理员
            if (users[idx - 1].type == SUPER_ADMIN) {
                printf("不能删除超级管理员！\n");
                break;
            }

            // 移动数组元素覆盖要删除的用户
            for (int i = idx - 1; i < userCount - 1; i++) {
                users[i] = users[i + 1];
            }
            userCount--;
            saveUsersData();
            printf("用户删除成功！\n");
        }
        break;

        case 0:
            return;

        default:
            printf("无效的选择，请重新输入！\n");
        }
    }
}

int isUsernameExists(const char* username) {
    for (int i = 0; i < userCount; i++) {
        if (strcmp(users[i].username, username) == 0) {
            return 1;
        }
    }
    return 0;
}

// 获取下一个物品ID
int getNextItemId() {
    int maxId = 0;
    for (int i = 0; i < itemCount; i++) {
        if (items[i].id > maxId) {
            maxId = items[i].id;
        }
    }
    return maxId + 1;
}

// 新增辅助函数：从 datetime（YYYY-MM-DD HH:MM）中提取日期部分（YYYY-MM-DD）
void extractDatePart(const char* datetime, char* datePart) {
    if (strlen(datetime) < 10) {
        strcpy(datePart, "unknown_date");
        return;
    }
    strncpy(datePart, datetime, 10);
    datePart[10] = '\0';
}

// 保存物品到文件：核心修改——按丢失时间命名文件（如2018-01-01.txt）
void saveItemToFile(LostItem* item) {
    char datePart[11];
    extractDatePart(item->lost_date, datePart); // 提取丢失日期（YYYY-MM-DD）

    // 构建文件名：丢失日期.txt
    char filename[30], fullPath[100];
    sprintf(filename, "%s.txt", datePart);
    buildFilePath(RECORDS_FOLDER, filename, fullPath);

    // 以追加模式打开文件（同一日期的物品保存在同一个文件中）
    FILE* file = fopen(fullPath, "ab");
    if (file == NULL) {
        printf("保存物品数据失败！路径: %s\n", fullPath);
        return;
    }

    fwrite(item, sizeof(LostItem), 1, file);
    fclose(file);
    printf("物品信息已保存到 %s\n", fullPath);
}

// 删除物品文件：核心修改——按丢失时间定位文件
void deleteItemFile(const char* dateStr, int id) {
    char datePart[11];
    extractDatePart(dateStr, datePart); // 提取物品对应的丢失日期

    // 构建文件路径：丢失日期.txt
    char filename[30], fullPath[100];
    sprintf(filename, "%s.txt", datePart);
    buildFilePath(RECORDS_FOLDER, filename, fullPath);

    // 步骤1：读取文件中所有物品，排除要删除的物品
    FILE* file = fopen(fullPath, "rb");
    if (file == NULL) {
        printf("警告：未找到物品对应的文件！\n");
        return;
    }

    LostItem tempItems[1000];
    int tempCount = 0;
    LostItem temp;
    while (fread(&temp, sizeof(LostItem), 1, file) == 1) {
        if (temp.id != id) { // 保留非目标ID的物品
            tempItems[tempCount++] = temp;
        }
    }
    fclose(file);

    // 步骤2：重新写入文件（覆盖原文件，不含已删除物品）
    file = fopen(fullPath, "wb");
    if (file == NULL) {
        printf("警告：更新文件失败！\n");
        return;
    }
    for (int i = 0; i < tempCount; i++) {
        fwrite(&tempItems[i], sizeof(LostItem), 1, file);
    }
    fclose(file);

    // 步骤3：若文件为空，删除空文件
    if (tempCount == 0) {
        if (remove(fullPath) != 0) {
            printf("警告：删除空文件失败！\n");
        }
    }
}

// 加载所有物品数据：适配按丢失时间命名的文件
void loadAllData() {
    itemCount = 0;
    // 遍历所有可能的日期文件（此处简化处理：尝试加载近3年的日期文件，可根据需求调整范围）
    time_t now;
    struct tm* timeinfo;
    time(&now);
    timeinfo = localtime(&now);

    // 遍历近1095天（3年）的日期，查找对应文件
    for (int dayOffset = 0; dayOffset < 1095; dayOffset++) {
        struct tm tempTm = *timeinfo;
        tempTm.tm_mday -= dayOffset;
        mktime(&tempTm); // 调整日期（自动处理月份/年份跨域）

        char datePart[11];
        strftime(datePart, 11, "%Y-%m-%d", &tempTm); // 生成YYYY-MM-DD格式日期

        // 构建文件路径：丢失日期.txt
        char filename[30], fullPath[100];
        sprintf(filename, "%s.txt", datePart);
        buildFilePath(RECORDS_FOLDER, filename, fullPath);

        FILE* file = fopen(fullPath, "rb");
        if (file == NULL) {
            continue; // 无该日期的文件，跳过
        }

        // 读取文件中所有物品
        LostItem temp;
        while (fread(&temp, sizeof(LostItem), 1, file) == 1 && itemCount < 1000) {
            items[itemCount++] = temp;
        }
        fclose(file);
    }

    printf("成功加载 %d 个物品数据\n", itemCount);
}

// 保存所有物品数据：按丢失时间重新分类保存
void saveAllItems() {
    // 步骤1：按日期分组存储物品
    LostItem dateGroups[1000][1000]; // 日期分组：[日期索引][物品]
    int groupCounts[1000] = { 0 };     // 每组物品数量
    char groupDates[1000][11];       // 每组对应的日期（YYYY-MM-DD）
    int groupCount = 0;              // 总分组数

    for (int i = 0; i < itemCount; i++) {
        char datePart[11];
        extractDatePart(items[i].lost_date, datePart);

        // 检查该日期是否已存在分组
        int groupIdx = -1;
        for (int j = 0; j < groupCount; j++) {
            if (strcmp(groupDates[j], datePart) == 0) {
                groupIdx = j;
                break;
            }
        }

        // 新日期：创建新分组
        if (groupIdx == -1) {
            if (groupCount >= 1000) break; // 超出分组上限
            strcpy(groupDates[groupCount], datePart);
            groupIdx = groupCount++;
        }

        // 将物品加入对应分组
        if (groupCounts[groupIdx] < 1000) {
            dateGroups[groupIdx][groupCounts[groupIdx]++] = items[i];
        }
    }

    // 步骤2：按分组保存到对应日期文件
    for (int i = 0; i < groupCount; i++) {
        char filename[30], fullPath[100];
        sprintf(filename, "%s.txt", groupDates[i]);
        buildFilePath(RECORDS_FOLDER, filename, fullPath);

        FILE* file = fopen(fullPath, "wb");
        if (file == NULL) {
            printf("警告：保存分组文件 %s 失败！\n", filename);
            continue;
        }

        for (int j = 0; j < groupCounts[i]; j++) {
            fwrite(&dateGroups[i][j], sizeof(LostItem), 1, file);
        }
        fclose(file);
    }
}

// 登记物品：仅保留手动输入类别
void registerItem(User* user) {
    if (itemCount >= 1000) {
        printf("物品数量已达上限，无法添加新物品！\n");
        return;
    }

    LostItem newItem = { 0 };

    printf("\n===== 登记新物品 =====\n");

    // 自动生成ID
    newItem.id = getNextItemId();
    printf("物品ID: %d（系统自动生成）\n", newItem.id);

    // 仅保留手动输入物品类别
    printf("请手动输入物品类别（如：书籍、生活用品、电子设备等）: ");
    if (fgets(newItem.category, 30, stdin) != NULL) {
        trimWhitespace(newItem.category);
        // 为空时默认设为「其他物品」
        if (strlen(newItem.category) == 0) {
            strcpy(newItem.category, "其他物品");
        }
    }
    else {
        strcpy(newItem.category, "其他物品");
    }

    // 物品名称
    printf("请输入物品名称: ");
    if (fgets(newItem.name, 50, stdin) != NULL) {
        trimWhitespace(newItem.name);
    }

    // 型号/特征
    printf("请输入物品型号/特征: ");
    if (fgets(newItem.model, 50, stdin) != NULL) {
        trimWhitespace(newItem.model);
    }

    // 物品描述
    printf("请输入物品详细描述: ");
    if (fgets(newItem.description, 200, stdin) != NULL) {
        trimWhitespace(newItem.description);
    }

    // 发现地点
    printf("请输入发现地点: ");
    if (fgets(newItem.location, 100, stdin) != NULL) {
        trimWhitespace(newItem.location);
    }

    // 遗失日期时间
    printf("请输入遗失日期时间 (YYYY-MM-DD HH:MM，直接回车则使用当前时间): ");
    char datetime[20];
    if (fgets(datetime, 20, stdin) != NULL) {
        trimWhitespace(datetime);
        if (strlen(datetime) == 0) {
            getCurrentDateTime(newItem.lost_date);
        }
        else if (!parseDateTime(datetime, &(struct tm){0})) {
            printf("日期格式错误，将使用当前时间\n");
            getCurrentDateTime(newItem.lost_date);
        }
        else {
            strcpy(newItem.lost_date, datetime);
        }
    }
    else {
        getCurrentDateTime(newItem.lost_date);
    }

    // 登记人
    strcpy(newItem.finder, user->username);

    // 状态初始化
    newItem.confirmed = (user->type == SUPER_ADMIN || user->type == ADMIN) ? 1 : 0;
    newItem.claimed = 0;

    // 添加到物品数组
    items[itemCount++] = newItem;

    // 保存到文件（按丢失时间命名）
    saveItemToFile(&newItem);

    // 提示信息
    printf("\n物品登记成功！\n");
    printf("物品信息: 类别=%s, 名称=%s (ID: %d)\n", newItem.category, newItem.name, newItem.id);
    if (newItem.confirmed) {
        printf("物品已自动确认\n");
    }
    else {
        printf("物品待管理员确认\n");
    }
}

// 确认物品
void confirmItem(User* admin) {
    if (admin->type != SUPER_ADMIN && admin->type != ADMIN) {
        printf("权限不足，只有管理员可以确认物品！\n");
        return;
    }

    printf("\n===== 物品确认 =====\n");

    // 显示所有未确认的物品
    int unconfirmedCount = 0;
    printf("未确认的物品列表:\n");
    printf("ID\t名称\t\t类别\t\t发现地点\n");
    printf("------------------------------------------------\n");
    for (int i = 0; i < itemCount; i++) {
        if (!items[i].confirmed) {
            printf("%d\t%s\t\t%s\t\t%s\n",
                items[i].id,
                items[i].name,
                items[i].category,
                items[i].location);
            unconfirmedCount++;
        }
    }

    if (unconfirmedCount == 0) {
        printf("没有需要确认的物品\n");
        return;
    }

    // 选择要确认的物品
    printf("\n请输入要确认的物品ID (输入0取消): ");
    int id;
    if (scanf("%d", &id) != 1) {
        printf("输入错误！\n");
        while (getchar() != '\n');
        return;
    }
    getchar();

    if (id == 0) {
        printf("已取消操作\n");
        return;
    }

    // 查找并确认物品
    for (int i = 0; i < itemCount; i++) {
        if (items[i].id == id && !items[i].confirmed) {
            items[i].confirmed = 1;
            saveItemToFile(&items[i]); // 重新保存到对应日期文件
            printf("物品ID: %d 已确认\n", id);
            return;
        }
    }

    printf("未找到ID为 %d 的未确认物品\n", id);
}

// 删除物品（仅管理员和超级管理员可用）
void deleteItem(User* admin) {
    if (admin->type != SUPER_ADMIN && admin->type != ADMIN) {
        printf("权限不足，只有管理员可以删除物品！\n");
        return;
    }

    printf("\n===== 删除物品 =====\n");

    // 显示所有物品
    int totalCount = 0;
    printf("所有物品列表:\n");
    printf("ID\t名称\t\t类别\t\t状态\t\t发现地点\t\t丢失时间\n");
    printf("------------------------------------------------------------------------\n");
    for (int i = 0; i < itemCount; i++) {
        printf("%d\t%s\t\t%s\t\t%s\t\t%s\t\t%s\n",
            items[i].id,
            items[i].name,
            items[i].category,
            items[i].confirmed ? "已确认" : "待确认",
            items[i].location,
            items[i].lost_date);
        totalCount++;
    }

    if (totalCount == 0) {
        printf("没有物品可删除\n");
        return;
    }

    // 选择要删除的物品
    printf("\n请输入要删除的物品ID (输入0取消): ");
    int id;
    if (scanf("%d", &id) != 1) {
        printf("输入错误！\n");
        while (getchar() != '\n');
        return;
    }
    getchar();

    if (id == 0) {
        printf("已取消操作\n");
        return;
    }

    // 查找物品并删除
    int foundIndex = -1;
    char lostDate[20]; // 存储目标物品的丢失时间
    for (int i = 0; i < itemCount; i++) {
        if (items[i].id == id) {
            foundIndex = i;
            strcpy(lostDate, items[i].lost_date);
            break;
        }
    }

    if (foundIndex == -1) {
        printf("未找到ID为 %d 的物品\n", id);
        return;
    }

    // 二次确认
    printf("确定要删除以下物品吗？\n");
    printf("ID: %d, 名称: %s, 类别: %s, 丢失时间: %s\n",
        items[foundIndex].id,
        items[foundIndex].name,
        items[foundIndex].category,
        items[foundIndex].lost_date);
    printf("输入1确认删除，其他键取消: ");
    char confirm[2];
    fgets(confirm, 2, stdin);
    if (confirm[0] != '1') {
        printf("已取消删除\n");
        return;
    }

    // 步骤1：删除文件中的物品记录
    deleteItemFile(lostDate, id);

    // 步骤2：从内存数组中删除
    for (int i = foundIndex; i < itemCount - 1; i++) {
        items[i] = items[i + 1]; // 覆盖当前元素
    }
    itemCount--; // 减少计数

    printf("物品ID: %d 已成功删除\n", id);
}

// 查询物品：仅按手动输入的类别查询
void queryItems() {
    printf("\n===== 物品查询 =====\n");
    printf("查询方式:\n");
    printf("1. 按物品类别查询（模糊匹配）\n");
    printf("2. 按物品名称查询\n");
    printf("3. 按发现地点查询\n");
    printf("4. 按状态查询（已认领/未认领）\n");
    printf("5. 按丢失日期查询\n"); // 新增按丢失日期查询
    printf("6. 查看所有物品\n");
    printf("请选择查询方式 (1-6): ");

    int choice;
    if (scanf("%d", &choice) != 1 || choice < 1 || choice > 6) {
        printf("无效的选择！\n");
        while (getchar() != '\n');
        return;
    }
    getchar();

    int count = 0;
    LostItem results[1000];

    switch (choice) {
    case 1: {
        // 按物品类别查询（模糊匹配手动输入的类别）
        char keyword[30];
        printf("请输入类别关键词（如：耳机、书、笔等）: ");
        if (fgets(keyword, 30, stdin) != NULL) {
            trimWhitespace(keyword);
        }
        else {
            printf("输入错误！\n");
            return;
        }

        // 模糊匹配类别
        for (int i = 0; i < itemCount; i++) {
            if (items[i].confirmed && strstr(items[i].category, keyword) != NULL) {
                results[count++] = items[i];
            }
        }
        break;
    }

    case 2: {
        // 按名称查询
        char name[50];
        printf("请输入物品名称关键字: ");
        if (fgets(name, 50, stdin) != NULL) {
            trimWhitespace(name);
        }
        else {
            printf("输入错误！\n");
            return;
        }

        for (int i = 0; i < itemCount; i++) {
            if (strstr(items[i].name, name) != NULL && items[i].confirmed) {
                results[count++] = items[i];
            }
        }
        break;
    }

    case 3: {
        // 按地点查询
        char location[100];
        printf("请输入发现地点关键字: ");
        if (fgets(location, 100, stdin) != NULL) {
            trimWhitespace(location);
        }
        else {
            printf("输入错误！\n");
            return;
        }

        for (int i = 0; i < itemCount; i++) {
            if (strstr(items[i].location, location) != NULL && items[i].confirmed) {
                results[count++] = items[i];
            }
        }
        break;
    }

    case 4: {
        // 按状态查询
        printf("1. 未认领\n");
        printf("2. 已认领\n");
        printf("请选择物品状态 (1-2): ");
        int status;
        if (scanf("%d", &status) != 1 || status < 1 || status > 2) {
            printf("无效的选择！\n");
            while (getchar() != '\n');
            return;
        }
        getchar();

        int claimed = (status == 2) ? 1 : 0;
        for (int i = 0; i < itemCount; i++) {
            if (items[i].claimed == claimed && items[i].confirmed) {
                results[count++] = items[i];
            }
        }
        break;
    }

    case 5: {
        // 新增：按丢失日期查询
        char targetDate[11];
        printf("请输入要查询的丢失日期（YYYY-MM-DD）: ");
        if (fgets(targetDate, 11, stdin) != NULL) {
            trimWhitespace(targetDate);
            // 验证日期格式（简单验证长度）
            if (strlen(targetDate) != 10 || targetDate[4] != '-' || targetDate[7] != '-') {
                printf("日期格式错误（应为YYYY-MM-DD）！\n");
                return;
            }
        }
        else {
            printf("输入错误！\n");
            return;
        }

        // 匹配丢失日期（忽略时间部分）
        for (int i = 0; i < itemCount; i++) {
            if (items[i].confirmed) {
                char itemDate[11];
                extractDatePart(items[i].lost_date, itemDate);
                if (strcmp(itemDate, targetDate) == 0) {
                    results[count++] = items[i];
                }
            }
        }
        break;
    }

    case 6: {
        // 查看所有物品
        for (int i = 0; i < itemCount; i++) {
            if (items[i].confirmed) {
                results[count++] = items[i];
            }
        }
        break;
    }
    }

    // 显示查询结果
    printf("\n===== 查询结果 =====\n");
    if (count == 0) {
        printf("没有找到符合条件的物品\n");
        return;
    }

    printf("共找到 %d 件物品:\n", count);
    printf("ID\t名称\t\t类别\t\t状态\t\t发现地点\t\t丢失时间\n");
    printf("------------------------------------------------------------------------\n");
    for (int i = 0; i < count; i++) {
        char statusStr[10];
        strcpy(statusStr, results[i].claimed ? "已认领" : "未认领");
        printf("%d\t%s\t\t%s\t\t%s\t\t%s\t\t%s\n",
            results[i].id,
            results[i].name,
            results[i].category,
            statusStr,
            results[i].location,
            results[i].lost_date);
    }

    // 查看详情
    printf("\n是否查看某件物品的详细信息？(1-是 0-否): ");
    int viewDetail;
    if (scanf("%d", &viewDetail) == 1 && viewDetail == 1) {
        printf("请输入物品ID: ");
        int id;
        if (scanf("%d", &id) == 1) {
            for (int i = 0; i < count; i++) {
                if (results[i].id == id) {
                    char statusStr[10], confirmStr[10];
                    strcpy(statusStr, results[i].claimed ? "已认领" : "未认领");
                    strcpy(confirmStr, results[i].confirmed ? "已确认" : "未确认");

                    printf("\n===== 物品详情 =====\n");
                    printf("物品ID: %d\n", results[i].id);
                    printf("名称: %s\n", results[i].name);
                    printf("类别: %s\n", results[i].category);
                    printf("型号/特征: %s\n", results[i].model);
                    printf("描述: %s\n", results[i].description);
                    printf("发现地点: %s\n", results[i].location);
                    printf("遗失日期时间: %s\n", results[i].lost_date);
                    printf("登记人: %s\n", results[i].finder);
                    printf("确认状态: %s\n", confirmStr);
                    printf("认领状态: %s\n", statusStr);

                    if (results[i].claimed) {
                        printf("认领人学号: %s\n", results[i].claimant_id);
                        printf("认领人电话: %s\n", results[i].claimant_phone);
                        printf("认领日期时间: %s\n", results[i].claim_date);
                    }
                    break;
                }
            }
        }
    }
    getchar();
}

// 物品排序
void sortItems() {
    if (itemCount == 0) {
        printf("没有物品可排序！\n");
        return;
    }

    printf("\n===== 物品排序 =====\n");
    printf("排序方式:\n");
    printf("1. 按物品ID排序\n");
    printf("2. 按遗失日期时间排序（由新到旧）\n");
    printf("3. 按物品类别排序（字母顺序）\n");
    printf("请选择排序方式 (1-3): ");

    int choice;
    if (scanf("%d", &choice) != 1 || choice < 1 || choice > 3) {
        printf("无效的选择！\n");
        while (getchar() != '\n');
        return;
    }
    getchar();

    // 复制物品数组用于排序显示，不改变原数组顺序
    LostItem sortedItems[1000];
    memcpy(sortedItems, items, itemCount * sizeof(LostItem));

    // 排序
    for (int i = 0; i < itemCount - 1; i++) {
        for (int j = 0; j < itemCount - i - 1; j++) {
            int swap = 0;

            switch (choice) {
            case 1: // 按ID排序
                if (sortedItems[j].id > sortedItems[j + 1].id) {
                    swap = 1;
                }
                break;

            case 2: // 按日期排序（新到旧）
                if (minutesBetweenDatetimes(sortedItems[j].lost_date, sortedItems[j + 1].lost_date) < 0) {
                    swap = 1;
                }
                break;

            case 3: // 按类别排序（字母顺序）
                if (strcmp(sortedItems[j].category, sortedItems[j + 1].category) > 0) {
                    swap = 1;
                }
                break;
            }

            if (swap) {
                LostItem temp = sortedItems[j];
                sortedItems[j] = sortedItems[j + 1];
                sortedItems[j + 1] = temp;
            }
        }
    }

    // 显示排序结果
    printf("\n===== 排序结果 =====\n");
    printf("ID\t名称\t\t类别\t\t状态\t\t发现日期\n");
    printf("--------------------------------------------------------\n");
    for (int i = 0; i < itemCount; i++) {
        if (!sortedItems[i].confirmed) continue;

        char statusStr[10];
        strcpy(statusStr, sortedItems[i].claimed ? "已认领" : "未认领");

        // 只显示日期部分
        char dateStr[11];
        extractDatePart(sortedItems[i].lost_date, dateStr);

        printf("%d\t%s\t\t%s\t\t%s\t\t%s\n",
            sortedItems[i].id,
            sortedItems[i].name,
            sortedItems[i].category,
            statusStr,
            dateStr);
    }
}

// 物品认领
void claimItem(User* user) {
    printf("\n===== 物品认领 =====\n");

    // 显示所有未认领的物品
    int availableCount = 0;
    printf("可认领的物品列表:\n");
    printf("ID\t名称\t\t类别\t\t发现地点\t\t丢失时间\n");
    printf("--------------------------------------------------------\n");
    for (int i = 0; i < itemCount; i++) {
        if (items[i].confirmed && !items[i].claimed) {
            printf("%d\t%s\t\t%s\t\t%s\t\t%s\n",
                items[i].id,
                items[i].name,
                items[i].category,
                items[i].location,
                items[i].lost_date);
            availableCount++;
        }
    }

    if (availableCount == 0) {
        printf("没有可认领的物品\n");
        return;
    }

    // 选择要认领的物品
    printf("\n请输入要认领的物品ID (输入0取消): ");
    int id;
    if (scanf("%d", &id) != 1) {
        printf("输入错误！\n");
        while (getchar() != '\n');
        return;
    }
    getchar();

    if (id == 0) {
        printf("已取消操作\n");
        return;
    }

    // 查找物品
    int foundIndex = -1;
    for (int i = 0; i < itemCount; i++) {
        if (items[i].id == id && items[i].confirmed && !items[i].claimed) {
            foundIndex = i;
            break;
        }
    }

    if (foundIndex == -1) {
        printf("未找到ID为 %d 的可认领物品\n", id);
        return;
    }

    // 填写认领信息
    printf("\n请填写认领信息:\n");

    printf("请输入您的学号: ");
    if (fgets(items[foundIndex].claimant_id, 20, stdin) != NULL) {
        trimWhitespace(items[foundIndex].claimant_id);
    }

    printf("请输入您的联系电话: ");
    if (fgets(items[foundIndex].claimant_phone, 20, stdin) != NULL) {
        trimWhitespace(items[foundIndex].claimant_phone);
    }

    // 确认认领
    printf("\n确认认领该物品？(1-确认 0-取消): ");
    int confirm;
    if (scanf("%d", &confirm) != 1 || confirm != 1) {
        printf("已取消认领\n");
        return;
    }

    // 更新物品状态
    items[foundIndex].claimed = 1;
    getCurrentDateTime(items[foundIndex].claim_date);
    saveItemToFile(&items[foundIndex]); // 重新保存到对应日期文件

    printf("物品认领成功！请携带有效证件到失物招领处领取\n");
}
