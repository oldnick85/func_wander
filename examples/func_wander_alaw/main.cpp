/**
 * @file main.cpp
 * @brief A-law audio encoding function synthesizer using genetic programming
 *
 * This program performs automatic synthesis of A-law audio encoding function
 * using evolutionary search over combinations of bitwise operations and constants.
 * The target function approximates the standard A-law compression curve used
 * in telecommunication systems for 8-bit audio encoding.
 *
 * The search space consists of:
 * - Input variable (16-bit audio sample)
 * - Bitwise operations (AND, OR, XOR, NOT, shifts)
 * - Population count (bitcount)
 * - Power-of-two constants (1..32768)
 * - And other
 *
 * The algorithm uses depth-limited expression tree exploration with periodic
 * state saving/loading via JSON serialization.
 *
 * @section features Features
 * - Resume interrupted searches from saved state
 * - Configurable search depth and population size
 * - Real-time progress monitoring
 * - Graceful shutdown on SIGINT
 *
 * @section usage Usage
 * Run with --help to see command line options
 */

#include <CLI/CLI.hpp>
#include <array>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <print>
#include <sstream>
#include <string>
#include <thread>

#include <func_node.h>
#include <search_task.h>

#include "atom_samples.h"
#include "target_sample.h"

using fw::AtomFuncs;
using fw::SearchTask;
using fw::Settings;

namespace
{
std::atomic_bool g_stop = false;
Settings g_settings;
std::string g_status;
bool g_print_target = false;

void SignalHandler(int signal)
{
    std::println("got signal {}", signal);
    if (signal == SIGINT) {
        std::println("terminating by Ctrl+C");
        g_stop = true;
    }
}

void MainLoop()
{
    constexpr std::size_t MAX_CONSTANTS = 4;
    constexpr std::size_t MAX_CONSTANTS_2_POW = 16;

    AtomFuncs<Value_t> atoms;
    auto af_x = std::make_unique<AF_ARG_X>();
    atoms.Add(af_x.get());

    std::vector<Value_t> consts;
    for (std::size_t i = 1; i <= MAX_CONSTANTS_2_POW; ++i) {
        const auto val = static_cast<Value_t>(1 << (i - 1));
        if (std::ranges::contains(consts, val)) {
            continue;
        }
        consts.push_back(val);
    }
    for (std::size_t i = 1; i <= MAX_CONSTANTS; ++i) {
        const auto val = static_cast<Value_t>(i);
        if (std::ranges::contains(consts, val)) {
            continue;
        }
        consts.push_back(static_cast<Value_t>(val));
    }

    std::vector<std::unique_ptr<AF_CONST>> af_c;
    for (const auto val : consts) {
        af_c.push_back(std::make_unique<AF_CONST>(val));
        atoms.Add(af_c.back().get());
    }
    //auto af_fw1 = std::make_unique<AF_FW1>();
    //atoms.Add(af_fw1.get());
    //auto af_fw2 = std::make_unique<AF_FW2>();
    //atoms.Add(af_fw2.get());

    //auto af_f_0_31 = std::make_unique<AF_F_0_31>();
    //atoms.Add(af_f_0_31.get());
    //auto af_f_128_159 = std::make_unique<AF_F_128_159>();
    //atoms.Add(af_f_128_159.get());

    auto af_not = std::make_unique<AF_NOT>();
    atoms.Add(af_not.get());
    auto af_bc = std::make_unique<AF_BITCOUNT>();
    atoms.Add(af_bc.get());
    //auto af_sum = std::make_unique<AF_SUM>();
    //atoms.Add(af_sum.get());
    //auto af_sub = std::make_unique<AF_SUB>();
    //atoms.Add(af_sub.get());
    auto af_and = std::make_unique<AF_AND>();
    atoms.Add(af_and.get());
    auto af_or = std::make_unique<AF_OR>();
    atoms.Add(af_or.get());
    auto af_xor = std::make_unique<AF_XOR>();
    atoms.Add(af_xor.get());
    auto af_shr = std::make_unique<AF_SHR>();
    atoms.Add(af_shr.get());
    auto af_shl = std::make_unique<AF_SHL>();
    atoms.Add(af_shl.get());

    MyTarget target;

    if (g_print_target) {
        std::println("{}", target.StrFull());
    }

    SearchTask<Value_t, true, true> task{g_settings, &atoms, &target};

    if (not g_settings.save_file.empty()) {
        const std::ifstream file(g_settings.save_file);
        if (file) {

            std::ostringstream buffer;
            buffer << file.rdbuf();
            const auto task_json = buffer.str();

            if (not task.FromJSON(task_json)) {
                std::println("Failed to parse JSON from file: {}", g_settings.save_file);
                return;
            }
            std::println("Loaded JSON from file: {}", g_settings.save_file);
        }
        else {
            std::println("Failed to open file: {}", g_settings.save_file);
        }
    }

    task.Run();

    while (not g_stop) {
        constexpr auto PauseOutput = std::chrono::seconds(10);
        std::this_thread::sleep_for(PauseOutput);
        if (task.Done()) {
            g_stop = true;
        }
        auto status = task.Status();
        std::println("{}", status);
    }

    task.Stop();

    if (not g_settings.save_file.empty()) {
        std::ofstream file(g_settings.save_file, std::ios::trunc);

        if (not file.is_open()) {
            std::println("Failed to open file: {}", g_settings.save_file);
            return;
        }

        auto task_json = task.ToJSON();
        file << task_json;

        std::println("Current status saved to {}", g_settings.save_file);
    }

    g_status = task.Status();
}

}  // namespace

int main(int argc, char* argv[])
{
    CLI::App app{"Synthesizes A-law audio encoding function using evolutionary bitwise operation search"};

    // Define command line options with descriptions
    app.add_option("--savefile", g_settings.save_file, "Path to JSON file for saving/resuming search state")
        ->check(CLI::ExistingFile);
    app.add_option("--max-depth", g_settings.max_depth, "Maximum expression tree depth (positive integer)")
        ->check(CLI::PositiveNumber);
    app.add_option("--max-best", g_settings.max_best, "Number of top solutions to retain (positive integer)")
        ->check(CLI::PositiveNumber);
    app.add_flag("--print-target", g_print_target, "Print target function");

    try {
        // Parse command line arguments
        app.parse(argc, argv);
    }
    catch (const CLI::ParseError& e) {
        // CLI11 automatically handles --help and error messages
        return app.exit(e);
    }

    auto previous_handler = std::signal(SIGINT, SignalHandler);
    if (previous_handler == SIG_ERR) {
        std::println("Failed to set signal handler");
        return EXIT_FAILURE;
    }

    MainLoop();

    std::println("{}", g_status);

    return EXIT_SUCCESS;
}
