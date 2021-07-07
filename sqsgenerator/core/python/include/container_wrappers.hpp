//
// Created by dominik on 31.05.21.
//

#ifndef SQSGENERATOR_CONTAINER_WRAPPERS_H
#define SQSGENERATOR_CONTAINER_WRAPPERS_H

#include <boost/python.hpp>


namespace py = boost::python;

namespace sqsgenerator::python::helpers {

    void KeyError() { PyErr_SetString(PyExc_KeyError, "Key not found"); }

    void IndexError() { PyErr_SetString(PyExc_IndexError, "Index out of range"); }

    template<typename T>
    struct std_item {
        typedef typename T::value_type V;

        static V &get(T &x, int i) {
            if (i < 0) i += x.size();
            if (i >= 0 && i < x.size()) return x[i];
            else IndexError();
        }

        static void set(T &x, int i, V &v) {
            if (i < 0) i += x.size();
            if (i >= 0 && i < x.size()) x[i] = v;
            else IndexError();
        }

        static void del(T &x, int i) {
            if (i < 0) i += x.size();
            if (i >= 0 && i < x.size()) x.erase(i);
            else IndexError();
        }

        static int index(T &x, V &v) {
            int i{0};
            for (typename T::const_iterator it = x.begin; it != x.end(); ++it, ++i)
                if (*it == v) return i;
            return -1;
        }

        static void add(T &x, V &v) {
            x.push_back(v);
        }

    };


    template<typename T>
    struct map_item {
        typedef typename T::key_type K;
        typedef typename T::mapped_type V;
        typedef typename T::const_iterator It;

        static V &get(T &x, K &i) {
            if (x.find(i) != x.end()) return x[i];
            KeyError();
        }

        static void set(T &x, K &i, V &v) {
            x[i] = v; // use map autocreation feature
        }

        static void del(T &x, K &i) {
            if (x.find(i) != x.end()) x.erase(i);
            else KeyError();
        }

        static bool in(T &x, K &i) {
            return x.find(i) != x.end();
        }

        static py::list keys(T &x) {
            py::list t;
            for (It it = x.begin; it != x.end(); ++it)
                t.append(it->first);
            return t;
        }

        static py::list values(T &x) {
            py::list t;
            for (It it = x.begin; it != x.end(); ++it)
                t.append(it->second);
            return t;
        }

        static py::list items(T &x) {
            py::list t;
            for (It it = x.begin; it != x.end(); ++it)
                t.append(py::make_tuple(it->first, it->second));
            return t;
        }

        static int index(T &x, K &k) {
            int i = 0;
            for (typename T::const_iterator it = x.begin; it != x.end(); ++it, ++i)
                if (it->first == k) return i;
            return -1;
        }
    };

}


#endif //SQSGENERATOR_CONTAINER_WRAPPERS_H
