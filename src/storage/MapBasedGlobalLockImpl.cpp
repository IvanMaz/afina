#include "MapBasedGlobalLockImpl.h"

#include <mutex>
#include <iostream>

namespace Afina {
namespace Backend {

// // See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::FreeSpace(int new_size) {
    while (_current_size + new_size > _max_size) {
        auto last = _lru.back();

        _current_size -= (int) (last.first.size() + last.second.size());

        _cacheMap.erase(last.first);
        _lru.pop_back();
    }
    return true;
}
bool MapBasedGlobalLockImpl::Put(const std::string &key, const std::string &value) {
    std::unique_lock<std::mutex> guard(_lock);

    auto p = _cacheMap.find(key);               //map iterator
    auto new_size = key.size() + value.size();

    if (new_size > _max_size) {
        return false;
    }

    if (p == _cacheMap.end()) {
        if (!FreeSpace((int)new_size)) {
            return false;
        }

        _lru.push_front(std::make_pair(key, value));        //put new element at head
        _cacheMap[std::cref(_lru.front().first)] = _lru.begin();
        _current_size += new_size;

    } else {

        auto old_size = p->second->first.size() + p->second->second.size();
        _lru.splice(_lru.begin(), _lru, p->second);

        if (!FreeSpace((int)(new_size - old_size))) {
            return false;
        }
        _lru.front().second = value;
    }
    return true;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::PutIfAbsent(const std::string &key, const std::string &value) {
    std::unique_lock<std::mutex> guard(_lock);

    auto p = _cacheMap.find(key);               //map iterator
    auto new_size = key.size() + value.size();

    if (new_size > _max_size) {
        return false;
    }

    if (p == _cacheMap.end()) {
        if (!FreeSpace((int)new_size)) {
            return false;
        }

        _lru.push_front(std::make_pair(key, value));        //put new element at head
        _cacheMap[std::cref(_lru.front().first)] = _lru.begin();
        _current_size += new_size;

    } else {
        return false;
    }
    return true;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Set(const std::string &key, const std::string &value) {
    std::unique_lock<std::mutex> guard(_lock);

    auto p = _cacheMap.find(key); //map iterator
    auto new_size = key.size() + value.size();

    if (new_size > _max_size) {
        return false;
    }

    if (p != _cacheMap.end()) {
        auto old_size = p->second->first.size() + p->second->second.size();
        _lru.splice(_lru.begin(), _lru, p->second);

        FreeSpace((int)(new_size - old_size));

        _current_size -= p->second->second.size();
        _lru.front().second = value;
        _current_size += new_size;
        return true;

    }
    return false;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Delete(const std::string &key) {
    std::unique_lock<std::mutex> guard(_lock);

    auto p = _cacheMap.find(key); //map iterator

    if (p != _cacheMap.end()) {
        auto item = _cacheMap.find(key);
        _lru.erase(item->second);
        _cacheMap.erase(key);
        return true;
    }
    return false;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Get(const std::string &key, std::string &value) const {
    std::lock_guard<std::mutex> lock(_lock);

    auto p = _cacheMap.find(key); //map iterator

    if (p != _cacheMap.end()) {
        _lru.splice(_lru.begin(), _lru, p->second);
        value = _lru.front().second;
        return true;
    }
    return false;
}

// See MapBasedGlobalLockImpl.h
int MapBasedGlobalLockImpl::GetSize() const {
    return _max_size;
}


int MapBasedGlobalLockImpl::GetCurrentSize() const {
    return _current_size;
}

bool MapBasedGlobalLockImpl::SetNewCurrentSize(const int newSize) {
    _current_size = newSize;
    return true;
}
}
}