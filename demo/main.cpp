#include <iostream>
#include <functional>
#include <limits>
#include <stdexcept>
#include "InterpolationTask.h"
#include "InterpolationExceptions.h"
#include "ScopeLogger.h"

void runScenario(const char* title, const std::function<void()>& action)
{
    std::cout << "--- " << title << " ---\n";
    try
    {
        action();
    }
    catch (const DuplicateAbscissaException& ex)
    {
        std::cout << "DuplicateAbscissaException: " << ex.what()
            << " (nodes " << ex.firstIndex() << " and " << ex.secondIndex() << ")\n";
    }
    catch (const InvalidNodeException& ex)
    {
        std::cout << "InvalidNodeException: " << ex.what()
            << " [index=" << ex.index() << ", param=" << ex.parameter() << "]\n";
    }
    catch (const ExtrapolationException& ex)
    {
        std::cout << "ExtrapolationException: " << ex.what() << '\n';
    }
    catch (const NodeCountException& ex)
    {
        std::cout << "NodeCountException: " << ex.what()
            << " (limit=" << ex.limit() << ", actual=" << ex.actual() << ")\n";
    }
    catch (const InterpolationException& ex)
    {
        std::cout << "InterpolationException: " << ex.what() << '\n';
    }
    catch (const std::exception& ex)
    {
        std::cout << "std::exception: " << ex.what() << '\n';
    }
    std::cout << '\n';
}

void level3()
{
    ScopeLogger logger("level3");
    std::cout << "Creating task with duplicate abscissas...\n";
    InterpolationTask task({ { 1.0, 1.0 },{ 2.0, 4.0 },{ 2.0, 5.0 } });
}

void level2()
{
    ScopeLogger logger("level2");
    try
    {
        level3();
    }
    catch (const std::exception& ex)
    {
        std::cout << "[LOG] level2 caught: " << ex.what() << '\n';
        throw;   
    }
}

void level1()
{
    ScopeLogger logger("level1");
    level2();
}

int main()
{
    const double inf = std::numeric_limits<double>::infinity();
    const double nan = std::numeric_limits<double>::quiet_NaN();

    InterpolationTask task({ { 0.0, 0.0 },{ 1.0, 1.0 },{ 2.0, 4.0 },{ 3.0, 9.0 } });
    auto r = task.computeAll(1.5);
    std::cout << "x = " << r.x << "  Linear = " << r.linear
        << "  Lagrange = " << r.lagrange << "\n\n";

    runScenario("1. Empty node set (std::invalid_argument)",
        [] { InterpolationTask t({}); });
    runScenario("2. Duplicate abscissas (DuplicateAbscissaException)",
        [] { InterpolationTask t({ { 1, 1 },{ 1, 2 } }); });
    runScenario("3. NaN/Inf in nodes (InvalidNodeException)",
        [&] { InterpolationTask t({ { 0, 0 },{ nan, 1 } }); });
    runScenario("4. Extrapolation, linear method (ExtrapolationException)",
        [&] { task.evaluate(10.0, Method::Linear); });
    runScenario("5. Too few nodes (NodeCountException)",
        [] { InterpolationTask t({ { 0, 0 } }); });
    runScenario("6. Node limit exceeded (NodeCountException)",
        [] { InterpolationTask t({ { 0, 0 },{ 1, 1 } }, 2); t.addNode({ 2, 4 }); });
    runScenario("7. Index out of range (std::out_of_range)",
        [&] { task.at(100); });
    runScenario("8. Non-finite point (std::invalid_argument)",
        [&] { task.evaluate(inf, Method::Lagrange); });
    runScenario("9. Result overflow (std::overflow_error)",
        [] { InterpolationTask t({ { 0, 0 },{ 1e-300, 1e308 },{ 1, 0 } }); t.evaluate(0.5, Method::Lagrange); });
    runScenario("10. Unknown method (std::invalid_argument)",
        [&] { task.evaluate(1.0, static_cast<Method>(99)); });

    std::cout << "--- Strong guarantee ---\n";
    try { task.computeAll(100.0); }   
    catch (const ExtrapolationException& ex) { std::cout << "Caught: " << ex.what() << '\n'; }
    std::cout << "Last stored x after failed call: " << task.lastResults()->x << " (expected 1.5)\n\n";

    std::cout << "--- Stack unwinding ---\n";
    try { level1(); }
    catch (const std::exception& ex) { std::cout << "[MAIN] Exception: " << ex.what() << '\n'; }
    std::cout << "Program continues normally.\n";
    return 0;
}