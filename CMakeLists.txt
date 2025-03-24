cmake_minimum_required(VERSION 3.10)
project(GitHub_Automation)

# Устанавливаем стандарт C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Исходные файлы проекта
add_executable(github_automation main.cpp)

# Для Windows необходимо добавить библиотеку shlwapi
if(WIN32)
    target_link_libraries(github_automation shlwapi)
endif() 