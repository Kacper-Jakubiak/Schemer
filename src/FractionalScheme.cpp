/**
 * @file FractionalScheme.cpp
 * @brief Matrix construction for simulations.
 */

#include "../include/FractionalScheme.h"
#include <numbers>
#include <cmath>

FractionalScheme::FractionalScheme(const Config& config) : config(config) {
    const double alpha = config.alpha;
    const double dx = config.get_dx();
    n = std::ceil(alpha);

    cauchyOmega = config.sigma * config.dt / (dx * std::numbers::pi);

    const double omega = config.sigma * config.dt / (std::tgamma(n + 1 - alpha) * pow(dx, alpha));
    const double denominator = 2.0 * std::cos(std::numbers::pi * alpha / 2.0);
    L_omega = -omega * (1.0 + config.beta) / denominator;
    R_omega = -omega * (1.0 - config.beta) / denominator;
}


std::vector<double> FractionalScheme::get_lambdas() const {
    const Eigen::Index grid_size = config.get_size();
    std::vector<double> result(grid_size + 2);
    for (int i = 1; i < grid_size + 2; i++) {
        result[i] = pow(i, n - config.alpha) - pow(i - 1, n - config.alpha);
    }

    return result;
}


Eigen::MatrixXd FractionalScheme::build_M1() const {
    const Eigen::Index grid_size = config.get_size();
    Eigen::MatrixXd M = Eigen::MatrixXd::Zero(grid_size, grid_size);
    const auto lambda = get_lambdas();

    const double v0 = lambda[1];
    M.diagonal().setConstant(-1.0 * (L_omega + R_omega) * v0);

    for (Eigen::Index i = 1; i < grid_size; i++) {
        double value = lambda[i] - lambda[i + 1];
        M.diagonal(-i).setConstant(value * L_omega);
        M.diagonal(i).setConstant(value * R_omega);
    }

    return M;
}


Eigen::MatrixXd FractionalScheme::build_M2() const {
    const Eigen::Index grid_size = config.get_size();
    Eigen::MatrixXd M = Eigen::MatrixXd::Zero(grid_size, grid_size);
    const auto lambda = get_lambdas();

    double v0 = lambda[2] - 2.0 * lambda[1];
    M.diagonal().setConstant(-1.0 * (L_omega + R_omega) * v0);

    double v1 = lambda[3] - 2 * lambda[2] + lambda[1];
    double v2 = lambda[1];
    M.diagonal(-1).setConstant(-1.0 * (L_omega * v1 + R_omega * v2));
    M.diagonal(1).setConstant(-1.0 * (R_omega * v1 + L_omega * v2));

    for (Eigen::Index i = 2; i < grid_size; i++) {
        double value = lambda[i + 2] - 2 * lambda[i + 1] + lambda[i];
        M.diagonal(-i).setConstant(-1.0 * value * L_omega);
        M.diagonal(i).setConstant(-1.0 * value * R_omega);
    }

    return M;
}


Eigen::MatrixXd FractionalScheme::build_M3() const {
    const Eigen::Index grid_size = config.get_size();
    Eigen::MatrixXd M = Eigen::MatrixXd::Zero(grid_size, grid_size);

    M.diagonal().setConstant(2.0 * cauchyOmega);

    for (Eigen::Index i = 1; i < grid_size; i++) {
        const auto double_i = static_cast<double>(i);
        double value = 1.0 / (2.0 * double_i + 1.0) - 1.0 / (2.0 * (double_i - 1) + 1.0);
        value *= cauchyOmega;
        M.diagonal(-i).setConstant(value);
        M.diagonal(i).setConstant(value);
    }

    return M;
}


Eigen::MatrixXd FractionalScheme::build_drift(const double mi) const {
    const Eigen::Index grid_size = config.get_size();
    const double dx = config.get_dx();
    Eigen::MatrixXd V = Eigen::MatrixXd::Zero(grid_size, grid_size);
    for (Eigen::Index i = 0; i < grid_size; i++) {
        V(i, i) = 3.0;
        if (i >= 1)
            V(i, i - 1) = -4.0;
        if (i >= 2)
            V(i, i - 2) = 1.0;
    }

    const double ni = mi * config.dt / (2.0 * dx);

    if (mi < 0)
        return -1.0 * ni * V.transpose();
    return ni * V;
}   


Eigen::MatrixXd FractionalScheme::build_force(const std::vector<double>& force) const {
    const Eigen::Index grid_size = config.get_size();
    const double dx = config.get_dx();
    Eigen::MatrixXd V = Eigen::MatrixXd::Zero(grid_size, grid_size);

    for (Eigen::Index i = 0; i < grid_size; i++) {
        if (i > 0) {
            V(i, i) -= force[i - 1];
            V(i, i - 1) -= force[i];
        }
        if (i < grid_size - 1) {
            V(i, i) += force[i + 1];
            V(i, i + 1) += force[i];
        }
    }
    V *= config.dt / (2 * dx);
    return V;
}


Eigen::MatrixXd FractionalScheme::build_diffusion_matrix() const {
    if (std::abs(config.alpha - 1.0) < ALPHA_EPSILON)
        return build_M3();

    if (config.alpha < 1.0)
        return build_M1();

    return build_M2();
}

Eigen::MatrixXd FractionalScheme::build_force_matrix(const std::variant<double, std::vector<double>>& force_variant) const {
    if (std::holds_alternative<double>(force_variant)) {
        return build_drift(std::get<double>(force_variant));
    }
    return build_force(std::get<std::vector<double>>(force_variant));
}
