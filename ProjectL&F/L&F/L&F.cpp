// L&F.cpp : 定义应用程序的入口点。
//
#pragma execution_character_set("utf-8")
#define _CRT_SECURE_NO_WARNINGS
#define MAX_LOADSTRING 100
#pragma comment(lib, "comctl32.lib")  // 必须链接
#include "framework.h"
#include "L&F.h"
#include "user.h"
#include "item.h"
#include <CommCtrl.h>
#include <windows.h>
#include <string>
#include <stdio.h>
#include <time.h>


#ifndef DateTime_GetSystemTime
#define DateTime_GetSystemTime(hdp, lpsst) \
    (DWORD)SendMessage((hdp), DTM_GETSYSTEMTIME, 0, (LPARAM)(lpsst))
#endif

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名
extern char g_currentUser[32] = "";
extern int user_count;
extern HWND ghMainWnd = NULL; 
extern User users[100];
HBITMAP ghBackgroundBitmap = NULL; // 全局位图句柄

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    Login(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    AddUser(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    UsersManagement(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    DeleteUser(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    ItemsManagement(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    ItemRegister(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    ItemDelete(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    ItemClaim(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    Query(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    CheckItem(HWND, UINT, WPARAM, LPARAM);




void                RefreshUserList(HWND hList);
void                RefreshListViewItems(HWND hListView);
void                RefreshUnconfirmedItems(HWND hDlg);
void                get_last_week_monday_and_sunday(char* start_date, char* end_date);
void                generateLastWeekItemsReport(HWND hwndParent);
void                generateAuctionItemsList(HWND hwndParent);
void                UpdateMenuForRole(HWND hWnd, UserRole role);
int                 ansi_to_utf8(const char* ansiStr, char* utf8Buf, int bufSize);
struct tm*          parseDateTimeToTm(const char* datetime_str);


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此处放置代码。

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_LF, szWindowClass, MAX_LOADSTRING);

    if (!init_user_system()) {
        MessageBoxW(NULL, L"用户系统初始化失败！", L"错误", MB_ICONERROR);
        return FALSE;
    }

    MyRegisterClass(hInstance);

    createNecessaryFolders();

    // === 2. 加载所有历史数据到内存 ===
    loadAllData();

    // === 3. 获取下一个可用 ID ===
    int newId = getNextItemId();

    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    if (ghMainWnd == NULL) {
        MessageBoxW(NULL, L"ghMainWnd 为空", L"错误", MB_ICONERROR);
        return FALSE;
    }

    HMENU testMenu = GetMenu(ghMainWnd);
    if (testMenu == NULL) {
        MessageBoxW(NULL, L"主窗口没有菜单！", L"错误", MB_ICONERROR);
        return FALSE;
    }

    UpdateMenuForRole(ghMainWnd, ROLE_NONE);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LF));

    MSG msg;

    // 主消息循环:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LF));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_LF);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   ghBackgroundBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP1));
   if (!ghBackgroundBitmap)
   {
       MessageBox(NULL, L"无法加载背景图片！", L"错误", MB_ICONERROR);
       return FALSE;
   }


   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);
   ghMainWnd = hWnd;

   return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case IDM_LOGIN:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_LOGIN), hWnd, Login);
                break;
            case IDM_USERS_MANAGEMENT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_USERS_MANAGER1), hWnd, UsersManagement);
                break;
            case IDM_ITEMS_MANAGEMENT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ITEMS_MANAGER), hWnd, ItemsManagement);
                break;
            case IDM_CONFIRM:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_CONFIRM), hWnd, CheckItem);
                break;
            case IDM_GENERATE_LAST_WEEK:
                generateLastWeekItemsReport(hWnd);
                break;
            case IDM_AUCTION:
                generateAuctionItemsList(hWnd);
                break;

            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;

    case WM_ERASEBKGND:
        return TRUE; // 告诉系统：别擦了，我自己画
        break;

    case WM_PAINT:
        {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // === 绘制背景位图 ===
        HDC hMemDC = CreateCompatibleDC(hdc);
        HBITMAP hBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP1)); // 使用你的位图ID
        if (hBitmap)
        {
            HGDIOBJ hOld = SelectObject(hMemDC, hBitmap);

            // 获取位图尺寸
            BITMAP bmp;
            GetObject(hBitmap, sizeof(BITMAP), &bmp);

            // 获取窗口客户区尺寸
            RECT rect;
            GetClientRect(hWnd, &rect);

            // 使用 StretchBlt 拉伸填充整个客户区
            StretchBlt(hdc,
                0, 0, rect.right, rect.bottom,  // 目标区域
                hMemDC,
                0, 0, bmp.bmWidth, bmp.bmHeight, // 源位图
                SRCCOPY);

            SelectObject(hMemDC, hOld);
            DeleteObject(hBitmap);
        }
        DeleteDC(hMemDC);

        // TODO: 在此处添加其他控件或文本的绘制（如果有）
        // 注意：如果你有按钮、列表等子窗口，它们会自动绘制，无需在这里处理

        EndPaint(hWnd, &ps);
    }
        break;
    case WM_DESTROY:
        if (ghBackgroundBitmap)
        {
            DeleteObject(ghBackgroundBitmap);
            ghBackgroundBitmap = NULL;
        }
        PostQuitMessage(0);        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// 根据用户角色更新菜单内容，体现权限
// 登录成功后，根据用户角色启用/禁用菜单
void UpdateMenuForRole(HWND hWnd, UserRole role) {
    HMENU hMenu = GetMenu(hWnd);

    if (role == ROLE_SUPER_ADMIN) {
        // 超级管理员
        EnableMenuItem(hMenu, IDM_EXIT, MF_BYCOMMAND | MF_ENABLED);// 可用
        EnableMenuItem(hMenu, IDM_GENERATE_LAST_WEEK, MF_BYCOMMAND | MF_ENABLED);// 可用
        EnableMenuItem(hMenu, IDM_AUCTION, MF_BYCOMMAND | MF_ENABLED);// 可用
        EnableMenuItem(hMenu, IDM_LOGIN, MF_BYCOMMAND | MF_GRAYED);// 禁用
        EnableMenuItem(hMenu, IDM_USERS_MANAGEMENT, MF_BYCOMMAND | MF_ENABLED);// 可用
        EnableMenuItem(hMenu, IDM_ITEMS_MANAGEMENT, MF_BYCOMMAND | MF_ENABLED);// 可用
        EnableMenuItem(hMenu, IDM_CONFIRM, MF_BYCOMMAND | MF_ENABLED);// 可用
        EnableMenuItem(hMenu, IDM_ABOUT, MF_BYCOMMAND | MF_ENABLED);// 可用
    }
    else if (role == ROLE_ADMIN) {
        // 管理员
        EnableMenuItem(hMenu, IDM_EXIT, MF_BYCOMMAND | MF_ENABLED);// 可用
        EnableMenuItem(hMenu, IDM_GENERATE_LAST_WEEK, MF_BYCOMMAND | MF_ENABLED);// 可用
        EnableMenuItem(hMenu, IDM_AUCTION, MF_BYCOMMAND | MF_ENABLED);// 可用
        EnableMenuItem(hMenu, IDM_LOGIN, MF_BYCOMMAND | MF_GRAYED);// 禁用
        EnableMenuItem(hMenu, IDM_USERS_MANAGEMENT, MF_BYCOMMAND | MF_GRAYED);// 禁用
        EnableMenuItem(hMenu, IDM_ITEMS_MANAGEMENT, MF_BYCOMMAND | MF_ENABLED);// 可用
        EnableMenuItem(hMenu, IDM_CONFIRM, MF_BYCOMMAND | MF_ENABLED);// 可用
        EnableMenuItem(hMenu, IDM_ABOUT, MF_BYCOMMAND | MF_ENABLED);// 可用
    }
    else if (role == ROLE_STUDENT) {
        // 学生
        EnableMenuItem(hMenu, IDM_EXIT, MF_BYCOMMAND | MF_ENABLED);// 可用
        EnableMenuItem(hMenu, IDM_GENERATE_LAST_WEEK, MF_BYCOMMAND | MF_ENABLED);// 可用
        EnableMenuItem(hMenu, IDM_AUCTION, MF_BYCOMMAND | MF_ENABLED);// 可用
        EnableMenuItem(hMenu, IDM_LOGIN, MF_BYCOMMAND | MF_GRAYED);// 禁用
        EnableMenuItem(hMenu, IDM_USERS_MANAGEMENT, MF_BYCOMMAND | MF_GRAYED);// 禁用
        EnableMenuItem(hMenu, IDM_ITEMS_MANAGEMENT, MF_BYCOMMAND | MF_ENABLED);// 可用
        EnableMenuItem(hMenu, IDM_CONFIRM, MF_BYCOMMAND | MF_GRAYED);// 禁用
        EnableMenuItem(hMenu, IDM_ABOUT, MF_BYCOMMAND | MF_ENABLED);// 可用
    }
    else if (role == ROLE_NONE) {
        EnableMenuItem(hMenu, IDM_EXIT, MF_BYCOMMAND | MF_ENABLED);// 可用
        EnableMenuItem(hMenu, IDM_GENERATE_LAST_WEEK, MF_BYCOMMAND | MF_GRAYED);// 禁用
        EnableMenuItem(hMenu, IDM_AUCTION, MF_BYCOMMAND | MF_GRAYED);// 禁用
        EnableMenuItem(hMenu, IDM_LOGIN, MF_BYCOMMAND | MF_ENABLED);// 可用
        EnableMenuItem(hMenu, IDM_USERS_MANAGEMENT, MF_BYCOMMAND | MF_GRAYED);// 禁用
        EnableMenuItem(hMenu, IDM_ITEMS_MANAGEMENT, MF_BYCOMMAND | MF_GRAYED);// 禁用
        EnableMenuItem(hMenu, IDM_CONFIRM, MF_BYCOMMAND | MF_GRAYED);// 禁用
        EnableMenuItem(hMenu, IDM_ABOUT, MF_BYCOMMAND | MF_ENABLED);// 可用
    }

    DrawMenuBar(hWnd); // 刷新菜单显示
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// 登录对话框
INT_PTR CALLBACK Login(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
        SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlg, IDC_USERNAME), TRUE);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        int wmId = LOWORD(wParam); // 控件ID
        int wmEvent = HIWORD(wParam); // 通知码（如 BN_CLICKED）

        // 处理按钮点击
        if (wmId == IDOK)
        {
            // 获取用户名和密码文本
            wchar_t username[64] = { 0 };
            wchar_t password[64] = { 0 };

            GetDlgItemText(hDlg, IDC_USERNAME, username, 64);
            GetDlgItemText(hDlg, IDC_PASSWORD, password, 64);

            //char格式用户名与密码
            char userName[256] = { 0 };
            char passWord[256] = { 0 };

            //宽格式转char格式
            WideCharToMultiByte(CP_UTF8, 0, username, -1, userName, 256, NULL, NULL);
            WideCharToMultiByte(CP_UTF8, 0, password, -1, passWord, 256, NULL, NULL);

            // 简单验证
            if (authenticate_user(userName, passWord))
            {
                UserRole role = get_user_role(userName);
                switch (role)
                {
                case ROLE_SUPER_ADMIN:
                    MessageBox(hDlg, L"欢迎您，高级管理员", L"登录成功", MB_ICONINFORMATION);
                    EndDialog(hDlg, IDOK);
                    strcpy(g_currentUser, userName);
                    UpdateMenuForRole(ghMainWnd, ROLE_SUPER_ADMIN);
                    break;

                case ROLE_ADMIN:
                    MessageBox(hDlg, L"欢迎您，管理员", L"登录成功", MB_ICONINFORMATION);
                    EndDialog(hDlg, IDOK);
                    strcpy(g_currentUser, userName);
                    UpdateMenuForRole(ghMainWnd, ROLE_ADMIN);
                    break;

                case ROLE_STUDENT:
                    MessageBox(hDlg, L"欢迎您，值班同学", L"登录成功", MB_ICONINFORMATION);
                    EndDialog(hDlg, IDOK);
                    strcpy(g_currentUser, userName);
                    UpdateMenuForRole(ghMainWnd, ROLE_STUDENT);
                    break;
                }
            }
            else
            {
                MessageBox(hDlg, L"账号或密码错误，请重新输入。", L"错误", MB_ICONERROR);
                // 不关闭对话框，允许重试
            }
            return (INT_PTR)TRUE;
        }
        else if (wmId == IDCANCEL)
        {
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }

    break;

    }
    return (INT_PTR)FALSE;
}

// 管理用户对话框
INT_PTR CALLBACK UsersManagement(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    HWND hList = GetDlgItem(hDlg, IDC_LIST4);  // 获取列表控件句柄

    switch (message)
    {
    case WM_INITDIALOG:
    {
        // 插入列
        LVCOLUMN lvCol = { 0 };
        lvCol.mask = LVCF_TEXT | LVCF_WIDTH;
        lvCol.cx = 100;

        lvCol.pszText = const_cast<LPWSTR>(L"用户类型");
        ListView_InsertColumn(hList, 0, &lvCol);

        lvCol.pszText = const_cast<LPWSTR>(L"用户名");
        lvCol.cx = 50;
        ListView_InsertColumn(hList, 1, &lvCol);

        lvCol.pszText = const_cast<LPWSTR>(L"密码");
        lvCol.cx = 80;
        ListView_InsertColumn(hList, 2, &lvCol);

        RefreshUserList(hList);

        return (INT_PTR)TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_SAVE_AND_QUIT)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == IDC_ADD_USERS)
        {
            DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ADD_USER), hDlg, AddUser);
            RefreshUserList(hList);
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == IDC_DELETE_USERS)
        {
            DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DELETE_USER), hDlg, DeleteUser);
            RefreshUserList(hList);
            return (INT_PTR)TRUE;
        }

        break;

    case WM_CLOSE:
        EndDialog(hDlg, 0);
        return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}

// 添加用户对话框
INT_PTR CALLBACK AddUser(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    HWND hCombo = GetDlgItem(hDlg, IDC_COMBO1);

    switch (message)
    {
    case WM_INITDIALOG:

        SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"值日同学");
        SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"管理员");
        SendMessage(hCombo, CB_SETCURSEL, 0, 0); // 默认选中“学生”

        SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlg, IDC_USERNAME), TRUE);

        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == IDOK)
        {
            //获取新增用户的用户名与密码
            wchar_t username[64] = { 0 };
            wchar_t password[64] = { 0 };

            GetDlgItemText(hDlg, IDC_USERNAME, username, 64);
            GetDlgItemText(hDlg, IDC_PASSWORD, password, 64);

            //char格式用户名与密码
            char userName[256] = { 0 };
            char passWord[256] = { 0 };

            //宽格式转char格式
            WideCharToMultiByte(CP_UTF8, 0, username, -1, userName, 256, NULL, NULL);
            WideCharToMultiByte(CP_UTF8, 0, password, -1, passWord, 256, NULL, NULL);

            int index = SendMessage(hCombo, CB_GETCURSEL, 0, 0);

            //获取新增用户身份
            if (index != CB_ERR) {
                wchar_t buffer[64];
                SendMessage(hCombo, CB_GETLBTEXT, index, (LPARAM)buffer);
            }

            UserRole selectRole = ROLE_STUDENT;

            switch (index)
            {
            case 0:
                selectRole = ROLE_ADMIN;
                break;

            case 1:
                selectRole = ROLE_STUDENT;
                break;

            default:
                MessageBox(hDlg, L"添加失败", L"错误", MB_ICONERROR);
                return (INT_PTR)FALSE;
            }

            //执行新增用户
            if (!add_user(userName, passWord, selectRole, g_currentUser))
            {
                MessageBox(hDlg, L"添加失败", L"错误", MB_ICONERROR);
                return (INT_PTR)FALSE;
            }
            else
            {
                MessageBox(hDlg, L"添加成功", L"成功", MB_ICONINFORMATION);
            }
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;

        }
        break;

    case WM_CLOSE:
        EndDialog(hDlg, 0);
        return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}

// 删除用户对话框
INT_PTR CALLBACK DeleteUser(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            // 获取用户名和密码文本
            wchar_t username[64] = { 0 };

            GetDlgItemText(hDlg, IDC_USER_TO_DELETE, username, 64);

            // char格式用户名与密码
            char userName[256] = { 0 };

            // 宽格式转char格式
            WideCharToMultiByte(CP_UTF8, 0, username, -1, userName, 256, NULL, NULL);

            // 执行删除
            if (!delete_user(userName, g_currentUser))
            {
                MessageBox(hDlg, L"删除失败", L"错误", MB_ICONERROR);
                return (INT_PTR)FALSE;
            }
            else
            {
                MessageBox(hDlg, L"删除成功", L"成功", MB_ICONINFORMATION);
            }

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;

    case WM_CLOSE:
        EndDialog(hDlg, 0);
        return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}

// 一个用于刷新用户列表的小函数
void RefreshUserList(HWND hList)
{
    ListView_DeleteAllItems(hList);

    if (!load_users()) {  // 调用 user.c 中的函数
        MessageBox(GetParent(hList), L"用户数据加载失败", L"错误", MB_ICONERROR);
        return;
    }

    LVITEM lvItem = { 0 };
    lvItem.mask = LVIF_TEXT;

    for (int i = 0; i < user_count; i++)
    {
        wchar_t userName[32] = { 0 }, passWord[32] = { 0 };
        MultiByteToWideChar(CP_UTF8, 0, users[i].username, -1, userName, 32);
        MultiByteToWideChar(CP_UTF8, 0, users[i].password, -1, passWord, 32);

        const wchar_t* roleStr = L"未知";
        switch (users[i].role)
        {
        case ROLE_SUPER_ADMIN: roleStr = L"超级管理员"; break;
        case ROLE_ADMIN:       roleStr = L"管理员";       break;
        case ROLE_STUDENT:     roleStr = L"值班同学";     break;
        }

        lvItem.iItem = i;
        lvItem.iSubItem = 0;
        lvItem.pszText = const_cast<LPWSTR>(roleStr);
        int row = ListView_InsertItem(hList, &lvItem);

        ListView_SetItemText(hList, row, 1, userName);
        ListView_SetItemText(hList, row, 2, passWord);
    }
}

// 物品管理对话框
INT_PTR CALLBACK ItemsManagement(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    HWND hListView = GetDlgItem(hDlg, IDC_LIST5);


    switch (message)
    {
    case WM_INITDIALOG:
    {
        if (hListView == NULL)
            return (INT_PTR)TRUE;

        // === 设置为报告视图 ===
        DWORD style = GetWindowLong(hListView, GWL_STYLE);
        SetWindowLong(hListView, GWL_STYLE, style | LVS_REPORT);

        //// === 插入列头 ===
        //LVCOLUMN lvCol;
        //ZeroMemory(&lvCol, sizeof(LVCOLUMN));
        //lvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

        //const int colWidths[] = { 60, 120, 100, 120, 150, 130, 80, 100 };
        //LPCWSTR colHeaders[] = {
        //    L"ID", L"名称", L"类别", L"型号",
        //    L"发现地点", L"遗失时间", L"管理员确认", L"拾得人"
        //};

        //for (int i = 0; i < 8; i++)
        //{
        //    lvCol.pszText = (LPWSTR)colHeaders[i];
        //    lvCol.cx = colWidths[i];
        //    lvCol.iSubItem = i;
        //    ListView_InsertColumn(hListView, i, &lvCol);
        //}

        // === 插入列头（共13列）===
        LVCOLUMN lvCol;
        ZeroMemory(&lvCol, sizeof(LVCOLUMN));
        lvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

        const int colWidths[] = {
            60,   // ID
            120,  // 名称
            100,  // 类别
            120,  // 型号
            150,  // 发现地点
            130,  // 遗失时间
            80,   // 状态
            100,  // 登记人
            180,  // 物品描述
            80,   // 是否认领
            100,  // 认领人学号
            120,  // 认领人电话
            130   // 认领时间
        };

        LPCWSTR colHeaders[] = {
            L"ID", L"名称", L"类别", L"型号",
            L"发现地点", L"遗失时间", L"状态", L"拾得人",
            L"物品描述",                // 新增
            L"是否认领",                // 新增
            L"认领人学号",              // 新增
            L"认领人电话",              // 新增
            L"认领时间"                 // 新增
        };

        for (int i = 0; i < 13; i++)  // 改为 13 列
        {
            lvCol.pszText = (LPWSTR)colHeaders[i];
            lvCol.cx = colWidths[i];
            lvCol.iSubItem = i;
            ListView_InsertColumn(hListView, i, &lvCol);
        }

        HWND hCombo = GetDlgItem(hDlg, IDC_SORT);
        if (hCombo)
        {
            SendMessage(hCombo, CB_SETMINVISIBLE, 5, 0);

            SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"按名称排序");
            SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"按ID排序");
            SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"按遗失时间排序");
        }

        // === 初始化完成后立即加载数据（默认按名称排序）===
        sortItemsByName();  // 默认排序
        RefreshListViewItems(hListView);
        SetWindowText(hDlg, L"遗失物品管理");

        return (INT_PTR)TRUE;
    }

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);  // 控件ID
        int wmEvent = HIWORD(wParam); // 通知码

        if (wmId == IDC_SORT && wmEvent == CBN_SELCHANGE)
        {
            // 获取当前选择的排序方式
            HWND hCombo = GetDlgItem(hDlg, IDC_SORT);
            int sel = (int)SendMessageW(hCombo, CB_GETCURSEL, 0, 0);

            // 根据选择调用对应的排序函数
            switch (sel)
            {
            case 1: // 按名称排序
                sortItemsByName();
                break;
            case 0: // 按 ID 排序
                sortItemsById();
                break;
            case 2: // 按遗失时间排序
                sortItemsByDate();
                break;
            }

            // 排序后刷新 ListView
            RefreshListViewItems(hListView);
            return (INT_PTR)TRUE;
        }

        // 处理按钮点击（BN_CLICKED）
        if (wmEvent != BN_CLICKED) break;

        switch (wmId)
        {
        case IDC_REGISTER:
            // 【登记物品】按钮
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ITEM_REGISTER), hDlg, ItemRegister);
            RefreshListViewItems(hListView);
            break;

        case IDC_CLAME:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ITEM_CLAIMER), hDlg, ItemClaim);
            RefreshListViewItems(hListView);
            break;

        case IDC_DELETE:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ITEM_DELETE), hDlg, ItemDelete);
            RefreshListViewItems(hListView);
            break;

        case IDC_QUERY:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_QUERY), hDlg, Query);
            break;

        case IDC_SAVE_AND_QUIT1:
            saveAllItems();
            EndDialog(hDlg, IDC_SAVE_AND_QUIT1); // 关闭对话框
            return (INT_PTR)TRUE;

        default:
            break;
        }
    }
    break;

    case WM_CLOSE:
        // 用户点击右上角关闭按钮
        if (MessageBox(hDlg, L"是否保存数据并退出？", L"确认", MB_YESNO | MB_ICONQUESTION) == IDYES)
        {
             saveAllItems();
        }
        EndDialog(hDlg, 0);
        return (INT_PTR)TRUE;

    default:
        break;
    }

    return (INT_PTR)FALSE;
}

// 一个用于刷新物品列表的小函数
//void RefreshListViewItems(HWND hListView)
//{
//    if (hListView == NULL) return;
//
//    // === 清空现有项 ===
//    ListView_DeleteAllItems(hListView);
//
//    // === 加载物品数据 ===
//    int count = getItemCount();
//    for (int i = 0; i < count; i++)
//    {
//        const LostItem* pItem = getItem(i);
//
//        // 转换 ID 为宽字符
//        wchar_t idStr[20] = { 0 };
//        swprintf(idStr, 20, L"%d", pItem->id);
//
//        // 插入主项（ID）
//        LVITEM lvItem;
//        ZeroMemory(&lvItem, sizeof(LVITEM));
//        lvItem.mask = LVIF_TEXT;
//        lvItem.iItem = i;
//        lvItem.iSubItem = 0;
//        lvItem.pszText = idStr;
//        int itemIndex = ListView_InsertItem(hListView, &lvItem);
//
//        // 临时缓冲区用于 ANSI -> Unicode 转换
//        wchar_t wBuf[256];
//
//        // 名称
//        ZeroMemory(wBuf, sizeof(wBuf));
//        mbstowcs(wBuf, pItem->name, 255);
//        ListView_SetItemText(hListView, itemIndex, 1, wBuf);
//
//        // 类别
//        ZeroMemory(wBuf, sizeof(wBuf));
//        mbstowcs(wBuf, pItem->category, 255);
//        ListView_SetItemText(hListView, itemIndex, 2, wBuf);
//
//        // 型号/特征
//        ZeroMemory(wBuf, sizeof(wBuf));
//        mbstowcs(wBuf, pItem->model, 255);
//        ListView_SetItemText(hListView, itemIndex, 3, wBuf);
//
//        // 发现地点
//        ZeroMemory(wBuf, sizeof(wBuf));
//        mbstowcs(wBuf, pItem->location, 255);
//        ListView_SetItemText(hListView, itemIndex, 4, wBuf);
//
//        // 遗失时间
//        ZeroMemory(wBuf, sizeof(wBuf));
//        mbstowcs(wBuf, pItem->lost_date, 255);
//        ListView_SetItemText(hListView, itemIndex, 5, wBuf);
//
//        // 状态
//        wchar_t statusStr[20] = { 0 };
//        wcscpy(statusStr, pItem->confirmed ? L"已确认" : L"待确认");
//        ListView_SetItemText(hListView, itemIndex, 6, statusStr);
//
//        // 登记人
//        ZeroMemory(wBuf, sizeof(wBuf));
//        mbstowcs(wBuf, pItem->finder, 255);
//        ListView_SetItemText(hListView, itemIndex, 7, wBuf);
//
//        // 物品描述（第8列）
//        ZeroMemory(wBuf, sizeof(wBuf));
//        mbstowcs(wBuf, pItem->description, 255);
//        ListView_SetItemText(hListView, itemIndex, 8, wBuf);
//
//        // 是否认领（第9列）
//        wchar_t claimedStr[20] = { 0 };
//        wcscpy(claimedStr, pItem->claimed ? L"已认领" : L"未认领");
//        ListView_SetItemText(hListView, itemIndex, 9, claimedStr);
//
//        // 静态空字符串，避免 const 问题
//        static wchar_t emptyStr[] = L"";
//
//        // 认领人学号（第10列）
//        if (pItem->claimed) {
//            ZeroMemory(wBuf, sizeof(wBuf));
//            mbstowcs(wBuf, pItem->claimant_id, 255);
//            ListView_SetItemText(hListView, itemIndex, 10, wBuf);
//        }
//        else {
//            ListView_SetItemText(hListView, itemIndex, 10, emptyStr);
//        }
//
//        // 认领人电话（第11列）
//        if (pItem->claimed) {
//            ZeroMemory(wBuf, sizeof(wBuf));
//            mbstowcs(wBuf, pItem->claimant_phone, 255);
//            ListView_SetItemText(hListView, itemIndex, 11, wBuf);
//        }
//        else {
//            ListView_SetItemText(hListView, itemIndex, 11, emptyStr);
//        }
//
//        // 认领时间（第12列）
//        if (pItem->claimed) {
//            ZeroMemory(wBuf, sizeof(wBuf));
//            mbstowcs(wBuf, pItem->claim_date, 255);
//            ListView_SetItemText(hListView, itemIndex, 12, wBuf);
//        }
//        else {
//            ListView_SetItemText(hListView, itemIndex, 12, emptyStr);
//        }
//    }
//}

// 一个用于刷新物品列表的小函数
void RefreshListViewItems(HWND hListView)
{
    if (hListView == NULL) return;

    // === 清空现有项 ===
    ListView_DeleteAllItems(hListView);

    // === 加载物品数据 ===
    int count = getItemCount();
    for (int i = 0; i < count; i++)
    {
        const LostItem* pItem = getItem(i);

        // 转换 ID 为宽字符
        wchar_t idStr[20] = { 0 };
        swprintf(idStr, 20, L"%d", pItem->id);

        // 插入主项（ID）
        LVITEM lvItem;
        ZeroMemory(&lvItem, sizeof(LVITEM));
        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = i;
        lvItem.iSubItem = 0;
        lvItem.pszText = idStr;
        int itemIndex = ListView_InsertItem(hListView, &lvItem);

        // 临时缓冲区用于 UTF-8 -> UTF-16 转换
        wchar_t wBuf[256];

        // --- 名称 ---
        ZeroMemory(wBuf, sizeof(wBuf));
        MultiByteToWideChar(CP_UTF8, 0, pItem->name, -1, wBuf, 255);
        ListView_SetItemText(hListView, itemIndex, 1, wBuf);

        // --- 类别 ---
        ZeroMemory(wBuf, sizeof(wBuf));
        MultiByteToWideChar(CP_UTF8, 0, pItem->category, -1, wBuf, 255);
        ListView_SetItemText(hListView, itemIndex, 2, wBuf);

        // --- 型号/特征 ---
        ZeroMemory(wBuf, sizeof(wBuf));
        MultiByteToWideChar(CP_UTF8, 0, pItem->model, -1, wBuf, 255);
        ListView_SetItemText(hListView, itemIndex, 3, wBuf);

        // --- 发现地点 ---
        ZeroMemory(wBuf, sizeof(wBuf));
        MultiByteToWideChar(CP_UTF8, 0, pItem->location, -1, wBuf, 255);
        ListView_SetItemText(hListView, itemIndex, 4, wBuf);

        // --- 遗失时间 ---
        ZeroMemory(wBuf, sizeof(wBuf));
        MultiByteToWideChar(CP_UTF8, 0, pItem->lost_date, -1, wBuf, 255);
        ListView_SetItemText(hListView, itemIndex, 5, wBuf);

        // --- 状态 ---
        wchar_t statusStr[20] = { 0 };
        wcscpy(statusStr, pItem->confirmed ? L"已确认" : L"待确认");
        ListView_SetItemText(hListView, itemIndex, 6, statusStr);

        // --- 登记人 ---
        ZeroMemory(wBuf, sizeof(wBuf));
        MultiByteToWideChar(CP_UTF8, 0, pItem->finder, -1, wBuf, 255);
        ListView_SetItemText(hListView, itemIndex, 7, wBuf);

        // --- 物品描述（第8列）---
        ZeroMemory(wBuf, sizeof(wBuf));
        MultiByteToWideChar(CP_UTF8, 0, pItem->description, -1, wBuf, 255);
        ListView_SetItemText(hListView, itemIndex, 8, wBuf);

        // --- 是否认领（第9列）---
        wchar_t claimedStr[20] = { 0 };
        wcscpy(claimedStr, pItem->claimed ? L"已认领" : L"未认领");
        ListView_SetItemText(hListView, itemIndex, 9, claimedStr);

        // 静态空字符串，避免 const 问题
        static wchar_t emptyStr[] = L"";

        // --- 认领人学号（第10列）---
        if (pItem->claimed) {
            ZeroMemory(wBuf, sizeof(wBuf));
            MultiByteToWideChar(CP_UTF8, 0, pItem->claimant_id, -1, wBuf, 255);
            ListView_SetItemText(hListView, itemIndex, 10, wBuf);
        }
        else {
            ListView_SetItemText(hListView, itemIndex, 10, emptyStr);
        }

        // --- 认领人电话（第11列）---
        if (pItem->claimed) {
            ZeroMemory(wBuf, sizeof(wBuf));
            MultiByteToWideChar(CP_UTF8, 0, pItem->claimant_phone, -1, wBuf, 255);
            ListView_SetItemText(hListView, itemIndex, 11, wBuf);
        }
        else {
            ListView_SetItemText(hListView, itemIndex, 11, emptyStr);
        }

        // --- 认领时间（第12列）---
        if (pItem->claimed) {
            ZeroMemory(wBuf, sizeof(wBuf));
            MultiByteToWideChar(CP_UTF8, 0, pItem->claim_date, -1, wBuf, 255);
            ListView_SetItemText(hListView, itemIndex, 12, wBuf);
        }
        else {
            ListView_SetItemText(hListView, itemIndex, 12, emptyStr);
        }
    }
}

// 登记物品对话框
INT_PTR CALLBACK ItemRegister(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
    {
        // 可在此设置默认值或初始化控件
        // 例如：将当前时间设为 DTP 默认值（系统自动处理）
        return (INT_PTR)TRUE;
    }

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        int wmEvent = HIWORD(wParam);

        // 处理按钮点击：IDOK 和 IDCANCEL
        if (wmEvent == BN_CLICKED)
        {
            switch (wmId)
            {
            case IDOK:
            {
                //// === 1. 获取所有 Edit Control 输入内容 ===
                //char name[50] = { 0 };
                //char category[50] = { 0 };
                //char model[50] = { 0 };
                //char describe[100] = { 0 };  // 描述可能较长
                //char location[50] = { 0 };
                //char finder[50] = { 0 };

                //GetDlgItemTextA(hDlg, IDC_ITEM_NAME, name, 49);
                //GetDlgItemTextA(hDlg, IDC_ITEM_CATEGORY, category, 49);
                //GetDlgItemTextA(hDlg, IDC_ITEM_MODEL, model, 49);
                //GetDlgItemTextA(hDlg, IDC_ITEM_DESCRIBE, describe, 99);
                //GetDlgItemTextA(hDlg, IDC_ITEM_LOCATION, location, 49);
                //GetDlgItemTextA(hDlg, IDC_ITEM_FINDER, finder, 49);

                char ansiCategory[64], ansiName[64], ansiModel[64], ansiDesc[256];
                char ansiLocation[64], ansiFinder[64];

                GetDlgItemTextA(hDlg, IDC_ITEM_CATEGORY, ansiCategory, 63);
                GetDlgItemTextA(hDlg, IDC_ITEM_NAME, ansiName, 63);
                GetDlgItemTextA(hDlg, IDC_ITEM_MODEL, ansiModel, 63);
                GetDlgItemTextA(hDlg, IDC_ITEM_DESCRIBE, ansiDesc, 255);
                GetDlgItemTextA(hDlg, IDC_ITEM_LOCATION, ansiLocation, 63);
                GetDlgItemTextA(hDlg, IDC_ITEM_FINDER, ansiFinder, 63);

                // === 关键：ANSI (GBK) -> UTF-8 转换 ===
                char utf8Category[64], utf8Name[64], utf8Model[64], utf8Desc[256];
                char utf8Location[64], utf8Finder[64];

                // ANSI -> UTF-8
                ansi_to_utf8(ansiCategory, utf8Category, 64);
                ansi_to_utf8(ansiName, utf8Name, 64);
                ansi_to_utf8(ansiModel, utf8Model, 64);
                ansi_to_utf8(ansiDesc, utf8Desc, 256);
                ansi_to_utf8(ansiLocation, utf8Location, 64);
                ansi_to_utf8(ansiFinder, utf8Finder, 64);

                // === 2. 获取 DateTimePicker 中的时间（格式: YYYY-MM-DD HH:MM）===
                SYSTEMTIME st;
                wchar_t dateBuf[20] = { 0 };
                char lost_date[20] = { 0 };

                if (DateTime_GetSystemTime(GetDlgItem(hDlg, IDC_LOST_DATE), &st) == GDT_VALID)
                {
                    swprintf(dateBuf, 20, L"%04d-%02d-%02d %02d:%02d",
                        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
                    // 宽字符 → 多字节
                    wcstombs(lost_date, dateBuf, 19);
                }
                else
                {
                    MessageBox(hDlg, L"请选择有效的遗失时间！", L"输入错误", MB_ICONWARNING);
                    return (INT_PTR)FALSE;
                }

                // === 3. 验证必填项 ===
                if (strlen(ansiName) == 0 || strlen(ansiLocation) == 0 ||
                    strlen(ansiFinder) == 0 || strlen(ansiCategory) == 0)
                {
                    MessageBox(hDlg, L"名称、类别、地点、登记人 为必填项，请填写完整。",
                        L"输入不完整", MB_ICONWARNING);
                    return (INT_PTR)FALSE;
                }

                // === 4. 调用 item.c 中的 registerItem 函数 ===
                int result = registerItem(utf8Category, utf8Name, utf8Model,
                    utf8Desc, utf8Location, lost_date, utf8Finder);
                if (get_user_role(g_currentUser) == ROLE_ADMIN || get_user_role(g_currentUser) == ROLE_SUPER_ADMIN)
                {
                    confirmItem(getItemCount());
                    MessageBox(hDlg, L"您是超级管理员或管理员，已为您直接确认该物品。",
                        L"默认确认", MB_ICONWARNING);
                }

                // === 5. 根据结果反馈用户 ===
                if (result > 0)
                {
                    EndDialog(hDlg, IDOK); // 关闭对话框
                }
                else
                {
                    MessageBox(hDlg, L"登记失败：可能是文件写入错误或数据冲突。",
                        L"失败", MB_ICONERROR);
                }
                break;
            }

            case IDCANCEL:
                // 用户取消
                EndDialog(hDlg, IDCANCEL);
                return (INT_PTR)TRUE;

            default:
                break;
            }
        }
    }
    break;

    case WM_CLOSE:
        EndDialog(hDlg, IDCANCEL);
        return (INT_PTR)TRUE;

    default:
        break;
    }

    return (INT_PTR)FALSE;
}

int ansi_to_utf8(const char* ansiStr, char* utf8Buf, int bufSize)
{
    if (!ansiStr || !utf8Buf || bufSize == 0) return 0;

    int len = MultiByteToWideChar(CP_ACP, 0, ansiStr, -1, NULL, 0);
    if (len == 0) return 0;

    wchar_t* wbuf = (wchar_t*)malloc(len * sizeof(wchar_t));
    if (!wbuf) return 0;

    MultiByteToWideChar(CP_ACP, 0, ansiStr, -1, wbuf, len);
    int ret = WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, utf8Buf, bufSize, NULL, NULL);

    free(wbuf);
    return ret ? 1 : 0;
}

// 删除物品对话框
INT_PTR CALLBACK ItemDelete(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
        // 可选：设置焦点到输入框
        SendDlgItemMessage(hDlg, IDC_ID_DELETE, EM_SETSEL, 0, -1);
        return (INT_PTR)TRUE;  // 返回 TRUE 表示设置焦点

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        int wmEvent = HIWORD(wParam);

        if (wmEvent == BN_CLICKED)
        {
            switch (wmId)
            {
            case IDOK:
            {
                // === 1. 获取输入的 ID 字符串 ===
                char idStr[20] = { 0 };
                GetDlgItemTextA(hDlg, IDC_ID_DELETE, idStr, 19);

                // === 2. 检查是否为空 ===
                if (strlen(idStr) == 0)
                {
                    MessageBoxW(hDlg, L"请输入要删除的物品ID。", L"输入为空", MB_ICONWARNING);
                    SetFocus(GetDlgItem(hDlg, IDC_ID_DELETE));
                    return (INT_PTR)FALSE;
                }

                // === 3. 检查是否为纯数字 ===
                for (int i = 0; i < strlen(idStr); i++)
                {
                    if (!isdigit(idStr[i]))
                    {
                        MessageBoxW(hDlg, L"ID 必须是正整数！", L"格式错误", MB_ICONWARNING);
                        SetFocus(GetDlgItem(hDlg, IDC_ID_DELETE));
                        return (INT_PTR)FALSE;
                    }
                }

                // === 4. 转换为整数 ===
                int itemId = atoi(idStr);

                if (itemId <= 0)
                {
                    MessageBoxW(hDlg, L"ID 必须是大于 0 的整数。", L"无效ID", MB_ICONWARNING);
                    return (INT_PTR)FALSE;
                }

                // === 5. 弹出确认对话框 ===
                wchar_t confirmMsg[100];
                swprintf(confirmMsg, 100, L"确定要删除 ID 为 %d 的物品吗？\n此操作不可恢复！", itemId);
                int result = MessageBoxW(hDlg, confirmMsg, L"确认删除", MB_YESNO | MB_ICONQUESTION);

                if (result != IDYES)
                {
                    return (INT_PTR)FALSE;
                }

                // === 6. 调用 deleteItem 函数 ===
                int deleteResult = deleteItem(itemId);

                // === 7. 根据结果反馈用户 ===
                if (deleteResult == 1)
                {
                    MessageBoxW(hDlg, L"物品删除成功！", L"成功", MB_ICONINFORMATION);
                    EndDialog(hDlg, IDOK);  // 关闭对话框
                }
                else if (deleteResult == -1)
                {
                    MessageBoxW(hDlg, L"删除失败：未找到指定ID的物品。", L"失败", MB_ICONWARNING);
                }
                else
                {
                    MessageBoxW(hDlg, L"删除失败：可能是文件访问错误。", L"失败", MB_ICONERROR);
                }

                return (INT_PTR)TRUE;
            }

            case IDCANCEL:
                EndDialog(hDlg, IDCANCEL);
                return (INT_PTR)TRUE;

            default:
                break;
            }
        }
    }
    break;

    case WM_CLOSE:
        EndDialog(hDlg, IDCANCEL);
        return (INT_PTR)TRUE;

    default:
        break;
    }

    return (INT_PTR)FALSE;
}

// 认领物品对话框
INT_PTR CALLBACK ItemClaim(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
        SetFocus(GetDlgItem(hDlg, IDC_CLAIM_ITEM));
        return (INT_PTR)TRUE;

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        if (HIWORD(wParam) == BN_CLICKED)
        {
            switch (wmId)
            {
            case IDOK:
            {
                // === 1. 获取所有输入 ===
                char itemIdStr[32] = { 0 };
                char idCard[64] = { 0 };
                char tel[32] = { 0 };
                GetDlgItemTextA(hDlg, IDC_CLAIM_ITEM, itemIdStr, 31);
                GetDlgItemTextA(hDlg, IDC_CLAIMER_ID, idCard, 63);
                GetDlgItemTextA(hDlg, IDC_CLAIMER_TEL, tel, 31);

                // === 2. 检查是否为空（仅非空校验）===
                if (itemIdStr[0] == '\0')
                {
                    MessageBoxW(hDlg, L"请输入物品ID。", L"提示", MB_ICONINFORMATION);
                    SetFocus(GetDlgItem(hDlg, IDC_CLAIM_ITEM));
                    return (INT_PTR)FALSE;
                }
                if (idCard[0] == '\0' || strlen(idCard) != 9)
                {
                    MessageBoxW(hDlg, L"请输入正确的认领人学号。", L"提示", MB_ICONINFORMATION);
                    SetFocus(GetDlgItem(hDlg, IDC_CLAIMER_ID));
                    return (INT_PTR)FALSE;
                }
                if (tel[0] == '\0' || strlen(tel) != 11)
                {
                    MessageBoxW(hDlg, L"请输入正确的联系手机号。", L"提示", MB_ICONINFORMATION);
                    SetFocus(GetDlgItem(hDlg, IDC_CLAIMER_TEL));
                    return (INT_PTR)FALSE;
                }

                // === 3. 获取认领时间 ===
                SYSTEMTIME st;
                char claimDate[20] = { 0 };
                HWND hPicker = GetDlgItem(hDlg, IDC_CLAIME_DATE);

                if (DateTime_GetSystemTime(hPicker, &st) != GDT_VALID)
                {
                    MessageBoxW(hDlg, L"请选择认领时间。", L"提示", MB_ICONINFORMATION);
                    return (INT_PTR)FALSE;
                }

                sprintf(claimDate, "%04d-%02d-%02d %02d:%02d",
                    st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);

                // === 4. 转换物品ID（atoi容错）===
                int itemId = atoi(itemIdStr);
                if (itemId <= 0)
                {
                    MessageBoxW(hDlg, L"物品ID无效，请输入大于0的数字。", L"提示", MB_ICONWARNING);
                    return (INT_PTR)FALSE;
                }

                // === 5. 确认操作 ===
                wchar_t confirmMsg[200];
                swprintf(confirmMsg, 200, L"确认认领物品ID: %d 吗？", itemId);
                if (MessageBoxW(hDlg, confirmMsg, L"确认", MB_YESNO | MB_ICONQUESTION) != IDYES)
                {
                    return (INT_PTR)FALSE;
                }

                // === 6. 调用 clameItem ===
                int result = claimItem(itemId, idCard, tel, claimDate);

                // === 7. 反馈结果 ===
                if (result == 1)
                {
                    MessageBoxW(hDlg, L"认领成功！", L"成功", MB_ICONINFORMATION);
                    EndDialog(hDlg, IDOK);
                }
                else if (result == -1)
                {
                    MessageBoxW(hDlg, L"认领失败：物品不存在或已被认领。", L"失败", MB_ICONWARNING);
                }
                else
                {
                    MessageBoxW(hDlg, L"认领失败：系统错误。", L"失败", MB_ICONERROR);
                }

                return (INT_PTR)TRUE;
            }

            case IDCANCEL:
                EndDialog(hDlg, IDCANCEL);
                return (INT_PTR)TRUE;

            default:
                break;
            }
        }
    }
    break;

    case WM_CLOSE:
        EndDialog(hDlg, IDCANCEL);
        return (INT_PTR)TRUE;

    default:
        break;
    }

    return (INT_PTR)FALSE;
}

// 查询物品对话框
INT_PTR CALLBACK Query(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    static const LostItem* g_queryResults[MAX_QUERY_RESULTS];

    switch (message)
    {
    case WM_INITDIALOG:
    {
        // === 初始化 ComboBox：使用 SendMessage 替代 ComboBox_AddString ===
        HWND hCombo = GetDlgItem(hDlg, IDC_WAY_OF_FIND);
        if (hCombo)
        {
            SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"按物品名称查询");
            SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"按物品类别查询");
            SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"按发现地点查询");
            SendMessageW(hCombo, CB_SETCURSEL, 0, 0); // 默认选中第1项
        }

        // === 初始化 ListView ===
        HWND hListView = GetDlgItem(hDlg, IDC_LIST1);
        if (hListView)
        {
            DWORD style = GetWindowLong(hListView, GWL_STYLE);
            SetWindowLong(hListView, GWL_STYLE, style | LVS_REPORT);

            LVCOLUMN lvCol;
            ZeroMemory(&lvCol, sizeof(LVCOLUMN));
            lvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

            const int colWidths[] = { 60, 120, 100, 120, 150, 130, 80, 100 };
            LPCWSTR colHeaders[] = {
                L"ID", L"名称", L"类别", L"型号/特征",
                L"发现地点", L"遗失时间", L"状态", L"登记人"
            };

            for (int i = 0; i < 8; i++)
            {
                lvCol.pszText = (LPWSTR)colHeaders[i];
                lvCol.cx = colWidths[i];
                lvCol.iSubItem = i;
                ListView_InsertColumn(hListView, i, &lvCol);
            }
        }

        return (INT_PTR)TRUE;
    }

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        if (HIWORD(wParam) == BN_CLICKED)
        {
            switch (wmId)
            {
            case IDOK:
            {
                // 获取查询方式
                HWND hCombo = GetDlgItem(hDlg, IDC_WAY_OF_FIND);
                int sel = (int)SendMessageW(hCombo, CB_GETCURSEL, 0, 0);
                if (sel == CB_ERR) sel = 0;

                // 获取关键词
                wchar_t wKeyword[64] = { 0 };
                GetDlgItemTextW(hDlg, IDC_KEYWORD, wKeyword, 64); // 获取宽字符版文本

                // 假设你需要一个UTF-8编码的版本来进行比较
                char keywordUtf8[256] = { 0 };
                WideCharToMultiByte(CP_UTF8, 0, wKeyword, -1, keywordUtf8, sizeof(keywordUtf8), NULL, NULL);

                if (wKeyword[0] == '\0')
                {
                    MessageBoxW(hDlg, L"请输入搜索关键词。", L"提示", MB_ICONINFORMATION);
                    SetFocus(GetDlgItem(hDlg, IDC_KEYWORD));
                    return (INT_PTR)FALSE;
                }

                // 执行查询（包含已认领和未认领）
                int count = 0;
                switch (sel)
                {
                case 2: // 名称
                    count = queryItemsByName(keywordUtf8, g_queryResults, MAX_QUERY_RESULTS);
                    break;
                case 1: // 类别
                    count = queryItemsByCategory(keywordUtf8, g_queryResults, MAX_QUERY_RESULTS);
                    break;
                case 0: // 地点
                    count = queryItemsByLocation(keywordUtf8, g_queryResults, MAX_QUERY_RESULTS);
                    break;
                }

                // 清空并显示结果
                //HWND hListView = GetDlgItem(hDlg, IDC_LIST1);
                //ListView_DeleteAllItems(hListView);

                //if (count == 0)
                //{
                //    MessageBoxW(hDlg, L"未找到匹配的物品。", L"查询结果", MB_ICONINFORMATION);
                //}
                //else
                //{
                //    wchar_t wBuf[256];
                //    for (int i = 0; i < count; i++)
                //    {
                //        const LostItem* pItem = g_queryResults[i];

                //        // ID
                //        wchar_t idStr[20];
                //        swprintf(idStr, 20, L"%d", pItem->id);
                //        LVITEM lvItem = { 0 };
                //        lvItem.mask = LVIF_TEXT;
                //        lvItem.iItem = i;
                //        lvItem.iSubItem = 0;
                //        lvItem.pszText = idStr;
                //        int itemIndex = ListView_InsertItem(hListView, &lvItem);

                //        // 名称
                //        ZeroMemory(wBuf, sizeof(wBuf));
                //        mbstowcs(wBuf, pItem->name, 255);
                //        ListView_SetItemText(hListView, itemIndex, 1, wBuf);

                //        // 类别
                //        ZeroMemory(wBuf, sizeof(wBuf));
                //        mbstowcs(wBuf, pItem->category, 255);
                //        ListView_SetItemText(hListView, itemIndex, 2, wBuf);

                //        // 型号/特征
                //        ZeroMemory(wBuf, sizeof(wBuf));
                //        mbstowcs(wBuf, pItem->model, 255);
                //        ListView_SetItemText(hListView, itemIndex, 3, wBuf);

                //        // 发现地点
                //        ZeroMemory(wBuf, sizeof(wBuf));
                //        mbstowcs(wBuf, pItem->location, 255);
                //        ListView_SetItemText(hListView, itemIndex, 4, wBuf);

                //        // 遗失时间
                //        ZeroMemory(wBuf, sizeof(wBuf));
                //        mbstowcs(wBuf, pItem->lost_date, 255);
                //        ListView_SetItemText(hListView, itemIndex, 5, wBuf);

                //        // 状态
                //        ListView_SetItemText(hListView, itemIndex, 6,
                //            const_cast<LPWSTR>(pItem->confirmed ? L"已确认" : L"待确认"));

                //        // 登记人
                //        ZeroMemory(wBuf, sizeof(wBuf));
                //        mbstowcs(wBuf, pItem->finder, 255);
                //        ListView_SetItemText(hListView, itemIndex, 7, wBuf);
                //    }
                //}
                HWND hListView = GetDlgItem(hDlg, IDC_LIST1);
                ListView_DeleteAllItems(hListView);

                if (count == 0)
                {
                    MessageBoxW(hDlg, L"未找到匹配的物品。", L"查询结果", MB_ICONINFORMATION);
                }
                else
                {
                    wchar_t wBuf[256];
                    for (int i = 0; i < count; i++)
                    {
                        const LostItem* pItem = g_queryResults[i];

                        // ID
                        wchar_t idStr[20];
                        swprintf(idStr, 20, L"%d", pItem->id);
                        LVITEM lvItem = { 0 };
                        lvItem.mask = LVIF_TEXT;
                        lvItem.iItem = i;
                        lvItem.iSubItem = 0;
                        lvItem.pszText = idStr;
                        int itemIndex = ListView_InsertItem(hListView, &lvItem);

                        // 名称
                        ZeroMemory(wBuf, sizeof(wBuf));
                        MultiByteToWideChar(CP_UTF8, 0, pItem->name, -1, wBuf, 255);
                        ListView_SetItemText(hListView, itemIndex, 1, wBuf);

                        // 类别
                        ZeroMemory(wBuf, sizeof(wBuf));
                        MultiByteToWideChar(CP_UTF8, 0, pItem->category, -1, wBuf, 255);
                        ListView_SetItemText(hListView, itemIndex, 2, wBuf);

                        // 型号/特征
                        ZeroMemory(wBuf, sizeof(wBuf));
                        MultiByteToWideChar(CP_UTF8, 0, pItem->model, -1, wBuf, 255);
                        ListView_SetItemText(hListView, itemIndex, 3, wBuf);

                        // 发现地点
                        ZeroMemory(wBuf, sizeof(wBuf));
                        MultiByteToWideChar(CP_UTF8, 0, pItem->location, -1, wBuf, 255);
                        ListView_SetItemText(hListView, itemIndex, 4, wBuf);

                        // 遗失时间
                        ZeroMemory(wBuf, sizeof(wBuf));
                        MultiByteToWideChar(CP_UTF8, 0, pItem->lost_date, -1, wBuf, 255);
                        ListView_SetItemText(hListView, itemIndex, 5, wBuf);

                        // 状态
                        ListView_SetItemText(hListView, itemIndex, 6,
                            const_cast<LPWSTR>(pItem->confirmed ? L"已确认" : L"待确认"));

                        // 登记人
                        ZeroMemory(wBuf, sizeof(wBuf));
                        MultiByteToWideChar(CP_UTF8, 0, pItem->finder, -1, wBuf, 255);
                        ListView_SetItemText(hListView, itemIndex, 7, wBuf);
                    }
                }
                break;
            }

            case IDCANCEL:
                EndDialog(hDlg, IDCANCEL);
                return (INT_PTR)TRUE;
            }
        }
    }
    break;

    case WM_CLOSE:
        EndDialog(hDlg, IDCANCEL);
        return (INT_PTR)TRUE;

    default:
        break;
    }

    return (INT_PTR)FALSE;
}

// 刷新未确认物品列表
//void RefreshUnconfirmedItems(HWND hDlg)
//{
//    HWND hListView = GetDlgItem(hDlg, IDC_LIST6);
//    ListView_DeleteAllItems(hListView);
//
//    const LostItem* results[MAX_QUERY_RESULTS];
//    int totalCount = queryItemsByCategory("", results, MAX_QUERY_RESULTS); // 查询所有
//
//    int itemIndex = 0;
//    for (int i = 0; i < totalCount; i++)
//    {
//        const LostItem* pItem = results[i];
//        if (pItem->confirmed) continue; // 只显示未确认
//
//        wchar_t buf[256];
//
//        // 第0列：ID
//        wchar_t idStr[20];
//        swprintf(idStr, 20, L"%d", pItem->id);
//        LVITEM lvItem = { 0 };
//        lvItem.mask = LVIF_TEXT;
//        lvItem.iItem = itemIndex;
//        lvItem.iSubItem = 0;
//        lvItem.pszText = idStr;
//        ListView_InsertItem(hListView, &lvItem);
//
//        // 名称
//        mbstowcs(buf, pItem->name, 255);
//        ListView_SetItemText(hListView, itemIndex, 1, buf);
//
//        // 类别
//        mbstowcs(buf, pItem->category, 255);
//        ListView_SetItemText(hListView, itemIndex, 2, buf);
//
//        // 型号
//        mbstowcs(buf, pItem->model, 255);
//        ListView_SetItemText(hListView, itemIndex, 3, buf);
//
//        // 地点
//        mbstowcs(buf, pItem->location, 255);
//        ListView_SetItemText(hListView, itemIndex, 4, buf);
//
//        // 遗失时间
//        mbstowcs(buf, pItem->lost_date, 255);
//        ListView_SetItemText(hListView, itemIndex, 5, buf);
//
//        // 登记人
//        mbstowcs(buf, pItem->finder, 255);
//        ListView_SetItemText(hListView, itemIndex, 6, buf);
//
//        itemIndex++;
//    }
//}

void RefreshUnconfirmedItems(HWND hDlg)
{
    HWND hListView = GetDlgItem(hDlg, IDC_LIST6);
    ListView_DeleteAllItems(hListView);

    const LostItem* results[MAX_QUERY_RESULTS];
    int totalCount = queryItemsByCategory("", results, MAX_QUERY_RESULTS); // 查询所有

    int itemIndex = 0;
    for (int i = 0; i < totalCount; i++)
    {
        const LostItem* pItem = results[i];
        if (pItem->confirmed) continue; // 只显示未确认

        wchar_t buf[256]; // 用于 UTF-8 -> UTF-16 转换的缓冲区

        // 第0列：ID
        wchar_t idStr[20];
        swprintf(idStr, 20, L"%d", pItem->id);
        LVITEM lvItem = { 0 };
        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = itemIndex;
        lvItem.iSubItem = 0;
        lvItem.pszText = idStr;
        ListView_InsertItem(hListView, &lvItem);

        // --- 名称 ---
        ZeroMemory(buf, sizeof(buf));
        MultiByteToWideChar(CP_UTF8, 0, pItem->name, -1, buf, 255);
        ListView_SetItemText(hListView, itemIndex, 1, buf);

        // --- 类别 ---
        ZeroMemory(buf, sizeof(buf));
        MultiByteToWideChar(CP_UTF8, 0, pItem->category, -1, buf, 255);
        ListView_SetItemText(hListView, itemIndex, 2, buf);

        // --- 型号 ---
        ZeroMemory(buf, sizeof(buf));
        MultiByteToWideChar(CP_UTF8, 0, pItem->model, -1, buf, 255);
        ListView_SetItemText(hListView, itemIndex, 3, buf);

        // --- 地点 ---
        ZeroMemory(buf, sizeof(buf));
        MultiByteToWideChar(CP_UTF8, 0, pItem->location, -1, buf, 255);
        ListView_SetItemText(hListView, itemIndex, 4, buf);

        // --- 遗失时间 ---
        ZeroMemory(buf, sizeof(buf));
        MultiByteToWideChar(CP_UTF8, 0, pItem->lost_date, -1, buf, 255);
        ListView_SetItemText(hListView, itemIndex, 5, buf);

        // --- 登记人 ---
        ZeroMemory(buf, sizeof(buf));
        MultiByteToWideChar(CP_UTF8, 0, pItem->finder, -1, buf, 255);
        ListView_SetItemText(hListView, itemIndex, 6, buf);

        itemIndex++;
    }
}

// 确认物品管理对话框
INT_PTR CALLBACK CheckItem(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
    {
        // 获取列表控件句柄
        HWND hListView = GetDlgItem(hDlg, IDC_LIST6);

        // 设置为报表风格
        DWORD style = GetWindowLong(hListView, GWL_STYLE);
        SetWindowLong(hListView, GWL_STYLE, style | LVS_REPORT | LVS_SINGLESEL);

        // 初始化列
        LVCOLUMN lvCol;
        ZeroMemory(&lvCol, sizeof(lvCol));
        lvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

        const wchar_t* columns[] = {
            L"ID",     L"名称",      L"类别",    L"型号",
            L"地点",   L"遗失时间",  L"登记人"
        };
        int widths[] = { 60, 120, 90, 100, 100, 120, 80 };

        for (int i = 0; i < 7; i++)
        {
            lvCol.pszText = (LPWSTR)columns[i];
            lvCol.cx = widths[i];
            lvCol.iSubItem = i;
            ListView_InsertColumn(hListView, i, &lvCol);
        }

        // 加载未确认物品
        RefreshUnconfirmedItems(hDlg);
        return (INT_PTR)TRUE;
    }

    case WM_COMMAND:
    {
        WORD wmId = LOWORD(wParam);
        WORD wmEvent = HIWORD(wParam);

        if (wmEvent == BN_CLICKED)
        {
            switch (wmId)
            {
            case IDC_CHECK: // 确认物品
            {
                HWND hListView = GetDlgItem(hDlg, IDC_LIST6);
                int selected = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
                if (selected == -1)
                {
                    MessageBoxW(hDlg, L"请先选择一条待确认的物品！", L"提示", MB_ICONWARNING);
                    break;
                }

                // 获取选中行的第一列（ID）
                wchar_t idStr[20];
                ListView_GetItemText(hListView, selected, 0, idStr, 20);
                int itemId = _wtoi(idStr);

                if (confirmItem(itemId))
                {
                    MessageBoxW(hDlg, L"物品已确认！", L"成功", MB_ICONINFORMATION);
                    RefreshUnconfirmedItems(hDlg); // 刷新列表
                }
                else
                {
                    MessageBoxW(hDlg, L"确认失败，可能该物品已被确认或不存在。", L"错误", MB_ICONERROR);
                }
                break;
            }

            case IDC_REFRESH: // 手动刷新
            {
                RefreshUnconfirmedItems(hDlg);
                break;
            }

            case IDC_SAVE_AND_QUIT2: // 保存并退出
            {
                saveAllItems(); // 将所有内存数据按日期写回文件（可选，registerItem 已保存）
                EndDialog(hDlg, 0);
                break;
            }
            }
        }
        break;
    }

    case WM_CLOSE:
    {
        // 用户点击右上角 ×
        EndDialog(hDlg, 0);
        return (INT_PTR)TRUE;
    }
    }

    return (INT_PTR)FALSE;
}

// 生成上周（周一到周日）的日期范围字符串（格式：YYYY-MM-DD）
void get_last_week_monday_and_sunday(char* start_date, char* end_date) {
    time_t now;
    struct tm today;
    time(&now);
    localtime_s(&today, &now); // Windows安全版本

    // 当前星期几 (0=Sunday, 1=Monday, ..., 6=Saturday)
    int wday = today.tm_wday == 0 ? 7 : today.tm_wday; // 转为 1~7，周一为1

    // 上周一距离今天天数 = 当前是第几天 - 1 + 7
    int days_to_last_monday = wday - 1 + 7;

    struct tm mon_start = today;
    mon_start.tm_hour = mon_start.tm_min = mon_start.tm_sec = 0;
    mktime(&mon_start);

    // 计算上周一和上周日的时间戳
    time_t last_monday = mktime(&mon_start) - (days_to_last_monday * 86400);
    time_t last_sunday = last_monday + 6 * 86400;

    // 转回结构体并格式化
    struct tm tm_mon, tm_sun;
    localtime_s(&tm_mon, &last_monday);
    localtime_s(&tm_sun, &last_sunday);

    strftime(start_date, 11, "%Y-%m-%d", &tm_mon);
    strftime(end_date, 11, "%Y-%m-%d", &tm_sun);
}

//// 生成上周物品清单
//void generateLastWeekItemsReport(HWND hwndParent) {
//    char start_date[11], end_date[11];
//    get_last_week_monday_and_sunday(start_date, end_date);
//
//    // 用于存储查询结果的指针数组
//    const LostItem* results[MAX_QUERY_RESULTS];
//    int count = 0;
//
//    // 遍历所有物品，筛选遗失时间在上周范围内的
//    for (int i = 0; i < itemCount && count < MAX_QUERY_RESULTS; i++) {
//        const LostItem* item = &items[i];
//        if (strlen(item->lost_date) < 10) continue;
//
//        char item_date[11];
//        strncpy(item_date, item->lost_date, 10);
//        item_date[10] = '\0';
//
//        // 简单字符串比较判断是否在日期范围内
//        if (strcmp(item_date, start_date) >= 0 && strcmp(item_date, end_date) <= 0) {
//            results[count++] = item;
//        }
//    }
//
//    // 输出文件名
//    char filename[256];
//    sprintf(filename, "%s\\_%s_ to _%s.txt", SUMMARY_FOLDER, start_date, end_date);
//
//    FILE* fp = fopen(filename, "w");
//    if (!fp) {
//        MessageBoxA(hwndParent, "无法创建文件！", "错误", MB_OK | MB_ICONERROR);
//        return;
//    }
//
//    fprintf(fp, "==================== 上周遗失物品清单 ====================\n");
//    fprintf(fp, "统计时间范围: %s 至 %s\n", start_date, end_date);
//    fprintf(fp, "共 %d 件物品\n", count);
//    fprintf(fp, "========================================================\n");
//    fprintf(fp, "%-4s %-15s %-10s %-12s %-15s %-12s %-10s\n",
//        "ID", "名称", "类别", "型号", "地点", "遗失时间", "登记人");
//    fprintf(fp, "--------------------------------------------------------\n");
//
//    for (int i = 0; i < count; i++) {
//        const LostItem* p = results[i];
//        fprintf(fp, "%-4d %-15s %-10s %-12s %-15s %-12.16s %-10s\n",
//            p->id,
//            p->name,
//            p->category,
//            p->model,
//            p->location,
//            p->lost_date,
//            p->finder);
//    }
//
//    fclose(fp);
//
//}
//
//// 生成拍卖清单
//void generateAuctionItemsList(HWND hwndParent) {
//    time_t now;
//    time(&now);
//
//    char filename[256];
//    sprintf(filename, "%s\\auction_%d-%02d-%02d.txt",
//        AUCTION_FOLDER,
//        (int)localtime(&now)->tm_year + 1900,
//        (int)localtime(&now)->tm_mon + 1,
//        (int)localtime(&now)->tm_mday);
//
//    FILE* fp = fopen(filename, "w");
//    if (!fp) {
//        MessageBoxA(hwndParent, "无法创建拍卖物品清单文件！", "错误", MB_OK | MB_ICONERROR);
//        return;
//    }
//
//    fprintf(fp, "==================== 待拍卖物品清单 ====================\n");
//    fprintf(fp, "生成时间: %d-%02d-%02d %02d:%02d\n",
//        (int)localtime(&now)->tm_year + 1900,
//        (int)localtime(&now)->tm_mon + 1,
//        (int)localtime(&now)->tm_mday,
//        (int)localtime(&now)->tm_hour,
//        (int)localtime(&now)->tm_min);
//    fprintf(fp, "说明: 丢失超过一年且未被认领的物品\n");
//    fprintf(fp, "========================================================\n\n");
//
//    int count = 0;
//    for (int i = 0; i < itemCount; i++) {
//        const LostItem* item = &items[i];
//
//        // 已认领的跳过
//        if (item->claimed) continue;
//
//        struct tm tm_item = { 0 };
//        if (!parseDateTime(item->lost_date, &tm_item)) continue;
//
//        time_t time_item = mktime(&tm_item);
//        if (time_item == -1) continue;
//
//        double days_diff = difftime(now, time_item) / (60 * 60 * 24);
//        if (days_diff > 365) {
//            fprintf(fp, "ID: %d\n", item->id);
//            fprintf(fp, "物品名称: %s\n", item->name);
//            fprintf(fp, "类别: %s\n", item->category);
//            fprintf(fp, "型号/特征: %s\n", item->model);
//            fprintf(fp, "发现地点: %s\n", item->location);
//            fprintf(fp, "遗失时间: %s\n", item->lost_date);
//            fprintf(fp, "登记人: %s\n", item->finder);
//            fprintf(fp, "----------------------------------------\n");
//            count++;
//        }
//    }
//
//    fclose(fp);
//
//}

// 修改后的生成上周物品清单函数
//void generateLastWeekItemsReport(HWND hwndParent) {
//    char start_date[11], end_date[11];
//    get_last_week_monday_and_sunday(start_date, end_date);
//
//    // 用于存储查询结果的指针数组
//    const LostItem* results[MAX_QUERY_RESULTS];
//    int count = 0;
//
//    // 遍历所有物品，筛选遗失时间在上周范围内的
//    for (int i = 0; i < itemCount && count < MAX_QUERY_RESULTS; i++) {
//        const LostItem* item = &items[i];
//        if (strlen(item->lost_date) < 10) continue;
//
//        char item_date[11];
//        strncpy(item_date, item->lost_date, 10);
//        item_date[10] = '\0';
//
//        // 简单字符串比较判断是否在日期范围内
//        if (strcmp(item_date, start_date) >= 0 && strcmp(item_date, end_date) <= 0) {
//            results[count++] = item;
//        }
//    }
//
//    // 使用宽字符路径
//    wchar_t filename[MAX_PATH];
//    swprintf(filename, MAX_PATH, L"%hs\\_%hs_ 至 _%hs一周物品清单.txt", SUMMARY_FOLDER, start_date, end_date);
//
//    FILE* fp = _wfopen(filename, L"w");
//    if (!fp) {
//        MessageBoxW(hwndParent, L"无法创建文件！", L"错误", MB_OK | MB_ICONERROR);
//        return;
//    }
//
//    fprintf(fp, "==================== 上周遗失物品清单 ====================\n");
//    fprintf(fp, "统计时间范围: %s 至 %s\n", start_date, end_date);
//    fprintf(fp, "共 %d 件物品\n", count);
//    fprintf(fp, "========================================================\n");
//    fprintf(fp, "%-4s %-15s %-10s %-12s %-15s %-12s %-10s\n",
//        "ID", "名称", "类别", "型号", "地点", "遗失时间", "登记人");
//    fprintf(fp, "--------------------------------------------------------\n");
//
//    for (int i = 0; i < count; i++) {
//        const LostItem* p = results[i];
//        fprintf(fp, "%-4d %-15s %-10s %-12s %-15s %-12.16s %-10s\n",
//            p->id,
//            p->name,
//            p->category,
//            p->model,
//            p->location,
//            p->lost_date,
//            p->finder);
//    }
//
//    fclose(fp);
//
//    // 添加保存完成提示
//    wchar_t message[MAX_PATH];
//    swprintf(message, MAX_PATH, L"上周物品清单已保存至：\n%ls", filename);
//    MessageBoxW(hwndParent, message, L"保存成功", MB_OK | MB_ICONINFORMATION);
//}

// ========================
// 生成上周遗失物品清单（整合优化版）
// ========================
void generateLastWeekItemsReport(HWND hwndParent) {
    char start_date[11], end_date[11];
    get_last_week_monday_and_sunday(start_date, end_date);

    // 用于存储符合条件的物品
    LostItem weeklyItems[MAX_QUERY_RESULTS];
    int count = 0;

    // 遍历所有物品，筛选遗失时间在上周范围内的且已确认的物品
    for (int i = 0; i < itemCount && count < MAX_QUERY_RESULTS; i++) {
        const LostItem* item = &items[i];
        if (!item->confirmed || strlen(item->lost_date) < 10) continue;

        char item_date[11];
        strncpy(item_date, item->lost_date, 10);
        item_date[10] = '\0';

        // 判断是否在上周范围内（字符串比较适用于 YYYY-MM-DD 格式）
        if (strcmp(item_date, start_date) >= 0 && strcmp(item_date, end_date) <= 0) {
            weeklyItems[count++] = *item;
        }
    }

    // 构建保存文件路径：使用宽字符
    wchar_t filename[MAX_PATH];
    swprintf(filename, MAX_PATH, L"%hs\\summary_%.4s%.2s%.2s.txt",
        SUMMARY_FOLDER,
        start_date, start_date + 5, start_date + 8); // 用起始日期命名，如 summary_20250317.txt

    FILE* fp = _wfopen(filename, L"w,ccs=UTF-8"); // 显式指定 UTF-8 编码
    if (!fp) {
        MessageBoxW(hwndParent, L"无法创建文件！", L"错误", MB_OK | MB_ICONERROR);
        return;
    }

    // 输出报告标题与元信息
    printDivider(fp, 80);
    fprintf(fp, "%*s\n", 60, "机房失物招领上周汇总");
    printDivider(fp, 80);
    fprintf(fp, "汇总日期范围: %s 至 %s\n", start_date, end_date);
    fprintf(fp, "\n物品总数: %d 件\n\n", count);

    if (count == 0) {
        fprintf(fp, "本周内无新增遗失物品记录。\n");
        fclose(fp);

        // 提示用户没有数据
        wchar_t message[MAX_PATH];
        swprintf(message, MAX_PATH, L"上周无新增物品记录，但已生成空报告：\n%ls", filename);
        MessageBoxW(hwndParent, message, L"提示", MB_OK | MB_ICONINFORMATION);
        return;
    }

    // 逐条输出物品详情
    for (int i = 0; i < count; i++) {
        const LostItem* p = &weeklyItems[i];
        fprintf(fp, "物品 %d/%d:\n", i + 1, count);
        fprintf(fp, "  ID:           %d\n", p->id);
        fprintf(fp, "  名称:         %s\n", p->name);
        fprintf(fp, "  类别:         %s\n", p->category);
        fprintf(fp, "  型号/特征:    %s\n", p->model);
        fprintf(fp, "  发现地点:     %s\n", p->location);
        fprintf(fp, "  发现时间:     %s\n", p->lost_date);
        fprintf(fp, "  登记人:       %s\n", p->finder);
        fprintf(fp, "  状态:         %s\n", p->claimed ? "已认领" : "未认领");
        printDivider(fp, 40);
        fprintf(fp, "\n");
    }

    fclose(fp);

    // 弹出成功提示（宽字符）
    wchar_t message[MAX_PATH];
    swprintf(message, MAX_PATH, L"上周物品清单已保存至：\n%ls", filename);
    MessageBoxW(hwndParent, message, L"保存成功", MB_OK | MB_ICONINFORMATION);
}

// 修改后的生成拍卖清单函数
//void generateAuctionItemsList(HWND hwndParent) {
//    time_t now;
//    time(&now);
//
//    struct tm* local_tm = localtime(&now);
//
//    // 使用宽字符路径
//    wchar_t filename[MAX_PATH];
//    swprintf(filename, MAX_PATH, L"%hs\\拍卖清单_%d-%02d-%02d.txt",
//        AUCTION_FOLDER,
//        (int)local_tm->tm_year + 1900,
//        (int)local_tm->tm_mon + 1,
//        (int)local_tm->tm_mday);
//
//    FILE* fp = _wfopen(filename, L"w");
//    if (!fp) {
//        MessageBoxW(hwndParent, L"无法创建拍卖物品清单文件！", L"错误", MB_OK | MB_ICONERROR);
//        return;
//    }
//
//    fprintf(fp, "==================== 待拍卖物品清单 ====================\n");
//    fprintf(fp, "生成时间: %d-%02d-%02d %02d:%02d\n",
//        (int)local_tm->tm_year + 1900,
//        (int)local_tm->tm_mon + 1,
//        (int)local_tm->tm_mday,
//        (int)local_tm->tm_hour,
//        (int)local_tm->tm_min);
//    fprintf(fp, "说明: 丢失超过一年且未被认领的物品\n");
//    fprintf(fp, "========================================================\n\n");
//
//    int count = 0;
//    for (int i = 0; i < itemCount; i++) {
//        const LostItem* item = &items[i];
//
//        // 已认领的跳过
//        if (item->claimed) continue;
//
//        struct tm tm_item = { 0 };
//        if (!parseDateTime(item->lost_date, &tm_item)) continue;
//
//        time_t time_item = mktime(&tm_item);
//        if (time_item == -1) continue;
//
//        double days_diff = difftime(now, time_item) / (60 * 60 * 24);
//        if (days_diff > 365) {
//            fprintf(fp, "ID: %d\n", item->id);
//            fprintf(fp, "物品名称: %s\n", item->name);
//            fprintf(fp, "类别: %s\n", item->category);
//            fprintf(fp, "型号/特征: %s\n", item->model);
//            fprintf(fp, "发现地点: %s\n", item->location);
//            fprintf(fp, "遗失时间: %s\n", item->lost_date);
//            fprintf(fp, "登记人: %s\n", item->finder);
//            fprintf(fp, "----------------------------------------\n");
//            count++;
//        }
//    }
//
//    fclose(fp);
//
//    // 添加保存完成提示
//    wchar_t message[MAX_PATH];
//    swprintf(message, MAX_PATH, L"待拍卖物品清单已保存至：\n%ls", filename);
//    MessageBoxW(hwndParent, message, L"保存成功", MB_OK | MB_ICONINFORMATION);
//}

// ========================
// 生成待拍卖物品清单（整合优化版）
// ========================
void generateAuctionItemsList(HWND hwndParent) {
    time_t now;
    time(&now);

    struct tm* local_tm = localtime(&now);
    char oneYearAgo[20], today[20];

    // 格式化当前时间
    strftime(today, sizeof(today), "%Y-%m-%d %H:%M", local_tm);

    // 计算一年前的日期
    struct tm tm_one_year_ago = *local_tm;
    tm_one_year_ago.tm_year -= 1;
    mktime(&tm_one_year_ago);
    strftime(oneYearAgo, sizeof(oneYearAgo), "%Y-%m-%d %H:%M", &tm_one_year_ago);

    // 收集符合条件的待拍卖物品
    LostItem auctionItems[MAX_QUERY_RESULTS];
    int count = 0;

    for (int i = 0; i < itemCount && count < MAX_QUERY_RESULTS; i++) {
        const LostItem* item = &items[i];

        // 必须已确认且未认领
        if (!item->confirmed || item->claimed) continue;

        struct tm tm_item = { 0 };
        if (!parseDateTime(item->lost_date, &tm_item)) continue;

        time_t time_item = mktime(&tm_item);
        if (time_item == -1) continue;

        // 判断是否早于一年前（即存放时间超过365天）
        if (difftime(time_item, mktime(&tm_one_year_ago)) < 0) {
            auctionItems[count++] = *item;
        }
    }

    // 构建宽字符文件路径
    wchar_t filename[MAX_PATH];
    swprintf(filename, MAX_PATH, L"%hs\\auction_%d-%02d.txt",
        AUCTION_FOLDER,
        (int)local_tm->tm_year + 1900,
        (int)local_tm->tm_mon + 1);

    FILE* fp = _wfopen(filename, L"w,ccs=UTF-8"); // 使用 UTF-8 编码避免中文乱码
    if (!fp) {
        MessageBoxW(hwndParent, L"无法创建待拍卖物品清单文件！", L"错误", MB_OK | MB_ICONERROR);
        return;
    }

    // 输出报告标题与元信息
    printDivider(fp, 80);
    fprintf(fp, "%*s\n", 60, "机房失物招领待拍卖物品清单");
    printDivider(fp, 80);
    fprintf(fp, "生成日期: %s\n", today);
    fprintf(fp, "筛选条件: 在 %s 之前丢失且未被认领的物品\n", oneYearAgo);
    fprintf(fp, "物品总数: %d 件\n\n", count);

    if (count == 0) {
        fprintf(fp, "目前没有符合拍卖条件的物品（丢失超过一年且未认领）。\n");
        fclose(fp);

        wchar_t message[MAX_PATH];
        swprintf(message, MAX_PATH, L"无待拍卖物品，但已生成空报告：\n%ls", filename);
        MessageBoxW(hwndParent, message, L"提示", MB_OK | MB_ICONINFORMATION);
        return;
    }

    // 逐条输出物品详情
    for (int i = 0; i < count; i++) {
        const LostItem* p = &auctionItems[i];
        int days_held = (int)(difftime(now, mktime(&tm_one_year_ago)) / (60 * 60 * 24)) +
            (int)(difftime(mktime(&tm_one_year_ago), mktime(parseDateTimeToTm(p->lost_date))) / (60 * 60 * 24)); // 更准确可写辅助函数
        // 简化：直接计算从遗失到今天的天数
        struct tm tm_lost = { 0 };
        parseDateTime(p->lost_date, &tm_lost);
        int days_since_lost = (int)difftime(now, mktime(&tm_lost)) / (60 * 60 * 24);

        fprintf(fp, "物品 %d/%d:\n", i + 1, count);
        fprintf(fp, "  ID:           %d\n", p->id);
        fprintf(fp, "  名称:         %s\n", p->name);
        fprintf(fp, "  类别:         %s\n", p->category);
        fprintf(fp, "  型号/特征:    %s\n", p->model);
        fprintf(fp, "  发现地点:     %s\n", p->location);
        fprintf(fp, "  发现时间:     %s\n", p->lost_date);
        fprintf(fp, "  登记人:       %s\n", p->finder);
        fprintf(fp, "  存放时间:     %d 天\n", days_since_lost);
        printDivider(fp, 40);
        fprintf(fp, "\n");
    }

    // 公益说明
    fprintf(fp, "\n");
    printDivider(fp, 80);
    fprintf(fp, "                     拍卖所得将捐赠给希望工程\n");
    printDivider(fp, 80);

    fclose(fp);

    // 弹出成功提示
    wchar_t message[MAX_PATH];
    swprintf(message, MAX_PATH, L"待拍卖物品清单已保存至：\n%ls", filename);
    MessageBoxW(hwndParent, message, L"保存成功", MB_OK | MB_ICONINFORMATION);
}


// 解析日期字符串为 struct tm（假设已有 parseDateTime）
// 示例：bool parseDateTime(const char* str, struct tm* tm);

// 从字符串生成 tm 结构（供内部使用）
struct tm* parseDateTimeToTm(const char* datetime_str) {
    static struct tm tm_result = { 0 };
    memset(&tm_result, 0, sizeof(struct tm));
    if (sscanf(datetime_str, "%d-%d-%d %d:%d:%d",
        &tm_result.tm_year, &tm_result.tm_mon, &tm_result.tm_mday,
        &tm_result.tm_hour, &tm_result.tm_min, &tm_result.tm_sec) == 6) {
        tm_result.tm_year -= 1900;
        tm_result.tm_mon -= 1;
        return &tm_result;
    }
    return NULL;
}