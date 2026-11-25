# Qt 6 + SQLite 联系人管理示例

一个基于 **Qt 6 / C++17 / SQLite** 的 Windows 桌面应用骨架，包含完整的增删改查（CRUD）与搜索功能。

## 功能

- 联系人列表（ID、姓名、电话、邮箱）
- 新增 / 修改 / 删除联系人（带参数绑定，防 SQL 注入）
- 按姓名、电话、邮箱模糊搜索
- 数据持久化到用户目录下的 SQLite 文件：`contacts.db`

## 项目结构

```
QtSqliteDemo/
├── CMakeLists.txt                  # 构建配置（CMake + Qt6）
├── main.cpp                        # 程序入口
├── mainwindow.h/.cpp               # UI 与交互逻辑
├── database.h/.cpp                 # SQLite 封装（单例 + CRUD）
├── installer/innosetup.iss         # Windows 安装包脚本
└── .github/workflows/build-windows.yml  # 云端自动构建 + 发布
```

## Windows 环境准备

1. **安装 Visual Studio 2022**：勾选“使用 C++ 的桌面开发”工作负载。
2. **安装 Qt 6**（使用 Qt 在线安装器）：
   - 选择 Qt 6.5 或更高版本（建议 6.6/6.8 LTS）；
   - 勾选 `MSVC 2019 64-bit` 或 `MSVC 2022 64-bit` 组件；
   - SQLite 驱动（`QSQLITE`）已包含在 Qt SQL 中，无需单独安装。
3. **安装 CMake**（3.16 以上，Qt 安装器也可一并勾选）。

> 如果使用 MinGW 而非 MSVC，选择 Qt 对应的 `MinGW 64-bit` 组件即可，代码无需改动。

## 方式一：Qt Creator（推荐）

1. 打开 Qt Creator → `文件` → `打开文件或项目` → 选择本目录的 `CMakeLists.txt`。
2. 在“项目”页选择 Kit（如 `Desktop Qt 6.6.0 MSVC2019 64bit`）。
3. 点击 `Configure Project`，然后按 `Ctrl+R` 构建并运行。

## 方式二：命令行构建

在 **x64 Native Tools Command Prompt for VS 2022** 中执行（按实际 Qt 路径修改）：

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
      -DCMAKE_PREFIX_PATH="C:\Qt\6.6.0\msvc2019_64"
cmake --build build --config Release
build\Release\QtSqliteDemo.exe
```

使用 MinGW 时：

```bat
cmake -S . -B build -G "MinGW Makefiles" ^
      -DCMAKE_PREFIX_PATH="C:\Qt\6.6.0\mingw_64"
cmake --build build
build\QtSqliteDemo.exe
```

## 生成 Windows 安装包

本项目已配置好自动化流水线，无需在本地安装 Qt 也能拿到安装包。

### 方式一：GitHub Actions 自动生成（推荐）

1. 在 GitHub 上新建仓库，把本目录内容推送上去。
2. 推送一个版本标签触发构建：

```bat
git tag v1.0.0
git push origin v1.0.0
```

3. 等待工作流 `Build Windows Installer` 完成，到仓库 **Releases** 页面下载 `QtSqliteDemo-1.0.0-setup.exe`；也可在 Actions 运行页的 Artifacts 中下载。
4. 也可以在 Actions 页面手动点击 `Run workflow` 触发构建（此时只生成 Artifact，不发布 Release）。

流水线做的事：安装 Qt 6.6.3 (MSVC) → CMake 编译 → `windeployqt` 收集 Qt 运行时 DLL → Inno Setup 打包 → 上传并发布。

### 方式二：Windows 本地打包

先按“方式二：命令行构建”编译，然后执行：

```bat
mkdir build\deploy
copy build\Release\QtSqliteDemo.exe build\deploy\
windeployqt --release --no-translations build\deploy\QtSqliteDemo.exe
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer\innosetup.iss
```

生成的安装包位于 `artifacts\QtSqliteDemo-1.0.0-setup.exe`。

### 修改版本 / 应用名

编辑 `installer/innosetup.iss` 顶部的 `AppName`、`AppVersion`、`AppPublisher`，以及 `CMakeLists.txt` 中的 `project(... VERSION ...)`。若修改了 `AppId` 的 GUID，升级安装时会被识别为不同程序。

## 数据库位置

程序首次启动时自动创建数据库和表。文件位置由 `QStandardPaths::AppDataLocation` 决定，通常为：

```
C:\Users\<用户名>\AppData\Local\Example\QtSqliteDemo\contacts.db
```

如需改为程序同目录，把 `mainwindow.cpp` 构造函数中的 `dbPath` 改为：

```cpp
const QString dbPath = QCoreApplication::applicationDirPath() + "/contacts.db";
```

## 后续可扩展

- 用 `QSqlTableModel` + `QTableView` 替代手工表格，减少样板代码
- 使用 `QSqlQuery::transaction()` / `commit()` 保证批量操作原子性
- 增加输入校验、分页、导入导出 CSV
- 用 Qt Designer 的 `.ui` 文件替代代码方式构建界面

## 构建提示

若出现 `Could NOT find Qt6 (missing: Qt6_DIR)`，说明 `CMAKE_PREFIX_PATH` 未指向正确的 Qt 安装目录（应指向包含 `lib/cmake/Qt6` 的一级目录）。
