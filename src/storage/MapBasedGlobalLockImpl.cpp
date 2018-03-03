#include "MapBasedGlobalLockImpl.h"

#include <mutex>

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Put(const std::string &key, const std::string &value) {
    std::unique_lock<std::mutex> guard(_lock);
    std::string tmp_value;
    if (get_value_if_exists(key, tmp_value)) {
        _list->front()->value = value;
    } else {
        auto new_elem_size = key.size() + value.size();
        if (new_elem_size > _max_size) {
            return false;
        }
        while (new_elem_size + _size > _max_size) {
            auto last = _list->back();
            _size -= (last->key.size() + last->key.size());
            _backend.erase(_list->back()->key);
            _list->pop_back();
        }
        _list->push_front(key, value);
        _backend.emplace(_list->front()->key, _list->front());
        _size += new_elem_size;
    }
    return true;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::PutIfAbsent(const std::string &key, const std::string &value) {
    std::unique_lock<std::mutex> guard(_lock);
    std::string tmp_value;
    if (get_value_if_exists(key, tmp_value)) {
        return false;
    } else {
        auto new_elem_size = key.size() + value.size();
        if (new_elem_size > _max_size) {
            return false;
        }
        while (new_elem_size + _size > _max_size) {
            auto last = _list->back();
            _size -= (last->key.size() + last->key.size());
            _backend.erase(_list->back()->key);
            _list->pop_back();
        }
        _list->push_front(key, value);
        _backend[key] = _list->front();
        _size += new_elem_size;
    }
    return true;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Set(const std::string &key, const std::string &value) {
    std::unique_lock<std::mutex> guard(_lock);
    std::string tmp_value;
    if (get_value_if_exists(key, tmp_value)) {
        _list->front()->value = value;
    }
    return true;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Delete(const std::string &key) {
    std::unique_lock<std::mutex> guard(_lock);
    std::string tmp_value;
    if (get_value_if_exists(key, tmp_value)) {
        auto item = _backend.find(key);
        _list->erase(item->second);
        _backend.erase(key);
        return true;
    }
    return false;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Get(const std::string &key, std::string &value) const {
    std::unique_lock<std::mutex> guard(_lock);
    auto it = _backend.find(key);
    if (it != _backend.end()) {
        _list->move_to_front(it->second);
        value = _list->front()->value;
        return true;
    }
    return false;
}

bool MapBasedGlobalLockImpl::get_value_if_exists(const std::string &key, std::string &value) {
    auto it = _backend.find(key);
    if (it != _backend.end()) {
        _list->move_to_front(it->second);
        value = _list->front()->value;
        return true;
    }
    return false;
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
