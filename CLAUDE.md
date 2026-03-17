# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a **Lost & Found Management System** - a Windows desktop application built with Win32 API (C/C++). The application manages lost items, user authentication, and administrative functions including weekly reports and auction preparation.

## Build & Development

### Prerequisites
- Visual Studio 2022 (v143 toolset)
- Windows SDK 10.0
- Unicode character set

### Building
Open `ProjectL&F/ProjectL&F.sln` in Visual Studio and build:
- **Debug|x64** - Standard development configuration
- **Release|x64** - Production build with optimizations

### Key Build Notes
- The project uses C++20 standard (`/std:c++20`)
- Links against `comctl32.lib` for common controls
- Uses `_CRT_SECURE_NO_WARNINGS` to suppress CRT deprecation warnings
- Character set: Unicode
- Resource file (`L&F.rc`) requires UTF-8 execution character set (`#pragma execution_character_set("utf-8")`)

## Architecture

### Core Modules

```
ProjectL&F/
├── L&F/
│   ├── L&F.cpp        # Main entry point, Win32 UI handling
│   ├── user.c/h       # User authentication & role management
│   ├── item.c/h       # Lost item CRUD operations
│   ├── framework.h    # Windows/CRT includes
│   └── resource.h     # Dialog/control IDs
```

### User System (user.c)
- **Roles**: `ROLE_SUPER_ADMIN`, `ROLE_ADMIN`, `ROLE_STUDENT`
- **Authentication**: Username/password stored in `users.dat` (binary format)
- **Default admin**: username=`sa`, password=`sa123` (created on first run)
- Users are persisted to `users.dat` in the application directory

### Item System (item.c)
- **Storage**: Items saved as text files in `lost_and_found_records/` folder
- **File naming**: `{date}.txt` (YYYY-MM-DD format)
- **Max capacity**: 1000 items
- **Features**: Register, confirm, delete, claim, query, sort

### Main Dialogs
- `IDD_LOGIN` (102) - User authentication
- `IDD_USERS_MANAGER` (131) - User administration
- `IDD_ITEMS_MANAGER` (134) - Item management
- `IDD_ITEM_REGISTER` (130) - New item registration
- `IDD_ITEM_DELETE` (136) - Item deletion
- `IDD_ITEM_CLAIMER` (137) - Item claiming
- `IDD_QUERY` (138) - Search/query interface

### Key Global Variables
- `users[100]` - User array from user.c
- `items[1000]` - Item array from item.c
- `current_operator[32]` - Current logged-in user
- `ghMainWnd` - Main window handle

### Data Flow
1. App starts → `init_user_system()` loads users from `users.dat`
2. `loadAllData()` loads existing item records
3. User logs in → `UpdateMenuForRole()` adjusts menu visibility
4. Items are registered → saved to dated text files
5. Weekly summaries generated to `weekly_summaries/`
6. Auction prep files written to `auction_preparation/`

### Folder Structure (created at runtime)
```
LF/                              # Main application folder
├── users.dat                    # User database
├── lost_and_found_records/      # Item records by date
├── weekly_summaries/            # Weekly reports
└── auction_preparation/         # Auction item lists
```

### Important Constants (item.h)
- `MAX_ITEMS`: 1000
- `MAX_CATEGORY`: 30
- `MAX_NAME`: 50
- `MAX_MODEL`: 50
- `MAX_DESC`: 200
- `MAX_LOCATION`: 100
- `MAX_USER`: 20
- `MAX_QUERY_RESULTS`: 100

### Testing
`L&FTest/` contains a minimal test harness using the same project structure.
