# Lost & Found Management System

A Windows desktop application for managing lost and found items, built with Win32 API (C/C++). Designed for classroom or library use in a college setting.

## Features

- **User Management**
  - Role-based access control (Super Admin / Admin / Student)
  - Secure authentication with persistent user data
  - Default admin account: `sa` / `sa123`

- **Item Management**
  - Register lost items with category, name, model, description, location, and date
  - Confirm, delete, and claim items
  - Search by name, category, or location
  - Sort by name, ID, or date

- **Reports**
  - Weekly summary generation
  - Auction preparation for unclaimed items
  - Text-based record storage organized by date

## Project Structure

```
├── ProjectL&F/           # Main application
│   ├── L&F/
│   │   ├── L&F.cpp      # Entry point and UI handling
│   │   ├── user.c/h     # User authentication & roles
│   │   └── item.c/h     # Item CRUD operations
│   └── ProjectL&F.sln   # Visual Studio solution
└── L&FTest/             # Test harness
```

## Build Requirements

- **OS:** Windows 10/11
- **IDE:** Visual Studio 2022
- **Toolset:** v143 (VS2022)
- **Windows SDK:** 10.0
- **Language:** C/C++ with C++20 standard

## Building

1. Open `ProjectL&F/ProjectL&F.sln` in Visual Studio
2. Select configuration:
   - `Debug|x64` - Development
   - `Release|x64` - Production (optimized)
3. Build → Build Solution (Ctrl+Shift+B)

### Build Notes

- Links against `comctl32.lib` for common controls
- Uses Unicode character set
- Resource file requires UTF-8 execution character set

## Usage

1. **First Run:** The application initializes with a default super admin account
   - Username: `sa`
   - Password: `sa123`

2. **Login:** Use credentials to access the system

3. **Menu Options** (varies by role):
   - User Management (Super Admin only)
   - Item Management
   - Generate Weekly Summary
   - Auction Preparation

## Data Storage

| Data Type | Location | Format |
|-----------|----------|--------|
| Users | `users.dat` | Binary |
| Items | `lost_and_found_records/*.txt` | Text (date-named files) |
| Weekly Reports | `weekly_summaries/` | Text |
| Auction Lists | `auction_preparation/` | Text |

## Resource Files

### bgp.bmp

The background image (`bgp.bmp`, ~281MB) is **excluded from the repository** due to GitHub's 100MB file size limit. The file is listed in `.gitignore` and should be present in your local copy if you cloned from the original source.

If the background image is missing, the application will still function normally without it.

## License

This project was created as a practical programming final project for educational purposes.
