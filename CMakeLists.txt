cmake_minimum_required(VERSION 3.16)

project(QtSqliteDemo VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt 6: Widgets 提供界面，Sql 提供 SQLite 访问
find_package(Qt6 REQUIRED COMPONENTS Widgets Sql)

qt_standard_project_setup()

# Windows 下作为纯 GUI 程序启动，不弹出控制台窗口
if(WIN32)
    set(CMAKE_WIN32_EXECUTABLE TRUE)
endif()

qt_add_executable(QtSqliteDemo
    main.cpp
    core/models.h
    core/crypto.h
    core/crypto.cpp
    core/database.h
    core/database.cpp
    core/session.h
    core/session.cpp
    ui/ipage.h
    ui/barchart.h
    ui/barchart.cpp
    ui/loginwidget.h
    ui/loginwidget.cpp
    ui/mainwindow.h
    ui/mainwindow.cpp
    ui/detectionpage.h
    ui/detectionpage.cpp
    ui/detectionrecordpage.h
    ui/detectionrecordpage.cpp
    ui/devicepage.h
    ui/devicepage.cpp
    ui/accountpage.h
    ui/accountpage.cpp
    ui/driverpage.h
    ui/driverpage.cpp
    ui/vehiclepage.h
    ui/vehiclepage.cpp
    ui/licensepage.h
    ui/licensepage.cpp
    ui/logpage.h
    ui/logpage.cpp
    ui/settingspage.h
    ui/settingspage.cpp
    ui/intervalpage.h
    ui/intervalpage.cpp
)

target_include_directories(QtSqliteDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(QtSqliteDemo PRIVATE
    Qt6::Widgets
    Qt6::Sql
)
