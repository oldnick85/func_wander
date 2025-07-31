#include <argh.h>
#include <csignal>
#include <cstdio>
#include <format>
#include <fstream>
#include <iostream>
#include <print>

#include <func_node.h>
#include <search_task.h>

#include "atom_samples.h"
#include "target_sample.h"

using namespace fw;

std::atomic_bool g_stop = false;
Settings g_settings;
std::string g_status;

void signalHandler(int signal)
{
    std::println("got signal {}", signal);
    if (signal == SIGINT) {
        std::println("terminating by Ctrl+C");
        g_stop = true;
    }
}

void MainLoop()
{
    constexpr std::size_t MAX_CONSTANT = 256;
    AtomFuncs<Value_t> atoms;
    atoms.Add(new AF_ARG_X{});
    for (std::size_t val = 1; val <= MAX_CONSTANT; ++val) {
        atoms.Add(new AF_CONST{static_cast<Value_t>(val)});
    }
    //atoms.Add(new AF_FW1{});
    //atoms.Add(new AF_FW2{});
    atoms.Add(new AF_NOT{});
    atoms.Add(new AF_BITCOUNT{});
    atoms.Add(new AF_SUM{});
    atoms.Add(new AF_SUB{});
    atoms.Add(new AF_AND{});
    atoms.Add(new AF_OR{});
    atoms.Add(new AF_XOR{});
    atoms.Add(new AF_SHR{});
    atoms.Add(new AF_SHL{});

    MyTarget target;

    SearchTask<Value_t, true, true> task{g_settings, &atoms, &target};

    if (not g_settings.save_file.empty()) {
        std::ifstream file(g_settings.save_file);
        if (file) {

            const auto task_json =
                std::string(std::istreambuf_iterator<char>(file),
                            std::istreambuf_iterator<char>());

            if (not task.FromJSON(task_json)) {
                std::println("Failed to parse JSON from file: {}",
                             g_settings.save_file);
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
        std::this_thread::sleep_for(std::chrono::seconds(10));
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

int main(int argc, char* argv[])
{

    argh::parser cmdl;
    cmdl.add_param("savefile");
    cmdl.add_param("max_depth");
    cmdl.add_param("savefile");
    cmdl.parse(argc, argv);

    if (cmdl("savefile")) {
        g_settings.save_file = cmdl("savefile").str();
    }

    if (cmdl("max-depth")) {
        g_settings.max_depth = std::stoi(cmdl("max-depth").str());
    }

    if (cmdl("max-best")) {
        g_settings.max_best = std::stoi(cmdl("max-best").str());
    }

    std::signal(SIGINT, signalHandler);
    MainLoop();

    std::println("{}", g_status);

    return 0;
}
