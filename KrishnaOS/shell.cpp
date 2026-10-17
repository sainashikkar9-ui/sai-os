#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
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
