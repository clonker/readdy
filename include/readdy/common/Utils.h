/**
 * This header file contains definitions for various common utils. Currently:
 *   - getOS: returns a string corresponding to the executing operating system
 *   - isWindows: returns true if we are in windows
 *   - collections::hasKey: convenience method to check if a map contains a certain key
 *   - testing::getPluginsDirectory: Method that checks for some environment variables and then returns a potential
 *     directory in which the kernels are likely located.
 *
 * @file ObservableFactory.h
 * @brief Header file containing some common utils.
 * @author clonker
 * @date 08.03.16
 */

#ifndef READDY_MAIN_UTILS_H
#define READDY_MAIN_UTILS_H

#include <string>
#include <iostream>
#include <memory>
#include <tuple>
#include <vector>

namespace readdy {
namespace util {
std::string getOS();

bool isWindows();

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);

std::vector<std::string> split(const std::string &s, char delim);

namespace collections {
template<typename MapType, typename KeyType = std::string>
inline bool hasKey(const MapType &map, const KeyType &key) {
    return map.find(key) != map.end();
}

template<template<class, class, class...> class C, typename K, typename V, typename... Args>
inline const V &getOrDefault(const C<K, V, Args...> &m, const K &key, const V &defaultValue) {
    typename C<K, V, Args...>::const_iterator it = m.find(key);
    if (it == m.end()) {
        return defaultValue;
    }
    return it->second;
}

template<typename Collection, typename Fun>
inline void for_each_value(const Collection& collection, Fun f)  {
    for(auto&& e : collection) {
        for(auto&& inner : e.second) {
            f(e.first, inner);
        }
    }
}

}

}
}

#endif //READDY_MAIN_UTILS_H
