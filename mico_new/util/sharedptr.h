#ifndef SHARED_PTR___H
#define SHARED_PTR___H
#include <memory>
#include <assert.h>

using std::shared_ptr;
using std::dynamic_pointer_cast;
using std::static_pointer_cast;

#if 0
template<typename T>
class shared_ptr
{
public:
    explicit shared_ptr(T *p) : m_ref(new int(1)), m_ptr(p)
    {}

    shared_ptr(const shared_ptr<T> &ot)
        : m_ref(ot.m_ref), m_ptr(ot.m_ptr)
    {
        incref();
    }

    //template<typename C>
    //shared_ptr(const shared_ptr<C> &ptr)
    //{
    //    T *tmp = dynamic_cast<T *>(ptr.ptr());
    //    if (this != &ptr && tmp != 0)
    //    {
    //        decref();
    //        m_ptr = ptr.m_ptr;
    //        m_ref = ptr.m_ref;
    //        incref();
    //    }
    //}

    shared_ptr<T> & operator = (const shared_ptr<T> &ot)
    {
        if (this != &ot)
        {
            decref();
            m_ptr = ot.m_ptr;
            m_ref = ot.m_ref;
            incref();
        }
        return *this;
    }

    ~shared_ptr()
    {
        decref();
    }

    T *ptr() const
    {
        return m_ptr;
    }

    T &operator * () const
    {
        return *m_ptr;
    }
    T *operator -> () const
    {
        return m_ptr;
    }

    operator bool ()
    {
        return m_ptr != 0;
    }
private:
    void decref()
    {
        (*m_ref)--;
        if ((*m_ref) <= 0)
            delete m_ptr;
    }

    void incref()
    {
        (*m_ref)++;
    }

    int *m_ref;
    T *m_ptr;

    template<typename A, typename C>
    friend shared_ptr<A> dynamic_pointer_cast(const shared_ptr<C> &o);
};

template<typename A, typename C>
shared_ptr<A> dynamic_pointer_cast(const shared_ptr<C> &o)
{
    A *tmp = dynamic_cast<A *>(o.ptr());
    assert(tmp != 0);
    shared_ptr<A> a(0);
    delete a.m_ref;
    a.m_ref = o.m_ref;
    a.m_ptr = tmp;
    a.incref();

    return a;
}
#endif

#endif
