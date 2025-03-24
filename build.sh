#!/bin/bash

echo "Компиляция GitHub Автоматизатора..."

# Проверяем наличие g++
if command -v g++ &> /dev/null; then
    echo "Компиляция с использованием g++..."
    g++ -std=c++17 main.cpp -o github_automation
    
    if [ $? -eq 0 ]; then
        echo "Компиляция успешно завершена!"
        chmod +x github_automation
    else
        echo "Ошибка компиляции"
        exit 1
    fi
else
    echo "Компилятор g++ не найден. Пожалуйста, установите его."
    exit 1
fi 