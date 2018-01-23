# coding=utf-8

# Copyright © 2016 Computational Molecular Biology Group,
#                  Freie Universität Berlin (GER)
#
# This file is part of ReaDDy.
#
# ReaDDy is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General
# Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.

"""
Created on 08.09.17

@author: clonker
"""

import numpy as _np

from typing import Optional as _Optional, Dict as _Dict, Union as _Union
from readdy.util.observable_utils import calculate_pressure as _calculate_pressure

def _parse_save_args(save_args):
    assert save_args is None or isinstance(save_args, (dict, bool)), \
        "save can only be None, bool, or a dictionary, not {}".format(type(save_args))
    if save_args is None or (isinstance(save_args, bool) and not save_args):
        return None, None
    else:
        assert "chunk_size" in save_args.keys(), "save needs to have a \"chunk_size\" key"
        assert "name" in save_args.keys(), "save needs to have a \"name\" key"
        return save_args["name"], save_args["chunk_size"]


class Observables(object):
    def __init__(self, simulation):
        self._simulation = simulation
        self._sim = self._simulation._simulation
        self._observable_handles = []

    def rdf(self, stride, bin_borders, types_count_from, types_count_to, particle_to_density, callback=None,
            save: _Optional[_Union[_Dict, str]]='default'):
        """
        Calculates and possibly records the radial distribution function.

        :param stride: skip `stride` time steps before evaluating the observable again
        :param bin_borders: the bin borders
        :param types_count_from: type(s) to calculate the distances "from", can be string or list of strings
        :param types_count_to: types to calculate the distances "to", can be string or list of strings
        :param particle_to_density: density of "to" particles, scaling factor in the resulting distribution
        :param callback: callback function that has as argument a tuple with two elements, containing lists of
                         scalars each
        :param save: dictionary containing `name` and `chunk_size` or None to not save the observable to file
        """
        if isinstance(save, str) and save == 'default':
            save = {"name": "rdf", "chunk_size": 1000}
        if isinstance(types_count_from, str):
            types_count_from = [types_count_from]
        if isinstance(types_count_to, str):
            types_count_to = [types_count_to]
        bin_borders = self._simulation._unit_conf.convert(bin_borders, self._simulation.length_unit)
        particle_to_density = self._simulation._unit_conf.convert(particle_to_density,
                                                                  1 / self._simulation.length_unit ** 3)
        assert all([isinstance(x, str) for x in types_count_from]), \
            "types_count_from={} has an invalid type".format(types_count_from)
        assert all([isinstance(x, str) for x in types_count_to]), \
            "types_count_to={} has an invalid type".format(types_count_to)
        handle = self._sim.register_observable_radial_distribution(stride, bin_borders, types_count_from,
                                                                   types_count_to,
                                                                   particle_to_density, callback)
        self._add_observable_handle(*_parse_save_args(save), handle)

    def reactions(self, stride, callback=None, save: _Optional[_Union[_Dict, str]]='default'):
        """
        Records all reactions.

        :param stride: skip `stride` time steps before evaluating the observable again
        :param callback: callback function that has as argument a list of reaction records
        :param save: dictionary containing `name` and `chunk_size` or None to not save the observable to file
        """
        if isinstance(save, str) and save == 'default':
            save = {"name": "reactions", "chunk_size": 500}
        handle = self._sim.register_observable_reactions(stride, callback)

        self._add_observable_handle(*_parse_save_args(save), handle)

    def particle_positions(self, stride, types=None, callback=None, save: _Optional[_Union[_Dict, str]]='default'):
        """
        Records particle positions of either all particles or a selection of particle types.

        :param stride: skip `stride` time steps before evaluating the observable again
        :param types: types for which to observe the particle positions, can be None for all types
        :param callback: callback function that has as argument a list of 3d vectors
        :param save: dictionary containing `name` and `chunk_size` or None to not save the observable to file
        """
        if isinstance(save, str) and save == 'default':
            save = {"name": "particle_positions", "chunk_size": 100}

        if types is None:
            types = []
        handle = self._sim.register_observable_particle_positions(stride, types, callback)

        self._add_observable_handle(*_parse_save_args(save), handle)

    def particles(self, stride, callback=None, save: _Optional[_Union[_Dict, str]]='default'):
        """
        Records all particles in the system.

        :param stride: skip `stride` time steps before evaluating the observable again
        :param callback: callback function that has as argument a list of particle objects
        :param save: dictionary containing `name` and `chunk_size` or None to not save the observable to file
        """
        if isinstance(save, str) and save == 'default':
            save = {"name": "particles", "chunk_size": 100}

        handle = self._sim.register_observable_particles(stride, callback)

        self._add_observable_handle(*_parse_save_args(save), handle)

    def number_of_particles(self, stride, types=None, callback=None,
                            save: _Optional[_Union[_Dict, str]]='default'):
        """
        Records the number of particles in the system.

        :param stride: skip `stride` time steps before evaluating the observable again
        :param types: types for which to observe the number of particles, can be None for all types
        :param callback: callback function that has as argument a list of particle numbers
        :param save: dictionary containing `name` and `chunk_size` or None to not save the observable to file
        """
        if isinstance(save, str) and save == 'default':
            save = {"name": "n_particles", "chunk_size": 100}

        if types is None:
            types = []
        handle = self._sim.register_observable_n_particles(stride, types, callback)

        self._add_observable_handle(*_parse_save_args(save), handle)

    def energy(self, stride, callback=None, save: _Optional[_Union[_Dict, str]]='default'):
        """
        Records the potential energy of the system.

        :param stride: skip `stride` time steps before evaluating the observable again
        :param callback: callback function that has as argument a scalar value representing the potential energy
        :param save: dictionary containing `name` and `chunk_size` or None to not save the observable to file
        """
        if isinstance(save, str) and save == 'default':
            save = {"name": "energy", "chunk_size": 10000}

        handle = self._sim.register_observable_energy(stride, callback)

        self._add_observable_handle(*_parse_save_args(save), handle)

    def forces(self, stride, types=None, callback=None, save: _Optional[_Union[_Dict, str]]='default'):
        """
        Records the forces acting on particles.

        :param stride: skip `stride` time steps before evaluating the observable again
        :param types: types for which to observe the forces on particles, can be None for all types
        :param callback: callback function that has as argument a list of vectors
        :param save: dictionary containing `name` and `chunk_size` or None to not save the observable to file
        """
        if isinstance(save, str) and save == 'default':
            save = {"name": "forces", "chunk_size": 100}

        if types is None:
            types = []
        handle = self._sim.register_observable_forces(stride, types, callback)

        self._add_observable_handle(*_parse_save_args(save), handle)

    def reaction_counts(self, stride, callback=None, save: _Optional[_Union[_Dict, str]]='default'):
        """
        Records the occurred number of reactions per registered reaction.

        :param stride: skip `stride` time steps before evaluating the observable again
        :param callback: callback function that has as argument a tuple containing two maps, one for reactions with
                         one educt and one for reactions with two educts
        :param save: dictionary containing `name` and `chunk_size` or None to not save the observable to file
        """
        if isinstance(save, str) and save == 'default':
            save = {"name": "reaction_counts", "chunk_size": 500}

        handle = self._sim.register_observable_reaction_counts(stride, callback)

        self._add_observable_handle(*_parse_save_args(save), handle)

    def virial(self, stride, callback=None, save: _Optional[_Union[_Dict, str]]='default'):
        """
        Records the virial tensor of the system.
        :param stride: skip `stride` time steps before evaluating the observable again
        :param callback: callback function that takes the current virial as argument in terms of a (3,3) numpy array
        :param save: dictionary containing `name` and `chunk_size` or None to not save the observable to file
        """
        if isinstance(save, str) and save == 'default':
            save = {"name": "virial", "chunk_size": 100}

        handle = self._sim.register_observable_virial(stride, lambda x: callback(_np.ndarray((3,3), buffer=x)))
        self._add_observable_handle(*_parse_save_args(save), handle)

    def pressure(self, stride, physical_particles=None, callback=None, save: _Optional[_Union[_Dict, str]]='default'):
        """
        This observable will report back the pressure of the system. As the pressure can be computed from the number of
        physical particles and the virial tensor, this observable actually delegates to the n_particles and virial
        observables. The default save behavior is to save virial and n_particles under
        "virial_pressure" and "n_particles_pressure", respectively.
        :param stride: skip `stride` time steps before evaluating the observable again
        :param physical_particles: a list of physical particles, if None, all particles are considered physical
        :param callback: callback function that takes the current pressure as argument
        :param save: dictionary containing `name` and `chunk_size` or None to not save the observable to file
        """
        if isinstance(save, str) and save == 'default':
            save = {"name": "_pressure", "chunk_size": 500}
        save_name, save_chunk_size = _parse_save_args(save)
        save_n_particles = None
        save_virial = None
        if save_name is not None and save_chunk_size is not None:
            save_n_particles = {"name": "n_particles{}".format(save_name), "chunk_size": save_chunk_size}
            save_virial = {"name": "virial{}".format(save_name), "chunk_size": save_chunk_size}

        class PressureCallback(object):

            def __init__(self, user_callback, kbt, volume):

                self._user_callback = user_callback
                self._n = None
                self._v = None
                self._kbt = kbt
                self._volume = volume

                def pressure_callback_n_particles(n):
                    self._n = n
                    self._eval_user_callback()

                def pressure_callback_virial(virial):
                    self._v = virial
                    self._eval_user_callback()

                self.n_particles_callback = pressure_callback_n_particles if user_callback is not None else None
                self.virial_callback = pressure_callback_virial if user_callback is not None else None

            def _eval_user_callback(self):
                if self._n is not None and self._v is not None:
                    self._user_callback(_calculate_pressure(box_volume=self._volume, kbt=self._kbt,
                                                            n_particles=self._n, virial=self._v))
                    self._n = None
                    self._v = None

        pressure_callback = PressureCallback(callback, self._sim.context.kbt, self._sim.context.box_volume())
        self.number_of_particles(stride, types=physical_particles, callback=pressure_callback.n_particles_callback,
                                 save=save_n_particles)
        self.virial(stride, callback=pressure_callback.virial_callback, save=save_virial)

    def _add_observable_handle(self, save_name, save_chunk_size, handle):
        if save_name is not None and save_chunk_size is not None:
            if next((n for n, c, h in self._observable_handles if n == save_name), None) is not None:
                raise RuntimeError("A observable with the name {} is already being recorded into the trajectory file."
                                   .format(save_name))
            self._observable_handles.append((save_name, save_chunk_size, handle))
