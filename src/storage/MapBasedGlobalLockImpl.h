#ifndef AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H
#define AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H

#include <functional>
#include <list>
#include <map>
#include <mutex>
#include <string>

#include "../../include/afina/Storage.h"

namespace Afina {
namespace Backend {

/**
 * # Map based implementation with global lock
 *
 *
 */
class Node {
public:
    std::string key;
    std::string value;
    Node *next;
    Node *prev;
};

class Dl_list {
public:
    Dl_list();
    ~Dl_list();
    void push_front(std::string, std::string);
    void pop_back();
    void erase(Node *);
    void move_to_front(Node *);
    void print();
    Node *front();
    Node *back();

private:
    Node *head;
    Node *tail;
};

class MapBasedGlobalLockImpl : public Afina::Storage {
public:
    MapBasedGlobalLockImpl(size_t max_size = 1024) : _max_size(max_size), _size(0), _list(new Dl_list()) {}
    ~MapBasedGlobalLockImpl() {delete(_list);}

    // Implements Afina::Storage interface
    bool Put(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool PutIfAbsent(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Set(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Delete(const std::string &key) override;

    // Implements Afina::Storage interface
    bool Get(const std::string &key, std::string &value) const override;

private:
    size_t _max_size;
    size_t _size;
    mutable std::mutex _lock;
    //mutable std::list<std::pair<std::string, std::string>> _list;
    //std::map<std::reference_wrapper<const std::string>, decltype(_list)::iterator, std::less<const std::string>>_backend;
    
    Dl_list *_list;
    std::map<std::reference_wrapper<const std::string>, Node *, std::less<const std::string>>_backend;

    bool get_value_if_exists(const std::string &key, std::string &value);
};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H
