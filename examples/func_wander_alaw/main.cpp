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
#include <chrono>
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

#include "atom_samples.h"
#include "interaction_cli.h"
#include "target_sample.h"

using fw::AtomFuncBase;
using fw::AtomFuncs;
using fw::SearchTask;
using fw::Settings;

namespace
{

bool g_print_target = false;
std::vector<std::unique_ptr<AtomFuncBase>> g_atoms;

void InitAtoms(AtomFuncs<Value_t>& atoms, MyTarget& target)
{
    constexpr std::size_t MAX_CONSTANTS = 8;
    constexpr std::size_t MAX_CONSTANTS_2_POW = 15;

    auto af_x = std::make_unique<AF_ARG_X>();
    atoms.Add(af_x.get());
    g_atoms.push_back(std::move(af_x));

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

    for (const auto val : consts) {
        auto af_c = std::make_unique<AF_CONST>(val);
        atoms.Add(af_c.get());
        g_atoms.push_back(std::move(af_c));
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
    g_atoms.push_back(std::move(af_not));
    auto af_bc = std::make_unique<AF_BITCOUNT>();
    atoms.Add(af_bc.get());
    g_atoms.push_back(std::move(af_bc));
    //auto af_sum = std::make_unique<AF_SUM>();
    //atoms.Add(af_sum.get());
    //g_atoms.push_back(std::move(af_sum));
    //auto af_sub = std::make_unique<AF_SUB>();
    //atoms.Add(af_sub.get());
    //g_atoms.push_back(std::move(af_sub));
    auto af_and = std::make_unique<AF_AND>();
    atoms.Add(af_and.get());
    g_atoms.push_back(std::move(af_and));
    auto af_or = std::make_unique<AF_OR>();
    atoms.Add(af_or.get());
    g_atoms.push_back(std::move(af_or));
    auto af_xor = std::make_unique<AF_XOR>();
    atoms.Add(af_xor.get());
    g_atoms.push_back(std::move(af_xor));
    auto af_shr = std::make_unique<AF_SHR>();
    atoms.Add(af_shr.get());
    g_atoms.push_back(std::move(af_shr));
    auto af_shl = std::make_unique<AF_SHL>();
    atoms.Add(af_shl.get());
    g_atoms.push_back(std::move(af_shl));

    if (g_print_target) {
        std::println("{}", target.StrFull());
    }
}

}  // namespace

int main(int argc, char* argv[])
{
    Settings settings;

    CLI::App app{"Synthesizes A-law audio encoding function using evolutionary bitwise operation search"};

    // Define command line options with descriptions
    app.add_option("--savefile", settings.save_file, "Path to JSON file for saving/resuming search state")
        ->check(CLI::ExistingFile);
    app.add_option("--max-depth", settings.max_depth, "Maximum expression tree depth (positive integer)")
        ->check(CLI::PositiveNumber);
    app.add_option("--max-best", settings.max_best, "Number of top solutions to retain (positive integer)")
        ->check(CLI::PositiveNumber);
    app.add_flag("--http", settings.http_enabled, "Enable HTTP server for remote control");
    app.add_option("--http-host", settings.http_host, "Host address for HTTP server (default: localhost)");
    app.add_option("--http-port", settings.http_port, "Port for HTTP server (default: 8080, range 1-65535)")
        ->check(CLI::Range(1, 65535));
    app.add_flag("--print-target", g_print_target, "Print target function");

    try {
        // Parse command line arguments
        app.parse(argc, argv);
    }
    catch (const CLI::ParseError& e) {
        // CLI11 automatically handles --help and error messages
        return app.exit(e);
    }

    AtomFuncs<Value_t> atoms;
    MyTarget target;

    InitAtoms(atoms, target);
    const auto result = MainLoop<Value_t>(settings, atoms, target);

    return result;
}
