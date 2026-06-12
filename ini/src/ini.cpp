#include "../include/ini.h"

#include <cassert>
#include <fstream>

IniFile& IniFile::instance() noexcept
{
    static IniFile instance;
    return instance;
}

void IniFile::load(const std::string& file_name)
{
    std::ifstream fin;
    fin.open(file_name, std::ios::in);
    if (fin.fail())
    {
        std::cout << "fail" << "\n";
        return;
    }
    m_ini.clear();

    std::string line;
    std::string current_section;

    while (getline(fin, line))
    {
        line = trim(line);

        // 跳过空行和注释
        if (line.empty() || line[0] == '#' || line[0] == ';')
            continue;

        // 检查是否是节
        if (line[0] == '[' && line.back() == ']')
        {
            current_section        = line.substr(1, line.size() - 2);
            current_section        = trim(current_section);
            m_ini[current_section] = Section();
        }
        else
        {
            // 解析键值对
            size_t pos = line.find('=');
            if (pos != std::string::npos)
            {
                std::string key   = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                key               = trim(key);
                value             = trim(value);

                // 如果当前节为空，则使用一个全局节
                if (current_section.empty())
                {
                    current_section        = "Global";
                    m_ini[current_section] = Section();
                }

                m_ini[current_section][key] = Value(value);
            }
        }
    }
}

IniFile::Section& IniFile::operator[](const std::string& section)
{
    auto it = m_ini.find(section);
    if (it == m_ini.end())
    {
        throw std::out_of_range("Section not found: " + section);
    }
    return it->second;
}

const IniFile::Section& IniFile::operator[](const std::string& section) const
{
    auto it = m_ini.find(section);
    if (it == m_ini.end())
    {
        throw std::out_of_range("Section not found: " + section);
    }
    return it->second;
}

void IniFile::show() const
{
    for (auto it = m_ini.begin(); it != m_ini.end(); it++)
    {
        std::cout << "[" << it->first << "]" << std::endl;
        for (auto p : it->second)
        {
            std::cout << p.first << " = " << p.second.val() << std::endl;
        }
    }
}

std::string IniFile::trim(const std::string& str) const
{
    auto start = str.find_first_not_of(" \t\r\n");
    auto end   = str.find_last_not_of(" \t\r\n");

    if (start == std::string::npos)
        return "";
    else
        return str.substr(start, end - start + 1);
}
