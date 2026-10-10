/**
 * @file SchemerRunner.cpp
 * @brief Simulation execution and result saving.
 */

#include "../include/SchemerRunner.h"
#include <iostream>
#include <ostream>
#include <fstream>
#include <Eigen/Dense>
#include <utility>
#include <chrono>
#include <iomanip>

#include "../include/SimulationSetup.h"

SchemerRunner::SchemerRunner(SimulationSetup setup) : setup(std::move(setup)) {
    reset();
}

void SchemerRunner::log(std::ostream &os, const double progress_percent) {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);

    os << '['
       << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
       << "] Progress: "
       << progress_percent
       << "%" << std::endl;
}

void SchemerRunner::save_parameters(std::ostream &os) const {
    const auto& config = setup.config;

    os << '#' << "alpha: " << config.alpha << '\n';
    os << '#' << "beta: " << config.beta << '\n';
    os << '#' << "sigma: " << config.sigma << '\n';
    os << '#' << "length: " << config.length << '\n';

    os << '#' << "dt: " << config.dt << '\n';
    os << '#' << "num_intervals: " << config.num_intervals << '\n';
    os << '#' << "theta: " << config.theta << '\n';

    os << '#' << "delimiter: " << config.delimiter << '\n';
    os << '#' << "log_interval_percent: " << config.log_interval_percent << '\n';
    os << '#' << "verbose: " << config.verbose << '\n';

    os << '#' << "initial_values: ";
    for (Eigen::Index i = 0; i < setup.initial_state.size(); i++)
        os << setup.initial_state(i) << config.delimiter;
    os << '\n';

    os << '#' << "force_variant: ";

    if (std::holds_alternative<double>(setup.force_variant)) {
        os << std::get<double>(setup.force_variant) << '\n';
    } else {
        for (const double force_value : std::get<std::vector<double>>(setup.force_variant)) {
            os << force_value << config.delimiter;
        }
        os << '\n';
    }

    os << '#' << "x_coordinates: ";
    auto coords = config.get_coordinates(setup.starting_index);
    for (const double position: coords) {
        os << position << config.delimiter;
    }
    os << std::endl;
}

void SchemerRunner::save_current_values(std::ostream &os) const {
    const auto& delimiter = setup.config.delimiter;
    for (Eigen::Index i = 0; i < current_values.size(); i++)
        os << current_values(i) << delimiter;
    os << std::endl;
}

void SchemerRunner::reset() {
    current_values = setup.initial_state;
    steps_taken = 0;
}

void SchemerRunner::run(const int steps, std::ostream *history_stream, const int save_every) {
    if (steps < 0)
        throw std::invalid_argument("amount of steps cannot be negative");
    if (save_every < 0)
        throw std::invalid_argument("save_every cannot be negative");
    if (save_every > 0 && history_stream == nullptr)
        std::cerr << "[WARNING] history_stream not provided, history will not be saved";

    const bool should_log_history = history_stream != nullptr && save_every > 0;
    if (should_log_history) {
        if (history_stream->tellp() <= 0)
            save_parameters(*history_stream);
    }

    std::ofstream log_stream;
    bool should_log_progress = setup.config.log_interval_percent > 0;
    if (should_log_progress) {
        log_stream.open(LOG_FILE_PATH.data());
        if (!log_stream) {
            std::cerr << "[WARNING] Failed opening log file " + std::string(LOG_FILE_PATH) << "logs will not be saved";
            should_log_progress = false;
        }
    }
    const int log_interval = std::max(1, static_cast<int>(steps * setup.config.log_interval_percent / 100.0));

    for (int step = 0; step < steps; step++) {
        if (should_log_progress && step % log_interval == 0)
            log(log_stream, 100.0 * step / steps);

        if (should_log_history && step % save_every == 0)
            save_current_values(*history_stream);

        current_values = setup.step_matrix * current_values;
        steps_taken++;
    }

    if (should_log_history)
        save_current_values(*history_stream);

    if (should_log_progress)
        log(log_stream, 100.0);
}

void SchemerRunner::save_result(const std::string &filepath) const {
    std::ofstream out_stream(filepath);
    if (!out_stream) {
        throw std::runtime_error("Failed to open save file: " + filepath);
    }
    save_parameters(out_stream);
    out_stream << '#' << "steps: " << steps_taken << std::endl;
    save_current_values(out_stream);
    if (setup.config.verbose > 0)
        std::cout << "Saved to " << filepath << std::endl;
}