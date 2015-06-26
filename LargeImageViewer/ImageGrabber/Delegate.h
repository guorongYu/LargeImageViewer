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
    // ��������Ʈ�� ���� �Լ��� �ּҸ� key�� �����, (�̷μ� C#ȣȯ ����)
    // �׷��� bind�� �Լ��� �Ϲ� �Լ��� �ƴ϶� �Լ� ��ü��.
    // std function�� (boost function��..) �Լ� ��ü�� ����ų �� �ִ� ��ü��.
    // ��������Ʈ�� ���������� std function���� �Լ��� ���ϱ⶧���� �Լ� ��ü�� ����ų �� ����.
    // ������ ������ std function���κ��� �Լ� ��ü�� ����Ű�� ������ �ּҸ� ���� �� ���ٴ� ��
    // �ӽ� ����������, Ű ������ ����� �ּҸ� Add�� �� ���� �־��ֵ��� ����.
    // ���⼭�� ������ �ϳ� �ִ°�, ����Լ��� �ּҴ� C++���� void*�� ĳ������ �ȵ�.
    // �׷��� ��� �Լ��� ����Ű�� �����͸� �ϳ� ����� �� �������� ���� void* �� ĳ�����ؾ���
    // �� �۾��� ���ִ� ���ø� �Լ� GetPtrMemFn�� �������. ����ٶ� (Delegate.h)
    // ��������Ʈ���� �Լ��� ������ ���� Ű������ ���� ������ �ϴ���� Ű ������ ���� ��.
    // �ٸ� ��� �Լ��� �ּҸ� Ű ������ �־��� ������ ������ ���� ������� �ּҸ� �����־�� ��.
    // Ű ���� void* ���·θ� �ѱ�� �ƹ��ų� ���������� ����Լ��� ���ε� �����Ƿ� ����Լ��� �ּҸ� ��õ.
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