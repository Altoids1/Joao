#include "Forward.h"
#include "Terminal.h"

bool Terminal::disableFormatting = false;

#ifdef _WIN32
#include <windows.h>
#include <VersionHelpers.h>
#include <WinCon.h>
enum class WindowsColor : WORD {
    Red = FOREGROUND_RED,
    Green = FOREGROUND_GREEN,
    Blue = FOREGROUND_BLUE,
    Yellow = FOREGROUND_RED | FOREGROUND_GREEN,
    Magneta = FOREGROUND_RED | FOREGROUND_BLUE,
    Cyan = FOREGROUND_GREEN | FOREGROUND_BLUE,
    White = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
};
//This may look kinda obfuscated
//and that's because it is - Microsoft has made this purposefully complicated for the sake of
//making it harder to have compatibility between seven and ten.
static bool IsWindows10() {
    const auto getSysOpType = []()
    {
        NTSTATUS(WINAPI * RtlGetVersion)(LPOSVERSIONINFOEXW);
        OSVERSIONINFOEXW osInfo;

        auto ntdll = GetModuleHandleA("ntdll");
        if (ntdll == NULL)
            return 0.0;

        *(FARPROC*)&RtlGetVersion = GetProcAddress(ntdll, "RtlGetVersion");
        if (RtlGetVersion == NULL)
            return 0.0;

        osInfo.dwOSVersionInfoSize = sizeof(osInfo);
        RtlGetVersion(&osInfo);
        return (double)osInfo.dwMajorVersion;
    };
    return getSysOpType() == 10.0;
}
#endif

#ifdef _WIN32
    static bool CanUseANSIColors = IsWindows10();
#else // All Unix OSes a user could reasonably use, supports ANSI colours in its terminal, so
    constexpr bool CanUseANSIColors = true;
#endif


void Terminal::SetColor(std::ostream& stream, Color color) {
    if(disableFormatting)
        return;
    if (CanUseANSIColors) {
        stream << "\x1b[" << std::to_string(static_cast<int>(color)) << 'm';
        return;
    }
    //Here lies an insane Windows 7 fallback for this
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE); // FIXME: Try to use the error handle when stream is std::cerr.
    if (!hConsole)
        return;
    if (color == Color::RESET || color == Color::DarkGrey) {
        SetConsoleTextAttribute(hConsole, 7);
        return;
    }
    static std::unordered_map<Color, WindowsColor> colorMap = {
        {Color::Red, WindowsColor::Red},
        {Color::Green, WindowsColor::Green},
        {Color::Yellow, WindowsColor::Yellow},
        {Color::Blue, WindowsColor::Blue},
        {Color::Magneta, WindowsColor::Magneta},
        {Color::Cyan, WindowsColor::Cyan},
        {Color::White, WindowsColor::White},
    };
    if (!colorMap.contains(color))
        return;
    SetConsoleTextAttribute(hConsole, static_cast<WORD>(colorMap.at(color)) | FOREGROUND_INTENSITY);
#endif
}

void Terminal::SetBold(std::ostream& stream, bool isBold) {
#ifdef _WIN32
    return; // FIXME: Do bolding in windows emissions!
#else
    if(disableFormatting)
        return;
    stream << "\x1b[" << (isBold ? '1' : '0') << 'm';
#endif
}

void Terminal::ClearFormatting(std::ostream& stream) {
    SetColor(stream, Color::RESET);
    SetBold(stream, false);
}