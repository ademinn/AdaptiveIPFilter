#pragma once

#include <stdexcept>
#include <string>

#include <jsoncpp/json/json.h>


class config_exception : public std::runtime_error
{
    public:
        config_exception(const std::string& what_arg)
            : std::runtime_error(what_arg)
        {}
};


class config
{
    public:
        config(const Json::Value& value);

        std::string interface;
        int mark;
        int queue_num;
};


Json::Value parse(int argc, char **argv);
