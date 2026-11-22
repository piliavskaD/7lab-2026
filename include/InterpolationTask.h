#pragma once
#include <cstddef>
#include <optional>
#include <vector>
#include "Interpolation.h"   

struct Node
{
    double x;
    double y;
};

enum class Method
{
    Linear = INTERP_METHOD_LINEAR,
    Lagrange = INTERP_METHOD_LAGRANGE
};

struct InterpolationResults
{
    double x;
    double linear;
    double lagrange;
};

class InterpolationTask
{
private:
    std::vector<double> x_;
    std::vector<double> y_;
    std::size_t maxNodes_;
    std::optional<InterpolationResults> last_;

    struct Data { std::vector<double> x; std::vector<double> y; };

    static Data prepareNodes(std::vector<Node> nodes, std::size_t maxNodes);
    std::vector<Node> nodesCopy() const;
    void throwIfError(int rc, double xq) const;

public:
    explicit InterpolationTask(std::vector<Node> nodes, std::size_t maxNodes = INTERP_MAX_N);

    std::size_t size() const noexcept { return x_.size(); }
    std::size_t maxNodes() const noexcept { return maxNodes_; }
    double minX() const noexcept { return x_.front(); }
    double maxX() const noexcept { return x_.back(); }
    const std::optional<InterpolationResults>& lastResults() const noexcept { return last_; }

    Node at(std::size_t index) const;   

    void replaceNodes(std::vector<Node> newNodes);
    void addNode(const Node& node);

    double evaluate(double xq, Method method) const;
    double approximationError(double xq, Method method, double exactValue) const;

    InterpolationResults computeAll(double xq);

    void replaceNodesUnsafe(std::vector<Node> newNodes);
};