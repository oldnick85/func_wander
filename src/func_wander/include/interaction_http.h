#pragma once

#include "third_party/cpp-httplib/httplib.h"

#include "status.h"

namespace fw
{

namespace interaction_http
{

std::string int128_to_string(__int128 value)
{
    if (value == 0)
        return "0";
    bool negative = (value < 0);
    if (negative)
        value = -value;

    std::string result;
    while (value > 0) {
        result.insert(result.begin(), static_cast<char>('0' + (value % 10)));
        value /= 10;
    }
    if (negative)
        result.insert(result.begin(), '-');
    return result;
}

/**
 * Generates an HTML page displaying the current system status.
 * The page auto-refreshes every 10 seconds and includes a progress bar,
 * a stop button, and a table of the best functions found so far.
 *
 * @param status The current status of the system.
 * @return A string containing the complete HTML5 document.
 */
std::string GenerateHTML(const status::Status& status)
{
    std::string html;
    auto it = std::back_inserter(html);

    auto format_duration = [](std::chrono::nanoseconds d) -> std::string
    {
        using namespace std::chrono;
        auto h = duration_cast<hours>(d);
        d -= h;
        auto m = duration_cast<minutes>(d);
        d -= m;
        auto s = duration_cast<seconds>(d);
        std::ostringstream oss;
        oss << h.count() << ':' << std::setw(2) << std::setfill('0') << m.count() << ':' << std::setw(2)
            << std::setfill('0') << s.count();
        return oss.str();
    };

    it = std::format_to(
        it,
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "    <meta charset=\"UTF-8\">\n"
        "    <meta http-equiv=\"refresh\" content=\"10\">\n"
        "    <title>System Status</title>\n"
        "    <style>\n"
        "        body {{ font-family: Arial, sans-serif; margin: 20px; }}\n"
        "        table {{ border-collapse: collapse; width: 100%; max-width: 800px; }}\n"
        "        th, td {{ border: 1px solid #ddd; padding: 8px; text-align: left; }}\n"
        "        th {{ background-color: #f2f2f2; }}\n"
        "        .progress {{ width: 100%; background-color: #f1f1f1; border-radius: 4px; margin: 10px 0; }}\n"
        "        .progress-bar {{ height: 20px; background-color: #4CAF50; border-radius: 4px; text-align: center; "
        "color: white; line-height: 20px; }}\n"
        "        button {{ padding: 8px 16px; font-size: 16px; cursor: pointer; }}\n"
        "    </style>\n"
        "    <script>\n"
        "        function stopProcess() {{\n"
        "            fetch('/stop', {{ method: 'POST' }})\n"
        "                .then(response => {{\n"
        "                    if (response.ok) {{\n"
        "                        alert('Process stopped');\n"
        "                    }} else {{\n"
        "                        alert('Failed to stop process');\n"
        "                    }}\n"
        "                }})\n"
        "                .catch(error => alert('Error: ' + error));\n"
        "            return false;\n"
        "        }}\n"
        "    </script>\n"
        "</head>\n"
        "<body>\n"
        "    <h1>System Status (auto-refresh every 10 seconds)</h1>\n"
        "\n"
        "     <table>\n"
        "         <tr><th>Field</th><th>Value</th></tr>\n");

    it = std::format_to(
        it,
        "         <tr><td>Current Serial Number</td><td>{}</td></tr>\n"
        "         <tr><td>Max Serial Number</td><td>{}</td></tr>\n"
        "         <tr><td>Done Percent</td><td>{:.2f}%</td></tr>\n"
        "         <tr><td>Elapsed Time</td><td>{}</td></tr>\n"
        "         <tr><td>Remaining Time</td><td>{}</td></tr>\n"
        "         <tr><td>Iterations/sec</td><td>{}</td></tr>\n"
        "         <tr><td>SN/sec</td><td>{}</td></tr>\n"
        "         <tr><td>Total Iterations</td><td>{}</td></tr>\n"
        "         <tr><td>Current Function</td><td>{}</td></tr>\n"
        "     </table>\n"
        "\n"
        "     <div class=\"progress\">\n"
        "         <div class=\"progress-bar\" style=\"width:{:.1f}%;\">{:.1f}%</div>\n"
        "     </div>\n"
        "\n"
        "     <button onclick=\"return stopProcess();\" style=\"margin: 10px 0;\">Stop Process</button>\n"
        "\n"
        "     <h2>Best Functions</h2>\n"
        "     <table>\n"
        "         <tr><th>Distance</th><th>Max Level</th><th>Functions Count</th><th>Functions Unique</th>"
        "<th>Function</th><th>Match Positions</th></tr>\n",
        int128_to_string(status.func_serial_number), int128_to_string(status.max_func_serial_number),
        status.done_percent, format_duration(status.elapsed), format_duration(status.remaining),
        status.iterations_per_sec, status.sn_per_sec, status.iterations_count, status.current_function,
        status.done_percent, status.done_percent);

    for (const auto& bf : status.best_functions) {
        it = std::format_to(it,
                            "         <tr>"
                            "<td>{}</td><td>{}</td><td>{}</td><td>{}</td><td>{}</td><td>{}</td>"
                            "</tr>\n",
                            bf.suit.distance(), bf.suit.max_level(), bf.suit.functions_count(),
                            bf.suit.functions_unique(), bf.function, bf.match_positions);
    }

    it = std::format_to(it, "     </table>\n\n");

    // Workers status section (updated)
    it = std::format_to(it, "     <h2>Workers Status</h2>\n");
    if (status.workers_status.empty()) {
        it = std::format_to(it, "     <p>No workers active.</p>\n");
    }
    else {
        it = std::format_to(
            it,
            "     <table>\n"
            "         <tr><th>Worker ID</th><th>SN Range from</th><th>SN Range to</th><th>Current SN</th>"
            "<th>Current Function</th><th>Progress</th><th>Task Count</th><th>Best Functions</th></tr>\n");
        size_t worker_id = 0;
        for (const auto& ws : status.workers_status) {
            const auto range_from_str = int128_to_string(ws.func_serial_number_from);
            const auto range_to_str = int128_to_string(ws.func_serial_number_to);
            std::string current_sn_str = int128_to_string(ws.func_serial_number);

            std::string progress_bar_html = std::format(
                "<div class=\"progress\" style=\"margin:0;\"><div class=\"progress-bar\" "
                "style=\"width:{:.1f}%;\">{:.1f}%</div></div>",
                ws.done_percent, ws.done_percent);

            size_t best_count = ws.best_functions.size();
            std::string best_functions_str = std::to_string(best_count);
            if (best_count > 0) {
                std::string tooltip;
                for (const auto& bf : ws.best_functions) {
                    if (!tooltip.empty()) {
                        tooltip += ", ";
                    }
                    tooltip += bf.function;
                }
                best_functions_str = std::format("<span title=\"{}\">{}</span>", tooltip, best_count);
            }

            it = std::format_to(it,
                                "         <tr>"
                                "<td>{}</td>"
                                "<td>{}</td>"
                                "<td>{}</td>"
                                "<td>{}</td>"
                                "<td>{}</td>"
                                "<td>{}</td>"
                                "<td>{}</td>"
                                "<td>{}</td>"
                                "</tr>\n",
                                worker_id, range_from_str, range_to_str, current_sn_str, ws.current_function,
                                progress_bar_html, ws.task_count, best_functions_str);
            ++worker_id;
        }
        it = std::format_to(it, "     </table>\n");
    }

    std::format_to(it,
                   "</body>\n"
                   "</html>\n");

    return html;
}

// Create HTTP server instance.
httplib::Server svr;
std::thread svr_thread;

/**
 * Runs an HTTP server that serves the system status page and provides an endpoint
 * to stop the background process.
 *
 * @param status     Constant reference to the current status (read‑only for the server).
 * @param stop_flag  Atomic flag that can be set to true via POST /stop to request termination.
 * @param host       Host address to bind to (e.g., "localhost").
 * @param port       Port number to listen on.
 */
void ServerThread(const status::Status& status, std::atomic<bool>& stop_flag, const std::string& host, int port)
{
    // Serve the main HTML page at the root.
    svr.Get("/", [&status](const httplib::Request& /*req*/, httplib::Response& res)
            { res.set_content(GenerateHTML(status), "text/html"); });

    // Optional JSON endpoint for AJAX requests (e.g., to fetch fresh data without reloading).
    svr.Get("/status",
            [&status](const httplib::Request& /*req*/, httplib::Response& res)
            {
                // In a real application you would use a proper JSON library.
                std::string json = "{\"snum\":\"" + int128_to_string(status.func_serial_number) +
                                   "\",\"done_percent\":" + std::to_string(status.done_percent) + "}";
                res.set_content(json, "application/json");
            });

    // Endpoint to stop the background process.
    // This is called asynchronously by the front‑end button via JavaScript fetch().
    svr.Post("/stop",
             [&stop_flag](const httplib::Request& /*req*/, httplib::Response& res)
             {
                 stop_flag.store(true);  // Signal the worker thread to stop
                 res.status = 200;       // OK
                 res.set_content("{\"result\":\"stopped\"}", "application/json");
             });

    std::println("Server starting on http://{}:{}", host, port);
    svr.listen(host, port);  // Blocks until the server is stopped
}

void Run(const status::Status& status, std::atomic<bool>& stop_flag, const std::string& host, int port)
{
    svr_thread = std::thread(ServerThread, std::ref(status), std::ref(stop_flag), host, port);
}

void Stop()
{
    svr.stop();
    if (svr_thread.joinable()) {
        svr_thread.join();
    }
}

}  // namespace interaction_http

}  // namespace fw