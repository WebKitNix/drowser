#ifndef FatalError_h
#define FatalError_h

#include <string>

class FatalError
{
public:
    explicit FatalError(const std::string& what) : m_what(what) {}
    std::string what() const { return m_what; };

private:
    std::string m_what;
};

#endif
