/**
 * @file SchemerBuilder_build.cpp
 * @brief Build and validation logic for the solver.
 */

#include "../include/SchemerBuilder.h"
#include <iostream>
#include <fstream>
#include <stdexcept>

#include "../include/FractionalScheme.h"

void SchemerBuilder::validate_parameters() const {
    if (inner_config.alpha <= 0.0 || inner_config.alpha > 2.0) throw std::invalid_argument("alpha must be in (0, 2]");
    if (std::abs(inner_config.beta) > 1.0) throw std::invalid_argument("beta must be in [-1, 1]");
    if (inner_config.sigma < 0.0) throw std::invalid_argument("sigma must be non-negative");
    if (inner_config.length <= 0.0) throw std::invalid_argument("length must be positive");

    if (inner_config.dt <= 0.0) throw std::invalid_argument("dt must be positive");
    if (inner_config.num_intervals <= 0) throw std::invalid_argument("grid_points must be positive");
    if (inner_config.theta < 0.0 || inner_config.theta > 1.0) throw std::invalid_argument("theta must be in [0, 1]");
    if (inner_config.log_interval_percent < 0.0) throw std::invalid_argument("log interval percent must be non-negative");

    // (beta != 0) is not supported when alpha = 1.0
    if (std::abs(inner_config.alpha - 1) < FractionalScheme::ALPHA_EPSILON && inner_config.beta != 0.0) {
        throw std::invalid_argument("beta != 0.0 not supported for alpha = 1.0");
    }

    // Soft warning for large time steps
    if (inner_config.dt > 1.0) {
        std::cerr << "[WARNING] dt > 1.0\n" << std::endl;
    }
}

Eigen::Index SchemerBuilder::compute_starting_index() const {
    switch (inner_config.zero_input.type) {
        case ZeroInput::Type::Middle:
            // Place zero reference point at the exact center of the grid
            return inner_config.num_intervals / 2;

        case ZeroInput::Type::Index:
            // Use the explicit user-provided grid index
            if (inner_config.zero_input.index < 0 || inner_config.zero_input.index > inner_config.num_intervals)
                throw std::invalid_argument("initial_index must be in [0, I]");
            return inner_config.zero_input.index;

        case ZeroInput::Type::Distance: {
            // Convert physical distance offset to nearest discrete grid index
            if (inner_config.zero_input.distance < 0.0 || inner_config.zero_input.distance > inner_config.length)
                throw std::invalid_argument("initial_distance must be in [0, length]");

            const auto grid_points_double = static_cast<double>(inner_config.num_intervals);
            const double ideal_index = grid_points_double * inner_config.zero_input.distance / inner_config.length;
            const auto real_index = static_cast<Eigen::Index>(std::round(ideal_index));
            const double actual_distance = static_cast<double>(real_index) * inner_config.length / grid_points_double;
            const double distance_difference = inner_config.zero_input.distance - actual_distance;

            // Warn if distance doesn't align perfectly with grid points
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

Eigen::VectorXd SchemerBuilder::compute_initial_state(const Eigen::Index starting_index) const {
    const Eigen::Index grid_size = inner_config.get_size();
    Eigen::VectorXd starting_values = Eigen::VectorXd::Zero(grid_size);

    switch (inner_config.values_input.type) {
        case ValuesInput::Type::Dirac:
            // Scale Dirac delta point mass based on spatial resolution (1 / dx)
            starting_values(starting_index) = static_cast<double>(inner_config.num_intervals) / inner_config.length;
            break;

        case ValuesInput::Type::Vector:
            // Copy custom user-defined initial vector directly
            if (inner_config.values_input.vector.size() != grid_size)
                throw std::invalid_argument("initial values must have size I+1");
            starting_values = inner_config.values_input.vector;
            break;

        default:
            throw std::logic_error("Unhandled ValuesInput::Type enum value");
    }

    return starting_values;
}

std::variant<double, std::vector<double>> SchemerBuilder::compute_force_variant(const Eigen::Index starting_index) const {
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
            // Use pre-computed spatial force vector
            if (inner_config.force_input.force_vector.size() != inner_config.get_size())
                throw std::invalid_argument("force vector must have size I+1");
            return inner_config.force_input.force_vector;

        default:
            throw std::logic_error("Unhandled ForceInput::Type enum value");
    }
}

Eigen::MatrixXd SchemerBuilder::compute_step_matrix(const std::variant<double, std::vector<double>>& force_variant) const {
    const FractionalScheme fractional_scheme{inner_config};

    const Eigen::MatrixXd diffusion_matrix = fractional_scheme.build_diffusion_matrix();
    const Eigen::MatrixXd force_matrix = fractional_scheme.build_force_matrix(force_variant);

    const Eigen::Index grid_size = inner_config.get_size();
    const Eigen::MatrixXd id = Eigen::MatrixXd::Identity(grid_size, grid_size);

    const Eigen::MatrixXd Lhs = id + inner_config.theta * (force_matrix + diffusion_matrix);
    const Eigen::MatrixXd Rhs = id - (1 - inner_config.theta) * (force_matrix + diffusion_matrix);

    const Eigen::PartialPivLU<Eigen::MatrixXd> solver(Lhs);
    return solver.solve(Rhs);
}

SimulationSetup SchemerBuilder::build_setup() const {
    validate_parameters();

    const Eigen::Index starting_index = compute_starting_index();

    auto force_variant = compute_force_variant(starting_index);
    auto initial_state = compute_initial_state(starting_index);

    Eigen::MatrixXd step_matrix = compute_step_matrix(force_variant);

    return SimulationSetup{
        inner_config,
        std::move(step_matrix),
        std::move(initial_state),
        starting_index,
        std::move(force_variant)
    };
}

SchemerRunner SchemerBuilder::build() const {
    return SchemerRunner(build_setup());
}