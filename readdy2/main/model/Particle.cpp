#include <readdy/model/Particle.h>
#include <boost/uuid/uuid_generators.hpp>

/**
 * << detailed description >>
 *
 * @file Particle.cpp
 * @brief << brief description >>
 * @author clonker
 * @date 19.04.16
 */

readdy::model::Particle::Particle() : id(boost::uuids::random_generator()()) {

}

bool readdy::model::Particle::operator==(const readdy::model::Particle &rhs) {
    return rhs.id == id;
}

bool readdy::model::Particle::operator!=(const readdy::model::Particle &rhs) {
    return !(*this == rhs);
}

readdy::model::Particle::Particle(double x, double y, double z, std::string type) : readdy::model::Particle(){
    pos[0] = x;
    pos[1] = y;
    pos[2] = z;
    this->type = type;
}


readdy::model::Particle::~Particle() = default;
