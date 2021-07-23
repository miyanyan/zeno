#pragma once

#include <cstdio>
#include <memory>

namespace zeno {

template <class T, class = void>
struct poly_base_of : std::false_type {
};

template <class T>
struct poly_base_of<T, std::void_t<typename T::poly_base>> : std::true_type {
    using type = typename T::poly_base;
};

class shared_any {
    struct _AnyBase {
        virtual std::shared_ptr<_AnyBase> clone() const = 0;
        virtual bool assign(_AnyBase const *b) = 0;
        virtual ~_AnyBase() = default;
    };

    template <class T, class = void>
    struct _AnyImpl : _AnyBase {
        T t;

        template <class ...Ts>
        _AnyImpl(Ts &&...ts) : t(std::forward<Ts>(ts)...) {}

        virtual std::shared_ptr<_AnyBase> clone() const {
            return std::make_shared<_AnyImpl<T>>(t);
        }

        virtual bool assign(_AnyBase const *b) {
            auto p = dynamic_cast<_AnyImpl<T> const *>(b);
            if (!p) return false;
            t = p->t;
            return true;
        }
    };

    template <class T>
    struct copiable_unique_ptr : std::unique_ptr<T> {
        using std::unique_ptr<T>::unique_ptr;
        using std::unique_ptr<T>::operator=;

        copiable_unique_ptr &operator=(copiable_unique_ptr const &o) {
            std::unique_ptr<T>::operator=(std::unique_ptr<T>(
                std::make_unique<T>(static_cast<T const &>(*o))));
            return *this;
        }

        copiable_unique_ptr(std::unique_ptr<T> &&o)
            : std::unique_ptr<T>(std::move(o)) {
        }

        copiable_unique_ptr(copiable_unique_ptr const &o)
            : std::unique_ptr<T>(std::make_unique<T>(
                static_cast<T const &>(*o))) {
        }

        operator std::unique_ptr<T> &() { return *this; }
        operator std::unique_ptr<T> const &() const { return *this; }
    };

    template <class T>
    struct _AnyImpl<copiable_unique_ptr<T>,
        std::void_t<typename poly_base_of<T>::type>> : _AnyBase {
        using P = copiable_unique_ptr<typename poly_base_of<T>::type>;

        P t;

        template <class ...Ts>
        _AnyImpl(Ts &&...ts) : t(std::forward<Ts>(ts)...) {}

        virtual std::shared_ptr<_AnyBase> clone() const {
            return std::make_shared<_AnyImpl<P>>(t);
        }

        virtual bool assign(_AnyBase const *b) {
            auto p = dynamic_cast<_AnyImpl<P> const *>(b);
            if (!p) return false;
            t = p->t;
            return true;
        }
    };

    std::shared_ptr<_AnyBase> m_ptr;

    shared_any(std::shared_ptr<_AnyBase> ptr)
        : m_ptr(ptr) {}

public:
    shared_any(std::nullptr_t = nullptr) : m_ptr(nullptr) {}
    ~shared_any() = default;

    shared_any(shared_any &&a) = default;
    shared_any(shared_any const &a)
        : m_ptr(a.m_ptr)
    {}

    shared_any &operator=(std::nullptr_t) {
        m_ptr = nullptr;
        return *this;
    }

    shared_any &operator=(shared_any const &a) {
        m_ptr = a.m_ptr;
        return *this;
    }

    template <class T, std::enable_if_t<poly_base_of<T>::value, int> = 0>
    T *cast() const {
        using P = copiable_unique_ptr<typename poly_base_of<T>::type>;
        auto p = dynamic_cast<_AnyImpl<P> *>(m_ptr.get());
        printf("!?%p %s %s\n", p, typeid(*m_ptr).name(),
                typeid(_AnyImpl<P>).name());
        if (!p) return nullptr;
        return dynamic_cast<T *>(p->t.get());
    }

    template <class T, std::enable_if_t<!poly_base_of<T>::value, int> = 0>
    T *cast() const {
        auto p = dynamic_cast<_AnyImpl<T> *>(m_ptr.get());
        if (!p) return nullptr;
        return &p->t;
    }

    template <class T, std::enable_if_t<poly_base_of<T>::value, int> = 0>
    T *unsafe_cast() const {
        using P = copiable_unique_ptr<typename poly_base_of<T>::type>;
        auto p = static_cast<_AnyImpl<P> *>(m_ptr.get());
        return static_cast<T *>(p->t.get());
    }

    template <class T, std::enable_if_t<!poly_base_of<T>::value, int> = 0>
    T *unsafe_cast() const {
        auto p = static_cast<_AnyImpl<T> *>(m_ptr.get());
        return &p->t;
    }

    template <class T>
    T &get() const {
        return *cast<T>();
    }

    operator bool() const {
        return (bool)m_ptr;
    }

    void assign(shared_any const &a) const {
        m_ptr->assign(a.m_ptr.get());
    }

    shared_any clone() const {
        return {m_ptr->clone()};
    }

    template <class T, class ...Ts,
             std::enable_if_t<!poly_base_of<T>::value, int> = 0>
    void emplace(Ts &&...ts) {
        m_ptr = std::make_shared<_AnyImpl<T>>(std::forward<Ts>(ts)...);
    }

    template <class T, class ...Ts,
             std::enable_if_t<poly_base_of<T>::value, int> = 0>
    void emplace(Ts &&...ts) {
        using P = copiable_unique_ptr<typename poly_base_of<T>::type>;
        m_ptr = std::make_shared<_AnyImpl<P>>(std::forward<Ts>(ts)...);
    }

    template <class T, class ...Ts>
    static shared_any make(Ts &&...ts) {
        shared_any a;
        a.emplace<T>(std::forward<Ts>(ts)...);
        return a;
    }
};

template <class T>
struct shared_cast {
    shared_any m_any;

    shared_cast() : m_any(nullptr) {}
    shared_cast(shared_any const &a) {
        *this = a;
    }

    shared_cast(shared_cast &&a) = default;
    shared_cast(shared_cast const &a) = default;
    ~shared_cast() = default;

    shared_cast &operator=(shared_any const &a) {
        if (!a.cast<T>())
            m_any = nullptr;
        else
            m_any = a;
    }

    T *get() const {
        return m_any.unsafe_cast<T>();
    }

    T *operator->() const {
        return get();
    }

    T &operator*() const {
        return *get();
    }

    operator bool() const {
        return (bool)m_any;
    }
};

}
