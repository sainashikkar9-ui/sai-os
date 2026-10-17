#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <conio.h>
#include <windows.h>
#include "TerminalUI.h"

using namespace std;
namespace fs = std::filesystem;

// Distinct argument types allow true function overloading.
struct FileTarget
{
    string name;
};

struct DirectoryTarget
{
    string name;
};

class Shell
{
private:
    fs::path currentPath;

    static string trim(const string &value)
    {
        const auto first = value.find_first_not_of(" \t\r\n");
        if (first == string::npos)
            return "";
        const auto last = value.find_last_not_of(" \t\r\n");
        return value.substr(first, last - first + 1);
    }

    // cr file <name>
    void create(const FileTarget &target)
    {
        if (target.name.empty())
        {
            cout << "Usage: cr file <filename>\n";
            return;
        }

        fs::path filePath = currentPath / target.name;
        if (fs::exists(filePath))
        {
            cout << "Error: A file or directory with that name already exists.\n";
            return;
        }

        ofstream file(filePath);
        if (file.is_open())
        {
            file.close();
            TerminalUI::success("File created: " + target.name);
        }
        else
            cout << "Error: Could not create file.\n";
    }

    // cr dir <name>
    void create(const DirectoryTarget &target)
    {
        if (target.name.empty())
        {
            cout << "Usage: cr dir <dirname>\n";
            return;
        }

        try
        {
            if (fs::create_directory(currentPath / target.name))
                TerminalUI::success("Directory created: " + target.name);
            else
                cout << "Error: Directory already exists or could not be created.\n";
        }
        catch (const fs::filesystem_error &e)
        {
            cout << "Error creating directory: " << e.what() << '\n';
        }
    }

    // rm file <name>
    void remove(const FileTarget &target)
    {
        if (target.name.empty())
        {
            cout << "Usage: rm file <filename>\n";
            return;
        }

        try
        {
            fs::path filePath = currentPath / target.name;
            if (!fs::exists(filePath) || !fs::is_regular_file(filePath))
            {
                cout << "Error: File not found.\n";
                return;
            }

            if (fs::remove(filePath))
                TerminalUI::success("File removed: " + target.name);
            else
                cout << "Error: Could not remove file.\n";
        }
        catch (const fs::filesystem_error &e)
        {
            cout << "Error removing file: " << e.what() << '\n';
        }
    }

    // rm dir <name>
    void remove(const DirectoryTarget &target)
    {
        if (target.name.empty())
        {
            cout << "Usage: rm dir <dirname>\n";
            return;
        }

        try
        {
            fs::path dirPath = currentPath / target.name;
            if (!fs::exists(dirPath) || !fs::is_directory(dirPath))
            {
                cout << "Error: Directory not found.\n";
                return;
            }

            if (fs::remove(dirPath))
                TerminalUI::success("Directory removed: " + target.name);
            else
                cout << "Error: Directory is not empty. Remove its contents first.\n";
        }
        catch (const fs::filesystem_error &e)
        {
            cout << "Error removing directory: " << e.what() << '\n';
        }
    }

    // rd file <name>: display the file's contents.
    void read(const FileTarget &target)
    {
        if (target.name.empty())
        {
            cout << "Usage: rd file <filename>\n";
            return;
        }

        fs::path filePath = currentPath / target.name;
        if (!fs::exists(filePath) || !fs::is_regular_file(filePath))
        {
            cout << "Error: File not found.\n";
            return;
        }

        ifstream file(filePath);
        if (!file.is_open())
        {
            cout << "Error: Could not open file.\n";
            return;
        }

        cout << "\n========== FILE CONTENTS ==========\n";
        string line;
        while (getline(file, line))
            cout << line << '\n';
        cout << "===================================\n";
    }

    // rd dir <name>: change the current working directory.
    void read(const DirectoryTarget &target)
    {
        if (target.name.empty())
        {
            cout << "Usage: rd dir <dirname>\n";
            return;
        }

        fs::path targetPath;
        if (target.name == "..")
            targetPath = currentPath.parent_path();
        else if (fs::path(target.name).is_absolute())
            targetPath = fs::path(target.name);
        else
            targetPath = currentPath / target.name;

        try
        {
            if (fs::exists(targetPath) && fs::is_directory(targetPath))
            {
                currentPath = fs::canonical(targetPath);
                TerminalUI::success("Current directory: " + currentPath.string());
            }
            else
                cout << "Error: Directory not found.\n";
        }
        catch (const fs::filesystem_error &e)
        {
            cout << "Error changing directory: " << e.what() << '\n';
        }
    }

    // --- TEXT EDITOR SYSTEM ---
    void runEditor(const string &filename)
    {
        if (filename.empty())
        {
            cout << "Usage: edit <filename>\n";
            return;
        }

        fs::path filePath = currentPath / filename;
        vector<string> lines;

        if (fs::exists(filePath) && fs::is_regular_file(filePath))
        {
            ifstream infile(filePath);
            string line;
            while (getline(infile, line))
            {
                lines.push_back(line);
            }
            infile.close();
            cout << "Loaded file: " << filename << " (" << lines.size() << " lines)\n";
        }
        else
        {
            cout << "Creating new file in editor: " << filename << "\n";
        }

        cout << "\n================ KRISHNA TEXT EDITOR ================\n";
        cout << "Commands:\n";
        cout << "  :list            - Show all lines with numbers\n";
        cout << "  <text>           - Append text as a new line\n";
        cout << "  :edit <num> <txt>- Modify line number <num>\n";
        cout << "  :del <num>       - Delete line number <num>\n";
        cout << "  :w               - Save file\n";
        cout << "  :q               - Quit editor without saving\n";
        cout << "  :wq              - Save and quit\n";
        cout << "  :help            - Show editor help\n";
        cout << "=====================================================\n";

        string input;
        while (true)
        {
            cout << "[EDIT][" << filename << "]> ";
            if (!getline(cin, input))
                break;

            if (input == ":q")
            {
                cout << "Exited editor without saving.\n";
                break;
            }
            else if (input == ":w")
            {
                ofstream outfile(filePath);
                for (const auto &l : lines)
                    outfile << l << '\n';
                outfile.close();
                TerminalUI::success("File saved successfully.");
            }
            else if (input == ":wq")
            {
                ofstream outfile(filePath);
                for (const auto &l : lines)
                    outfile << l << '\n';
                outfile.close();
                TerminalUI::success("File saved and exited.");
                break;
            }
            else if (input == ":list")
            {
                cout << "\n--- " << filename << " ---\n";
                for (size_t i = 0; i < lines.size(); ++i)
                {
                    cout << (i + 1) << ": " << lines[i] << '\n';
                }
                cout << "--------------------\n";
            }
            else if (input == ":help")
            {
                cout << "Editor commands: :list, :edit <num> <txt>, :del <num>, :w, :q, :wq\n";
            }
            else if (input.rfind(":edit ", 0) == 0)
            {
                istringstream iss(input.substr(6));
                size_t lineNum;
                string newText;
                if (iss >> lineNum)
                {
                    getline(iss >> ws, newText);
                    if (lineNum >= 1 && lineNum <= lines.size())
                    {
                        lines[lineNum - 1] = newText;
                        cout << "Line " << lineNum << " updated.\n";
                    }
                    else
                    {
                        cout << "Error: Invalid line number.\n";
                    }
                }
                else
                {
                    cout << "Usage: :edit <line_num> <new_text>\n";
                }
            }
            else if (input.rfind(":del ", 0) == 0)
            {
                istringstream iss(input.substr(5));
                size_t lineNum;
                if (iss >> lineNum)
                {
                    if (lineNum >= 1 && lineNum <= lines.size())
                    {
                        lines.erase(lines.begin() + (lineNum - 1));
                        cout << "Line " << lineNum << " deleted.\n";
                    }
                    else
                    {
                        cout << "Error: Invalid line number.\n";
                    }
                }
                else
                {
                    cout << "Usage: :del <line_num>\n";
                }
            }
            else
            {
                lines.push_back(input);
            }
        }
    }

    // --- GAMES SYSTEM ---
    void runSnakeGame()
    {
        cout << "\n=== NOKIA SNAKE GAME ===\n";
        cout << "Controls: W (Up), A (Left), S (Down), D (Right), Q (Quit)\n";
        cout << "Press any key to start...\n";
        _getch();

        int width = 20;
        int height = 10;
        int x = width / 2, y = height / 2;
        int fruitX = rand() % width, fruitY = rand() % height;
        int score = 0;
        enum eDirection { STOP = 0, LEFT, RIGHT, UP, DOWN };
        eDirection dir = RIGHT;

        vector<pair<int, int>> tail;
        bool gameOver = false;

        while (!gameOver)
        {
            if (_kbhit())
            {
                char ch = _getch();
                switch (ch)
                {
                case 'a': case 'A': if (dir != RIGHT) dir = LEFT; break;
                case 'd': case 'D': if (dir != LEFT) dir = RIGHT; break;
                case 'w': case 'W': if (dir != DOWN) dir = UP; break;
                case 's': case 'S': if (dir != UP) dir = DOWN; break;
                case 'q': case 'Q': gameOver = true; break;
                }
            }

            int prevX = tail.empty() ? x : tail[0].first;
            int prevY = tail.empty() ? y : tail[0].second;
            int prev2X, prev2Y;
            if (!tail.empty())
            {
                tail[0] = {x, y};
                for (size_t i = 1; i < tail.size(); ++i)
                {
                    prev2X = tail[i].first;
                    prev2Y = tail[i].second;
                    tail[i] = {prevX, prevY};
                    prevX = prev2X;
                    prevY = prev2Y;
                }
            }

            switch (dir)
            {
            case LEFT: x--; break;
            case RIGHT: x++; break;
            case UP: y--; break;
            case DOWN: y++; break;
            default: break;
            }

            if (x >= width || x < 0 || y >= height || y < 0)
                gameOver = true;

            for (const auto& t : tail)
            {
                if (t.first == x && t.second == y)
                    gameOver = true;
            }

            if (x == fruitX && y == fruitY)
            {
                score += 10;
                fruitX = rand() % width;
                fruitY = rand() % height;
                tail.push_back({x, y});
            }

            system("cls");
            cout << "=== SNAKE === Score: " << score << "\n";
            for (int i = 0; i < width + 2; i++) cout << "#";
            cout << "\n";

            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    if (j == 0) cout << "#";
                    if (i == y && j == x) cout << "O";
                    else if (i == fruitY && j == fruitX) cout << "F";
                    else
                    {
                        bool printTail = false;
                        for (const auto& t : tail)
                        {
                            if (t.first == j && t.second == i)
                            {
                                cout << "o";
                                printTail = true;
                                break;
                            }
                        }
                        if (!printTail) cout << " ";
                    }
                    if (j == width - 1) cout << "#";
                }
                cout << "\n";
            }

            for (int i = 0; i < width + 2; i++) cout << "#";
            cout << "\nControls: W,A,S,D | Q to quit\n";

            Sleep(150);
        }
        cout << "\nGame Over! Final Score: " << score << "\nPress any key to return to shell...\n";
        _getch();
    }

    void runDoodleJumpGame()
    {
        cout << "\n=== DOODLE JUMP ===\n";
        cout << "Controls: A (Left), D (Right), Q (Quit)\n";
        cout << "Press any key to start...\n";
        _getch();

        int width = 20;
        int height = 15;
        int playerX = width / 2;
        int playerY = height - 2;
        float playerVY = 0;
        int score = 0;
        bool gameOver = false;

        struct Platform { int x; int y; };
        vector<Platform> platforms = {
            {width / 2 - 2, height - 2},
            {3, height - 5},
            {12, height - 8},
            {6, height - 11},
            {10, height - 14}
        };

        srand(static_cast<unsigned int>(time(nullptr)));

        while (!gameOver)
        {
            if (_kbhit())
            {
                char ch = _getch();
                switch (ch)
                {
                case 'a': case 'A': playerX = max(0, playerX - 2); break;
                case 'd': case 'D': playerX = min(width - 1, playerX + 2); break;
                case 'q': case 'Q': gameOver = true; break;
                }
            }

            playerVY += 0.4f;
            playerY += static_cast<int>(playerVY);

            for (auto& p : platforms)
            {
                if (playerVY > 0 && playerY == p.y && playerX >= p.x && playerX <= p.x + 4)
                {
                    playerVY = -1.2f;
                    score += 5;
                }
            }

            if (playerY < height / 2)
            {
                int diff = (height / 2) - playerY;
                playerY = height / 2;
                score += diff * 10;
                for (auto& p : platforms)
                {
                    p.y += diff;
                    if (p.y >= height)
                    {
                        p.y = 0;
                        p.x = rand() % (width - 5);
                    }
                }
            }

            if (playerY >= height)
            {
                gameOver = true;
            }

            system("cls");
            cout << "=== NOKIA DOODLE JUMP === Score: " << score << "\n";
            for (int i = 0; i < width + 2; i++) cout << "-";
            cout << "\n";

            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    if (j == 0) cout << "|";
                    
                    bool rendered = false;
                    if (i == playerY && j == playerX)
                    {
                        cout << "^";
                        rendered = true;
                    }
                    else
                    {
                        for (const auto& p : platforms)
                        {
                            if (i == p.y && j >= p.x && j < p.x + 5)
                            {
                                cout << "=";
                                rendered = true;
                                break;
                            }
                        }
                    }
                    if (!rendered) cout << " ";

                    if (j == width - 1) cout << "|";
                }
                cout << "\n";
            }
            for (int i = 0; i < width + 2; i++) cout << "-";
            cout << "\nControls: A, D | Q to quit\n";

            Sleep(100);
        }
        cout << "\nGame Over! Final Score: " << score << "\nPress any key to return to shell...\n";
        _getch();
    }

public:
    Shell() : currentPath(fs::current_path()) {}

    void displayWelcome()
    {
        TerminalUI::header();
    }

    void listFiles()
    {
        try
        {
            cout << "\nContents of: " << currentPath.string() << "\n\n";
            for (const auto &entry : fs::directory_iterator(currentPath))
            {
                cout << (entry.is_directory() ? "[DIR]  " : "[FILE] ")
                     << entry.path().filename().string() << '\n';
            }
        }
        catch (const fs::filesystem_error &e)
        {
            cout << "Error listing directory: " << e.what() << '\n';
        }
    }

    void showHelp()
    {
        TerminalUI::border();
        TerminalUI::info("AVAILABLE COMMANDS");
        TerminalUI::command("ls", "List files and directories");
        TerminalUI::command("cr file", "Create a new file");
        TerminalUI::command("cr dir", "Create a new directory");
        TerminalUI::command("rm file", "Remove a file");
        TerminalUI::command("rm dir", "Remove an empty directory");
        TerminalUI::command("rd file", "Display file contents");
        TerminalUI::command("rd dir", "Change directory");
        TerminalUI::command("edit <filename>", "Open text editor to write/edit a file");
        TerminalUI::command("games", "Play Games");
        TerminalUI::command("pwd", "Display current directory");
        TerminalUI::command("help", "Display available commands");
        TerminalUI::command("exit", "Exit Krishna OS");
        TerminalUI::border();
    }

    void run()
    {
        displayWelcome();
        string input;

        while (true)
        {
            cout << '\n';
            TerminalUI::prompt(currentPath.string());
            if (!getline(cin, input))
                break;
            if (input.empty())
                continue;

            istringstream iss(input);
            string command, type, name;
            iss >> command;

            if (command == "ls")
                listFiles();
            else if (command == "cr" || command == "rm" || command == "rd")
            {
                iss >> type;
                getline(iss >> ws, name);
                name = trim(name);

                if (type != "file" && type != "dir")
                {
                    cout << "Usage: " << command << " file <name> OR "
                         << command << " dir <name>\n";
                }
                else if (type == "file")
                {
                    FileTarget target{name};
                    if (command == "cr")
                        create(target);
                    else if (command == "rm")
                        remove(target);
                    else
                        read(target);
                }
                else
                {
                    DirectoryTarget target{name};
                    if (command == "cr")
                        create(target);
                    else if (command == "rm")
                        remove(target);
                    else
                        read(target);
                }
            }
            else if (command == "edit")
            {
                string filename;
                getline(iss >> ws, filename);
                filename = trim(filename);
                runEditor(filename);
            }
            else if (command == "games")
            {
                string gameName;
                getline(iss >> ws, gameName);
                gameName = trim(gameName);
                if (gameName == "snake")
                    runSnakeGame();
                else if (gameName == "doodle_jump")
                    runDoodleJumpGame();
                else
                {
                    cout << "<---Games--->\n";
                    cout << "Available options:\n";
                    cout << "  1. snake\n";
                    cout << "  2. doodle_jump\n";
                }
            }
            else if (command == "pwd")
                cout << currentPath.string() << '\n';
            else if (command == "help")
                showHelp();
            else if (command == "exit")
            {
                cout << "Shutting down Krishna OS. Hare Krishna!\n";
                break;
            }
            else
                cout << "Unknown command. Type 'help' for assistance.\n";
        }
    }
};

int main()
{
    Shell krishnaOS;
    krishnaOS.run();
    return 0;
}