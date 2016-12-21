/********************************************************************
 * Copyright © 2016 Computational Molecular Biology Group,          *
 *                  Freie Universität Berlin (GER)                  *
 *                                                                  *
 * This file is part of ReaDDy.                                     *
 *                                                                  *
 * ReaDDy is free software: you can redistribute it and/or modify   *
 * it under the terms of the GNU Lesser General Public License as   *
 * published by the Free Software Foundation, either version 3 of   *
 * the License, or (at your option) any later version.              *
 *                                                                  *
 * This program is distributed in the hope that it will be useful,  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
 * GNU Lesser General Public License for more details.              *
 *                                                                  *
 * You should have received a copy of the GNU Lesser General        *
 * Public License along with this program. If not, see              *
 * <http://www.gnu.org/licenses/>.                                  *
 ********************************************************************/


/**
 * Header file containing a console() logger which gives the spd console logger.
 *
 * @file logging.h
 * @brief Header responsible for the logging system.
 * @author clonker
 * @date 14.10.16
 */

#ifndef READDY_MAIN_LOGGING_H
#define READDY_MAIN_LOGGING_H

#include <spdlog/spdlog.h>

namespace readdy {
namespace log {
inline std::shared_ptr<spdlog::logger> console() {
    if(!spdlog::get("console")) {
        spdlog::set_sync_mode();
        auto console = spdlog::stdout_color_mt("console");
        console->set_pattern("[          ] [%Y-%m-%d %H:%M:%S] [%t] [%l] %v");
    }
    return spdlog::get("console");
}
}
}
#endif //READDY_MAIN_LOGGING_H