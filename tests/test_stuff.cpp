// Runs the self tests that ship with the Stuff math/utility library (mclib/stuff/*_test.cpp).
// Each TestClass() returns false (and SPEWs the failed assumption) on failure.

#include "doctest/doctest.h"

#include "stuff/stuffheaders.hpp"

using namespace Stuff;

TEST_CASE("Stuff: angles")
{
    CHECK(Radian::TestClass());
    CHECK(Degree::TestClass());
    CHECK(SinCosPair::TestClass());
}

TEST_CASE("Stuff: vectors and points")
{
    CHECK(Vector3D::TestClass());
    CHECK(Vector4D::TestClass());
    CHECK(Point3D::TestClass());
    CHECK(UnitVector3D::TestClass());
}

TEST_CASE("Stuff: rotations, origins and rays")
{
    CHECK(EulerAngles::TestClass());
    // UnitQuaternion::TestClass() is not run: its body is compiled out and it always returns false
    CHECK(Origin3D::TestClass());
    CHECK(Ray3D::TestClass());
}
