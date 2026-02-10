/*
 *  c-gomoku-cli, a command line interface for Gomocup engines. Copyright 2021
 * Chao Ma. c-gomoku-cli is derived from c-chess-cli, originally authored by
 * lucasart 2020.
 *
 *  c-gomoku-cli is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 *  c-gomoku-cli is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 *  You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "options.h"
#include "options_cli.h"
#include "TournamentManager.h"
#include "util.h"

#include <csignal>
#include <cstdlib>
#include <iostream>

static TournamentManager *manager = nullptr;

static void signal_handler([[maybe_unused]] int signal)
{
    if (manager) {
        printf("Stopping tournament...\n");
        manager->stop();
    }
    _Exit(EXIT_SUCCESS);
}

static void main_destroy(void)
{
    if (manager) {
        delete manager;
        manager = nullptr;
    }
}

int main(int argc, const char **argv)
{
    signal(SIGINT, signal_handler);
    atexit(main_destroy);

    Options options;
    std::vector<EngineOptions> eo;
    options_parse(argc, argv, options, eo);

    manager = new TournamentManager();
    manager->init(options, eo);
    manager->start();

    // Main thread loop: check deadline overdue at regular intervals
    bool running = true;
    do {
        system_sleep(100);
        running = manager->update();
    } while (running);

    return 0;
}
