// item.c
#define _CRT_SECURE_NO_WARNINGS
#include "item.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <direct.h>  // Windows mkdir
#include <sys/stat.h>
#include <io.h>

// ========================
// 全局变量定义
// ========================
LostItem items[MAX_ITEMS];
int itemCount = 0;

// ========================
// 工具函数（内部使用）
// ========================
static void getCurrentDateTime(char* datetime) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    strftime(datetime, 20, "%Y-%m-%d %H:%M", t);
}

int parseDateTime(const char* dateStr, struct tm* tm) {
    if (strlen(dateStr) != 16) return 0;
    if (dateStr[4] != '-' || dateStr[7] != '-' || dateStr[10] != ' ' || dateStr[13] != ':') return 0;

    char yearStr[5] = { 0 }, monthStr[3] = { 0 }, dayStr[3] = { 0 };
    char hourStr[3] = { 0 }, minStr[3] = { 0 };
    strncpy(yearStr, dateStr, 4);
    strncpy(monthStr, dateStr + 5, 2);
    strncpy(dayStr, dateStr + 8, 2);
    strncpy(hourStr, dateStr + 11, 2);
    strncpy(minStr, dateStr + 14, 2);

    int year = atoi(yearStr), month = atoi(monthStr), day = atoi(dayStr);
    int hour = atoi(hourStr), min = atoi(minStr);

    if (year < 2000 || year > 2100 || month < 1 || month > 12 || day < 1 || day > 31 ||
        hour < 0 || hour > 23 || min < 0 || min > 59) return 0;

    tm->tm_year = year - 1900;
    tm->tm_mon = month - 1;
    tm->tm_mday = day;
    tm->tm_hour = hour;
    tm->tm_min = min;
    tm->tm_sec = 0;
    tm->tm_isdst = -1;
    return 1;
}

static int daysBetweenDates(const char* date1, const char* date2) {
    struct tm tm1 = { 0 }, tm2 = { 0 };
    char d1[17], d2[17];
    snprintf(d1, sizeof(d1), "%.*s 00:00", 10, date1);
    snprintf(d2, sizeof(d2), "%.*s 00:00", 10, date2);

    if (!parseDateTime(d1, &tm1) || !parseDateTime(d2, &tm2)) return 0;
    time_t t1 = mktime(&tm1), t2 = mktime(&tm2);
    if (t1 == -1 || t2 == -1) return 0;
    return (int)difftime(t2, t1) / (60 * 60 * 24);
}

static void extractDatePart(const char* datetime, char* datePart) {
    if (strlen(datetime) < 10) {
        strcpy(datePart, "unknown_date");
        return;
    }
    strncpy(datePart, datetime, 10);
    datePart[10] = '\0';
}

static void buildFilePath(const char* folder, const char* filename, char* fullPath) {
#ifdef _WIN32
    sprintf(fullPath, "%s\\%s", folder, filename);
#else
    sprintf(fullPath, "%s/%s", folder, filename);
#endif
}

// ========================
// 文件操作
// ========================
//static void saveItemToFile(const LostItem* item) {
//    char datePart[11];
//    extractDatePart(item->lost_date, datePart);
//    char filename[30], fullPath[100];
//    sprintf(filename, "%s.txt", datePart);
//    buildFilePath(RECORDS_FOLDER, filename, fullPath);
//
//    FILE* file = fopen(fullPath, "ab");
//    if (file) {
//        fwrite(item, sizeof(LostItem), 1, file);
//        fclose(file);
//    }
//}

static void saveItemToFile(LostItem* item) {
    char datePart[11];
    extractDatePart(item->lost_date, datePart);

    char subfolderPath[200], filename[30], fullPath[200];
    buildSubfolderPath(RECORDS_FOLDER, subfolderPath);
    sprintf(filename, "%s.txt", datePart);
    buildFilePath(subfolderPath, filename, fullPath);

    // 以文本方式打开文件，方便阅读和排版
    FILE* file = fopen(fullPath, "a");
    if (file == NULL) {
        printf("保存物品数据失败！路径: %s\n", fullPath);
        return;
    }

    // 打印分隔线
    printDivider(file, 80);

    // 物品信息（左对齐，字段名固定宽度）
    fprintf(file, "物品ID:         %d\n", item->id);
    fprintf(file, "名称:           %s\n", item->name);
    fprintf(file, "类别:           %s\n", item->category);
    fprintf(file, "型号/特征:      %s\n", item->model);
    fprintf(file, "描述:           %s\n", item->description);
    fprintf(file, "发现地点:       %s\n", item->location);
    fprintf(file, "遗失日期时间:   %s\n", item->lost_date);
    fprintf(file, "登记人:         %s\n", item->finder);
    fprintf(file, "确认状态:       %s\n", item->confirmed ? "已确认" : "未确认");
    fprintf(file, "认领状态:       %s\n", item->claimed ? "已认领" : "未认领");

    // 如果已认领，显示认领信息
    if (item->claimed) {
        fprintf(file, "认领人学号:     %s\n", item->claimant_id);
        fprintf(file, "认领人电话:     %s\n", item->claimant_phone);
        fprintf(file, "认领日期时间:   %s\n", item->claim_date);
    }

    // 打印底部分隔线
    printDivider(file, 80);
    fprintf(file, "\n");  // 物品之间空一行

    fclose(file);
    printf("物品信息已保存到 %s\n", fullPath);
}

//static void deleteItemFile(const char* dateStr, int id) {
//    char datePart[11];
//    extractDatePart(dateStr, datePart);
//    char filename[30], fullPath[100];
//    sprintf(filename, "%s.txt", datePart);
//    buildFilePath(RECORDS_FOLDER, filename, fullPath);
//
//    FILE* file = fopen(fullPath, "rb");
//    if (!file) return;
//
//    LostItem tempItems[MAX_ITEMS];
//    int tempCount = 0;
//    LostItem temp;
//    while (fread(&temp, sizeof(LostItem), 1, file) == 1) {
//        if (temp.id != id) {
//            tempItems[tempCount++] = temp;
//        }
//    }
//    fclose(file);
//
//    file = fopen(fullPath, "wb");
//    if (file) {
//        for (int i = 0; i < tempCount; i++) {
//            fwrite(&tempItems[i], sizeof(LostItem), 1, file);
//        }
//        fclose(file);
//    }
//
//    if (tempCount == 0) {
//        remove(fullPath);
//    }
//}

static void deleteItemFile(const char* dateStr, int id) {
    char datePart[11];
    extractDatePart(dateStr, datePart);

    char subfolderPath[200], filename[30], fullPath[200];
    buildSubfolderPath(RECORDS_FOLDER, subfolderPath);
    sprintf(filename, "%s.txt", datePart);
    buildFilePath(subfolderPath, filename, fullPath);

    FILE* file = fopen(fullPath, "r");
    if (file == NULL) {
        printf("警告：未找到物品对应的文件！\n");
        return;
    }

    // 创建临时缓冲区存储所有内容（除了要删除的物品）
    char buffer[100000];  // 足够大的缓冲区
    char tempLine[1000];
    int bufferPos = 0;
    int inItem = 0;
    int foundItem = 0;
    int currentId = -1;

    while (fgets(tempLine, sizeof(tempLine), file)) {
        // 检查是否是物品开始的分隔线
        if (strstr(tempLine, "----------------") && strlen(tempLine) > 50) {
            if (inItem) {
                // 结束当前物品
                inItem = 0;
                if (currentId == id) {
                    foundItem = 1;  // 标记为要删除的物品
                }
                else {
                    // 将物品内容添加到缓冲区
                    int len = strlen(tempLine);
                    if (bufferPos + len < sizeof(buffer)) {
                        strcpy(buffer + bufferPos, tempLine);
                        bufferPos += len;
                    }
                }
                currentId = -1;
            }
            else {
                // 开始新物品
                inItem = 1;
                int len = strlen(tempLine);
                if (bufferPos + len < sizeof(buffer)) {
                    strcpy(buffer + bufferPos, tempLine);
                    bufferPos += len;
                }
            }
        }
        else if (inItem) {
            // 检查是否是物品ID行
            if (strstr(tempLine, "物品ID:") && currentId == -1) {
                sscanf(tempLine, "物品ID: %d", &currentId);
            }

            // 如果不是要删除的物品，添加到缓冲区
            if (currentId != id || currentId == -1) {
                int len = strlen(tempLine);
                if (bufferPos + len < sizeof(buffer)) {
                    strcpy(buffer + bufferPos, tempLine);
                    bufferPos += len;
                }
            }
        }
    }
    fclose(file);

    // 写回处理后的内容
    file = fopen(fullPath, "w");
    if (file == NULL) {
        printf("警告：更新文件失败！\n");
        return;
    }
    fwrite(buffer, 1, bufferPos, file);
    fclose(file);

    // 如果文件为空，删除文件
    if (bufferPos == 0) {
        if (remove(fullPath) != 0) {
            printf("警告：删除空文件失败！\n");
        }
    }
}


// ========================
// 公共函数实现
// ========================
//void createNecessaryFolders(void) {
//#ifdef _WIN32
//    _mkdir(RECORDS_FOLDER);
//    _mkdir(SUMMARY_FOLDER);
//    _mkdir(AUCTION_FOLDER);
//#else
//    mkdir(RECORDS_FOLDER, 0755);
//    mkdir(SUMMARY_FOLDER, 0755);
//    mkdir(AUCTION_FOLDER, 0755);
//#endif
//}

void createNecessaryFolders() {
    // 先创建主文件夹
#ifdef _WIN32
    _mkdir(MAIN_FOLDER);
#else
    mkdir(MAIN_FOLDER, 0755);
#endif

    // 再创建主文件夹下的子文件夹
    char subfolderPath[200];

    buildSubfolderPath(RECORDS_FOLDER, subfolderPath);
#ifdef _WIN32
    _mkdir(subfolderPath);
#else
    mkdir(subfolderPath, 0755);
#endif

    buildSubfolderPath(SUMMARY_FOLDER, subfolderPath);
#ifdef _WIN32
    _mkdir(subfolderPath);
#else
    mkdir(subfolderPath, 0755);
#endif

    buildSubfolderPath(AUCTION_FOLDER, subfolderPath);
#ifdef _WIN32
    _mkdir(subfolderPath);
#else
    mkdir(subfolderPath, 0755);
#endif

    // 创建备份主文件夹
#ifdef _WIN32
    _mkdir(BACKUP_FOLDER);
#else
    mkdir(BACKUP_FOLDER, 0755);
#endif
}


//void loadAllData(void) {
//    itemCount = 0;
//    time_t now;
//    time(&now);
//    struct tm* timeinfo = localtime(&now);
//
//    for (int dayOffset = 0; dayOffset < 1095; dayOffset++) {
//        struct tm tempTm = *timeinfo;
//        tempTm.tm_mday -= dayOffset;
//        mktime(&tempTm);
//        char datePart[11];
//        strftime(datePart, 11, "%Y-%m-%d", &tempTm);
//
//        char filename[30], fullPath[100];
//        sprintf(filename, "%s.txt", datePart);
//        buildFilePath(RECORDS_FOLDER, filename, fullPath);
//
//        FILE* file = fopen(fullPath, "rb");
//        if (!file) continue;
//
//        while (fread(&items[itemCount], sizeof(LostItem), 1, file) == 1 && itemCount < MAX_ITEMS) {
//            itemCount++;
//        }
//        fclose(file);
//    }
//}

//void loadAllData() {
//    itemCount = 0;
//    time_t now;
//    struct tm* timeinfo;
//    time(&now);
//    timeinfo = localtime(&now);
//
//    char subfolderPath[200];
//    buildSubfolderPath(RECORDS_FOLDER, subfolderPath);
//
//    for (int dayOffset = 0; dayOffset < 1095; dayOffset++) {
//        struct tm tempTm = *timeinfo;
//        tempTm.tm_mday -= dayOffset;
//        mktime(&tempTm);
//
//        char datePart[11];
//        strftime(datePart, 11, "%Y-%m-%d", &tempTm);
//
//        char filename[30], fullPath[200];
//        sprintf(filename, "%s.txt", datePart);
//        buildFilePath(subfolderPath, filename, fullPath);
//
//        FILE* file = fopen(fullPath, "r");
//        if (file == NULL) {
//            continue;
//        }
//
//        LostItem temp = { 0 };
//        char line[1000];
//        int inItem = 0;
//
//        while (fgets(line, sizeof(line), file)) {
//            // 检测物品分隔线
//            if (strstr(line, "----------------") && strlen(line) > 50) {
//                if (inItem) {
//                    // 完成一个物品的读取
//                    if (itemCount < 1000) {
//                        items[itemCount++] = temp;
//                    }
//                    // 重置临时物品
//                    memset(&temp, 0, sizeof(LostItem));
//                }
//                inItem = !inItem;
//            }
//            else if (inItem) {
//                // 解析物品字段
//                if (strstr(line, "物品ID:")) {
//                    sscanf(line, "物品ID: %d", &temp.id);
//                }
//                else if (strstr(line, "名称:")) {
//                    sscanf(line, "名称: %49[^\n]", temp.name);
//                    trimWhitespace(temp.name);
//                }
//                else if (strstr(line, "类别:")) {
//                    sscanf(line, "类别: %29[^\n]", temp.category);
//                    trimWhitespace(temp.category);
//                }
//                else if (strstr(line, "型号/特征:")) {
//                    sscanf(line, "型号/特征: %49[^\n]", temp.model);
//                    trimWhitespace(temp.model);
//                }
//                else if (strstr(line, "描述:")) {
//                    sscanf(line, "描述: %199[^\n]", temp.description);
//                    trimWhitespace(temp.description);
//                }
//                else if (strstr(line, "发现地点:")) {
//                    sscanf(line, "发现地点: %99[^\n]", temp.location);
//                    trimWhitespace(temp.location);
//                }
//                else if (strstr(line, "遗失日期时间:")) {
//                    sscanf(line, "遗失日期时间: %19[^\n]", temp.lost_date);
//                    trimWhitespace(temp.lost_date);
//                }
//                else if (strstr(line, "登记人:")) {
//                    sscanf(line, "登记人: %19[^\n]", temp.finder);
//                    trimWhitespace(temp.finder);
//                }
//                else if (strstr(line, "确认状态:")) {
//                    char status[10];
//                    sscanf(line, "确认状态: %9[^\n]", status);
//                    temp.confirmed = (strcmp(status, "已确认") == 0) ? 1 : 0;
//                }
//                else if (strstr(line, "认领状态:")) {
//                    char status[10];
//                    sscanf(line, "认领状态: %9[^\n]", status);
//                    temp.claimed = (strcmp(status, "已认领") == 0) ? 1 : 0;
//                }
//                else if (strstr(line, "认领人学号:")) {
//                    sscanf(line, "认领人学号: %19[^\n]", temp.claimant_id);
//                    trimWhitespace(temp.claimant_id);
//                }
//                else if (strstr(line, "认领人电话:")) {
//                    sscanf(line, "认领人电话: %19[^\n]", temp.claimant_phone);
//                    trimWhitespace(temp.claimant_phone);
//                }
//                else if (strstr(line, "认领日期时间:")) {
//                    sscanf(line, "认领日期时间: %19[^\n]", temp.claim_date);
//                    trimWhitespace(temp.claim_date);
//                }
//            }
//        }
//        fclose(file);
//    }
//
//}

// ========================
// 加载所有文本格式的失物数据（整合版）
// ========================
void loadAllData(void) {
    itemCount = 0;
    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);

    // 构建记录目录路径
    char subfolderPath[200];
    buildSubfolderPath(RECORDS_FOLDER, subfolderPath); // 确保目录路径正确（如带反斜杠）

    // 遍历过去 1095 天（约 3 年）
    for (int dayOffset = 0; dayOffset < 1095; dayOffset++) {
        struct tm tempTm = *timeinfo;
        tempTm.tm_mday -= dayOffset;
        mktime(&tempTm);

        char datePart[11];
        strftime(datePart, sizeof(datePart), "%Y-%m-%d", &tempTm);

        char filename[30], fullPath[200];
        sprintf(filename, "%s.txt", datePart);
        buildFilePath(subfolderPath, filename, fullPath);

        FILE* file = fopen(fullPath, "r", NULL);
        if (!file) {
            // 文件不存在是正常情况，跳过即可
            continue;
        }

        // 临时变量用于解析单个物品
        LostItem temp = { 0 };
        char line[1024];           // 每行最大长度
        int inItem = 0;            // 是否在读取一个物品块内

        while (fgets(line, sizeof(line), file)) {
            // 去除行尾换行符（可选，便于处理）
            line[strcspn(line, "\n")] = '\0';

            // 检查是否为长分隔线（标志物品开始/结束）
            if (strstr(line, "----------------------------------------") ||
                strstr(line, "====================")) {
                if (inItem) {
                    // 当前物品结束，保存到全局数组
                    if (itemCount < MAX_ITEMS) {
                        items[itemCount++] = temp;
                    }
                    // 重置临时物品
                    memset(&temp, 0, sizeof(LostItem));
                }
                inItem = !inItem; // 切换状态
            }
            else if (inItem) {
                // 在物品块内，解析各个字段
                if (strstr(line, "物品ID:")) {
                    sscanf(line, "物品ID: %d", &temp.id);
                }
                else if (strstr(line, "名称:")) {
                    sscanf(line, "名称: %49[^\n]", temp.name);
                    trimWhitespace(temp.name);
                }
                else if (strstr(line, "类别:")) {
                    sscanf(line, "类别: %29[^\n]", temp.category);
                    trimWhitespace(temp.category);
                }
                else if (strstr(line, "型号/特征:")) {
                    sscanf(line, "型号/特征: %49[^\n]", temp.model);
                    trimWhitespace(temp.model);
                }
                else if (strstr(line, "描述:")) {
                    sscanf(line, "描述: %199[^\n]", temp.description);
                    trimWhitespace(temp.description);
                }
                else if (strstr(line, "发现地点:")) {
                    sscanf(line, "发现地点: %99[^\n]", temp.location);
                    trimWhitespace(temp.location);
                }
                else if (strstr(line, "遗失日期时间:")) {
                    sscanf(line, "遗失日期时间: %19[^\n]", temp.lost_date);
                    trimWhitespace(temp.lost_date);
                }
                else if (strstr(line, "登记人:")) {
                    sscanf(line, "登记人: %19[^\n]", temp.finder);
                    trimWhitespace(temp.finder);
                }
                else if (strstr(line, "确认状态:")) {
                    char status[10] = { 0 };
                    sscanf(line, "确认状态: %9[^\n]", status);
                    trimWhitespace(status);
                    temp.confirmed = (strcmp(status, "已确认") == 0) ? 1 : 0;
                }
                else if (strstr(line, "认领状态:")) {
                    char status[10] = { 0 };
                    sscanf(line, "认领状态: %9[^\n]", status);
                    trimWhitespace(status);
                    temp.claimed = (strcmp(status, "已认领") == 0) ? 1 : 0;
                }
                else if (strstr(line, "认领人学号:")) {
                    sscanf(line, "认领人学号: %19[^\n]", temp.claimant_id);
                    trimWhitespace(temp.claimant_id);
                }
                else if (strstr(line, "认领人电话:")) {
                    sscanf(line, "认领人电话: %19[^\n]", temp.claimant_phone);
                    trimWhitespace(temp.claimant_phone);
                }
                else if (strstr(line, "认领日期时间:")) {
                    sscanf(line, "认领日期时间: %19[^\n]", temp.claim_date);
                    trimWhitespace(temp.claim_date);
                }
            }
        }

        fclose(file);
    }
}

//void saveAllItems(void) {
//    // 临时分组
//    LostItem* groups[1000] = { 0 };
//    int counts[1000] = { 0 };
//    char dates[1000][11];
//    int groupCount = 0;
//
//    for (int i = 0; i < itemCount; i++) {
//        char datePart[11];
//        extractDatePart(items[i].lost_date, datePart);
//
//        int found = -1;
//        for (int j = 0; j < groupCount; j++) {
//            if (strcmp(dates[j], datePart) == 0) {
//                found = j;
//                break;
//            }
//        }
//
//        if (found == -1) {
//            strcpy(dates[groupCount], datePart);
//            groups[groupCount] = (LostItem*)malloc(MAX_ITEMS * sizeof(LostItem));
//            found = groupCount++;
//        }
//
//        groups[found][counts[found]++] = items[i];
//    }
//
//    for (int i = 0; i < groupCount; i++) {
//        char filename[30], fullPath[100];
//        sprintf(filename, "%s.txt", dates[i]);
//        buildFilePath(RECORDS_FOLDER, filename, fullPath);
//
//        FILE* file = fopen(fullPath, "wb");
//        if (file) {
//            fwrite(groups[i], sizeof(LostItem), counts[i], file);
//            fclose(file);
//        }
//        free(groups[i]);
//    }
//}

void saveAllItems() {
    char subfolderPath[200];
    buildSubfolderPath(RECORDS_FOLDER, subfolderPath);

    // 按日期分组
    char* groupDates[1000] = { 0 };
    LostItem* dateGroups[1000] = { 0 };
    int groupCounts[1000] = { 0 };
    int groupCount = 0;

    for (int i = 0; i < itemCount; i++) {
        char datePart[11];
        extractDatePart(items[i].lost_date, datePart);

        int groupIdx = -1;
        for (int j = 0; j < groupCount; j++) {
            if (strcmp(groupDates[j], datePart) == 0) {
                groupIdx = j;
                break;
            }
        }

        if (groupIdx == -1) {
            if (groupCount >= 1000) break;
            groupDates[groupCount] = (char*)malloc(11 * sizeof(char));
            strcpy(groupDates[groupCount], datePart);
            dateGroups[groupCount] = (LostItem*)malloc(1000 * sizeof(LostItem));
            groupIdx = groupCount++;
        }

        if (groupCounts[groupIdx] < 1000) {
            dateGroups[groupIdx][groupCounts[groupIdx]++] = items[i];
        }
    }

    // 保存每个日期组
    for (int i = 0; i < groupCount; i++) {
        char filename[30], fullPath[200];
        sprintf(filename, "%s.txt", groupDates[i]);
        buildFilePath(subfolderPath, filename, fullPath);

        FILE* file = fopen(fullPath, "w");
        if (file == NULL) {
            printf("警告：保存分组文件 %s 失败！\n", filename);
            continue;
        }

        // 写入该日期的所有物品
        for (int j = 0; j < groupCounts[i]; j++) {
            LostItem* item = &dateGroups[i][j];

            // 打印分隔线
            printDivider(file, 80);

            // 物品信息（优化排版）
            fprintf(file, "物品ID:         %d\n", item->id);
            fprintf(file, "名称:           %s\n", item->name);
            fprintf(file, "类别:           %s\n", item->category);
            fprintf(file, "型号/特征:      %s\n", item->model);
            fprintf(file, "描述:           %s\n", item->description);
            fprintf(file, "发现地点:       %s\n", item->location);
            fprintf(file, "遗失日期时间:   %s\n", item->lost_date);
            fprintf(file, "登记人:         %s\n", item->finder);
            fprintf(file, "确认状态:       %s\n", item->confirmed ? "已确认" : "未确认");
            fprintf(file, "认领状态:       %s\n", item->claimed ? "已认领" : "未认领");

            // 如果已认领，显示认领信息
            if (item->claimed) {
                fprintf(file, "认领人学号:     %s\n", item->claimant_id);
                fprintf(file, "认领人电话:     %s\n", item->claimant_phone);
                fprintf(file, "认领日期时间:   %s\n", item->claim_date);
            }

            // 打印底部分隔线
            printDivider(file, 80);
            fprintf(file, "\n");  // 物品之间空一行
        }

        fclose(file);
        free(groupDates[i]);
        free(dateGroups[i]);
    }
}

// 备份所有数据（整个LF文件夹）
void backupAllData() {
    // 检查LF文件夹是否存在
    if (_access(MAIN_FOLDER, 0) == -1) {
        printf("主数据文件夹 %s 不存在，无法备份！\n", MAIN_FOLDER);
        return;
    }

    // 创建按当前时间命名的备份子文件夹
    char datetime[20], backupSubfolder[50], fullSubfolderPath[200];
    getCurrentDateTime(datetime);
    // 替换时间中的冒号（Windows不允许文件夹名包含冒号）
    for (int i = 0; i < strlen(datetime); i++) {
        if (datetime[i] == ':') datetime[i] = '-';
    }
    sprintf(backupSubfolder, "LF_backup_%s", datetime);
    buildFilePath(BACKUP_FOLDER, backupSubfolder, fullSubfolderPath);

    // 复制整个LF文件夹到备份文件夹
    copyDirectory(MAIN_FOLDER, fullSubfolderPath);

    printf("所有数据备份成功！备份路径: %s\n", fullSubfolderPath);
}

int getNextItemId(void) {
    int maxId = 0;
    for (int i = 0; i < itemCount; i++) {
        if (items[i].id > maxId) maxId = items[i].id;
    }
    return maxId + 1;
}

// ========================
// 物品操作
// ========================
int registerItem(const char* category, const char* name, const char* model,
    const char* description, const char* location,
    const char* lost_date, const char* finder) {
    if (itemCount >= MAX_ITEMS) return 0;

    LostItem item = { 0 };
    strncpy(item.category, category ? category : "其他物品", MAX_CATEGORY - 1);
    strncpy(item.name, name, MAX_NAME - 1);
    strncpy(item.model, model, MAX_MODEL - 1);
    strncpy(item.description, description, MAX_DESC - 1);
    strncpy(item.location, location, MAX_LOCATION - 1);
    strncpy(item.lost_date, lost_date ? lost_date : "", 19);
    if (!item.lost_date[0]) getCurrentDateTime(item.lost_date);
    strncpy(item.finder, finder, MAX_USER - 1);
    item.id = getNextItemId();
    item.confirmed = 0;
    item.claimed = 0;

    items[itemCount++] = item;
    saveItemToFile(&item);
    return 1;
}

int confirmItem(int itemId) {
    for (int i = 0; i < itemCount; i++) {
        if (items[i].id == itemId && !items[i].confirmed) {
            items[i].confirmed = 1;
            saveItemToFile(&items[i]);
            return 1;
        }
    }
    return 0;
}

int deleteItem(int itemId) {
    for (int i = 0; i < itemCount; i++) {
        if (items[i].id == itemId) {
            deleteItemFile(items[i].lost_date, itemId);
            for (int j = i; j < itemCount - 1; j++) {
                items[j] = items[j + 1];
            }
            itemCount--;
            return 1;
        }
    }
    return 0;
}

int claimItem(int itemId, const char* claimantId, const char* phone, const char* claimDate) {
    for (int i = 0; i < itemCount; i++) {
        if (items[i].id == itemId && !items[i].claimed) {
            strncpy(items[i].claimant_id, claimantId, MAX_USER - 1);
            strncpy(items[i].claimant_phone, phone, MAX_USER - 1);
            strncpy(items[i].claim_date, claimDate ? claimDate : "", 19);
            if (!items[i].claim_date[0]) getCurrentDateTime(items[i].claim_date);
            items[i].claimed = 1;
            saveItemToFile(&items[i]);
            return 1;
        }
    }
    return 0;
}

/**
 * 按类别查询物品（模糊匹配，包含已认领）
 */
int queryItemsByCategory(const char* category, const LostItem* results[], int maxCount) {
    if (category == NULL || results == NULL || maxCount <= 0) {
        return 0;
    }

    int count = 0;
    for (int i = 0; i < itemCount && count < maxCount; i++) {
        if (items[i].category[0] != '\0' && strstr(items[i].category, category)) {
            results[count++] = &items[i];
        }
    }
    return count;
}

/**
 * 按发现地点查询物品（模糊匹配，包含已认领）
 */
int queryItemsByLocation(const char* location, const LostItem* results[], int maxCount) {
    if (location == NULL || results == NULL || maxCount <= 0) {
        return 0;
    }

    int count = 0;
    for (int i = 0; i < itemCount && count < maxCount; i++) {
        if (items[i].location[0] != '\0' && strstr(items[i].location, location)) {
            results[count++] = &items[i];
        }
    }
    return count;
}

/**
 * 按物品名称查询物品（模糊匹配，包含已认领）
 */
int queryItemsByName(const char* name, const LostItem* results[], int maxCount) {
    if (name == NULL || results == NULL || maxCount <= 0) {
        return 0;
    }

    int count = 0;
    for (int i = 0; i < itemCount && count < maxCount; i++) {
        if (items[i].name[0] != '\0' && strstr(items[i].name, name)) {
            results[count++] = &items[i];
        }
    }
    return count;
}

void sortItemsByName(void) {
    for (int i = 0; i < itemCount - 1; i++) {
        for (int j = i + 1; j < itemCount; j++) {
            if (strcmp(items[i].name, items[j].name) > 0) {
                LostItem temp = items[i];
                items[i] = items[j];
                items[j] = temp;
            }
        }
    }
}

void sortItemsById(void) {
    for (int i = 0; i < itemCount - 1; i++) {
        for (int j = i + 1; j < itemCount; j++) {
            if (items[i].id > items[j].id) {
                LostItem temp = items[i];
                items[i] = items[j];
                items[j] = temp;
            }
        }
    }
}

void sortItemsByDate(void) {
    for (int i = 0; i < itemCount - 1; i++) {
        for (int j = i + 1; j < itemCount; j++) {
            if (strcmp(items[i].lost_date, items[j].lost_date) > 0) {
                LostItem temp = items[i];
                items[i] = items[j];
                items[j] = temp;
            }
        }
    }
}

// ========================
// 报告生成
// ========================
//void generateWeeklySummary(void) {
//    // 实现略（可参考原代码）
//}

void prepareAuctionItems(void) {
    // 实现略
}

// ========================
// 工具函数（供 UI 使用）
// ========================
int getItemCount(void) {
    return itemCount;
}

const LostItem* getItem(int index) {
    if (index < 0 || index >= itemCount) return NULL;
    return &items[index];
}

const LostItem* findItemById(int id) {
    for (int i = 0; i < itemCount; i++) {
        if (items[i].id == id) return &items[i];
    }
    return NULL;
}

void printDivider(FILE* file, int length) {
    for (int i = 0; i < length; i++) {
        fprintf(file, "-");
    }
    fprintf(file, "\n");
}

void buildSubfolderPath(const char* subfolder, char* fullPath) {
#ifdef _WIN32
    sprintf(fullPath, "%s\\%s", MAIN_FOLDER, subfolder);
#else
    sprintf(fullPath, "%s/%s", MAIN_FOLDER, subfolder);
#endif
}

void copyFile(const char* source, const char* dest) {
    FILE* srcFile = fopen(source, "rb");
    if (srcFile == NULL) {
        printf("无法打开源文件: %s\n", source);
        return;
    }

    FILE* destFile = fopen(dest, "wb");
    if (destFile == NULL) {
        printf("无法创建目标文件: %s\n", dest);
        fclose(srcFile);
        return;
    }

    char buffer[1024];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), srcFile)) > 0) {
        fwrite(buffer, 1, bytesRead, destFile);
    }

    fclose(srcFile);
    fclose(destFile);
}

// 复制目录（包括子目录和文件）
void copyDirectory(const char* source, const char* dest) {
    // 创建目标目录
#ifdef _WIN32
    _mkdir(dest);
#else
    mkdir(dest, 0755);
#endif

    // 查找源目录中的所有文件和子目录
    char searchPath[200];
#ifdef _WIN32
    sprintf(searchPath, "%s\\*", source);
#else
    sprintf(searchPath, "%s/*", source);
#endif

    struct _finddata_t fileInfo;
    intptr_t handle = _findfirst(searchPath, &fileInfo);

    if (handle == -1) {
        return;
    }

    do {
        // 跳过当前目录和父目录
        if (strcmp(fileInfo.name, ".") == 0 || strcmp(fileInfo.name, "..") == 0) {
            continue;
        }

        // 构建源文件/目录和目标文件/目录的路径
        char srcPath[200], destPath[200];
#ifdef _WIN32
        sprintf(srcPath, "%s\\%s", source, fileInfo.name);
        sprintf(destPath, "%s\\%s", dest, fileInfo.name);
#else
        sprintf(srcPath, "%s/%s", source, fileInfo.name);
        sprintf(destPath, "%s/%s", dest, fileInfo.name);
#endif

        // 如果是目录，则递归复制
        if (fileInfo.attrib & _A_SUBDIR) {
            copyDirectory(srcPath, destPath);
        }
        // 如果是文件，则直接复制
        else {
            copyFile(srcPath, destPath);
        }
    } while (_findnext(handle, &fileInfo) == 0);

    _findclose(handle);
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
