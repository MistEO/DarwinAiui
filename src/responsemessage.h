#ifndef RESPONSEMESSAGE_H
#define RESPONSEMESSAGE_H

#include "abstractmessage.h"

class ResponseMessage : private AbstractMessage {
public:
    ResponseMessage(const std::string& source_message);
    ~ResponseMessage() = default;
    const std::map<std::string, std::string>& get_header_map() const;
    const std::string& get_version() const;
    const std::string& get_data() const;

    std::string to_string() const;
    std::string first_line() const;
    std::string header() const;

private:
    void _unpack(const std::string& message);
    void _unpack_status_line(const std::string& status_line);
    void _unpack_header_line(const std::string& header_line);

    std::string _status_line;
    int _status;
    std::string _status_name;
    std::string _header;
    std::string _source;
};

#endif // RESPONSEMESSAGE_H
