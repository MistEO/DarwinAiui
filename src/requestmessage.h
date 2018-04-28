#ifndef REQUESTMESSAGE_H
#define REQUESTMESSAGE_H

#include "abstractmessage.h"

class RequestMessage : public AbstractMessage {
public:
    RequestMessage() = default;
    ~RequestMessage() = default;

    std::string first_line() const;
    std::string header() const;
    std::string to_string() const;

    std::string method;
    std::string uri;
};

#endif // REQUESTMESSAGE_H
