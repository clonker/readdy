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
 * << detailed description >>
 *
 * @file logging.cpp
 * @brief << brief description >>
 * @author chrisfrö
 * @author clonker
 * @date 27.03.17
 * @copyright GNU Lesser General Public License v3.0
 */

#include <readdy/common/logging.h>
#include <iostream>

namespace readdy {
namespace log {

#ifdef READDY_DEBUG
std::shared_ptr<spdlog::logger> console() {
    static auto logger = spdlog::get("console");
    if (!logger) {
        spdlog::set_sync_mode();
        logger = spdlog::stdout_color_mt("console");
        logger->set_pattern("[          ] [%Y-%m-%d %H:%M:%S] [%t] [%l] %v");
    }
    return logger;
}
#endif

}
}
