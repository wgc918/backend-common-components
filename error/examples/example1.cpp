
#include <iostream>
#include <string>

#include "include/ef.h"

template <typename T>
using SR = Result<T, ef::Error>;

SR<void> func1()
{
    using SR = SR<void>;
    SR ret;
    return ret.OK();
}

SR<int> func2(int a)
{
    using SR = SR<int>;

    SR ret(a + 1);
    return ret;
}

SR<std::string> func3(bool flag)
{
    if (flag)
    {
        return EF_ERROR(ef::errors::kInvalidArg, "arg is %s", flag ? "true" : "false");
    }
    else
    {
        return SR<std::string>("ok");
    }
}

int main()
{
    auto r1 = func1();
    if (r1.is_ok())
    {
        std::cout << "r1.is_ok()" << std::endl;
    }

    auto r2 = func2(123);
    if (r2.is_ok())
    {
        std::cout << "r2.is_ok()" << std::endl;
        std::cout << "r2.unwrap() " << r2.unwrap() << std::endl;
    }

    auto r3 = func3(false);
    if (r3.is_ok())
    {
        std::cout << "r3.is_ok()" << std::endl;
        std::cout << "r3.unwrap() " << r3.unwrap() << std::endl;
    }
    else
    {
        std::cout << "r3.is_err()" << std::endl;
        std::cout << "r3.unwrap_err() " << r3.unwrap_err().to_string() << std::endl;
    }
    auto r4 = func3(true);
    if (r4.is_ok())
    {
        std::cout << "r4.is_ok()" << std::endl;
        std::cout << "r4.unwrap() " << r4.unwrap() << std::endl;
    }
    else
    {
        std::cout << "r4.is_err()" << std::endl;
        std::cout << "r4.unwrap_err() " << r4.unwrap_err().to_string() << std::endl;
    }

    return 0;
}
