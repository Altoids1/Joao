#pragma once
#include "Forward.h"
//These are some compatibility/customization wrappers for aesthetic terminal modification. 
class Terminal { // A static class, so we can remember if formatting is disabled for one reason or another.
public:
    enum class Color {
        RESET = 0,
        Background = 10,
        DarkGrey = 90,
        Red,
        Green,
        Yellow,
        Blue,
        Magneta,
        Cyan,
        White
    };
    static bool disableFormatting;
    static void SetColor(std::ostream& stream, Color color = Color::RESET);
    static void SetBold(std::ostream&, bool);
    static void ClearFormatting(std::ostream&);
};