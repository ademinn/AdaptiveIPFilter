#include "config.h"

#include <fstream>
#include <sstream>


Json::Value parse(int argc, char **argv)
{
    if (argc < 2)
    {
        throw config_exception("config file not specified");
    }

    std::ifstream config_stream(argv[1], std::ios::in);
    Json::Value value;
    Json::Reader reader;
    if (!reader.parse(config_stream, value))
    {
        std::stringstream error;
        error << "failed to parse config file" << std::endl <<
            reader.getFormatedErrorMessages();
        throw config_exception(error.str());
    }
    config_stream.close();
    return value;
}


config::config(const Json::Value& value)
    : interface(value["interface"].asString()),
    mark(value["mark"].asInt()),
    queue_num(value["queue_num"].asInt())
{
}
