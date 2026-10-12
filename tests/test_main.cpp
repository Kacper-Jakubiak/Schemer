#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "SchemerBuilder.h"

TEST_CASE("SchemerBuilder basic construction") {
    SchemerBuilder builder;
    builder.set_num_intervals(10)
           .set_alpha(1.0)
           .set_beta(0.0)
           .set_sigma(1.0)
           .set_length(10.0)
           .set_dt(0.001)
           .set_theta(0.5);
}

TEST_CASE("SchemerBuilder validate_parameters") {
    SchemerBuilder base;
    base.set_num_intervals(10).set_beta(0.0).set_sigma(1.0).set_length(10.0).set_dt(0.1).set_theta(0.5);

    SUBCASE("alpha too small") {
        auto b = base.set_alpha(0.0);
        CHECK_THROWS_AS(b.build_setup(), std::invalid_argument);
    }

    SUBCASE("alpha too large") {
        auto b = base.set_alpha(2.1);
        CHECK_THROWS_AS(b.build_setup(), std::invalid_argument);
    }

    SUBCASE("beta out of range") {
        auto b = base.set_alpha(1.5).set_beta(1.5);
        CHECK_THROWS_AS(b.build_setup(), std::invalid_argument);
    }

    SUBCASE("sigma negative") {
        auto b = base.set_sigma(-0.1);
        CHECK_THROWS_AS(b.build_setup(), std::invalid_argument);
    }

    SUBCASE("length nonpositive") {
        auto b = base.set_length(0.0);
        CHECK_THROWS_AS(b.build_setup(), std::invalid_argument);
    }

    SUBCASE("dt nonpositive") {
        auto b = base.set_dt(0.0);
        CHECK_THROWS_AS(b.build_setup(), std::invalid_argument);
    }

    SUBCASE("num_intervals nonpositive") {
        SchemerBuilder b2;
        b2.set_num_intervals(0).set_alpha(1.2).set_beta(0.0).set_sigma(1.0).set_length(10.0).set_dt(0.1).set_theta(0.5);
        CHECK_THROWS_AS(b2.build_setup(), std::invalid_argument);
    }

    SUBCASE("theta outside [0,1]") {
        auto b = base.set_theta(1.5);
        CHECK_THROWS_AS(b.build_setup(), std::invalid_argument);
    }

    SUBCASE("beta nonzero when alpha==1") {
        auto b = base.set_alpha(1.0).set_beta(0.5);
        CHECK_THROWS_AS(b.build_setup(), std::invalid_argument);
    }
}
