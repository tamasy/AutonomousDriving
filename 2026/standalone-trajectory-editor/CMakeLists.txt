cmake_minimum_required(VERSION 3.16)
project(trajectory_editor 
    VERSION 1.0.0
    DESCRIPTION "Standalone Trajectory Editor for Automotive/Robotics Applications"
    LANGUAGES CXX)

# C++標準の設定
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# デバッグ情報とコンパイラオプション
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Qt5の検索
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

# MOC（Meta-Object Compiler）の自動実行
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# インクルードディレクトリ
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ソースファイル
set(SOURCES
  src/main.cpp
  src/core/trajectory_data.cpp
  src/core/edit_history.cpp
  src/core/track_boundaries.cpp
  src/utils/csv_parser.cpp
  src/gui/graphics_trajectory_view.cpp
)

# ヘッダーファイル
set(HEADERS
  src/core/trajectory_data.hpp
  src/core/edit_history.hpp
  src/core/track_boundaries.hpp
  src/utils/csv_parser.hpp
  src/gui/graphics_trajectory_view.hpp
)

# 実行ファイル
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# ライブラリのリンク
target_link_libraries(${PROJECT_NAME}
  Qt5::Core
  Qt5::Widgets
)

# インストール設定
install(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION bin
)

# OSM to CSV converter
add_executable(osm_to_csv_converter osm_to_csv_converter.cpp src/utils/osm_parser.cpp)
target_include_directories(osm_to_csv_converter PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})