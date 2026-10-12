#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "Config.h"
#include "SchemerBuilder.h"

TEST_CASE("Config default values") {
    const Config config;

    CHECK(config.alpha == 1.5);
    CHECK(config.beta == 0.0);
    CHECK(config.sigma == 1.0);
    CHECK(config.length == 40.0);
    CHECK(config.num_intervals == 1000);
    CHECK(config.dt == 0.001);
    CHECK(config.theta == 0.5);
    CHECK(config.verbose == 1);
    CHECK(config.log_interval_percent == 5.0);
    CHECK(config.delimiter == "\t");

    CHECK(config.zero_input.type == ZeroInput::Type::Middle);
    CHECK(config.zero_input.index == -1);
    CHECK(config.zero_input.distance == 0.0);
    CHECK(config.values_input.type == ValuesInput::Type::Dirac);
    CHECK(config.values_input.vector.size() == 0);
    CHECK(config.force_input.type == ForceInput::Type::Drift);
    CHECK(config.force_input.drift_value == 0.0);
    CHECK_FALSE(config.force_input.force_function);
    CHECK(config.force_input.force_vector.empty());
    CHECK(config.reset_input.type == ResetInput::Type::Zero);
    CHECK(config.reset_input.index == 0);
    CHECK(config.reset_input.rate == 0.0);
}

TEST_CASE("Config grid helpers") {
    Config config;
    config.length = 12.0;
    config.num_intervals = 4;

    CHECK_UNARY(config.get_dx() == doctest::Approx(3.0));
    CHECK(config.get_size() == 5);
    CHECK_UNARY(config.get_coordinates() == (std::vector<double>{0.0, 3.0, 6.0, 9.0, 12.0}));
    CHECK_UNARY(config.get_coordinates(2) == (std::vector<double>{-6.0, -3.0, 0.0, 3.0, 6.0}));
}

TEST_CASE("SchemerBuilder build_setup uses default inputs") {
    SchemerBuilder builder;

    const SimulationSetup setup = builder.build_setup();

    CHECK(setup.config.alpha == 1.5);
    CHECK(setup.config.length == 40.0);
    CHECK(setup.config.num_intervals == 1000);
    CHECK(setup.starting_index == 500);
    CHECK(setup.initial_state.size() == 1001);
    CHECK(setup.initial_state(0) == 0.0);
    CHECK(setup.initial_state(1) == 0.0);
    CHECK(setup.initial_state(2) == 0.0);
    CHECK(setup.initial_state(3) == 0.0);
    CHECK(setup.initial_state(4) == 0.0);
    CHECK(setup.initial_state(setup.starting_index) >= 0.0);
    REQUIRE(std::holds_alternative<double>(setup.force_variant));
    CHECK(std::get<double>(setup.force_variant) == 0.0);
    CHECK(setup.step_matrix.rows() == setup.initial_state.size());
    CHECK(setup.step_matrix.cols() == setup.initial_state.size());
    CHECK(setup.step_matrix.allFinite());
}

TEST_CASE("SchemerBuilder build_setup places initial state at zero index") {
    SchemerBuilder builder;
    builder.set_alpha(1.5)
           .set_num_intervals(4)
           .set_length(4.0)
           .set_dt(0.01)
           .set_zero_index(1);

    const SimulationSetup setup = builder.build_setup();

    CHECK(setup.starting_index == 1);
    CHECK(setup.initial_state(0) == 0.0);
    CHECK(setup.initial_state(1) >= 0.0);
    CHECK(setup.initial_state(2) == 0.0);
    CHECK(setup.initial_state(3) == 0.0);
    CHECK(setup.initial_state(4) == 0.0);
}

TEST_CASE("SchemerBuilder build_setup places initial state at zero distance") {
    SchemerBuilder builder;
    builder.set_alpha(1.5)
           .set_num_intervals(4)
           .set_length(8.0)
           .set_dt(0.01)
           .set_zero_distance(2.0);

    const SimulationSetup setup = builder.build_setup();

    CHECK(setup.config.zero_input.type == ZeroInput::Type::Distance);
    CHECK(setup.config.zero_input.distance == 2.0);
    CHECK(setup.starting_index == 1);
    CHECK(setup.initial_state(0) == 0.0);
    CHECK(setup.initial_state(1) == 0.5);
    CHECK(setup.initial_state(2) == 0.0);
    CHECK(setup.initial_state(3) == 0.0);
    CHECK(setup.initial_state(4) == 0.0);
}

TEST_CASE("SchemerBuilder build_setup uses configured index and vectors") {
    SchemerBuilder builder;
    const std::vector<double> initial_values{1.0, 2.0, 3.0, 4.0, 5.0};
    const std::vector<double> force_values{-1.0, -0.5, 0.0, 0.5, 1.0};
    builder.set_alpha(1.5)
           .set_num_intervals(4)
           .set_length(4.0)
           .set_dt(0.01)
           .set_zero_index(1)
           .set_initial_conditions(initial_values)
           .set_force(force_values)
           .set_reset_index(3)
           .set_reset_rate(0.2);

    const SimulationSetup setup = builder.build_setup();

    CHECK(setup.starting_index == 1);
    CHECK(setup.config.zero_input.type == ZeroInput::Type::Index);
    CHECK(setup.config.values_input.type == ValuesInput::Type::Vector);
    CHECK(setup.config.force_input.type == ForceInput::Type::Vector);
    CHECK(setup.config.reset_input.type == ResetInput::Type::Index);
    CHECK(setup.config.reset_input.index == 3);
    CHECK_UNARY(setup.config.reset_input.rate == doctest::Approx(0.2));
    REQUIRE(setup.initial_state.size() == 5);
    for (Eigen::Index i = 0; i < setup.initial_state.size(); ++i) {
        CHECK(setup.initial_state(i) == initial_values[static_cast<std::size_t>(i)]);
    }
    REQUIRE(std::holds_alternative<std::vector<double>>(setup.force_variant));
    CHECK_UNARY(std::get<std::vector<double>>(setup.force_variant) == force_values);
    CHECK(setup.step_matrix.rows() == 5);
    CHECK(setup.step_matrix.cols() == 5);
    CHECK(setup.step_matrix.allFinite());
}

TEST_CASE("SchemerBuilder build_setup evaluates force at configured coordinates") {
    SchemerBuilder builder;
    builder.set_alpha(1.5)
           .set_num_intervals(4)
           .set_length(8.0)
           .set_dt(0.01)
           .set_zero_distance(4.0)
           .set_force([](const double x) { return 2.0 * x; });

    const SimulationSetup setup = builder.build_setup();

    CHECK(setup.starting_index == 2);
    CHECK(setup.config.zero_input.type == ZeroInput::Type::Distance);
    CHECK(setup.config.zero_input.distance == 4.0);
    CHECK(setup.config.force_input.type == ForceInput::Type::Function);
    CHECK_UNARY(static_cast<bool>(setup.config.force_input.force_function));
    REQUIRE(std::holds_alternative<std::vector<double>>(setup.force_variant));
    CHECK_UNARY(std::get<std::vector<double>>(setup.force_variant) ==
                (std::vector<double>{-8.0, -4.0, 0.0, 4.0, 8.0}));
}

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
    SUBCASE("alpha too small") {
        SchemerBuilder builder;
        builder.set_alpha(0.0);
        CHECK_THROWS_AS(builder.build_setup(), std::invalid_argument);
    }

    SUBCASE("alpha too large") {
        SchemerBuilder builder;
        builder.set_alpha(2.0);
        CHECK_THROWS_AS(builder.build_setup(), std::invalid_argument);
    }

    SUBCASE("beta out of range") {
        SchemerBuilder builder;
        builder.set_beta(1.5);
        CHECK_THROWS_AS(builder.build_setup(), std::invalid_argument);
    }

    SUBCASE("sigma negative") {
        SchemerBuilder builder;
        builder.set_sigma(-0.1);
        CHECK_THROWS_AS(builder.build_setup(), std::invalid_argument);
    }

    SUBCASE("length nonpositive") {
        SchemerBuilder builder;
        builder.set_length(0.0);
        CHECK_THROWS_AS(builder.build_setup(), std::invalid_argument);
    }

    SUBCASE("dt nonpositive") {
        SchemerBuilder builder;
        builder.set_dt(0.0);
        CHECK_THROWS_AS(builder.build_setup(), std::invalid_argument);
    }

    SUBCASE("num_intervals nonpositive") {
        SchemerBuilder builder;
        builder.set_num_intervals(0);
        CHECK_THROWS_AS(builder.build_setup(), std::invalid_argument);
    }

    SUBCASE("theta outside [0,1]") {
        SchemerBuilder builder;
        builder.set_theta(1.5);
        CHECK_THROWS_AS(builder.build_setup(), std::invalid_argument);
    }

    SUBCASE("beta nonzero when alpha==1") {
        SchemerBuilder builder;
        builder.set_alpha(1.0).set_beta(0.5);
        CHECK_THROWS_AS(builder.build_setup(), std::invalid_argument);
    }
}
