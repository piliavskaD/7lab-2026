#pragma once
// Ієрархія власних винятків бібліотеки інтерполяції
#include <stdexcept>
#include <string>
#include <cstddef>

// Базовий клас усіх винятків бібліотеки
class InterpolationException : public std::runtime_error
{
public:
    explicit InterpolationException(const std::string& message)
        : std::runtime_error(message) {}
};

// Вузол містить NaN/Inf (індекс, параметр "x"/"y" та значення)
class InvalidNodeException : public InterpolationException
{
    std::size_t index_;
    std::string parameter_;
    double value_;
public:
    InvalidNodeException(std::size_t index, const std::string& parameter, double value)
        : InterpolationException("Invalid node #" + std::to_string(index) +
            ": parameter '" + parameter + "' is not finite"),
        index_(index), parameter_(parameter), value_(value) {
    }

    std::size_t index() const noexcept { return index_; }
    const std::string& parameter() const noexcept { return parameter_; }
    double value() const noexcept { return value_; }
};

class DuplicateAbscissaException : public InterpolationException
{
    double x_;
    std::size_t firstIndex_;
    std::size_t secondIndex_;
public:
    DuplicateAbscissaException(double x, std::size_t first, std::size_t second)
        : InterpolationException("Duplicate abscissa in nodes: x = " + std::to_string(x)),
        x_(x), firstIndex_(first), secondIndex_(second) {
    }

    double x() const noexcept { return x_; }
    std::size_t firstIndex() const noexcept { return firstIndex_; }
    std::size_t secondIndex() const noexcept { return secondIndex_; }
};

// Точка поза інтервалом вузлів, а екстраполяція заборонена
class ExtrapolationException : public InterpolationException
{
    double x_, min_, max_;
public:
    ExtrapolationException(double x, double minX, double maxX)
        : InterpolationException("Extrapolation is not allowed: x = " + std::to_string(x) +
            " is outside [" + std::to_string(minX) + ", " +
            std::to_string(maxX) + "]"),
        x_(x), min_(minX), max_(maxX) {
    }

    double x() const noexcept { return x_; }
    double minX() const noexcept { return min_; }
    double maxX() const noexcept { return max_; }
};

class NodeCountException : public InterpolationException
{
    std::size_t limit_;
    std::size_t actual_;
public:
    NodeCountException(const std::string& message, std::size_t limit, std::size_t actual)
        : InterpolationException(message), limit_(limit), actual_(actual) {
    }

    std::size_t limit() const noexcept { return limit_; }   // мінімум або максимум залежно від ситуації
    std::size_t actual() const noexcept { return actual_; }
};