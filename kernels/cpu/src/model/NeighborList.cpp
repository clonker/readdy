/**
 * << detailed description >>
 *
 * @file NeighborList.cpp
 * @brief << brief description >>
 * @author clonker
 * @date 08.09.16
 */

#include <set>
#include <sstream>
#include <future>

#include <readdy/common/numeric.h>
#include <readdy/common/thread/scoped_thread.h>
#include <readdy/common/thread/barrier.h>
#include <readdy/common/thread/semaphore.h>
#include <readdy/common/Timer.h>

#include <readdy/kernel/cpu/model/NeighborList.h>
#include <readdy/kernel/cpu/util/hilbert.h>
#include <readdy/common/thread/notification_barrier.h>

namespace readdy {
namespace kernel {
namespace cpu {
namespace model {

namespace thd = readdy::util::thread;


template<class OutputIterator, class Size, class Assignable>
void iota_n(OutputIterator first, Size n, Assignable value) {
    std::generate_n(first, n, [&value]() {
        return value++;
    });
}

struct NeighborList::Cell {
    std::vector<Cell *> neighbors{};
    double maximal_displacements[2];
    cell_index contiguous_index;
    bool enoughCells;


    // dirty flag indicating whether the cell and its neighboring cells have to be re-created
    bool dirty{false};

    Cell(cell_index i, cell_index j, cell_index k, const std::array<cell_index, 3> &nCells);

    void addNeighbor(Cell *cell);

    void checkDirty(skin_size_t skin);

    friend bool operator==(const Cell &lhs, const Cell &rhs);

    friend bool operator!=(const Cell &lhs, const Cell &rhs);

    void addParticleIndex(const particle_index idx) {
        particleIndices.push_back(idx);
    }

    const std::vector<particle_index> &particles() const {
        return particleIndices;
    }

    std::vector<particle_index> &particles() {
        return particleIndices;
    }

private:
    std::vector<particle_index> particleIndices{};
};

template<typename T, typename Dims>
T get_contiguous_index(T i, T j, T k, Dims I, Dims J) {
    return k + j * J + i * I * J;
};

class ParticleTravelledTooFarException : public std::runtime_error {
public:
    ParticleTravelledTooFarException() : runtime_error("") {}
};

const static std::vector<NeighborList::neighbor_t> no_neighbors{};

NeighborList::NeighborList(const ctx_t *const context, data_t &data, readdy::util::thread::Config const *const config,
                           skin_size_t skin)
        : ctx(context), config(config), cells(std::vector<Cell>()),
          simBoxSize(ctx->getBoxSize()), skin_size(skin), data(data), groupParticlesOnCreation(true) {}

void NeighborList::setupCells() {
    if (cells.empty()) {
        double maxCutoff = 0;
        for (auto &&e : ctx->getAllOrder2RegisteredPotentialTypes()) {
            for (auto &&p : ctx->getOrder2Potentials(std::get<0>(e), std::get<1>(e))) {
                maxCutoff = maxCutoff < p->getCutoffRadius() ? p->getCutoffRadius() : maxCutoff;
            }
        }
        for (auto &&e : ctx->getAllOrder2Reactions()) {
            maxCutoff = maxCutoff < e->getEductDistance() ? e->getEductDistance() : maxCutoff;
        }
        NeighborList::maxCutoff = maxCutoff;
        NeighborList::maxCutoffPlusSkin = maxCutoff + skin_size;

        log::console()->debug("got maxCutoff={}, skin={} => r_c + r_s = {}", maxCutoff, skin_size,
                              maxCutoff + skin_size);

        if (maxCutoff > 0) {
            const auto desiredCellWidth = .5 * maxCutoffPlusSkin;

            for (unsigned short i = 0; i < 3; ++i) {
                nCells[i] = static_cast<cell_index>(floor(simBoxSize[i] / desiredCellWidth));
                if (nCells[i] == 0) nCells[i] = 1;
                cellSize[i] = simBoxSize[i] / static_cast<double>(nCells[i]);
            }
            log::console()->debug("resulting cell size = {}", cellSize);

            for (cell_index i = 0; i < nCells[0]; ++i) {
                for (cell_index j = 0; j < nCells[1]; ++j) {
                    for (cell_index k = 0; k < nCells[2]; ++k) {
                        cells.push_back({i, j, k, nCells});
                    }
                }
            }

            for (cell_index i = 0; i < nCells[0]; ++i) {
                for (cell_index j = 0; j < nCells[1]; ++j) {
                    for (cell_index k = 0; k < nCells[2]; ++k) {
                        setupNeighboringCells(i, j, k);
                    }
                }
            }
        } else {
            nCells = {{1, 1, 1}};
            cells.push_back({0, 0, 0, nCells});
            setupNeighboringCells(0, 0, 0);
        }
    }
}

void
NeighborList::setupNeighboringCells(const signed_cell_index i, const signed_cell_index j, const signed_cell_index k) {
    auto me = getCell(i, j, k);
    for (signed_cell_index _i = -2; _i < 3; ++_i) {
        for (signed_cell_index _j = -2; _j < 3; ++_j) {
            for (signed_cell_index _k = -2; _k < 3; ++_k) {
                // don't add me as neighbor to myself
                if (!(_i == 0 && _j == 0 && _k == 0)) {
                    me->addNeighbor(getCell(i + _i, j + _j, k + _k));
                }
            }
        }
    }
}

void NeighborList::clear() {
    cells.clear();
    for (auto &list : data.neighbors) {
        list.clear();
    }
    initialSetup = true;
}

/**
 * Potentially marks a cell dirty. If there is a particle that has travelled too far, it will return false, otherwise
 * true. Travelling too far means:
 *  - with boxes of width .5(r_c + r_s): displacement > r_c + r_s (this is our case)
 *  - with boxes of width r_c + r_s: displacement > 2r_c + 2r_s
 *
 * @param cell the cell
 * @param data the underlying data
 * @param skin the skin width
 * @return true if everything is ok, false if a particle travelled too far
 */
bool markCell(NeighborList::Cell &cell, const NeighborList::data_t &data, const double r_c, const double r_s) {
    bool travelledTooFar = false;
    cell.maximal_displacements[0] = 0;
    cell.maximal_displacements[1] = 0;

    for (const auto idx : cell.particles()) {
        const auto &entry = data.entry_at(idx);
        if (!entry.is_deactivated()) {
            const double disp = entry.displacement;
            travelledTooFar = disp > r_c + r_s;
            if (travelledTooFar) break;
            if (disp > cell.maximal_displacements[0]) {
                cell.maximal_displacements[0] = disp;
            } else if (disp > cell.maximal_displacements[1]) {
                cell.maximal_displacements[1] = disp;
            }
        }
    }
    cell.checkDirty(r_s);
    return travelledTooFar;
}

bool cellOrNeighborDirty(const NeighborList::Cell &cell) {
    if (cell.dirty) return true;
    for (const auto n : cell.neighbors) {
        if (n->dirty) return true;
    }
    return false;
}

/**
 * Groups portion of particles together
 * @param begin the begin of the portion
 * @param end end of the portion (exclusive)
 * @param thread_number my number
 * @param n_buckets number of cells in this case
 * @param that the neighbor list
 * @param data_entries entries vector of the underlying data structure
 * @param allBucketSizes the bucket sizes of all threads, available after waiting for aggregation
 * @param wait_for_aggregation notification barrier that waits until main thread has collected all bucket sizes
 * @param bucketSizesPromise future promise of this thread's bucket sizes
 */
void groupParticles(NeighborList::data_iter_t begin, NeighborList::data_iter_t end, std::size_t thread_number,
                    std::size_t n_buckets, const NeighborList &that, NeighborList::data_t::entries_t &data_entries,
                    const std::vector<std::vector<std::size_t>> &allBucketSizes,
                    const thd::notification_barrier &wait_for_aggregation,
                    std::promise<std::pair<std::size_t, std::vector<std::size_t>>> bucketSizesPromise) {
    std::vector<NeighborList::data_t::Entry> grouped(std::make_move_iterator(begin), std::make_move_iterator(end));
    {
        std::vector<std::size_t> bucketSizes;
        bucketSizes.resize(n_buckets);
        auto partition_begin = grouped.begin();
        for (unsigned int bucket = 0; bucket < n_buckets; ++bucket) {
            auto next = std::partition(partition_begin, grouped.end(),
                                       [&that, bucket](const NeighborList::data_t::Entry &e) {
                                           return !e.is_deactivated() && that.hash_pos(e.position()) == bucket;
                                       });
            const auto groupSize = static_cast<std::size_t>(std::distance(partition_begin, next));
            bucketSizes.at(bucket) = groupSize;
            partition_begin = next;
        }
        // main thread is waiting for this
        bucketSizesPromise.set_value(std::make_pair(thread_number, std::move(bucketSizes)));
    }
    // now wait until main thread reports back that everything is aggregated
    {
        wait_for_aggregation.wait();
    }
    {
        // the offsets are now in allBucketSizes, move own data into respective buckets (contained in
        // allBucketSizes[thread_number]
        std::size_t offset = 0;
        std::size_t local_offset = 0;
        for (std::size_t bucket = 0; bucket < n_buckets; ++bucket) {
            for(std::size_t i = 0; i < thread_number; ++i) {
                offset += allBucketSizes[i][bucket];
            }
            const auto my_size = allBucketSizes[thread_number][bucket];
            if (my_size > 0) {
                log::console()->debug("inserting particles {} - {} in thread {}", local_offset, local_offset + my_size, thread_number);
                auto mBegin = std::make_move_iterator(grouped.begin() + local_offset);
                auto mEnd = std::make_move_iterator(grouped.begin() + local_offset + my_size);

                data_entries.insert(data_entries.begin() + offset, mBegin, mEnd);
                local_offset += my_size;
                offset += my_size;
            }
            for(std::size_t i = thread_number+1; i < allBucketSizes.size(); ++i) {
                offset += allBucketSizes[i][bucket];
            }
        }
    }
}

void NeighborList::fillCells() {
    if (maxCutoff > 0) {
        const auto c2 = maxCutoffPlusSkin * maxCutoffPlusSkin;
        auto d2 = ctx->getDistSquaredFun();

        if (initialSetup) {

            for (auto &cell : cells) {
                cell.particles().clear();
            }
            data.neighbors.clear();
            data.neighbors.resize(data.size());

            if (groupParticlesOnCreation) {
                const auto grainSize = data.size() / config->nThreads();
                std::vector<std::vector<std::size_t>> allBucketSizes;
                allBucketSizes.resize(config->nThreads());
                {
                    using future_content = std::pair<std::size_t, std::vector<std::size_t>>;
                    thd::notification_barrier notify;
                    std::vector<std::future<future_content>> futures;
                    futures.reserve(config->nThreads());
                    auto it = data.begin();
                    std::vector<thd::scoped_thread> threads;
                    threads.reserve(config->nThreads());
                    for (std::size_t i = 0; i < config->nThreads() - 1; ++i) {
                        std::promise<future_content> promise;
                        futures.push_back(promise.get_future());
                        threads.push_back(thd::scoped_thread(std::thread(
                                groupParticles, it, it + grainSize, i, cells.size(), std::cref(*this),
                                std::ref(data.entries), std::cref(allBucketSizes), std::cref(notify), std::move(promise)
                        )));
                        it += grainSize;
                    }
                    {
                        std::promise<future_content> promise;
                        futures.push_back(promise.get_future());
                        threads.push_back(thd::scoped_thread(std::thread(
                                groupParticles, it, data.end(), config->nThreads() - 1, cells.size(), std::cref(*this),
                                std::ref(data.entries), std::cref(allBucketSizes), std::cref(notify), std::move(promise)
                        )));
                    }

                    std::vector<std::size_t> cellSizes;
                    cellSizes.resize(cells.size());

                    for (auto &future : futures) {
                        future.wait();
                        auto content = std::move(future.get());
                        const auto t_id = std::get<0>(content);
                        allBucketSizes[t_id] = std::move(std::get<1>(content));

                        auto cellIt = cellSizes.begin();
                        for (const auto cellSize : allBucketSizes.at(t_id)) {
                            *cellIt += cellSize;
                            ++cellIt;
                        }
                    }

                    notify.ready();

                    {
                        auto cellIt = cells.begin();
                        auto cellSizeIt = cellSizes.begin();
                        std::size_t offset = 0;
                        for (; cellIt != cells.end(); ++cellIt, ++cellSizeIt) {
                            const auto cellSize = *cellSizeIt;
                            if (cellSize > 0) {
                                std::vector<data_t::index_t> indices;
                                indices.reserve(cellSize);
                                iota_n(std::back_inserter(indices), cellSize, offset);
                                cellIt->particles() = std::move(indices);
                                offset += cellSize;
                            }
                        }
                    }

                }

                data.blanks_moved_to_end();
                {
                    // sanity: all particles are in the correct boxes
                    for(const auto& cell : cells) {
                        for(const auto idx : cell.particles()) {
                            auto refCell = getCell(data.entry_at(idx).pos);
                            if(refCell && refCell->contiguous_index == cell.contiguous_index) {
                                // allgood
                            } else {
                                log::console()->critical("shit");
                            }
                        }
                    }
                }
            } else {
                data_t::index_t idx = 0;
                for (const auto &entry : data) {
                    if (!entry.is_deactivated()) {
                        auto cell = getCell(entry.pos);
                        if (cell) {
                            cell->addParticleIndex(idx);
                        }
                    }
                    ++idx;
                }
            }

            using cell_it_t = decltype(cells.begin());
            const auto size = cells.size();
            const std::size_t grainSize = size / config->nThreads();

            auto worker = [this, c2, d2](cell_it_t cellsBegin, cell_it_t cellsEnd) -> void {
                for (cell_it_t _b = cellsBegin; _b != cellsEnd; ++_b) {
                    auto &cell = *_b;
                    setUpCell(cell, c2, d2);
                }
            };

            std::vector<thd::scoped_thread> threads;
            threads.reserve(config->nThreads());

            auto it_cells = cells.begin();
            for (int i = 0; i < config->nThreads() - 1; ++i) {
                threads.push_back(thd::scoped_thread(std::thread(worker, it_cells, it_cells + grainSize)));
                it_cells += grainSize;
            }
            threads.push_back(thd::scoped_thread(std::thread(worker, it_cells, cells.end())));
        } else {
            auto dirtyCells = findDirtyCells();

            const double fraction = 8.;
            if (dirtyCells.size() > cells.size() * fraction) {
                initialSetup = true;
                log::console()->debug("had more than {}% dirty cells, recreate neighbor list", fraction * 100);
                setupCells();
                fillCells();
                initialSetup = false;
                return;
            } else {

                const std::size_t grainSize = dirtyCells.size() / config->nThreads();

                using cell_it_t = decltype(dirtyCells.begin());

                auto worker = [this, c2, d2, grainSize](cell_it_t begin, cell_it_t end, const thd::barrier &barrier) {
                    // first gather updates
                    std::vector<std::vector<NeighborList::particle_index>> cellUpdates;
                    cellUpdates.reserve(grainSize);
                    {
                        for (cell_it_t _b = begin; _b != end; ++_b) {
                            auto cell = *_b;
                            cellUpdates.push_back({});
                            auto &updatedIndices = cellUpdates.back();
                            updatedIndices.reserve(cell->particles().size());
                            for (const auto idx : cell->particles()) {
                                const auto &entry = data.entry_at(idx);
                                if (!entry.is_deactivated() &&
                                    cell->contiguous_index == getCellIndex(entry.position())) {
                                    updatedIndices.push_back(idx);
                                }
                            }
                            for (const auto neigh : cell->neighbors) {
                                if (cell->dirty || neigh->dirty) {
                                    for (const auto idx : neigh->particles()) {
                                        const auto &entry = data.entry_at(idx);
                                        if (!entry.is_deactivated()
                                            && cell->contiguous_index == getCellIndex(entry.position())) {
                                            updatedIndices.push_back(idx);
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // wait until all threads have their updates gathered
                    barrier.wait();
                    // apply updates
                    {
                        auto it = begin;
                        for (auto &&update : cellUpdates) {
                            auto cell = *it;
                            cell->particles() = std::move(update);
                            ++it;
                        }
                    }
                    // wait until all threads have applied their updates
                    barrier.wait();
                    // update neighbor list in respective cells
                    {
                        for (cell_it_t _b = begin; _b != end; ++_b) {
                            auto cell = *_b;
                            setUpCell(*cell, c2, d2);
                        }
                    }
                };

                // readdy::util::Timer t("        update dirty cells");
                thd::barrier b(config->nThreads());

                std::vector<thd::scoped_thread> threads;
                threads.reserve(config->nThreads());

                // log::console()->warn("got dirty cells {} vs total cells {}", dirtyCells.size(), cells.size());

                {
                    auto it_cells = dirtyCells.begin();
                    for (int i = 0; i < config->nThreads() - 1; ++i) {
                        auto advanced = std::next(it_cells, grainSize);
                        threads.push_back(thd::scoped_thread(std::thread(worker, it_cells, advanced, std::cref(b))));
                        it_cells = advanced;
                    }
                    threads.push_back(
                            thd::scoped_thread(std::thread(worker, it_cells, dirtyCells.end(), std::cref(b))));
                }
            }
            {
                // auto dirtyCells2 = findDirtyCells();
                // log::console()->warn("dirty cells after: {}", dirtyCells2.size());
            }
        }
    }
}

void NeighborList::create() {
    simBoxSize = ctx->getBoxSize();
    data.setFixPosFun(ctx->getFixPositionFun());
    {
        setupCells();
    }
    bool redoFillCells = false;
    try {
        fillCells();
    } catch (const ParticleTravelledTooFarException &) {
        initialSetup = true;
        redoFillCells = true;
        log::console()->warn("A particle's displacement has been more than r_c + r_s = {} + {} = {}, which means that "
                                     "it might have left its cell linked-list cell. This should, if at all, only happen "
                                     "very rarely and triggers a complete rebuild of the neighbor list.",
                             maxCutoff, skin_size, maxCutoffPlusSkin);
    }
    if (redoFillCells) fillCells();
    initialSetup = false;
}

NeighborList::Cell *NeighborList::getCell(signed_cell_index i, signed_cell_index j, signed_cell_index k) {
    const auto &periodic = ctx->getPeriodicBoundary();
    if (periodic[0]) i = readdy::util::numeric::positive_modulo(i, nCells[0]);
    else if (i < 0 || i >= nCells[0]) return nullptr;
    if (periodic[1]) j = readdy::util::numeric::positive_modulo(j, nCells[1]);
    else if (j < 0 || j >= nCells[1]) return nullptr;
    if (periodic[2]) k = readdy::util::numeric::positive_modulo(k, nCells[2]);
    else if (k < 0 || k >= nCells[2]) return nullptr;
    const auto cix = get_contiguous_index(i, j, k, nCells[1], nCells[2]);
    if (cix < cells.size()) {
        return &cells.at(static_cast<cell_index>(cix));
    } else {
        log::console()->critical("NeighborList::getCell(nonconst): Requested cell ({},{},{})={}, but there are "
                                         "only {} cells.", i, j, k, cix, cells.size());
        throw std::runtime_error("tried to get cell index that was too large");
    }
}

void NeighborList::remove(const particle_index idx) {
    auto cell = getCell(data.pos(idx));
    if (cell != nullptr) {
        auto remove_predicate = [idx](const ParticleData::Neighbor n) {
            return n == idx;
        };
        try {
            for (auto &neighbor : neighbors(idx)) {
                auto &neighbors_2nd = data.neighbors.at(neighbor);
                auto it = std::find_if(neighbors_2nd.begin(), neighbors_2nd.end(), remove_predicate);
                if (it != neighbors_2nd.end()) {
                    neighbors_2nd.erase(it);
                }
            }
            auto &particles = cell->particles();
            auto find_it = std::find(particles.begin(), particles.end(), idx);
            if (find_it != particles.end()) {
                particles.erase(find_it);
            }
        } catch (const std::out_of_range &) {
            log::console()->error("tried to remove particle with id {} but it was not in the neighbor list", idx);
        }
    }
}

void NeighborList::insert(const particle_index idx) {
    const auto &d2 = ctx->getDistSquaredFun();
    const auto &pos = data.pos(idx);
    const auto cutoffSquared = maxCutoffPlusSkin * maxCutoffPlusSkin;
    auto cell = getCell(pos);
    if (cell) {
        cell->addParticleIndex(idx);
        auto &myNeighbors = data.neighbors.at(idx);
        for (const auto pJ : cell->particles()) {
            if (idx != pJ) {
                const auto distSquared = d2(pos, data.pos(pJ));
                if (distSquared < cutoffSquared) {
                    myNeighbors.push_back(pJ);
                    data.neighbors.at(pJ).push_back(idx);
                }
            }
        }
        for (auto &neighboringCell : cell->neighbors) {
            for (const auto &pJ : neighboringCell->particles()) {
                const auto distSquared = d2(pos, data.pos(pJ));
                if (distSquared < cutoffSquared) {
                    myNeighbors.push_back(pJ);
                    data.neighbors.at(pJ).push_back(idx);
                }
            }
        }
    } else {
        //log::console()->error("could not assign particle (index={}) to any cell!", data.getEntryIndex(idx));
    }
}

NeighborList::Cell *NeighborList::getCell(const readdy::model::Particle::pos_type &pos) {
    const cell_index i = static_cast<const cell_index>(floor((pos[0] + .5 * simBoxSize[0]) / cellSize[0]));
    const cell_index j = static_cast<const cell_index>(floor((pos[1] + .5 * simBoxSize[1]) / cellSize[1]));
    const cell_index k = static_cast<const cell_index>(floor((pos[2] + .5 * simBoxSize[2]) / cellSize[2]));
    return getCell(i, j, k);
}

void NeighborList::updateData(ParticleData::update_t &&update) {
    if (maxCutoff > 0) {
        for (const auto &p : std::get<1>(update)) {
            remove(p);
        }
    }
    auto newEntries = data.update(std::move(update));
    if (maxCutoff > 0) {
        for (const auto p : newEntries) {
            insert(p);
        }
    }
}

const std::vector<NeighborList::neighbor_t> &NeighborList::neighbors(NeighborList::particle_index const entry) const {
    if (maxCutoff > 0) {
        return data.neighbors.at(entry);
    }
    return no_neighbors;
}

const NeighborList::Cell *const NeighborList::getCell(const readdy::model::Particle::pos_type &pos) const {
    const cell_index i = static_cast<const cell_index>(floor((pos[0] + .5 * simBoxSize[0]) / cellSize[0]));
    const cell_index j = static_cast<const cell_index>(floor((pos[1] + .5 * simBoxSize[1]) / cellSize[1]));
    const cell_index k = static_cast<const cell_index>(floor((pos[2] + .5 * simBoxSize[2]) / cellSize[2]));
    return getCell(i, j, k);
}

const NeighborList::Cell *const NeighborList::getCell(NeighborList::signed_cell_index i,
                                                      NeighborList::signed_cell_index j,
                                                      NeighborList::signed_cell_index k) const {

    const auto &periodic = ctx->getPeriodicBoundary();
    if (periodic[0]) i = readdy::util::numeric::positive_modulo(i, nCells[0]);
    else if (i < 0 || i >= nCells[0]) return nullptr;
    if (periodic[1]) j = readdy::util::numeric::positive_modulo(j, nCells[1]);
    else if (j < 0 || j >= nCells[1]) return nullptr;
    if (periodic[2]) k = readdy::util::numeric::positive_modulo(k, nCells[2]);
    else if (k < 0 || k >= nCells[2]) return nullptr;
    const auto cix = get_contiguous_index(i, j, k, nCells[1], nCells[2]);
    if (cix < cells.size()) {
        return &cells.at(static_cast<cell_index>(cix));
    } else {
        log::console()->critical("NeighborList::getCell(const): Requested cell ({},{},{})={}, but there are "
                                         "only {} cells.", i, j, k, cix, cells.size());
        throw std::out_of_range("tried to access an invalid cell");
    }
}

const std::vector<NeighborList::neighbor_t> &NeighborList::find_neighbors(particle_index const entry) const {
    if (maxCutoff > 0 && entry < data.neighbors.size()) {
        return data.neighbors.at(entry);
    }
    return no_neighbors;
}

NeighborList::~NeighborList() = default;

void NeighborList::Cell::addNeighbor(NeighborList::Cell *cell) {
    if (cell && cell->contiguous_index != contiguous_index
        && (enoughCells || std::find(neighbors.begin(), neighbors.end(), cell) == neighbors.end())) {
        neighbors.push_back(cell);
    }
}

NeighborList::Cell::Cell(cell_index i, cell_index j, cell_index k,
                         const std::array<NeighborList::cell_index, 3> &nCells)
        : contiguous_index(get_contiguous_index(i, j, k, nCells[1], nCells[2])),
          enoughCells(nCells[0] >= 5 && nCells[1] >= 5 && nCells[2] >= 5), maximal_displacements{0, 0} {
}

bool operator==(const NeighborList::Cell &lhs, const NeighborList::Cell &rhs) {
    return lhs.contiguous_index == rhs.contiguous_index;
}

bool operator!=(const NeighborList::Cell &lhs, const NeighborList::Cell &rhs) {
    return !(lhs == rhs);
}

void NeighborList::Cell::checkDirty(skin_size_t skin) {
    dirty = maximal_displacements[0] + maximal_displacements[1] > skin;
}

NeighborList::hilbert_index_t NeighborList::getHilbertIndex(std::size_t i, std::size_t j, std::size_t k) const {
    bitmask_t coords[3]{i, j, k};
    return static_cast<unsigned int>(hilbert_c2i(3, CHAR_BIT, coords));
}

void NeighborList::displace(data_t::Entry &entry, const data_t::particle_type::pos_type &delta) {
    data.displace(entry, delta);
}

void NeighborList::displace(data_iter_t iter, const readdy::model::Vec3 &vec) {
    auto &entry = *iter;
    displace(entry, vec);
}

void NeighborList::displace(data_t::index_t idx, const data_t::particle_type::pos_type &delta) {
    auto &entry = data.entry_at(idx);
    displace(entry, delta);
}

void NeighborList::setPosition(data_t::index_t idx, data_t::particle_type::pos_type &&newPosition) {
    auto &entry = data.entry_at(idx);
    const auto delta = newPosition - entry.position();
    displace(entry, delta);
}

std::unordered_set<NeighborList::Cell *> NeighborList::findDirtyCells() {
    using it_type = decltype(cells.begin());

    using future_t = std::future<std::vector<Cell *>>;
    using promise_t = std::promise<std::vector<Cell *>>;

    std::unordered_set<NeighborList::Cell *> result;


    auto worker = [this](it_type begin, it_type end, promise_t update, const thd::barrier &barrier,
                         std::atomic<bool> &interrupt) {
        // interrupt if there is a particle that travelled too far (r_c + r_s)
        bool shouldInterrupt = false;
        for (it_type it = begin; it != end; ++it) {
            shouldInterrupt |= markCell(*it, data, maxCutoff, skin_size);
            if (shouldInterrupt) break;
        }
        if (shouldInterrupt) interrupt = shouldInterrupt;
        barrier.wait();
        std::vector<Cell *> foundDirtyCells;
        if (!interrupt.load()) {
            for (it_type it = begin; it != end; ++it) {
                if (cellOrNeighborDirty(*it)) {
                    foundDirtyCells.push_back(&*it);
                }
            }
        }
        update.set_value(std::move(foundDirtyCells));
    };

    std::atomic<bool> interrupt(false);
    thd::barrier b(config->nThreads());

    std::vector<future_t> dirtyCells;
    dirtyCells.reserve(config->nThreads());
    {
        auto it_cells = cells.begin();
        std::vector<thd::scoped_thread> threads;
        const std::size_t grainSize = cells.size() / config->nThreads();
        for (int i = 0; i < config->nThreads() - 1; ++i) {
            promise_t promise;
            dirtyCells.push_back(promise.get_future());
            threads.push_back(
                    thd::scoped_thread(
                            std::thread(worker, it_cells, it_cells + grainSize, std::move(promise), std::cref(b),
                                        std::ref(interrupt)))
            );
            it_cells += grainSize;
        }
        promise_t promise;
        dirtyCells.push_back(promise.get_future());
        threads.push_back(thd::scoped_thread(
                std::thread(worker, it_cells, it_cells + grainSize, std::move(promise), std::cref(b),
                            std::ref(interrupt))));
    }
    if (interrupt.load()) {
        throw ParticleTravelledTooFarException();
    }
    for (auto &&dirties : dirtyCells) {
        auto v = std::move(dirties.get());
        std::copy(v.begin(), v.end(), std::inserter(result, result.end()));
    }
    return std::move(result);
}

void NeighborList::setUpCell(NeighborList::Cell &cell, const double cutoffSquared, const ctx_t::dist_squared_fun &d2) {
    cell.dirty = false;
    cell.maximal_displacements[0] = 0;
    cell.maximal_displacements[1] = 0;
    for (const auto &pI : cell.particles()) {
        auto &entry_i = data.entry_at(pI);
        entry_i.displacement = 0;
        auto &neighbors_i = data.neighbors_at(pI);
        neighbors_i.clear();
        for (const auto &pJ : cell.particles()) {
            if (pI != pJ) {
                const auto distSquared = d2(entry_i.position(), data.pos(pJ));
                if (distSquared < cutoffSquared) {
                    neighbors_i.push_back(pJ);
                }
            }
        }
        for (const auto &neighboringCell : cell.neighbors) {
            for (const auto pJ : neighboringCell->particles()) {
                const auto distSquared = d2(entry_i.position(), data.pos(pJ));
                if (distSquared < cutoffSquared) {
                    neighbors_i.push_back(pJ);
                }
            }
        }

    }
}

int NeighborList::getCellIndex(const readdy::model::Particle::pos_type &pos) const {
    signed_cell_index i = static_cast<const signed_cell_index>(floor((pos[0] + .5 * simBoxSize[0]) / cellSize[0]));
    signed_cell_index j = static_cast<const signed_cell_index>(floor((pos[1] + .5 * simBoxSize[1]) / cellSize[1]));
    signed_cell_index k = static_cast<const signed_cell_index>(floor((pos[2] + .5 * simBoxSize[2]) / cellSize[2]));
    const auto &periodic = ctx->getPeriodicBoundary();
    if (periodic[0]) i = readdy::util::numeric::positive_modulo(i, nCells[0]);
    else if (i < 0 || i >= nCells[0]) return -1;
    if (periodic[1]) j = readdy::util::numeric::positive_modulo(j, nCells[1]);
    else if (j < 0 || j >= nCells[1]) return -1;
    if (periodic[2]) k = readdy::util::numeric::positive_modulo(k, nCells[2]);
    else if (k < 0 || k >= nCells[2]) return -1;
    return get_contiguous_index(i, j, k, nCells[1], nCells[2]);
}

void NeighborList::setSkinSize(NeighborList::skin_size_t skin_size) {
    initialSetup = true;
    cells.clear();
    NeighborList::skin_size = skin_size;
}

bool NeighborList::isInCell(const NeighborList::Cell &cell, const data_t::particle_type::pos_type &pos) const {
    return hash_pos(pos) == cell.contiguous_index;
}

std::size_t NeighborList::hash_pos(const data_t::particle_type::pos_type &pos) const {
    const cell_index i = static_cast<const cell_index>(floor((pos[0] + .5 * simBoxSize[0]) / cellSize[0]));
    const cell_index j = static_cast<const cell_index>(floor((pos[1] + .5 * simBoxSize[1]) / cellSize[1]));
    const cell_index k = static_cast<const cell_index>(floor((pos[2] + .5 * simBoxSize[2]) / cellSize[2]));
    return get_contiguous_index(i, j, k, nCells[1], nCells[2]);
}

void NeighborList::setGroupParticlesOnCreation(bool groupParticlesOnCreation) {
    NeighborList::groupParticlesOnCreation = groupParticlesOnCreation;
}

}
}
}
}
