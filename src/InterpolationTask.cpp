#include "InterpolationTask.h"
#include "InterpolationExceptions.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

InterpolationTask::Data InterpolationTask::prepareNodes(std::vector<Node> nodes, std::size_t maxNodes)
{
    if (nodes.empty())
        throw std::invalid_argument("Node set must not be empty");
    if (nodes.size() > maxNodes)
        throw NodeCountException("Node count exceeds the limit", maxNodes, nodes.size());
    if (nodes.size() < 2)
        throw NodeCountException("At least 2 nodes are required", 2, nodes.size());

    for (std::size_t i = 0; i < nodes.size(); ++i)
    {
        if (!std::isfinite(nodes[i].x))
            throw InvalidNodeException(i, "x", nodes[i].x);
        if (!std::isfinite(nodes[i].y))
            throw InvalidNodeException(i, "y", nodes[i].y);
    }

    std::sort(nodes.begin(), nodes.end(),
        [](const Node& a, const Node& b) { return a.x < b.x; });

    for (std::size_t i = 1; i < nodes.size(); ++i)
        if (nodes[i].x == nodes[i - 1].x)
            throw DuplicateAbscissaException(nodes[i].x, i - 1, i);

    Data data;
    data.x.reserve(nodes.size());
    data.y.reserve(nodes.size());
    for (const Node& n : nodes)
    {
        data.x.push_back(n.x);
        data.y.push_back(n.y);
    }
    return data;
}

InterpolationTask::InterpolationTask(std::vector<Node> nodes, std::size_t maxNodes)
    : x_(), y_(), maxNodes_(maxNodes)
{
    if (maxNodes == 0 || maxNodes > INTERP_MAX_N)
        throw std::invalid_argument("maxNodes must be in [1, INTERP_MAX_N]");
    Data data = prepareNodes(std::move(nodes), maxNodes);
    x_ = std::move(data.x);
    y_ = std::move(data.y);
}

std::vector<Node> InterpolationTask::nodesCopy() const
{
    std::vector<Node> result;
    result.reserve(x_.size() + 1);
    for (std::size_t i = 0; i < x_.size(); ++i)
        result.push_back({ x_[i], y_[i] });
    return result;
}

Node InterpolationTask::at(std::size_t index) const
{
    if (index >= x_.size())
        throw std::out_of_range("Node index is out of range");
    return { x_[index], y_[index] };
}

void InterpolationTask::throwIfError(int rc, double xq) const
{
    switch (rc)
    {
    case INTERP_OK:
        return;
    case INTERP_ERR_NULL:
        throw std::invalid_argument("Null pointer passed to the interpolation library");
    case INTERP_ERR_SIZE:
        throw NodeCountException("Invalid node count for the interpolation library",
            INTERP_MAX_N, x_.size());
    case INTERP_ERR_DUPLICATE_X:
        throw InterpolationException("Duplicate abscissa detected by the library");
    case INTERP_ERR_NOT_SORTED:
        throw InterpolationException("Nodes are not sorted");
    case INTERP_ERR_OUT_OF_RANGE:
        throw ExtrapolationException(xq, minX(), maxX());
    case INTERP_ERR_NOT_FINITE:
        throw std::invalid_argument("Non-finite value passed to the interpolation library");
    case INTERP_ERR_BAD_METHOD:
        throw std::invalid_argument("Unknown interpolation method");
    case INTERP_ERR_OVERFLOW:
        throw std::overflow_error("Interpolation result overflow");
    default:
        throw InterpolationException("Unknown interpolation library error");
    }
}

void InterpolationTask::replaceNodes(std::vector<Node> newNodes)
{
    Data data = prepareNodes(std::move(newNodes), maxNodes_);
    x_.swap(data.x);
    y_.swap(data.y);
    last_.reset();   
}

void InterpolationTask::addNode(const Node& node)
{
    std::vector<Node> all = nodesCopy();   
    all.push_back(node);
    Data data = prepareNodes(std::move(all), maxNodes_);
    x_.swap(data.x);
    y_.swap(data.y);
    last_.reset();
}

double InterpolationTask::evaluate(double xq, Method method) const
{
    double result = 0.0;
    const int rc = ::interpolate(x_.data(), y_.data(), static_cast<int>(x_.size()),
        xq, static_cast<int>(method), &result);
    throwIfError(rc, xq);
    return result;
}

double InterpolationTask::approximationError(double xq, Method method, double exactValue) const
{
    double result = 0.0;
    const int rc = ::interpolationError(x_.data(), y_.data(), static_cast<int>(x_.size()),
        xq, static_cast<int>(method), exactValue, &result);
    throwIfError(rc, xq);
    return result;
}

InterpolationResults InterpolationTask::computeAll(double xq)
{
    InterpolationResults r{};
    r.x = xq;
    r.lagrange = evaluate(xq, Method::Lagrange);   
    r.linear = evaluate(xq, Method::Linear);       
    last_ = r;
    return r;
}

void InterpolationTask::replaceNodesUnsafe(std::vector<Node> newNodes)
{
    x_.clear();
    y_.clear();
    last_.reset();
    Data data = prepareNodes(std::move(newNodes), maxNodes_);   
    x_ = std::move(data.x);
    y_ = std::move(data.y);
}