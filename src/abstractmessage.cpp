#include "abstractmessage.h"

#include <algorithm>

std::vector<std::string> AbstractMessage::_split_string(const std::string& source, const std::string& c)
{
    std::string::size_type pos1 = 0, pos2 = source.find(c);
    std::vector<std::string> result;
    while (std::string::npos != pos2) {
        result.push_back(source.substr(pos1, pos2 - pos1));
        pos1 = pos2 + c.size();
        pos2 = source.find(c, pos1);
    }
    if (pos1 != source.length())
        result.push_back(source.substr(pos1));
    return result;
}

std::ostream& operator<<(std::ostream& out, const AbstractMessage& amsg)
{
    out << amsg.first_line()
        << amsg.header()
        << "\r\n"
        << (amsg.data.empty() ? std::string() : "std::string data, size:" + std::to_string(amsg.data.size()));
    return out;
}