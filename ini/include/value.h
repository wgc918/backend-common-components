//-----------------------------------------------------------------------------
// 版权所有 (C) 2026 backend-common-components 项目
//
// 文件: value.h
// 作者: wgc
// 创建日期: 2026年6月
// 最后修改: 2026年6月
//
// 描述:
//     对字符串值的轻量封装，提供 `bool` / `int` / `double` / `std::string` 的隐式类型转换。
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

#include <string>

class Value
{
public:
    Value()  = default;
    ~Value() = default;
    Value(bool val);
    Value(int val);
    Value(double val);
    Value(const std::string& val);
    Value(std::string&& val);
    Value(const char* val);
    Value(const Value& other)            = default;
    Value& operator=(const Value& other) = default;
    Value(Value&& other)                 = default;
    Value& operator=(Value&& other)      = default;

    // 类型转换
    operator bool() const;
    operator int() const;
    operator double() const;
    operator std::string() const;

    std::string val() const;

private:
    std::string m_val;
};