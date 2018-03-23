#include "MapBasedGlobalLockImpl.h"

#include <mutex>
#include <iostream>

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Put(const std::string &key, const std::string &value) {
    std::unique_lock<std::mutex> guard(_lock);
    std::cout << "key3 = " << key << '\n';
    if (exists(key)) {
        _size -= key.size() + _list->front()->value.size();
        if (!free_space(key.size() + value.size())) {
            _size += key.size() + _list->front()->value.size();
            return false;
        }
        _list->front()->value = value;
    } else {
        if (!free_space(key.size() + value.size())) {
            return false;
        }
        _list->push_front(key, value);
        _backend.emplace(_list->front()->key, _list->front());
    }
    return true;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::PutIfAbsent(const std::string &key, const std::string &value) {
    std::unique_lock<std::mutex> guard(_lock);
    std::cout << "key2 = " << key << '\n';
    if (exists(key)) {
        return false;
    } else {
        if (!free_space(key.size() + value.size())) {
            return false;
        }
        _list->push_front(key, value);
        _backend.emplace(_list->front()->key, _list->front());
    }
    return true;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Set(const std::string &key, const std::string &value) {
    std::unique_lock<std::mutex> guard(_lock);
    std::cout << "key1 = " << key << '\n';
    if (exists(key)) {
        _size -= key.size() + _list->front()->value.size();
        if (!free_space(key.size() + value.size())) {
            _size += key.size() + _list->front()->value.size();
            return false;
        }
        _list->front()->value = value;
        return true;
    }
    return false;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Delete(const std::string &key) {
    std::unique_lock<std::mutex> guard(_lock);

    if (exists(key)) {
        auto item = _backend.find(key);
        _size -= key.size() + item->second->value.size();
        _list->erase(item->second);
        _backend.erase(key);
        return true;
    }
    return false;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Get(const std::string &key, std::string &value) const {
    std::unique_lock<std::mutex> guard(_lock);
    std::cout << "key4 = " << key << '\n';
    if (exists(key)) {
        value = _list->front()->value;
        return true;
    }
    return false;
}

// Check if record for given key exists and move it to the beginnind of LRU _list
bool MapBasedGlobalLockImpl::exists(const std::string &key) const {
    std::cout << "key5 = " << key << '\n';
    auto it = _backend.find(key);
    if (it != _backend.end()) {
        std::cout << "key6 = " << key << '\n';
        _list->move_to_front(it->second);
        return true;
    }
    std::cout << "key7 = " << key << '\n';
    return false;
}

// Check if new pair key/value fits into memory
// Remove least used records from cache until there is enought space for new record
bool MapBasedGlobalLockImpl::free_space(size_t elem_size) {
    if (elem_size > _max_size) {
        return false;
    }
    while (elem_size + _size > _max_size) {
        auto last = _list->back();
        _size -= last->key.size() + last->key.size();
        _backend.erase(_list->back()->key);
        _list->pop_back();
    }
    _size += elem_size;
    return true;
}

Dl_list::Dl_list() { head = NULL; }

Dl_list::~Dl_list() {
    while (head) {
        Node *tmp = head;
        head = head->next;
        delete (tmp);
    }
}

void Dl_list::push_front(std::string key, std::string value) {

    Node *node = new Node();
    node->key = key;
    node->value = value;
    node->prev = NULL;

    if (head == NULL) {
        node->next = NULL;
        head = node;
        tail = node;
    } else {
        node->next = head;
        node->next->prev = node;
        head = node;
    }
}

void Dl_list::pop_back() {
    Node *tmp = tail;
    tail->prev->next = NULL;
    tail = tail->prev;
    delete (tmp);
}

void Dl_list::erase(Node *node) {
    if (node == head) {
        Node *tmp = head;
        head = node->next;
        head->prev = NULL;
        delete (tmp);
    } else if (node == tail) {
        pop_back();
    } else {
        node->prev->next = node->next;
        delete (node);
    }
}

void Dl_list::move_to_front(Node *node) {
    if (node != head) {
        if (node == tail) {
            tail = node->prev;
        } else {
            node->next->prev = node->prev;
        }
        node->prev->next = node->next;
        node->prev = NULL;
        head->prev = node;
        node->next = head;
        head = node;
    }
}

Node *Dl_list::front() { return head; }

Node *Dl_list::back() { return tail; }

} // namespace Backend
} // namespace Afina