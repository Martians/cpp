
#include "Common/Display.hpp"
#include "Advance/AppHelper.hpp"

namespace common {

int
CurrentStatus::Error(int error, const std::string& string)
{
    m_errno  = error;
    m_string = string;
    return m_errno;
}

int
CurrentStatus::Error(int error, const char* fmt, ...)
{
    FORMAT(fmt);

    m_errno  = error;
    m_string = data;
    return m_errno;
}

}
