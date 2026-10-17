#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include "TerminalUI.h"

using namespace std;
namespace fs = std::filesystem;

class Shell
{
private:
    fs::path currentPath;

public:
    Shell()
    {
        currentPath = fs::current_path();
    }

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
                if (entry.is_directory())
                    cout << "[DIR]  ";
                else
                    cout << "[FILE] ";

                cout << entry.path().filename().string() << '\n';
            }
        }
        catch (const fs::filesystem_error &e)
        {
            cout << "Error listing directory: " << e.what() << '\n';
        }
    }

    void createFile(const string &filename)
    {
        if (filename.empty())
        {
            cout << "Usage: cr file <filename>\n";
            return;
        }

        fs::path filePath = currentPath / filename;

        if (fs::exists(filePath))
        {
            TerminalUI::info("Krishna OS is running.");
            return;
        }

        ofstream file(filePath);

        if (file.is_open())
        {
            file.close();
            TerminalUI::success("File created: " + filename);
        }
        else
        {
            cout << "Error: Could not create file.\n";
        }
    }

    void createDirectory(const string &dirname)
    {
        if (dirname.empty())
        {
            cout << "Usage: cr dir <dirname>\n";
            return;
        }

        try
        {
            if (fs::create_directory(currentPath / dirname))
                cout << "Directory created successfully: " << dirname << '\n';
            else
                cout << "Error: Directory already exists or could not be created.\n";
        }
        catch (const fs::filesystem_error &e)
        {
            cout << "Error creating directory: " << e.what() << '\n';
        }
    }

    void removeFile(const string &filename)
    {
        if (filename.empty())
        {
            cout << "Usage: rm file <filename>\n";
            return;
        }

        try
        {
            fs::path filePath = currentPath / filename;

            if (!fs::exists(filePath) || !fs::is_regular_file(filePath))
            {
                cout << "Error: File not found.\n";
                return;
            }

            if (fs::remove(filePath))
                cout << "File removed successfully: " << filename << '\n';
            else
                cout << "Error: Could not remove file.\n";
        }
        catch (const fs::filesystem_error &e)
        {
            cout << "Error removing file: " << e.what() << '\n';
        }
    }

    void removeDirectory(const string &dirname)
    {
        if (dirname.empty())
        {
            cout << "Usage: rm dir <dirname>\n";
            return;
        }

        try
        {
            fs::path dirPath = currentPath / dirname;

            if (!fs::exists(dirPath) || !fs::is_directory(dirPath))
            {
                cout << "Error: Directory not found.\n";
                return;
            }

            if (fs::remove(dirPath))
                cout << "Directory removed successfully: " << dirname << '\n';
            else
                cout << "Error: Directory is not empty. Remove its contents first.\n";
        }
        catch (const fs::filesystem_error &e)
        {
            cout << "Error removing directory: " << e.what() << '\n';
        }
    }

    void openFile(const string &filename)
    {
        if (filename.empty())
        {
            cout << "Usage: rd file <filename>\n";
            return;
        }

        fs::path filePath = currentPath / filename;

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

        file.close();
    }

    void changeDirectory(const string &dirname)
    {
        if (dirname.empty())
        {
            cout << "Usage: rd dir <dirname>\n";
            return;
        }

        fs::path targetPath;

        if (dirname == "..")
            targetPath = currentPath.parent_path();
        else if (fs::path(dirname).is_absolute())
            targetPath = fs::path(dirname);
        else
            targetPath = currentPath / dirname;

        try
        {
            if (fs::exists(targetPath) && fs::is_directory(targetPath))
            {
                currentPath = fs::canonical(targetPath);
                cout << "Current directory: " << currentPath.string() << '\n';
            }
            else
            {
                cout << "Error: Directory not found.\n";
            }
        }
        catch (const fs::filesystem_error &e)
        {
            cout << "Error changing directory: " << e.what() << '\n';
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

            string command, type;
            iss >> command;

            if (command == "ls")
            {
                listFiles();
            }
            else if (command == "cr")
            {
                iss >> type;

                string name;
                getline(iss >> ws, name);

                if (type == "file")
                    createFile(name);
                else if (type == "dir")
                    createDirectory(name);
                else
                    cout << "Usage: cr file <filename> OR cr dir <dirname>\n";
            }
            else if (command == "rm")
            {
                iss >> type;

                string name;
                getline(iss >> ws, name);

                if (type == "file")
                    removeFile(name);
                else if (type == "dir")
                    removeDirectory(name);
                else
                    cout << "Usage: rm file <filename> OR rm dir <dirname>\n";
            }
            else if (command == "rd")
            {
                iss >> type;

                string name;
                getline(iss >> ws, name);

                if (type == "file")
                    openFile(name);
                else if (type == "dir")
                    changeDirectory(name);
                else
                    cout << "Usage: rd file <filename> OR rd dir <dirname>\n";
            }
            else if (command == "pwd")
            {
                cout << currentPath.string() << '\n';
            }
            else if (command == "help")
            {
                showHelp();
            }
            else if (command == "exit")
            {
                cout << "Shutting down Krishna OS. Hare Krishna!\n";
                break;
            }
            else
            {
                cout << "Unknown command. Type 'help' for assistance.\n";
            }
        }
    }
};

int main()
{
    Shell krishnaOS;
    krishnaOS.run();

    return 0;
}
