#include "MapBasedGlobalLockImpl.h"

#include <mutex>

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Put(const std::string &key, const std::string &value) { 
    std::unique_lock<std::mutex> guard(_lock);
    std::string tmp_value;
    if (get_value_if_exists(key, tmp_value)){
        _list.front().second = value;
    } else {
        auto new_elem_size = key.size() + value.size();
        if (new_elem_size > _max_size){
            return false;
        }
        while (new_elem_size + _size > _max_size){
            auto last = _list.back();
            _size -= (last.first.size() + last.second.size());
            _backend.erase(_list.back().first);
            _list.pop_back();
        }
        _list.push_front(std::pair<std::string, std::string>(key, value));
        _backend.emplace(_list.front().first, _list.begin());
        _size += new_elem_size;
    }
    return true; 
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::PutIfAbsent(const std::string &key, const std::string &value) {
    std::unique_lock<std::mutex> guard(_lock);
    std::string tmp_value;
    if (get_value_if_exists(key, tmp_value)){
        return false;
    } else {
        auto new_elem_size = key.size() + value.size();
        if (new_elem_size > _max_size){
            return false;
        }
        while (new_elem_size + _size > _max_size){
            auto last = _list.back();
            _size -= (last.first.size() + last.second.size());
            _backend.erase(_list.back().first);
            _list.pop_back();
        }
        _list.push_front(std::pair<std::string, std::string>(key, value));
        _backend[key] = _list.begin();
        _size += new_elem_size;
    }
    return true; 
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Set(const std::string &key, const std::string &value) {
    std::unique_lock<std::mutex> guard(_lock);
    std::string tmp_value;
    if (get_value_if_exists(key, tmp_value)){
        _list.front().second = value;
    }
    return true; 
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Delete(const std::string &key) { 
    std::unique_lock<std::mutex> guard(_lock);
    std::string tmp_value;
    if (get_value_if_exists(key, tmp_value)){
        auto item = _backend.find(key);
        _list.erase(item->second);
        _backend.erase(key);
        return true;
    }
    return false; 
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Get(const std::string &key, std::string &value) const { 
    std::unique_lock<std::mutex> guard(_lock);
    auto it = _backend.find(key);
    if (it != _backend.end()){
        _list.splice(_list.begin(), _list, it->second);
        value = _list.front().second;
        return true;
    }
    return false; 
}

bool MapBasedGlobalLockImpl::get_value_if_exists(const std::string &key, std::string &value) { 
    auto it = _backend.find(key);
    if (it != _backend.end()){
        _list.splice(_list.begin(), _list, it->second);
        value = _list.front().second;
        return true;
    }
    return false; 
}

} // namespace Backend
} // namespace Afina
