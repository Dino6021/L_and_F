// item.h
#ifndef ITEM_H
#define ITEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

    // ========================
    // 宏定义（与原代码一致）
    // ========================
#define MAIN_FOLDER "LF"                     // 主文件夹
#define BACKUP_FOLDER "LF_backups"  // 备份主文件夹
#define RECORDS_FOLDER "lost_and_found_records"
#define SUMMARY_FOLDER "weekly_summaries"
#define AUCTION_FOLDER "auction_preparation"
#define USERS_FILE "users.dat"  // 用户数据文件（位于主文件夹下）
#define MAX_ITEMS 1000
#define MAX_CATEGORY 30
#define MAX_NAME 50
#define MAX_MODEL 50
#define MAX_DESC 200
#define MAX_LOCATION 100
#define MAX_USER 20
#define MAX_QUERY_RESULTS 100
#define QUERY_BY_NAME     0
#define QUERY_BY_CATEGORY 1
#define QUERY_BY_LOCATION 2

// ========================
// 物品结构体
// ========================
    typedef struct {
        char category[MAX_CATEGORY];     // 手动输入的物品类别
        char name[MAX_NAME];             // 物品名称
        int id;                          // 物品ID
        char model[MAX_MODEL];           // 型号/特征
        char description[MAX_DESC];      // 物品描述
        char location[MAX_LOCATION];     // 发现地点
        char lost_date[20];              // 遗失日期时间(YYYY-MM-DD HH:MM)
        char finder[MAX_USER];           // 登记人
        int confirmed;                   // 0-未确认，1-已确认
        // 认领信息
        int claimed;                     // 0-未认领，1-已认领
        char claimant_id[MAX_USER];      // 认领人学号
        char claimant_phone[MAX_USER];   // 认领人电话
        char claim_date[20];             // 认领日期时间(YYYY-MM-DD HH:MM)
    } LostItem;

    // ========================
    // 全局变量声明（在 item.c 中定义）
    // ========================
    extern LostItem items[MAX_ITEMS];
    extern int itemCount;

    // ========================
    // 函数声明
    // ========================
    void createNecessaryFolders(void);
    void loadAllData(void);
    void saveAllItems(void);
    int getNextItemId(void);

    // 物品操作
    int registerItem(const char* category, const char* name, const char* model,
        const char* description, const char* location,
        const char* lost_date, const char* finder);
    int confirmItem(int itemId);
    int deleteItem(int itemId);
    int claimItem(int itemId, const char* claimantId, const char* phone, const char* claimDate);
    void backupAllData();
    void buildSubfolderPath(const char* subfolder, char* fullPath);
    void copyFile(const char* source, const char* dest);
    void copyDirectory(const char* source, const char* dest);

    // 查询与排序
    int queryItemsByName(const char* name, const LostItem* results[], int maxCount);
    int queryItemsByCategory(const char* category, const LostItem* results[], int maxCount);
    int queryItemsByLocation(const char* location, const LostItem* results[], int maxCount);
    void sortItemsByName(void);
    void sortItemsById(void);
    void sortItemsByDate(void);

    // 生成报告
    void generateWeeklySummary(void);
    void prepareAuctionItems(void);

    // 工具函数（供 UI 调用）
    int getItemCount(void);
    const LostItem* getItem(int index);
    const LostItem* findItemById(int id);
    int parseDateTime(const char* dateStr, struct tm* tm);
    void printDivider(FILE* file, int length);
    void trimWhitespace(char* str);


#ifdef __cplusplus
}
#endif

#endif // ITEM_H
