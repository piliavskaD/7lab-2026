#include <iostream>
#include <string>
#include <stdexcept>
#include <exception>

void throwInteger()
{
    throw 42;
}

void throwString()
{
    throw std::string("Invalid interpolation nodes");
}

void throwStandard()
{
    throw std::invalid_argument("Invalid interpolation nodes");
}

void demonstrateSlicing()
{
    try
    {
        throw std::invalid_argument("Invalid argument");
    }
    catch (const std::exception& ex)
    {
        throw ex;
    }
}

int main()
{
    std::cout << "=== 1: int. catch (const std::exception&) does NOT catch it ===\n";
    try
    {
        throwInteger();
    }
    catch (const std::exception& ex)
    {
        std::cout << "std::exception: " << ex.what() << '\n';   
    }
    catch (...)
    {
        std::cout << "catch (...) caught an unknown exception (type and data unavailable)\n";
    }

    std::cout << "\n=== 2: std::string. catch (const std::exception&) does NOT catch it ===\n";
    try
    {
        throwString();
    }
    catch (const std::exception& ex)
    {
        std::cout << "std::exception: " << ex.what() << '\n';   
    }
    catch (...)
    {
        std::cout << "catch (...) caught an unknown exception (type and data unavailable)\n";
    }

    std::cout << "\n=== 3: standard exception (recommended) ===\n";
    try
    {
        throwStandard();
    }
    catch (const std::exception& ex)
    {
        std::cout << "Caught std::exception: " << ex.what() << '\n';
    }

    std::cout << "\n=== 4: object slicing (throw ex;) ===\n";
    try
    {
        demonstrateSlicing();
    }
    catch (const std::invalid_argument&)
    {
        std::cout << "Caught invalid_argument\n";
    }
    catch (const std::exception&)
    {
        std::cout << "Caught base std::exception (derived type lost!)\n";
    }

    return 0;
}