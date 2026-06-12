//-----------------------------------------------------------------------------
// 版权所有 (C) 2026 backend-common-components 项目
//
// 文件: ini.h
// 作者: wgc
// 创建日期: 2026年6月
// 最后修改: 2026年6月
//
// 描述:
//     采用 Meyers' Singleton 实现，通过静态局部变量保证线程安全（C++11
//     起）且延迟初始化。全局仅存在一个实例。
//
// 功能特性:
//    - 提供单例访问模式与隐式类型转换，支持标准 INI 格式的读取与查询。
//
// 许可证:
//     MIT License
//
//     版权所有 (c) 2026 wgc
//
//     特此免费授予获得本软件副本和相关文档文件（以下简称"软件"）的任何人以处理软件的权利，
//     包括但不限于使用、复制、修改、合并、出版、分发、再许可和/或出售软件副本，
//     以及允许软件适用者这样做，须在下列条件下：
//
//     上述版权声明和本许可声明应包含在软件的所有副本或实质性部分中。
//
//     软件按"原样"提供，不提供任何形式的明示或暗示的保证，
//     包括但不限于对适销性、特定用途适用性和非侵权性的保证。
//     在任何情况下，作者或版权持有人均不对任何索赔、损害或其他责任负责，
//     无论是在合同诉讼、侵权诉讼或其他诉讼中，
//     由于软件或软件的使用或其他交易产生的。
//-----------------------------------------------------------------------------

#pragma once

#include <iostream>
#include <string>
#include <unordered_map>

#include "value.h"

class IniFile
{
public:
    using Section = std::unordered_map<std::string, Value>;

    IniFile(const IniFile& other)            = delete;
    IniFile& operator=(const IniFile& other) = delete;
    IniFile(IniFile&& other)                 = delete;
    IniFile& operator=(IniFile&& other)      = delete;

    static IniFile& instance() noexcept;

    void load(const std::string& file_name);
    Section& operator[](const std::string& sectin);
    const Section& operator[](const std::string& section) const;

    void show() const;
    std::string trim(const std ::string& str) const;

private:
    IniFile()  = default;
    ~IniFile() = default;

private:
    std::unordered_map<std::string, Section> m_ini;
};