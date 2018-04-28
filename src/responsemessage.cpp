#include "responsemessage.h"

#include <algorithm>
#include <string>
#include <vector>

ResponseMessage::ResponseMessage(const std::string& source)
{
    _source = source;
    _unpack(source);
}

void ResponseMessage::_unpack(const std::string& message)
{
    std::string source(message);
    //转换为小写
    std::transform(source.begin(), source.end(), source.begin(), ::tolower);
    //按行分割
    std::vector<std::string> lines(_split_string(source, "\n"));
    if (lines.size() < 2) {
        std::cerr << "Response message segmentation error:" << source;
        return;
    }
    auto iter = lines.begin();
    _status_line = *iter;
    _unpack_status_line(*iter);

    _header.clear();
    header_map.clear();
    for (++iter; !iter->empty(); ++iter) {
        _header += *iter + "\r\n";
        _unpack_header_line(*iter);
    }

    int data_pos = (first_line() + header() + "\r\n").length();
    data = source.substr(data_pos, source.length() - data_pos);
}

void ResponseMessage::_unpack_status_line(const std::string& status_line)
{
    std::vector<std::string> words(_split_string(status_line, " "));
    if (words.size() < 3) {
        std::cerr << "Response status segmentation error:" << status_line;
        return;
    }
    version = words[0];
    _status = std::stoi(words[1]);
    _status_name.clear();
    for (auto i = words.begin() + 2; i != words.end(); ++i) {
        if (!_status_name.empty()) {
            _status_name += " ";
        }
        _status_name += *i;
    }
}

void ResponseMessage::_unpack_header_line(const std::string& header_line)
{
    auto pair = _split_string(header_line, ": ");
    if (pair.size() != 2) {
        std::cerr << "Request header segmentation error: " << header_line << std::endl;
        return;
    }
    header_map[pair[0]] = pair[1];
}

std::string ResponseMessage::first_line() const
{
    return _status_line + "\r\n";
}

std::string ResponseMessage::header() const
{
    return _header;
}

const std::map<std::string, std::string>& ResponseMessage::get_header_map() const
{
    return header_map;
}

const std::string& ResponseMessage::get_version() const
{
    return version;
}

const std::string& ResponseMessage::get_data() const
{
    return data;
}

std::string ResponseMessage::to_string() const
{
    return _source;
}
