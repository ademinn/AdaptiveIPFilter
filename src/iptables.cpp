#include "iptables.h"

#include <iostream>

#include <cstdlib>


iptables::~iptables()
{
    std::cout << "destructor" << std::endl;
    clear();
}


void iptables::clear()
{
    for (const std::pair<std::string, std::string> &rule : rules)
    {
        delete_rule(rule.first, rule.second);
    }
    rules.clear();
}


void iptables::delete_rule(const std::string &chain, const std::string &rule)
{
    std::cout << "delete rule " << rule << std::endl;
    system((std::string("iptables -D ") + chain + " " + rule).c_str());
}


void iptables::add_rule(const std::string &chain, const std::string &rule)
{
    std::cout << "iptables -A " << chain << " " << rule << std::endl;
    rules.push_back(std::pair<std::string, std::string>(chain, rule));
    system((std::string("iptables -A ") + chain + " " + rule).c_str());
}
