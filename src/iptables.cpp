#include "iptables.h"

#include <iostream>

#include <cstdlib>


iptables::~iptables()
{
    std::cout << "destructor" << std::endl;
    for (const std::pair<std::string, std::string> &rule : rules)
    {
        std::cout << "delete rule " << rule.second << std::endl;
        delete_rule(rule.first, rule.second);
    }
}


void iptables::delete_rule(const std::string &chain, const std::string &rule)
{
    std::cout << "delete rule " << rule << std::endl;
    system((std::string("iptables -D ") + chain + std::string(" ") + rule).c_str());
}


void iptables::add_rule(const std::string &chain, const std::string &rule)
{
    std::cout << "iptables -A " << chain << " " << rule << std::endl;
    rules.push_back(std::pair<std::string, std::string>(chain, rule));
    system((std::string("iptables -A ") + chain + std::string(" ") + rule).c_str());
}
