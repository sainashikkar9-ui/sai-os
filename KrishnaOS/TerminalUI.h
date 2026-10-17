#ifndef TERMINAL_UI_H
#define TERMINAL_UI_H

#include <iostream>
#include <string>
#include <windows.h>

class TerminalUI
{
private:
    static HANDLE console()
    {
        return GetStdHandle(STD_OUTPUT_HANDLE);
    }

public:
    enum Color
    {
        BLUE,
        CYAN,
        GREEN,
        YELLOW,
        RED,
        WHITE,
        GRAY
    };

    static void setColor(Color color)
    {
        WORD attribute = FOREGROUND_INTENSITY;

        switch (color)
        {
            case BLUE:
                attribute |= FOREGROUND_BLUE;
                break;

            case CYAN:
                attribute |= FOREGROUND_BLUE |
                             FOREGROUND_GREEN;
                break;

            case GREEN:
                attribute |= FOREGROUND_GREEN;
                break;

            case YELLOW:
                attribute |= FOREGROUND_RED |
                             FOREGROUND_GREEN;
                break;

            case RED:
                attribute |= FOREGROUND_RED;
                break;

            case WHITE:
                attribute |= FOREGROUND_RED |
                             FOREGROUND_GREEN |
                             FOREGROUND_BLUE;
                break;

            case GRAY:
                attribute = FOREGROUND_RED |
                            FOREGROUND_GREEN |
                            FOREGROUND_BLUE;
                break;
        }

        SetConsoleTextAttribute(console(), attribute);
    }

    static void resetColor()
    {
        SetConsoleTextAttribute(
            console(),
            FOREGROUND_RED |
            FOREGROUND_GREEN |
            FOREGROUND_BLUE
        );
    }

    static void border()
    {
        setColor(BLUE);

        std::cout
            << "+----------------------------------------------------------+\n";

        resetColor();
    }

    static void header()
    {
        system("cls");

        border();

        setColor(CYAN);
        std::cout
            << "|       KRISHNA OS - MISSION CONTROL TERMINAL             |\n";

        setColor(BLUE);
        std::cout
            << "|                         VERSION 1.0                     |\n";

        border();

        setColor(GREEN);
        std::cout << "| SYSTEM STATUS : ONLINE                                   |\n";

        setColor(CYAN);
        std::cout << "| SHELL         : KRISHNA-SH                               |\n";

        border();

        resetColor();

        std::cout
            << "| Type 'help' to view available commands.                 |\n";

        border();

        std::cout << '\n';
    }

    static void prompt(const std::string& path)
    {
        setColor(BLUE);
        std::cout << path;

        setColor(YELLOW);
        std::cout << " $>";

        resetColor();
        std::cout << ' ';
    }

    static void success(const std::string& message)
    {
        setColor(GREEN);
        std::cout << "[SUCCESS] " << message << '\n';
        resetColor();
    }

    static void error(const std::string& message)
    {
        setColor(RED);
        std::cout << "[ERROR] " << message << '\n';
        resetColor();
    }

    static void info(const std::string& message)
    {
        setColor(CYAN);
        std::cout << "[SYSTEM] " << message << '\n';
        resetColor();
    }

    static void command(const std::string& name,
                        const std::string& description)
    {
        setColor(YELLOW);
        std::cout << "  " << name;

        resetColor();

        std::cout << "    " << description << '\n';
    }
};

#endif