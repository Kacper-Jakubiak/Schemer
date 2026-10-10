/**
 * @file SchemerBuilder_setters.cpp
 * @brief Setter methods for builder configuration.
 */

#include <iostream>
#include <utility>

#include "../include/SchemerBuilder.h"

SchemerBuilder &SchemerBuilder::set_alpha(const double value) {
    inner_config.alpha = value;
    return *this;
}

SchemerBuilder &SchemerBuilder::set_beta(const double value) {
    inner_config.beta = value;
    return *this;
}

SchemerBuilder &SchemerBuilder::set_sigma(const double value) {
    inner_config.sigma = value;
    return *this;
}

SchemerBuilder &SchemerBuilder::set_length(const double value) {
    inner_config.length = value;
    return *this;
}

// --- Force Configuration ---

SchemerBuilder &SchemerBuilder::set_drift(const double value) {
    // Warn the user if a custom force function or vector is being overridden
    if (inner_config.force_input.type != ForceInput::Type::Drift) {
        std::cerr << "[WARNING]: force was previously set. Drift will be used instead\n";
    }
    inner_config.force_input.drift_value = value;
    inner_config.force_input.type = ForceInput::Type::Drift;
    inner_config.force_input.force_function = nullptr;
    inner_config.force_input.force_vector.clear();
    return *this;
}

SchemerBuilder &SchemerBuilder::set_force(std::vector<double> value) {
    // Warn if replacing an existing drift setting
    if (inner_config.force_input.type == ForceInput::Type::Drift && inner_config.force_input.drift_value != 0.0) {
        std::cerr << "[WARNING]: drift was previously set. Force vector will be used instead\n";
    }
    inner_config.force_input.force_vector = std::move(value);
    inner_config.force_input.type = ForceInput::Type::Vector;
    inner_config.force_input.force_function = nullptr;
    return *this;
}

SchemerBuilder &SchemerBuilder::set_force(std::function<double(double)> value) {
    // Warn if replacing an existing drift setting
    if (inner_config.force_input.type == ForceInput::Type::Drift && inner_config.force_input.drift_value != 0.0) {
        std::cerr << "[WARNING]: drift was previously set. Force function will be used instead\n";
    }
    inner_config.force_input.force_function = std::move(value);
    inner_config.force_input.type = ForceInput::Type::Function;
    inner_config.force_input.force_vector.clear();
    return *this;
}

// --- Numerical Grid & Stepping ---

SchemerBuilder &SchemerBuilder::set_dt(const double value) {
    inner_config.dt = value;
    return *this;
}

SchemerBuilder &SchemerBuilder::set_num_intervals(const long long int value) {
    inner_config.num_intervals = value;
    return *this;
}

SchemerBuilder &SchemerBuilder::set_theta(const double value) {
    inner_config.theta = value;
    return *this;
}

// --- Logging & Output ---

SchemerBuilder &SchemerBuilder::set_verbosity(const int value) {
    inner_config.verbose = value;
    return *this;
}

SchemerBuilder &SchemerBuilder::set_delimiter(std::string value) {
    inner_config.delimiter = std::move(value);
    return *this;
}

SchemerBuilder &SchemerBuilder::set_log_interval_percent(const double value) {
    inner_config.log_interval_percent = value;
    return *this;
}

// --- Initial Conditions ---

SchemerBuilder &SchemerBuilder::set_initial_conditions(const Eigen::VectorXd &value) {
    inner_config.values_input.vector = value;
    inner_config.values_input.type = ValuesInput::Type::Vector;
    return *this;
}

SchemerBuilder &SchemerBuilder::set_initial_conditions(const std::vector<double> &value) {
    // Map standard vector data directly into the Eigen vector format
    inner_config.values_input.vector = Eigen::VectorXd::Map(value.data(), static_cast<Eigen::Index>(value.size()));
    inner_config.values_input.type = ValuesInput::Type::Vector;
    return *this;
}

// --- Zero Placement Options ---

SchemerBuilder &SchemerBuilder::set_zero_index(const long long int value) {
    inner_config.zero_input.index = value;
    inner_config.zero_input.type = ZeroInput::Type::Index;
    return *this;
}

SchemerBuilder &SchemerBuilder::set_zero_distance(const double value) {
    inner_config.zero_input.distance = value;
    inner_config.zero_input.type = ZeroInput::Type::Distance;
    return *this;
}

SchemerBuilder &SchemerBuilder::set_zero_middle() {
    inner_config.zero_input.type = ZeroInput::Type::Middle;
    return *this;
}