#include <gtest/gtest.h>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>
#include "InterpolationTask.h"
#include "InterpolationExceptions.h"

namespace
{
    const double kInf = std::numeric_limits<double>::infinity();
    const double kNaN = std::numeric_limits<double>::quiet_NaN();

    std::vector<Node> parabola() { return { { 0, 0 },{ 1, 1 },{ 2, 4 },{ 3, 9 } }; }
}

TEST(LegacyCApiTest, StillReturnsErrorCodes)
{
    const double x[] = { 0, 1, 2 };
    const double y[] = { 0, 1, 4 };
    double r = 0;
    EXPECT_EQ(linearInterpolation(x, y, 3, 0.5, &r), INTERP_OK);
    EXPECT_NEAR(r, 0.5, 1e-12);
    EXPECT_EQ(linearInterpolation(x, y, 3, 5.0, &r), INTERP_ERR_OUT_OF_RANGE);
    EXPECT_EQ(lagrange(nullptr, y, 3, 0.5, &r), INTERP_ERR_NULL);
    EXPECT_EQ(interpolate(x, y, 3, 0.5, 99, &r), INTERP_ERR_BAD_METHOD);
}

TEST(InterpolationTaskTest, CreatesAndSortsNodes)
{
    InterpolationTask t({ { 2, 4 },{ 0, 0 },{ 1, 1 } });
    ASSERT_EQ(t.size(), 3u);
    EXPECT_DOUBLE_EQ(t.at(0).x, 0.0);   
    EXPECT_DOUBLE_EQ(t.at(2).x, 2.0);
    EXPECT_DOUBLE_EQ(t.minX(), 0.0);
    EXPECT_DOUBLE_EQ(t.maxX(), 2.0);
}

TEST(InterpolationTaskTest, LinearInterpolatesBetweenNodes)
{
    InterpolationTask t(parabola());
    EXPECT_NEAR(t.evaluate(1.5, Method::Linear), 2.5, 1e-12);   
}

TEST(InterpolationTaskTest, LagrangeReproducesParabola)
{
    InterpolationTask t(parabola());
    EXPECT_NEAR(t.evaluate(1.5, Method::Lagrange), 2.25, 1e-12);
    EXPECT_NEAR(t.evaluate(2.7, Method::Lagrange), 7.29, 1e-12);
}

TEST(InterpolationTaskTest, ReturnsNodeValueAtNode)
{
    InterpolationTask t(parabola());
    EXPECT_NEAR(t.evaluate(2.0, Method::Linear), 4.0, 1e-12);
    EXPECT_NEAR(t.evaluate(2.0, Method::Lagrange), 4.0, 1e-12);
}

TEST(InterpolationTaskTest, LagrangeAllowsExtrapolation)
{
    InterpolationTask t(parabola());
    EXPECT_NEAR(t.evaluate(4.0, Method::Lagrange), 16.0, 1e-9);
}

TEST(InterpolationTaskTest, ApproximationErrorUsesLibrary)
{
    InterpolationTask t(parabola());
    EXPECT_NEAR(t.approximationError(1.5, Method::Lagrange, 2.25), 0.0, 1e-12);
    EXPECT_NEAR(t.approximationError(1.5, Method::Linear, 2.25), 0.25, 1e-12);
}

TEST(InterpolationErrors, RejectsEmptySet)
{
    EXPECT_THROW(InterpolationTask(std::vector<Node>{}), std::invalid_argument);
}

TEST(InterpolationErrors, RejectsInvalidMaxNodes)
{
    EXPECT_THROW(InterpolationTask(parabola(), 0), std::invalid_argument);
    EXPECT_THROW(InterpolationTask(parabola(), INTERP_MAX_N + 1), std::invalid_argument);
}

TEST(InterpolationErrors, RejectsDuplicateAbscissas)
{
    EXPECT_THROW(InterpolationTask({ { 1, 1 },{ 1, 2 } }), DuplicateAbscissaException);
}

TEST(InterpolationErrors, DuplicateExceptionCarriesDiagnostics)
{
    try
    {
        InterpolationTask t({ { 0, 0 },{ 2, 1 },{ 2, 5 } });
        FAIL() << "Exception expected";
    }
    catch (const DuplicateAbscissaException& ex)
    {
        EXPECT_DOUBLE_EQ(ex.x(), 2.0);
        EXPECT_EQ(ex.firstIndex(), 1u);
        EXPECT_EQ(ex.secondIndex(), 2u);
    }
}

TEST(InterpolationErrors, RejectsNonFiniteNodes)
{
    EXPECT_THROW(InterpolationTask({ { 0, 0 },{ kNaN, 1 } }), InvalidNodeException);
    EXPECT_THROW(InterpolationTask({ { 0, 0 },{ 1, kInf } }), InvalidNodeException);
}

TEST(InterpolationErrors, InvalidNodeExceptionCarriesDiagnostics)
{
    try
    {
        InterpolationTask t({ { 0, 0 },{ 1, kInf } });
        FAIL() << "Exception expected";
    }
    catch (const InvalidNodeException& ex)
    {
        EXPECT_EQ(ex.index(), 1u);
        EXPECT_EQ(ex.parameter(), "y");
        EXPECT_TRUE(std::isinf(ex.value()));
    }
}

TEST(InterpolationErrors, RejectsTooManyNodes)
{
    EXPECT_THROW(InterpolationTask(parabola(), 3), NodeCountException);
}

TEST(InterpolationErrors, RejectsTooFewNodes)
{
    try
    {
        InterpolationTask t({ { 0, 0 } });
        FAIL() << "Exception expected";
    }
    catch (const NodeCountException& ex)
    {
        EXPECT_EQ(ex.limit(), 2u);
        EXPECT_EQ(ex.actual(), 1u);
    }
}

TEST(InterpolationErrors, LinearRejectsExtrapolation)
{
    InterpolationTask t(parabola());
    EXPECT_THROW(t.evaluate(-0.1, Method::Linear), ExtrapolationException);
    EXPECT_THROW(t.evaluate(3.1, Method::Linear), ExtrapolationException);
}

TEST(InterpolationErrors, ExtrapolationExceptionCarriesRange)
{
    InterpolationTask t(parabola());
    try
    {
        t.evaluate(10.0, Method::Linear);
        FAIL() << "Exception expected";
    }
    catch (const ExtrapolationException& ex)
    {
        EXPECT_DOUBLE_EQ(ex.x(), 10.0);
        EXPECT_DOUBLE_EQ(ex.minX(), 0.0);
        EXPECT_DOUBLE_EQ(ex.maxX(), 3.0);
    }
}

TEST(InterpolationErrors, RejectsNonFinitePoint)
{
    InterpolationTask t(parabola());
    EXPECT_THROW(t.evaluate(kNaN, Method::Lagrange), std::invalid_argument);
    EXPECT_THROW(t.evaluate(kInf, Method::Linear), std::invalid_argument);
    EXPECT_THROW(t.approximationError(1.0, Method::Linear, kNaN), std::invalid_argument);
}

TEST(InterpolationErrors, RejectsUnknownMethod)
{
    InterpolationTask t(parabola());
    EXPECT_THROW(t.evaluate(1.0, static_cast<Method>(99)), std::invalid_argument);
}

TEST(InterpolationErrors, AtThrowsOutOfRange)
{
    InterpolationTask t(parabola());
    EXPECT_THROW(t.at(4), std::out_of_range);
}

TEST(InterpolationErrors, LagrangeDetectsOverflow)
{
    InterpolationTask t({ { 0, 0 },{ 1e-300, 1e308 },{ 1, 0 } });
    EXPECT_THROW(t.evaluate(0.5, Method::Lagrange), std::overflow_error);
}

TEST(InterpolationErrors, LinearDetectsOverflow)
{
    InterpolationTask t({ { 0, -1e308 },{ 1, 1e308 } });
    EXPECT_THROW(t.evaluate(0.5, Method::Linear), std::overflow_error);
}

TEST(InterpolationErrors, CustomExceptionsDeriveFromBase)
{
    EXPECT_THROW(InterpolationTask({ { 1, 1 },{ 1, 2 } }), InterpolationException);
    EXPECT_THROW(InterpolationTask({ { 1, 1 },{ 1, 2 } }), std::runtime_error);
}

TEST(InterpolationBoundary, WorksAtIntervalEnds)
{
    InterpolationTask t(parabola());
    EXPECT_NEAR(t.evaluate(0.0, Method::Linear), 0.0, 1e-12);
    EXPECT_NEAR(t.evaluate(3.0, Method::Linear), 9.0, 1e-12);
    EXPECT_NEAR(t.evaluate(3.0, Method::Lagrange), 9.0, 1e-12);
}

TEST(InterpolationBoundary, TwoNodesIsMinimum)
{
    InterpolationTask t({ { 0, 0 },{ 2, 4 } });
    EXPECT_NEAR(t.evaluate(1.0, Method::Linear), 2.0, 1e-12);
    EXPECT_NEAR(t.evaluate(1.0, Method::Lagrange), 2.0, 1e-12);
}

TEST(InterpolationBoundary, NodeLimitExactlyReached)
{
    EXPECT_NO_THROW(InterpolationTask(parabola(), 4));
}

TEST(InterpolationInvariants, NodesAlwaysSortedAndUnique)
{
    InterpolationTask t({ { 5, 1 },{ 1, 2 },{ 3, 3 } });
    t.addNode({ 2, 7 });
    for (std::size_t i = 1; i < t.size(); ++i)
        EXPECT_LT(t.at(i - 1).x, t.at(i).x);
}

TEST(InterpolationInvariants, SizeNeverExceedsLimit)
{
    InterpolationTask t({ { 0, 0 },{ 1, 1 } }, 3);
    t.addNode({ 2, 4 });
    EXPECT_THROW(t.addNode({ 3, 9 }), NodeCountException);
    EXPECT_LE(t.size(), t.maxNodes());
}

TEST(StrongGuarantee, ReplaceNodesPreservesStateOnDuplicate)
{
    InterpolationTask t(parabola());
    t.computeAll(1.5);
    const auto before = *t.lastResults();

    EXPECT_THROW(t.replaceNodes({ { 0, 0 },{ 1, 1 },{ 1, 5 } }), DuplicateAbscissaException);

    ASSERT_EQ(t.size(), 4u);
    for (std::size_t i = 0; i < 4; ++i)
    {
        EXPECT_DOUBLE_EQ(t.at(i).x, static_cast<double>(i));
        EXPECT_DOUBLE_EQ(t.at(i).y, static_cast<double>(i * i));
    }
    ASSERT_TRUE(t.lastResults().has_value());
    EXPECT_DOUBLE_EQ(t.lastResults()->lagrange, before.lagrange);
}

TEST(StrongGuarantee, ReplaceNodesPreservesStateOnNaN)
{
    InterpolationTask t(parabola());
    EXPECT_THROW(t.replaceNodes({ { 0, 0 },{ 1, kNaN } }), InvalidNodeException);
    ASSERT_EQ(t.size(), 4u);
    EXPECT_DOUBLE_EQ(t.at(3).y, 9.0);
}

TEST(StrongGuarantee, ReplaceNodesPreservesStateOnTooMany)
{
    InterpolationTask t(parabola(), 4);
    EXPECT_THROW(t.replaceNodes({ { 0, 0 },{ 1, 1 },{ 2, 2 },{ 3, 3 },{ 4, 4 } }), NodeCountException);
    EXPECT_EQ(t.size(), 4u);
}

TEST(StrongGuarantee, ReplaceNodesPreservesStateOnEmpty)
{
    InterpolationTask t(parabola());
    EXPECT_THROW(t.replaceNodes({}), std::invalid_argument);
    EXPECT_EQ(t.size(), 4u);
}

TEST(StrongGuarantee, AddNodePreservesStateOnDuplicate)
{
    InterpolationTask t(parabola());
    EXPECT_THROW(t.addNode({ 2, 100 }), DuplicateAbscissaException);
    ASSERT_EQ(t.size(), 4u);
    EXPECT_DOUBLE_EQ(t.at(2).y, 4.0);
}

TEST(StrongGuarantee, ReplaceNodesSucceeds)
{
    InterpolationTask t(parabola());
    t.computeAll(1.5);
    t.replaceNodes({ { 0, 0 },{ 10, 100 } });
    ASSERT_EQ(t.size(), 2u);
    EXPECT_DOUBLE_EQ(t.maxX(), 10.0);
    EXPECT_FALSE(t.lastResults().has_value());
}

TEST(StrongGuarantee, ComputeAllStoresResultsOnSuccess)
{
    InterpolationTask t(parabola());
    auto r = t.computeAll(1.5);
    ASSERT_TRUE(t.lastResults().has_value());
    EXPECT_NEAR(r.lagrange, 2.25, 1e-12);
    EXPECT_NEAR(t.lastResults()->linear, 2.5, 1e-12);
    EXPECT_NEAR(t.lastResults()->lagrange, 2.25, 1e-12);
}

TEST(StrongGuarantee, ComputeAllKeepsPreviousResultsOnFailure)
{
    InterpolationTask t(parabola());
    t.computeAll(1.5);
    const auto before = *t.lastResults();

    EXPECT_THROW(t.computeAll(100.0), ExtrapolationException);

    ASSERT_TRUE(t.lastResults().has_value());
    EXPECT_DOUBLE_EQ(t.lastResults()->x, before.x);
    EXPECT_DOUBLE_EQ(t.lastResults()->linear, before.linear);
    EXPECT_DOUBLE_EQ(t.lastResults()->lagrange, before.lagrange);
}

TEST(StrongGuarantee, ComputeAllStoresNothingIfFirstEverCallFails)
{
    InterpolationTask t(parabola());
    EXPECT_THROW(t.computeAll(100.0), ExtrapolationException);
    EXPECT_FALSE(t.lastResults().has_value());
}

TEST(NegativeSelfCheck, DISABLED_UnsafeReplaceViolatesStrongGuarantee)
{
    InterpolationTask t(parabola());
    EXPECT_THROW(t.replaceNodesUnsafe({ { 0, 0 },{ 1, 1 },{ 1, 5 } }), DuplicateAbscissaException);

    ASSERT_EQ(t.size(), 4u);   
    EXPECT_DOUBLE_EQ(t.at(3).y, 9.0);
}