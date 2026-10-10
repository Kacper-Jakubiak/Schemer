/**
 * @file FractionalScheme.h
 * @brief Builds matrices for fractional diffusion simulations.
 */

#pragma once

#include "Config.h"
#include <Eigen/Dense>
#include <vector>
#include <variant>

/**
 * @class FractionalScheme
 * @brief Builds diffusion and force matrices for time stepping.
 *
 * Calculates fractional derivative matrices based on alpha, beta, and grid spacing.
 */
class FractionalScheme {
public:
    /**
     * @brief Small tolerance value used to check if $\alpha = 1$.
     */
    static constexpr double ALPHA_EPSILON = 1e-8;

    /**
     * @brief Construct a new FractionalScheme object with configuration settings.
     * @param config Reference to the Config object containing simulation settings.
     */
    explicit FractionalScheme(const Config& config);

    /**
     * @brief Builds the main spatial diffusion matrix.
     *
     * Calls build_M1(), build_M2(), or build_M3() depending on the value of $\alpha$.
     *
     * @return Eigen::MatrixXd The spatial diffusion matrix.
     */
    [[nodiscard]] Eigen::MatrixXd build_diffusion_matrix() const;

    /**
     * @brief Builds the force matrix.
     *
     * Handles both constant forces (a single number) and variable forces (a value per grid point).
     *
     * @param force_variant Either a single value (`double`) or grid values (`std::vector<double>`).
     * @return Eigen::MatrixXd The force matrix.
     */
    [[nodiscard]] Eigen::MatrixXd build_force_matrix(const std::variant<double, std::vector<double>>& force_variant) const;

private:
    /// Reference to simulation settings.
    const Config& config;

    /// Weight for the left-side fractional derivative ($\omega_L$).
    double L_omega;

    /// Weight for the right-side fractional derivative ($\omega_R$).
    double R_omega;

    /// Weight factor used when $\alpha = 1$.
    double cauchyOmega;

    /// $\alpha$ rounded up.
    double n;

    /**
     * @brief Calculates fractional weight coefficients ($\lambda_k^{(\alpha)}$).
     *
     * Generates a list of weight values used to construct the diffusion matrix.
     *
     * @return std::vector<double> List of calculated weights.
     */
    [[nodiscard]] std::vector<double> get_lambdas() const;

    /**
     * @brief Builds the diffusion matrix for $0 < \alpha < 1$.
     * @return Eigen::MatrixXd Diffusion matrix for $\alpha < 1$.
     */
    [[nodiscard]] Eigen::MatrixXd build_M1() const;

    /**
     * @brief Builds the diffusion matrix for $1 < \alpha \le 2$.
     * @return Eigen::MatrixXd Diffusion matrix for $\alpha > 1$.
     */
    [[nodiscard]] Eigen::MatrixXd build_M2() const;

    /**
     * @brief Builds the diffusion matrix for $\alpha = 1$.
     * @return Eigen::MatrixXd Diffusion matrix for $\alpha = 1$.
     */
    [[nodiscard]] Eigen::MatrixXd build_M3() const;

    /**
     * @brief Builds the force matrix for a constant force.
     * @param mi Constant force value.
     * @return Eigen::MatrixXd Constant force matrix.
     */
    [[nodiscard]] Eigen::MatrixXd build_drift(double mi) const;

    /**
     * @brief Builds the force matrix for a variable force across grid points.
     * @param force Force value at each grid point.
     * @return Eigen::MatrixXd Variable force matrix.
     */
    [[nodiscard]] Eigen::MatrixXd build_force(const std::vector<double>& force) const;
};