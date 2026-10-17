#include <windows.h>
#include <richedit.h>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

constexpr int ID_OUTPUT = 1001;
constexpr int ID_INPUT  = 1002;
constexpr int ID_RUN    = 1003;

HWND g_output = nullptr;
HWND g_input = nullptr;
HWND g_runButton = nullptr;
HFONT g_font = nullptr;
HBRUSH g_background = nullptr;
HMODULE g_richEditModule = nullptr;

const COLORREF BG_COLOR     = RGB(10, 16, 30);
const COLORREF PANEL_COLOR  = RGB(15, 25, 44);
const COLORREF BLUE_COLOR   = RGB(75, 170, 255);
const COLORREF CYAN_COLOR   = RGB(100, 220, 235);
const COLORREF GREEN_COLOR  = RGB(100, 230, 150);
const COLORREF YELLOW_COLOR = RGB(255, 205, 90);
const COLORREF RED_COLOR    = RGB(255, 105, 115);
const COLORREF WHITE_COLOR  = RGB(225, 235, 250);

class Shell
{
private:
    fs::path currentPath;

    static std::string trim(const std::string& value)
    {
        const auto first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return {};
        const auto last = value.find_last_not_of(" \t\r\n");
        return value.substr(first, last - first + 1);
    }

public:
    Shell() : currentPath(fs::current_path()) {}

    std::string path() const
    {
        return currentPath.string();
    }

    std::string execute(const std::string& input, bool& shouldExit)
    {
        shouldExit = false;
        std::istringstream iss(input);
        std::string command, type, name;
        iss >> command;

        if (command.empty()) return {};

        if (command == "ls")
        {
            std::ostringstream out;
            out << "Contents of: " << currentPath.string() << "\n\n";
            try
            {
                bool found = false;
                for (const auto& entry : fs::directory_iterator(currentPath))
                {
                    found = true;
                    out << (entry.is_directory() ? "[DIR]  " : "[FILE] ")
                        << entry.path().filename().string() << '\n';
                }
                if (!found) out << "(Directory is empty)\n";
            }
            catch (const fs::filesystem_error& e)
            {
                out << "ERROR: Could not list directory: " << e.what() << '\n';
            }
            return out.str();
        }

        if (command == "cr" || command == "rm" || command == "rd")
        {
            iss >> type;
            std::getline(iss >> std::ws, name);
            name = trim(name);

            if (type != "file" && type != "dir")
                return "Usage: " + command + " file <name> OR " + command + " dir <name>\n";

            if (name.empty())
                return "Usage: " + command + " " + type + " <name>\n";

            try
            {
                fs::path target = currentPath / name;

                if (command == "cr" && type == "file")
                {
                    if (fs::exists(target)) return "ERROR: A file or directory with that name already exists.\n";
                    std::ofstream file(target);
                    if (!file) return "ERROR: Could not create file.\n";
                    return "SUCCESS: File created: " + name + "\n";
                }

                if (command == "cr" && type == "dir")
                {
                    if (fs::create_directory(target))
                        return "SUCCESS: Directory created: " + name + "\n";
                    return "ERROR: Directory already exists or could not be created.\n";
                }

                if (command == "rm" && type == "file")
                {
                    if (!fs::exists(target) || !fs::is_regular_file(target))
                        return "ERROR: File not found.\n";
                    if (fs::remove(target)) return "SUCCESS: File removed: " + name + "\n";
                    return "ERROR: Could not remove file.\n";
                }

                if (command == "rm" && type == "dir")
                {
                    if (!fs::exists(target) || !fs::is_directory(target))
                        return "ERROR: Directory not found.\n";
                    if (fs::remove(target)) return "SUCCESS: Directory removed: " + name + "\n";
                    return "ERROR: Directory is not empty. Remove its contents first.\n";
                }

                if (command == "rd" && type == "file")
                {
                    if (!fs::exists(target) || !fs::is_regular_file(target))
                        return "ERROR: File not found.\n";
                    std::ifstream file(target);
                    if (!file) return "ERROR: Could not open file.\n";
                    std::ostringstream out;
                    out << "========== FILE CONTENTS ==========\n";
                    std::string line;
                    while (std::getline(file, line)) out << line << '\n';
                    out << "===================================\n";
                    return out.str();
                }

                if (command == "rd" && type == "dir")
                {
                    fs::path destination;
                    if (name == "..")
                        destination = currentPath.parent_path();
                    else if (fs::path(name).is_absolute())
                        destination = fs::path(name);
                    else
                        destination = currentPath / name;

                    if (!fs::exists(destination) || !fs::is_directory(destination))
                        return "ERROR: Directory not found.\n";

                    currentPath = fs::canonical(destination);
                    return "SUCCESS: Current directory changed.\n";
                }
            }
            catch (const fs::filesystem_error& e)
            {
                return std::string("ERROR: Filesystem operation failed: ") + e.what() + "\n";
            }
        }

        if (command == "pwd")
            return currentPath.string() + "\n";

        if (command == "help")
        {
            return
                "AVAILABLE COMMANDS\n"
                "  ls                     List files and directories\n"
                "  cr file <name>         Create a file\n"
                "  cr dir <name>          Create a directory\n"
                "  rm file <name>         Remove a file\n"
                "  rm dir <name>          Remove an empty directory\n"
                "  rd file <name>         Display file contents\n"
                "  rd dir <name>          Change directory (use .. to go up)\n"
                "  pwd                    Display current directory\n"
                "  help                   Show this help\n"
                "  clear                  Clear terminal output\n"
                "  exit                   Close Krishna OS\n";
        }

        if (command == "clear")
            return "__CLEAR__";

        if (command == "exit")
        {
            shouldExit = true;
            return "Shutting down Krishna OS. Hare Krishna!\n";
        }

        return "ERROR: Unknown command. Type 'help' for assistance.\n";
    }
};

Shell g_shell;

void AppendText(const std::string& text, COLORREF color)
{
    if (!g_output || text.empty()) return;

    CHARRANGE range{};
    range.cpMin = -1;
    range.cpMax = -1;
    SendMessageA(g_output, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&range));

    CHARFORMAT2A format{};
    format.cbSize = sizeof(format);
    format.dwMask = CFM_COLOR | CFM_FACE | CFM_SIZE;
    format.crTextColor = color;
    format.yHeight = 200;
    lstrcpynA(format.szFaceName, "Consolas", LF_FACESIZE);
    SendMessageA(g_output, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&format));

    SendMessageA(g_output, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(text.c_str()));
    SendMessageA(g_output, EM_SCROLLCARET, 0, 0);
}

void SetOutputBackground()
{
    if (g_output)
        SendMessageA(g_output, EM_SETBKGNDCOLOR, 0, static_cast<LPARAM>(BG_COLOR));
}

void SubmitCommand()
{
    if (!g_input || !g_output) return;

    char buffer[2048]{};
    GetWindowTextA(g_input, buffer, static_cast<int>(sizeof(buffer)));
    std::string input(buffer);
    if (input.empty()) return;

    AppendText(g_shell.path() + "$> ", BLUE_COLOR);
    AppendText(input + "\r\n", YELLOW_COLOR);
    SetWindowTextA(g_input, "");

    bool shouldExit = false;
    std::string result = g_shell.execute(input, shouldExit);

    if (result == "__CLEAR__")
    {
        SetWindowTextA(g_output, "");
    }
    else if (!result.empty())
    {
        COLORREF color = CYAN_COLOR;
        if (result.rfind("ERROR:", 0) == 0) color = RED_COLOR;
        else if (result.rfind("SUCCESS:", 0) == 0) color = GREEN_COLOR;
        AppendText(result, color);
    }

    if (shouldExit)
        PostMessageA(GetParent(g_input), WM_CLOSE, 0, 0);

    SetFocus(g_input);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        g_font = CreateFontA(
            18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");

        CreateWindowA("STATIC", "KRISHNA OS  //  PERSONAL TERMINAL",
            WS_CHILD | WS_VISIBLE,
            22, 14, 650, 28, hwnd, nullptr, nullptr, nullptr);

        CreateWindowA("STATIC", "WIN32 TERMINAL  |  FILE SYSTEM SHELL",
            WS_CHILD | WS_VISIBLE,
            24, 39, 500, 20, hwnd, nullptr, nullptr, nullptr);

        g_output = CreateWindowExA(
            WS_EX_CLIENTEDGE, "RICHEDIT20A", "",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE |
            ES_AUTOVSCROLL | ES_READONLY,
            20, 70, 760, 400, hwnd,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_OUTPUT)),
            GetModuleHandleA(nullptr), nullptr);

        g_input = CreateWindowExA(
            WS_EX_CLIENTEDGE, "EDIT", "",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            20, 485, 630, 34, hwnd,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_INPUT)),
            GetModuleHandleA(nullptr), nullptr);

        g_runButton = CreateWindowA(
            "BUTTON", "EXECUTE",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            662, 485, 118, 34, hwnd,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_RUN)),
            GetModuleHandleA(nullptr), nullptr);

        HWND controls[] = {
            FindWindowExA(hwnd, nullptr, "STATIC", "KRISHNA OS  //  PERSONAL TERMINAL"),
            FindWindowExA(hwnd, nullptr, "STATIC", "WIN32 TERMINAL  |  FILE SYSTEM SHELL"),
            g_output, g_input, g_runButton
        };
        for (HWND control : controls)
            if (control) SendMessageA(control, WM_SETFONT, reinterpret_cast<WPARAM>(g_font), TRUE);

        SetOutputBackground();
        AppendText("KRISHNA OS\n", BLUE_COLOR);
        AppendText("Native Windows terminal initialized.\n", GREEN_COLOR);
        AppendText("Type 'help' to view available commands.\n\n", CYAN_COLOR);
        AppendText(g_shell.path() + "$> ", BLUE_COLOR);
        return 0;
    }

    case WM_SIZE:
    {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        int outputHeight = height - 150;
        int inputY = height - 55;
        int inputWidth = width - 165;

        if (g_output) MoveWindow(g_output, 20, 70, width - 40, outputHeight > 100 ? outputHeight : 100, TRUE);
        if (g_input) MoveWindow(g_input, 20, inputY, inputWidth > 100 ? inputWidth : 100, 34, TRUE);
        if (g_runButton) MoveWindow(g_runButton, width - 138, inputY, 118, 34, TRUE);
        return 0;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_RUN)
        {
            SubmitCommand();
            return 0;
        }
        if (LOWORD(wParam) == ID_INPUT && HIWORD(wParam) == EN_MAXTEXT)
            return 0;
        break;

    case WM_CTLCOLOREDIT:
    {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetTextColor(hdc, YELLOW_COLOR);
        SetBkColor(hdc, PANEL_COLOR);
        static HBRUSH editBrush = CreateSolidBrush(PANEL_COLOR);
        return reinterpret_cast<LRESULT>(editBrush);
    }

    case WM_CTLCOLORSTATIC:
    {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetTextColor(hdc, BLUE_COLOR);
        SetBkColor(hdc, BG_COLOR);
        static HBRUSH staticBrush = CreateSolidBrush(BG_COLOR);
        return reinterpret_cast<LRESULT>(staticBrush);
    }

    case WM_ERASEBKGND:
    {
        RECT rect;
        GetClientRect(hwnd, &rect);
        FillRect(reinterpret_cast<HDC>(wParam), &rect, g_background);
        return 1;
    }

    case WM_DESTROY:
        if (g_font) DeleteObject(g_font);
        if (g_background) DeleteObject(g_background);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    g_richEditModule = LoadLibraryA("Riched20.dll");
    if (!g_richEditModule)
    {
        MessageBoxA(nullptr, "Could not load the Windows Rich Edit component.", "Krishna OS", MB_ICONERROR);
        return 1;
    }

    g_background = CreateSolidBrush(BG_COLOR);

    WNDCLASSA wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "KrishnaOSNativeWindow";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = g_background;

    if (!RegisterClassA(&wc))
    {
        MessageBoxA(nullptr, "Could not register the Krishna OS window.", "Krishna OS", MB_ICONERROR);
        FreeLibrary(g_richEditModule);
        return 1;
    }

    HWND hwnd = CreateWindowExA(
        0, wc.lpszClassName, "Krishna OS | Native Terminal",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 900, 650,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd)
    {
        MessageBoxA(nullptr, "Could not create the Krishna OS window.", "Krishna OS", MB_ICONERROR);
        FreeLibrary(g_richEditModule);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageA(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    FreeLibrary(g_richEditModule);
    return static_cast<int>(msg.wParam);
}
