#pragma once

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <utility>

inline SchemerBuilder &SchemerBuilder::set_alpha(const double value) {
    inner_config.alpha = value;
    return *this;
}

inline SchemerBuilder &SchemerBuilder::set_beta(const double value) {
    inner_config.beta = value;
    return *this;
}

inline SchemerBuilder &SchemerBuilder::set_sigma(const double value) {
    inner_config.sigma = value;
    return *this;
}

inline SchemerBuilder &SchemerBuilder::set_length(const double value) {
    inner_config.length = value;
    return *this;
}

inline SchemerBuilder &SchemerBuilder::set_drift(const double value) {
    if (inner_config.force_input.type != ForceInput::Type::Drift) {
        std::cerr << "[WARNING]: force was previously set. Drift will be used instead\n";
    }
    inner_config.force_input.drift_value = value;
    inner_config.force_input.type = ForceInput::Type::Drift;
    inner_config.force_input.force_function = nullptr;
    inner_config.force_input.force_vector.clear();
    return *this;
}

inline SchemerBuilder &SchemerBuilder::set_force(std::vector<double> value) {
    if (inner_config.force_input.type == ForceInput::Type::Drift && inner_config.force_input.drift_value != 0.0) {
        std::cerr << "[WARNING]: drift was previously set. Force vector will be used instead\n";
    }
    inner_config.force_input.force_vector = std::move(value);
    inner_config.force_input.type = ForceInput::Type::Vector;
    inner_config.force_input.force_function = nullptr;
    return *this;
}

inline SchemerBuilder &SchemerBuilder::set_force(std::function<double(double)> value) {
    if (inner_config.force_input.type == ForceInput::Type::Drift && inner_config.force_input.drift_value != 0.0) {
        std::cerr << "[WARNING]: drift was previously set. Force function will be used instead\n";
    }
    inner_config.force_input.force_function = std::move(value);
    inner_config.force_input.type = ForceInput::Type::Function;
    inner_config.force_input.force_vector.clear();
    return *this;
}

inline SchemerBuilder &SchemerBuilder::set_dt(const double value) {
    inner_config.dt = value;
    return *this;
}

inline SchemerBuilder &SchemerBuilder::set_num_intervals(const Eigen::Index value) {
    inner_config.num_intervals = value;
    return *this;
}

inline SchemerBuilder &SchemerBuilder::set_theta(const double value) {
    inner_config.theta = value;
    return *this;
}

inline SchemerBuilder &SchemerBuilder::set_verbosity(const int value) {
    inner_config.verbose = value;
    return *this;
}

inline SchemerBuilder &SchemerBuilder::set_delimiter(std::string value) {
    inner_config.delimiter = std::move(value);
    return *this;
}

inline SchemerBuilder &SchemerBuilder::set_log_interval_percent(const double value) {
    inner_config.log_interval_percent = value;
    return *this;
}

inline SchemerBuilder &SchemerBuilder::set_initial_conditions(const Eigen::VectorXd &value) {
    inner_config.values_input.vector = value;
    inner_config.values_input.type = ValuesInput::Type::Vector;
    return *this;
}

inline SchemerBuilder &SchemerBuilder::set_initial_conditions(const std::vector<double> &value) {
    inner_config.values_input.vector = Eigen::VectorXd::Map(value.data(), static_cast<Eigen::Index>(value.size()));
    inner_config.values_input.type = ValuesInput::Type::Vector;
    return *this;
}

inline SchemerBuilder &SchemerBuilder::set_zero_index(const Eigen::Index value) {
    inner_config.zero_input.index = value;
    inner_config.zero_input.type = ZeroInput::Type::Index;
    return *this;
}

inline SchemerBuilder &SchemerBuilder::set_zero_distance(const double value) {
    inner_config.zero_input.distance = value;
    inner_config.zero_input.type = ZeroInput::Type::Distance;
    return *this;
}

inline SchemerBuilder &SchemerBuilder::set_zero_middle() {
    inner_config.zero_input.type = ZeroInput::Type::Middle;
    return *this;
}

inline SchemerBuilder &SchemerBuilder::set_reset_index_zero() {
    inner_config.reset_input.type = ResetInput::Type::Zero;
    return *this;
}

inline SchemerBuilder &SchemerBuilder::set_reset_index(const Eigen::Index value) {
    inner_config.reset_input.index = value;
    inner_config.reset_input.type = ResetInput::Type::Index;
    return *this;
}

inline SchemerBuilder &SchemerBuilder::set_reset_rate(const double value) {
    inner_config.reset_input.rate = value;
    return *this;
}

inline void SchemerBuilder::validate_parameters() const {
    if (inner_config.alpha <= 0.0 || inner_config.alpha >= 2.0) throw std::invalid_argument("alpha must be in (0, 2]");
    if (std::abs(inner_config.beta) > 1.0) throw std::invalid_argument("beta must be in [-1, 1]");
    if (inner_config.sigma < 0.0) throw std::invalid_argument("sigma must be non-negative");
    if (inner_config.length <= 0.0) throw std::invalid_argument("length must be positive");

    if (inner_config.dt <= 0.0) throw std::invalid_argument("dt must be positive");
    if (inner_config.num_intervals <= 0) throw std::invalid_argument("grid_points must be positive");
    if (inner_config.theta < 0.0 || inner_config.theta > 1.0) throw std::invalid_argument("theta must be in [0, 1]");
    if (inner_config.log_interval_percent < 0.0) throw std::invalid_argument("log interval percent must be non-negative");
    if (inner_config.reset_input.rate < 0.0) throw std::invalid_argument("reset input rate must be non-negative");

    if (std::abs(inner_config.alpha - 1) < FractionalScheme::ALPHA_EPSILON && inner_config.beta != 0.0) {
        throw std::invalid_argument("beta != 0.0 not supported for alpha = 1.0");
    }

    if (inner_config.dt > 1.0) {
        std::cerr << "[WARNING] dt > 1.0\n" << std::endl;
    }
}

inline Eigen::Index SchemerBuilder::compute_starting_index() const {
    switch (inner_config.zero_input.type) {
        case ZeroInput::Type::Middle:
            return inner_config.num_intervals / 2;

        case ZeroInput::Type::Index:
            if (inner_config.zero_input.index < 0 || inner_config.zero_input.index > inner_config.num_intervals)
                throw std::invalid_argument("initial_index must be in [0, I]");
            return inner_config.zero_input.index;

        case ZeroInput::Type::Distance: {
            if (inner_config.zero_input.distance < 0.0 || inner_config.zero_input.distance > inner_config.length)
                throw std::invalid_argument("initial_distance must be in [0, length]");

            const auto grid_points_double = static_cast<double>(inner_config.num_intervals);
            const double ideal_index = grid_points_double * inner_config.zero_input.distance / inner_config.length;
            const auto real_index = static_cast<Eigen::Index>(std::round(ideal_index));
            const double actual_distance = static_cast<double>(real_index) * inner_config.length / grid_points_double;
            const double distance_difference = inner_config.zero_input.distance - actual_distance;

            if (std::abs(distance_difference) > 1e-9) {
                std::cerr << "[WARNING] Target distance " << inner_config.zero_input.distance
                          << " snapped to grid index " << real_index
                          << " (discrepancy: " << distance_difference << ")\n";
            }

            if (real_index < 0 || real_index > inner_config.num_intervals)
                throw std::logic_error("Calculated grid index is outside the grid boundaries");

            return real_index;
        }

        default:
            throw std::logic_error("Unhandled ZeroInput::Type enum value");
    }
}

inline Eigen::VectorXd SchemerBuilder::compute_initial_state(const Eigen::Index starting_index) const {
    const Eigen::Index grid_size = inner_config.get_size();
    Eigen::VectorXd starting_values = Eigen::VectorXd::Zero(grid_size);

    switch (inner_config.values_input.type) {
        case ValuesInput::Type::Dirac:
            starting_values(starting_index) = static_cast<double>(inner_config.num_intervals) / inner_config.length;
            break;

        case ValuesInput::Type::Vector:
            if (inner_config.values_input.vector.size() != grid_size)
                throw std::invalid_argument("initial values must have size I+1");
            starting_values = inner_config.values_input.vector;
            break;

        default:
            throw std::logic_error("Unhandled ValuesInput::Type enum value");
    }

    return starting_values;
}

inline std::variant<double, std::vector<double>> SchemerBuilder::compute_force_variant(
    const Eigen::Index starting_index) const {
    switch (inner_config.force_input.type) {
        case ForceInput::Type::Drift:
            return inner_config.force_input.drift_value;
        case ForceInput::Type::Function: {
            if (!inner_config.force_input.force_function)
                throw std::invalid_argument("force function not set");

            const auto coords = inner_config.get_coordinates(starting_index);
            std::vector<double> force_values(coords.size());

            for (size_t i = 0; i < coords.size(); ++i) {
                force_values[i] = inner_config.force_input.force_function(coords[i]);
            }
            return force_values;
        }

        case ForceInput::Type::Vector:
            if (inner_config.force_input.force_vector.size() != inner_config.get_size())
                throw std::invalid_argument("force vector must have size I+1");
            return inner_config.force_input.force_vector;

        default:
            throw std::logic_error("Unhandled ForceInput::Type enum value");
    }
}

inline Eigen::Index SchemerBuilder::compute_reset_index(const Eigen::Index starting_index) const {
    switch (inner_config.reset_input.type) {
        case ResetInput::Type::Index:
            if (inner_config.reset_input.index < 0 || inner_config.reset_input.index >= inner_config.get_size())
                throw std::invalid_argument("reset input index must be in [0, I)");
            return inner_config.reset_input.index;
        case ResetInput::Type::Zero:
            return starting_index;

        default:
            throw std::logic_error("Unhandled ResetInput::Type enum value");
    }
}

inline Eigen::MatrixXd SchemerBuilder::compute_step_matrix(
    const std::variant<double, std::vector<double>>& force_variant, const Eigen::Index reset_index) const {
    const FractionalScheme fractional_scheme{inner_config};

    const Eigen::MatrixXd diffusion_matrix = fractional_scheme.build_diffusion_matrix();
    const Eigen::MatrixXd force_matrix = fractional_scheme.build_force_matrix(force_variant);
    const Eigen::MatrixXd reset_rate_matrix = fractional_scheme.build_reset_matrix(reset_index);

    const Eigen::MatrixXd total_operator = diffusion_matrix + force_matrix + reset_rate_matrix;

    const Eigen::Index grid_size = inner_config.get_size();
    const Eigen::MatrixXd id = Eigen::MatrixXd::Identity(grid_size, grid_size);

    const Eigen::MatrixXd Lhs = id + inner_config.theta * total_operator;
    const Eigen::MatrixXd Rhs = id - (1.0 - inner_config.theta) * total_operator;

    const Eigen::PartialPivLU<Eigen::MatrixXd> solver(Lhs);
    return solver.solve(Rhs);
}

inline SimulationSetup SchemerBuilder::build_setup() const {
    validate_parameters();

    const Eigen::Index starting_index = compute_starting_index();
    const Eigen::Index reset_index = compute_reset_index(starting_index);

    auto force_variant = compute_force_variant(starting_index);
    auto initial_state = compute_initial_state(starting_index);

    Eigen::MatrixXd step_matrix = compute_step_matrix(force_variant, reset_index);

    return SimulationSetup{
        .config = inner_config,
        .step_matrix = std::move(step_matrix),
        .initial_state = std::move(initial_state),
        .starting_index = starting_index,
        .force_variant = std::move(force_variant)
    };
}

inline SchemerRunner SchemerBuilder::build() const {
    return SchemerRunner(build_setup());
}
