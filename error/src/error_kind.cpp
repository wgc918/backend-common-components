#include "../include/error_kind.h"

namespace ef
{
std::string ErrorLevel2string(ErrorLevel level)
{
    switch (level)
    {
        case ErrorLevel::Avoid:
            return std::string("Avoid");
        case ef::ErrorLevel::CBC:
            return std::string("Case By Case");
        case ErrorLevel::Warning:
            return std::string("Warning");
        case ef::ErrorLevel::Fatal:
            return std::string("Fatal");
        default:
            return std::string("unknown");
    }
}

}  // namespace ef