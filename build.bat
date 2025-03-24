@echo off
echo Компиляция GitHub Автоматизатора...

if "%1" == "encoding" (
    set COMPILE_MAIN=encoding_test.cpp
    set OUT_FILE=encoding_test.exe
    echo Режим: тестирование кодировки
) else (
    set COMPILE_MAIN=main.cpp
    set OUT_FILE=github_automation.exe
    echo Режим: основное приложение
)

REM Проверяем наличие MSVC
where cl >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo Компиляция с использованием MSVC...
    cl /EHsc /std:c++17 %COMPILE_MAIN% /Fe:%OUT_FILE%
    if %ERRORLEVEL% EQU 0 (
        echo Компиляция успешно завершена!
        goto end
    ) else (
        echo Ошибка компиляции с MSVC
    )
)

REM Пробуем MinGW, если MSVC не найден
where g++ >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo Компиляция с использованием MinGW...
    g++ -std=c++17 %COMPILE_MAIN% -o %OUT_FILE%
    if %ERRORLEVEL% EQU 0 (
        echo Компиляция успешно завершена!
        goto end
    ) else (
        echo Ошибка компиляции с MinGW
    )
)

echo Не найден подходящий компилятор C++. Установите MSVC или MinGW.

:end
echo.
echo Для запуска программы выполните: %OUT_FILE%
echo.
pause 