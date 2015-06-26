#ifndef _VPL_DELEGATE_H_
#define _VPL_DELEGATE_H_

#include <map>
#include <list>
#include <tuple>
#include <vector>
#include <functional>
#include "Threading.h"

namespace vee {

template <class T>
void* GetPtrMemFn(T member_fn)
{
    //typedef void(*FPtr)(void); // Hide the ugliness
    //FPtr f = nullptr;          // Function pointer to convert
    //void* ptr = *(void**)(&f);  // Data pointer
    //FPtr f2 = *(FPtr*)(&ptr);   // Function pointer restored
    auto origin = member_fn;
    void* ret = *(void**)(&origin);
    return ret;
}

template <class MutexTy, typename DelegateRTy, typename ... DelegateArgs>
class VectorDelegate
{
public:
    typedef DelegateRTy return_type;
    typedef DelegateRTy(__stdcall *funcptr_type)(DelegateArgs ...);
    typedef std::function<DelegateRTy(DelegateArgs ...)> function_type;
    typedef std::pair<void*, function_type> element_type;
    typedef std::vector<element_type> container_type;
    typedef MutexTy mutex_type;
    typedef VectorDelegate<MutexTy, DelegateRTy, DelegateArgs...> this_type;
    typedef this_type& reference_type;
    typedef this_type&& rreference_type;
    typedef std::tuple<DelegateArgs...> args_tuple_type;
public:
    // Constructor
    VectorDelegate() = default;
    // Destructor
    ~VectorDelegate() = default;
    // Copy constructor
    VectorDelegate(const reference_type other)
    {
        LockGuard<MutexTy> guard(other._m);
        _c = other._c;
    }
    VectorDelegate(rreference_type other):
        _m(std::move(other._m)),
        _c(std::move(other._c))
    {
        // nothing to do.
    }
    void swap(reference_type other)
    {
        // std::swap(_m, other._m);
        std::swap(_c, other._c);
    }
    reference_type operator=(reference_type other)
    {
        LockGuard<MutexTy> guard(other._m);
        _c = other._c;
        return *this;
    }
    reference_type operator=(rreference_type other)
    {
        _m = std::move(other._m);
        _c = std::move(other._c);
        return *this;
    }
    bool AddCallback(void* f)
    {
        if (f == nullptr)
            return false;
        function_type callback = static_cast<funcptr_type>(f);
        LockGuard<MutexTy> guard(_m);
        _c.push_back(std::make_pair(f, std::move(callback)));
        return true;
    }
    bool AddCallback(function_type& callback)
    {
        if (callback.target<funcptr_type>() == nullptr)
            return false;
        LockGuard<MutexTy> guard(_m);
        _c.push_back(std::make_pair(*(callback.target<funcptr_type>()), callback));
        return true;
    }
    // 딜리게이트는 원래 함수의 주소를 key로 사용함, (이로서 C#호환 가능)
    // 그런데 bind된 함수는 일반 함수가 아니라 함수 객체임.
    // std function은 (boost function도..) 함수 객체도 가리킬 수 있는 객체임.
    // 딜리게이트는 내부적으로 std function으로 함수를 콜하기때문에 함수 객체도 가리킬 수 있음.
    // 문제의 원인은 std function으로부터 함수 객체가 가리키는 원래의 주소를 구할 수 없다는 것
    // 임시 방편이지만, 키 값으로 사용할 주소를 Add할 때 직접 넣어주도록 만듦.
    // 여기서도 문제가 하나 있는게, 멤버함수의 주소는 C++에서 void*로 캐스팅이 안됨.
    // 그래서 멤버 함수를 가리키는 포인터를 하나 만들고 그 포인터의 값을 void* 로 캐스팅해야함
    // 이 작업을 해주는 템플릿 함수 GetPtrMemFn을 만들었음. 참고바람 (Delegate.h)
    // 딜리게이트에서 함수를 제거할 때는 키값으로 뺴기 때문에 하던대로 키 값으로 빼면 됨.
    // 다만 멤버 함수의 주소를 키 값으로 주었기 때문에 뺄때도 같은 방식으로 주소를 구해주어야 함.
    // 키 값은 void* 형태로만 넘기면 아무거나 가능하지만 멤버함수를 바인딩 했으므로 멤버함수의 주소를 추천.
    bool AddCallback(void* key, function_type callback)
    {
        LockGuard<MutexTy> guard(_m);
        _c.push_back(std::make_pair(key, callback));
        return true;
    }
    bool RemoveCallback(void *f)
    {
        LockGuard<MutexTy> guard(_m);
        for (auto it = _c.begin(); it != _c.end();)
        {
            if ((*it).first == f)
            {
                _c.erase(it++);
                return true;
            }
            else
            {
                ++it;
            }
        }
        return false;
    }
    bool RemoveCallback(function_type& callback)
    {
        funcptr_type f = *(callback.target<funcptr_type>());
        if (f == nullptr)
            return false;
        LockGuard<MutexTy> guard(_m);
        for (auto it = _c.begin(); it != _c.end();)
        {
            if ((*it).first == f)
            {
                _c.erase(it++);
                return true;
            }
            else
            {
                ++it;
            }
        }
        return false;
    }
    void RemoveBack()
    {
        LockGuard<MutexTy> guard(_m);
        _c.pop_back();
    }
    template <typename ...FwdArgs>
    void Notify(FwdArgs&&... args)
    {
        LockGuard<MutexTy> guard(_m);
        for (auto& it : _c)
        {
            it.second(static_cast<DelegateArgs>(args) ...);
        }
    }
    template <typename ...FwdArgs>
    void operator()(FwdArgs&&... args)
    {
        LockGuard<MutexTy> guard(_m);
        for (auto& it : _c)
        {
            it.second(static_cast<DelegateArgs>(args) ...);
        }
    }
    inline bool operator+=(void* f)
    {
        return AddCallback(f);
    }
    inline bool operator+=(function_type& fn)
    {
        return AddCallback(fn);
    }
    inline bool operator-=(void* f)
    {
        return RemoveCallback(f);
    }
    inline bool operator-=(function_type& fn)
    {
        return RemoveCallback(fn);
    }
private:
    container_type _c;
    mutable mutex_type _m;
};

template <class MutexTy, typename DelegateRTy, typename ... DelegateArgs>
class ListDelegate
{
public:
    typedef DelegateRTy return_type;
    typedef DelegateRTy(__stdcall *funcptr_type)(DelegateArgs ...);
    typedef std::function<DelegateRTy(DelegateArgs ...)> function_type;
    typedef std::pair<void*, function_type> element_type;
    typedef std::list<element_type> container_type;
    typedef MutexTy mutex_type;
    typedef ListDelegate<MutexTy, DelegateRTy, DelegateArgs...> this_type;
    typedef this_type& reference_type;
    typedef this_type&& rreference_type;
    typedef std::tuple<DelegateArgs...> args_tuple_type;
public:
    // Constructor
    ListDelegate() = default;
    // Destructor
    ~ListDelegate() = default;
    // Copy constructor
    ListDelegate(const reference_type other)
    {
        LockGuard<MutexTy> guard(other._m);
        _c = other._c;
    }
    ListDelegate(rreference_type other):
        _m(std::move(other._m)),
        _c(std::move(other._c))
    {
        // nothing to do.
    }
    void swap(reference_type other)
    {
        //   std::swap(_m, other._m);
        std::swap(_c, other._c);
    }
    reference_type operator=(reference_type other)
    {
        LockGuard<MutexTy> guard(other._m);
        _c = other._c;
        return *this;
    }
    reference_type operator=(rreference_type other)
    {
        _m = std::move(other._m);
        _c = std::move(other._c);
        return *this;
    }
    bool AddCallback(void* f)
    {
        if (f == nullptr)
            return false;
        function_type callback = static_cast<funcptr_type>(f);
        LockGuard<MutexTy> guard(_m);
        _c.push_back(std::make_pair(f, std::move(callback)));
        return true;
    }
    bool AddCallback(function_type& callback)
    {
        if (callback.target<funcptr_type>() == nullptr)
            return false;
        LockGuard<MutexTy> guard(_m);
        _c.push_back(std::make_pair(*(callback.target<funcptr_type>()), callback));
        return true;
    }
    bool AddCallback(void* key, function_type callback)
    {
        LockGuard<MutexTy> guard(_m);
        _c.push_back(std::make_pair(key, callback));
        return true;
    }
    bool RemoveCallback(void *f)
    {
        LockGuard<MutexTy> guard(_m);
        for (auto it = _c.begin(); it != _c.end();)
        {
            if ((*it).first == f)
            {
                _c.erase(it++);
                return true;
            }
            else
            {
                ++it;
            }
        }
        return false;
    }
    bool RemoveCallback(function_type& callback)
    {
        funcptr_type f = *(callback.target<funcptr_type>());
        if (f == nullptr)
            return false;
        LockGuard<MutexTy> guard(_m);
        for (auto it = _c.begin(); it != _c.end();)
        {
            if ((*it).first == f)
            {
                _c.erase(it++);
                return true;
            }
            else
            {
                ++it;
            }
        }
        return false;
    }
    void RemoveBack()
    {
        LockGuard<MutexTy> guard(_m);
        _c.pop_back();
    }
    void RemoveFront()
    {
        LockGuard<MutexTy> guard(_m);
        _c.pop_front();
    }
    template <typename ...FwdArgs>
    void Notify(FwdArgs&&... args)
    {
        LockGuard<MutexTy> guard(_m);
        for (auto& it : _c)
        {
            it.second(static_cast<DelegateArgs>(args) ...);
        }
    }
    template <typename ...FwdArgs>
    void operator()(FwdArgs&&... args)
    {
        LockGuard<MutexTy> guard(_m);
        for (auto& it : _c)
        {
            it.second(static_cast<DelegateArgs>(args) ...);
        }
    }
    inline bool operator+=(void* f)
    {
        return AddCallback(f);
    }
    inline bool operator+=(function_type& fn)
    {
        return AddCallback(fn);
    }
    inline bool operator-=(void* f)
    {
        return RemoveCallback(f);
    }
    inline bool operator-=(function_type& fn)
    {
        return RemoveCallback(fn);
    }
private:
    container_type _c;
    mutable mutex_type _m;
};

template <class MutexTy, typename DelegateRTy, typename ... DelegateArgs>
class Delegate
{
    // Define types and constant variables
public:
    typedef DelegateRTy return_type;
    typedef DelegateRTy(__stdcall *funcptr_type)(DelegateArgs ...);
    typedef std::function<DelegateRTy(DelegateArgs ...)> function_type;
    typedef std::multimap<void*, function_type> container_type;
    typedef MutexTy  mutex_type;
    typedef Delegate<MutexTy, DelegateRTy, DelegateArgs...> this_type;
    typedef this_type& reference_type;
    typedef this_type&& rreference_type;
    typedef std::tuple<DelegateArgs...> args_tuple_type;
    // Define member functions
public:
    // Constructor
    Delegate() = default;
    // Destructor
    ~Delegate() = default;
    // Copy constructor
    Delegate(const reference_type other)
    {
        LockGuard<MutexTy> guard(other._m);
        _c = other._c;
    }
    Delegate(rreference_type other):
        _m(std::move(other._m)),
        _c(std::move(other._c))
    {
        // nothing to do.
    }
    void swap(reference_type other)
    {
        //  std::swap(_m, other._m);
        std::swap(_c, other._c);
    }
    reference_type operator=(reference_type other)
    {
        LockGuard<MutexTy> guard(other._m);
        _c = other._c;
        return *this;
    }
    reference_type operator=(rreference_type other)
    {
        _m = std::move(other._m);
        _c = std::move(other._c);
        return *this;
    }
    bool AddCallback(void* f)
    {
        if (f == nullptr)
            return false;
        function_type callback = static_cast<funcptr_type>(f);
        LockGuard<MutexTy> guard(_m);
        _c.insert(std::map<void*, function_type>::value_type(f, callback));
        return true;
    }
    bool AddCallback(function_type& callback)
    {
        if (callback.target<funcptr_type>() == nullptr)
            return false;
        LockGuard<MutexTy> guard(_m);
        _c.insert(std::map<void*, function_type>::value_type(*(callback.target<funcptr_type>()), callback));
        return true;
    }
    bool AddCallback(void* key, function_type callback)
    {
        LockGuard<MutexTy> guard(_m);
        _c.push_back(std::make_pair(key, callback));
        return true;
    }
    bool RemoveCallback(void* f)
    {
        LockGuard<MutexTy> guard(_m);
        auto dst = _c.end();
        for (auto& it = _c.begin(); it != _c.end(); ++it)
        {
            if (it->first == f)
            {
                dst = it;
                break;
            }
        }
        if (dst == _c.end())
            return false;
        _c.erase(dst);
        return true;
    }
    bool RemoveCallback(function_type& callback)
    {
        funcptr_type f = *(callback.target<funcptr_type>());
        if (f == nullptr)
            return false;
        LockGuard<MutexTy> guard(_m);
        auto dst = _c.end();
        for (auto& it = _c.begin(); it != _c.end(); ++it)
        {
            if (it->first == f)
            {
                dst = it;
                break;
            }
        }
        if (dst == _c.end())
            return false;
        _c.erase(dst);
        return true;
    }
    template <typename ...FwdArgs>
    void Notify(FwdArgs&&... args)
    {
        LockGuard<MutexTy> guard(_m);
        for (auto& it : _c)
        {
            it.second(static_cast<DelegateArgs>(args)...);
        }
    }
    template <typename ...FwdArgs>
    void operator()(FwdArgs&&... args)
    {
        LockGuard<MutexTy> guard(_m);
        for (auto& it : _c)
        {
            it.second(static_cast<DelegateArgs>(args)...);
        }
    }
    inline bool operator+=(void* f)
    {
        return AddCallback(f);
    }
    inline bool operator+=(function_type& fn)
    {
        return AddCallback(fn);
    }
    inline bool operator-=(void* f)
    {
        return RemoveCallback(f);
    }
    inline bool operator-=(function_type& fn)
    {
        return RemoveCallback(fn);
    }
    // Define member variables
private:
    std::multimap<void*, function_type> _c;
    mutable mutex_type _m;
};

} // namespace vee

#endif // _VPL_DELEGATE_H_