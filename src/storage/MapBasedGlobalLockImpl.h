#ifndef AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H
#define AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H

#include <map>
#include <mutex>
#include <string>
#include <functional>
#include <list>

#include <afina/Storage.h>
//#include "Storage.h"

namespace Afina {
namespace Backend {

/**
 * # Map based implementation with global lock
 *
 *
 */
class MapBasedGlobalLockImpl : public Afina::Storage {
public:
    MapBasedGlobalLockImpl(int max_size = 1024) : _max_size(max_size), _current_size(0) {}

    ~MapBasedGlobalLockImpl() {}

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


    int GetSize() const;
    int GetCurrentSize() const;
    bool SetNewCurrentSize(int newSize);
    bool FreeSpace(int new_size);

private:

    int _max_size;
    int _current_size;

    mutable std::mutex _lock;

    //ключ, значение
    mutable std::list<std::pair<std::string, std::string> > _lru;

    // ссылка на ключ, итератор на соответствующий элемент в списке
    std::map<std::reference_wrapper<const std::string>,
            std::list<std::pair<std::string, std::string> >::iterator,
            std::less<const std::string> > _cacheMap;
};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H