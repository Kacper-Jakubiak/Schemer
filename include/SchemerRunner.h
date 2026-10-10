/**
 * @file SchemerRunner.h
 * @brief Runs time-dependent simulations and manages data output.
 */

#pragma once

#include <Eigen/Dense>
#include <string>
#include "SimulationSetup.h"

/**
 * @class SchemerRunner
 * @brief Manages simulation progress, time stepping, and file saving.
 *
 * Controls the step-by-step execution of the simulation, keeps track of current
 * state values, and logs progress or saves output data to files.
 */
class SchemerRunner {
private:
    /// File path used for writing progress log messages.
    static constexpr std::string_view LOG_FILE_PATH = "progress.log";

    /// Setup used to create this simulation.
    SimulationSetup setup;

    /// Total number of time steps completed so far.
    int steps_taken = 0;

    /// Values across all grid points at the current time step.
    Eigen::VectorXd current_values;

    /**
     * @brief Writes current progress percentage to an output stream.
     * @param os Stream where log messages are written.
     * @param progress_percent Completion percentage (0 to 100).
     */
    static void log(std::ostream &os, double progress_percent);

    /**
     * @brief Writes configuration settings to an output stream.
     * @param os Stream where parameters are written.
     */
    void save_parameters(std::ostream &os) const;

    /**
     * @brief Writes current grid values to an output stream.
     * @param os Stream where current state values are written.
     */
    void save_current_values(std::ostream &os) const;

public:
    /**
     * @brief Creates a simulation runner using a given setup.
     * @param setup Setup object containing settings, matrices, and initial values.
     */
    explicit SchemerRunner(SimulationSetup setup);

    ~SchemerRunner() = default;

    SchemerRunner(const SchemerRunner&) = default;
    SchemerRunner& operator=(const SchemerRunner&) = default;

    SchemerRunner(SchemerRunner&&) = default;
    SchemerRunner& operator=(SchemerRunner&&) = default;

    /**
     * @brief Resets the simulation back to step 0 and initial state values.
     */
    void reset();

    /**
     * @brief Advances the simulation forward by a specified number of time steps.
     *
     * Steps through time, updates current grid values, logs progress, and
     * optionally writes history states to a stream at specified step intervals.
     *
     * @param steps Total number of time steps to run.
     * @param history_stream Optional output stream for saving history data (pass nullptr to skip).
     * @param save_every How often to save state history in steps (set to 0 to skip saving).
     */
    void run(int steps, std::ostream *history_stream = nullptr, int save_every = 0);

    /**
     * @brief Saves the current simulation state and settings to a file.
     * @param filepath Path to the output file on disk.
     */
    void save_result(const std::string &filepath) const;
};