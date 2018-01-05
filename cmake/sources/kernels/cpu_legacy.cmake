#####################################################################
# Copyright (c) 2016 Computational Molecular Biology Group,         #
#                    Freie Universitaet Berlin (GER)                #
#                                                                   #
# This file is part of ReaDDy.                                      #
#                                                                   #
# ReaDDy is free software: you can redistribute it and/or modify    #
# it under the terms of the GNU Lesser General Public License as    #
# published by the Free Software Foundation, either version 3 of    #
# the License, or (at your option) any later version.               #
#                                                                   #
# This program is distributed in the hope that it will be useful,   #
# but WITHOUT ANY WARRANTY; without even the implied warranty of    #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the     #
# GNU Lesser General Public License for more details.               #
#                                                                   #
# You should have received a copy of the GNU Lesser General         #
# Public License along with this program. If not, see               #
# <http://www.gnu.org/licenses/>.                                   #
#####################################################################


SET(SOURCES_DIR "${READDY_GLOBAL_DIR}/kernels/cpu_legacy/src")
SET(CPU_LEGACY_INCLUDE_DIR "${READDY_GLOBAL_DIR}/kernels/cpu_legacy/include")

# --- main sources ---
LIST(APPEND CPU_LEGACY_SOURCES "${SOURCES_DIR}/CPULegacyKernel.cpp")
LIST(APPEND CPU_LEGACY_SOURCES "${SOURCES_DIR}/CPUStateModel.cpp")
LIST(APPEND CPU_LEGACY_SOURCES "${SOURCES_DIR}/data/DefaultDataContainer.cpp")
LIST(APPEND CPU_LEGACY_SOURCES "${SOURCES_DIR}/data/NLDataContainer.cpp")
LIST(APPEND CPU_LEGACY_SOURCES "${SOURCES_DIR}/observables/CPUObservableFactory.cpp")
LIST(APPEND CPU_LEGACY_SOURCES "${SOURCES_DIR}/observables/CPUObservables.cpp")

# --- neighbor list ---
LIST(APPEND CPU_LEGACY_SOURCES "${SOURCES_DIR}/nl/NeighborListIterator.cpp")
LIST(APPEND CPU_LEGACY_SOURCES "${SOURCES_DIR}/nl/CellContainer.cpp")
LIST(APPEND CPU_LEGACY_SOURCES "${SOURCES_DIR}/nl/SubCell.cpp")
LIST(APPEND CPU_LEGACY_SOURCES "${SOURCES_DIR}/nl/AdaptiveNeighborList.cpp")
LIST(APPEND CPU_LEGACY_SOURCES "${SOURCES_DIR}/nl/CellDecompositionNeighborList.cpp")
LIST(APPEND CPU_LEGACY_SOURCES "${SOURCES_DIR}/nl/CellLinkedList.cpp")
LIST(APPEND CPU_LEGACY_SOURCES "${SOURCES_DIR}/nl/CLLNeighborList.cpp")
LIST(APPEND CPU_LEGACY_SOURCES "${SOURCES_DIR}/nl/NeighborListContainer.cpp")

# --- actions ---
LIST(APPEND CPU_LEGACY_SOURCES "${SOURCES_DIR}/actions/CPUActionFactory.cpp")
LIST(APPEND CPU_LEGACY_SOURCES "${SOURCES_DIR}/actions/CPUEulerBDIntegrator.cpp")
LIST(APPEND CPU_LEGACY_SOURCES "${SOURCES_DIR}/actions/CPUEvaluateCompartments.cpp")
LIST(APPEND CPU_LEGACY_SOURCES "${SOURCES_DIR}/actions/CPUEvaluateTopologyReactions.cpp")
LIST(APPEND CPU_LEGACY_SOURCES "${SOURCES_DIR}/actions/reactions/ReactionUtils.cpp")
LIST(APPEND CPU_LEGACY_SOURCES "${SOURCES_DIR}/actions/reactions/Event.cpp")
LIST(APPEND CPU_LEGACY_SOURCES "${SOURCES_DIR}/actions/reactions/CPUUncontrolledApproximation.cpp")
LIST(APPEND CPU_LEGACY_SOURCES "${SOURCES_DIR}/actions/reactions/CPUGillespie.cpp")
#LIST(APPEND CPU_LEGACY_SOURCES "${SOURCES_DIR}/actions/reactions/CPUGillespieParallel.cpp")
#LIST(APPEND CPU_LEGACY_SOURCES "${SOURCES_DIR}/actions/reactions/NextSubvolumesReactionScheduler.cpp")
LIST(APPEND CPU_LEGACY_SOURCES "${SOURCES_DIR}/actions/topologies/CPUTopologyActionFactory.cpp")
