#pragma once

#include <vector>
#include <string>
#include <utility>

class iptables
{
    public:
        iptables(){};
        ~iptables();

        void add_rule(const std::string& chain, const std::string& rule);
    private:
        void delete_rule(const std::string& chain, const std::string& rule);

        std::vector<std::pair<std::string, std::string>> rules;
};
