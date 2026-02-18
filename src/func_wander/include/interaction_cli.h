#pragma once

#include <atomic>
#include <csignal>
#include <print>

#include "func_node.h"
#include "interaction_http.h"
#include "search_task.h"

namespace fw
{
std::atomic_bool g_stop = false;

void SignalHandler(int signal)
{
    std::println("got signal {}", signal);
    if (signal == SIGINT) {
        std::println("terminating by Ctrl+C");
        g_stop = true;
    }
}

template <typename TVal>
int MainLoop(const Settings& settings, AtomFuncs<TVal>& atoms, Target<TVal>& target)
{
    auto previous_handler = std::signal(SIGINT, SignalHandler);
    if (previous_handler == SIG_ERR) {
        std::println("Failed to set signal handler");
        return EXIT_FAILURE;
    }

    SearchTask<Value_t, true, true> task{settings, &atoms, &target};

    if (not settings.save_file.empty()) {
        const std::ifstream file(settings.save_file);
        if (file) {

            std::ostringstream buffer;
            buffer << file.rdbuf();
            const auto task_json = buffer.str();

            if (not task.FromJSON(task_json)) {
                std::println("Failed to parse JSON from file: {}", settings.save_file);
                return EXIT_FAILURE;
            }
            std::println("Loaded JSON from file: {}", settings.save_file);
        }
        else {
            std::println("Failed to open file: {}", settings.save_file);
        }
    }

    status::Status status;

    if (settings.http_enabled) {
        interaction_http::Run(status, settings.http_host, settings.http_port);
    }

    task.Run();

    while (not g_stop) {
        constexpr auto PauseOutput = std::chrono::seconds(10);
        std::this_thread::sleep_for(PauseOutput);
        if (task.Done()) {
            g_stop = true;
        }
        status = task.GetStatus();
        if (settings.http_enabled) {
            std::println("iterations_count={}", status.iterations_count);
        }
        else {
            std::println("{}", status.to_string());
        }
    }

    task.Stop();

    if (settings.http_enabled) {
        interaction_http::Stop();
    }

    if (not settings.save_file.empty()) {
        std::ofstream file(settings.save_file, std::ios::trunc);

        if (not file.is_open()) {
            std::println("Failed to open file: {}", settings.save_file);
            return EXIT_FAILURE;
        }

        auto task_json = task.ToJSON();
        file << task_json;

        std::println("Current status saved to {}", settings.save_file);
    }

    std::println("{}", task.GetStatus().to_string());

    return EXIT_SUCCESS;
}

}  // namespace fw