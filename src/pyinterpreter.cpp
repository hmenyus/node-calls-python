#include "pyinterpreter.h"
#include <sstream>
#include <iostream>

using namespace nodecallspython;

std::mutex nodecallspython::GIL::m_mutex;
bool nodecallspython::PyInterpreter::inited = false;

#define CHECK(func) { if (func != napi_ok) { napi_throw_error(env, "error", #func); return nullptr; } }

namespace
{
    PyObject* handleInteger(napi_env env, napi_value arg)
    {
        //handle integers
        int32_t i = 0;
        auto res = napi_get_value_int32(env, arg, &i);
        if (res != napi_ok)
        {
            uint32_t i = 0;
            res = napi_get_value_uint32(env, arg, &i);
            if (res != napi_ok)
            {
                int64_t i = 0;
                res = napi_get_value_int64(env, arg, &i);
                if (res == napi_ok)
                    return PyLong_FromLong(i);
                else
                    return nullptr;
            }
            else
                return PyLong_FromLong(i);
        }
        else
            return PyLong_FromLong(i);
    }

    PyObject* convert(napi_env env, napi_value arg)
    {
        napi_valuetype type;
        CHECK(napi_typeof(env, arg, &type));

        bool isarray = false;
        CHECK(napi_is_array(env, arg, &isarray));

        if (isarray)
        {
            uint32_t length = 0;
            CHECK(napi_get_array_length(env, arg, &length));

            auto* list = PyList_New(length);

            for (auto i = 0u; i < length; ++i)
            {
                napi_value value;
                CHECK(napi_get_element(env, arg, i, &value));
                PyList_SetItem(list, i, ::convert(env, value));
            }

            return list;
        }
        else if (type == napi_undefined || type == napi_null)
        {
            Py_INCREF(Py_None);
            return Py_None;
        }
        else if (type == napi_string)
        {
            size_t length = 0;
            CHECK(napi_get_value_string_utf8(env, arg, NULL, 0, &length));
            std::string s(length, ' ');
            CHECK(napi_get_value_string_utf8(env, arg, &s[0], length + 1, &length));

            return PyUnicode_FromString(s.c_str());
        }
        else if (type == napi_number)
        {
            double d = 0.0;
            CHECK(napi_get_value_double(env, arg, &d));
            if ((double) ((int) d) == d)
                return handleInteger(env, arg);
            else
                return PyFloat_FromDouble(d);
        }
        else if (type == napi_boolean)
        {
            bool b = false;
            CHECK(napi_get_value_bool(env, arg, &b));
            return b ? PyBool_FromLong(1) : PyBool_FromLong(0);
        }
        else if (type == napi_object)
        {
            napi_value properties;
            CHECK(napi_get_property_names(env, arg, &properties));

            uint32_t length = 0;
            CHECK(napi_get_array_length(env, properties, &length));

            auto* dict = PyDict_New();

            for (auto i = 0u; i < length; ++i)
            {
                napi_value key;
                CHECK(napi_get_element(env, properties, i, &key));
                
                CPyObject pykey = ::convert(env, key);

                napi_value value;
                CHECK(napi_get_property(env, arg, key, &value));
                CPyObject pyvalue = ::convert(env, value);

                PyDict_SetItem(dict, *pykey, *pyvalue);
            }

            return dict;
        }
        else
            return handleInteger(env, arg);

        return nullptr;
    }
}

CPyObject PyInterpreter::convert(napi_env env, const std::vector<napi_value>& args)
{
    if (args.size() == 0)
        return CPyObject();

    CPyObject params = PyTuple_New(args.size());

    for (auto i=0u;i<args.size();++i)
    {
        auto cparams = ::convert(env, args[i]);
        if (!cparams)
            return CPyObject();

        PyTuple_SetItem(*params, i, cparams);
    }
    return params;
}

namespace
{
    napi_value fillArray(napi_env env, CPyObject& iterator, napi_value array)
    {
        PyObject *item;
        auto i = 0;
        while ((item = PyIter_Next(*iterator))) 
        {
            CHECK(napi_set_element(env, array, i, PyInterpreter::convert(env, item)));
            Py_DECREF(item);
            ++i;
        }

        return array;
    }
}

napi_value PyInterpreter::convert(napi_env env, PyObject* obj)
{
    if (PyBool_Check(obj))
    {
        napi_value result;
        CHECK(napi_get_boolean(env, obj == Py_True, &result));
        return result;
    }
    else if (PyUnicode_Check(obj))
    {
        Py_ssize_t size;
        auto str = PyUnicode_AsUTF8AndSize(obj, &size);

        napi_value result;
        CHECK(napi_create_string_utf8(env, str, size, &result));
        return result;
    }
    else if (PyLong_Check(obj))
    {
        napi_value result;
        CHECK(napi_create_int64(env, PyLong_AsLong(obj), &result));
        return result;
    }
    else if (PyFloat_Check(obj))
    {
        napi_value result;
        CHECK(napi_create_double(env, PyFloat_AsDouble(obj), &result));
        return result;
    }
    else if (PyList_Check(obj))
    {
        auto length = PyList_Size(obj);
        napi_value array;
        CHECK(napi_create_array_with_length(env, length, &array));

        for (auto i = 0u; i < length; ++i)
            CHECK(napi_set_element(env, array, i, convert(env, PyList_GetItem(obj, i))));

        return array;
    }
    else if (PyTuple_Check(obj))
    {
        auto length = PyTuple_Size(obj);
        napi_value array;
        CHECK(napi_create_array_with_length(env, length, &array));

        for (auto i = 0u; i < length; ++i)
            CHECK(napi_set_element(env, array, i, convert(env, PyTuple_GetItem(obj, i))));

        return array;
    }
    else if (PySet_Check(obj))
    {
        auto length = PySet_Size(obj);
        napi_value array;
        CHECK(napi_create_array_with_length(env, length, &array));

        CPyObject iterator = PyObject_GetIter(obj);

        return fillArray(env, iterator, array);
    }
    else if (PyDict_Check(obj))
    {
        napi_value object;
        CHECK(napi_create_object(env, &object));

        PyObject *key, *value;
        Py_ssize_t pos = 0;

        while (PyDict_Next(obj, &pos, &key, &value))
            CHECK(napi_set_property(env, object, convert(env, key), convert(env, value)));

        return object;
    }
    else if (obj == Py_None)
    {
        napi_value undefined;
        CHECK(napi_get_undefined(env, &undefined));
        return undefined;
    }
    else
    {
        CPyObject iterator = PyObject_GetIter(obj);
        if (iterator)
        {
            napi_value array;
            CHECK(napi_create_array(env, &array));

            return fillArray(env, iterator, array);
        }
        else
        {
            napi_value undefined;
            CHECK(napi_get_undefined(env, &undefined));
            return undefined;
        }
    }
}

namespace
{
    static const char MODULE = '@';
    static const char INSTANCE = '#';

    std::string getUUID(bool module, CPyObject& obj)
    {
        std::stringstream ss;
        ss << (module ? MODULE : INSTANCE);
        ss << "nodecallspython-";
        ss << rand() << "-";
        ss << *obj;
        return ss.str();
    }
}

std::string PyInterpreter::import(const std::string& modulename)
{
    auto name = modulename;
    auto pos = name.find_last_of("\\");
    if (pos == std::string::npos)
        pos =  name.find_last_of("/");

    if (pos != std::string::npos)
    {
        auto dir = name.substr(0, pos);
        name = name.substr(pos + 1);

        auto sysPath = PySys_GetObject((char*)"path");
        CPyObject dirName = PyUnicode_FromString(dir.c_str());
        PyList_Append(sysPath, *dirName);
    }

    auto len = name.length();
    if (len > 2 && name[len - 3] == '.' && name[len - 2] == 'p' && name[len - 1] == 'y')
        name = name.substr(0, len - 3);

    CPyObject pyModule = PyImport_ImportModule(name.c_str());

    if(pyModule)
    {
        auto name = getUUID(true, pyModule);
        m_objs[name] = pyModule;
        return name;
    }
    else
        PyErr_Print();
        
    return std::string();
}

CPyObject PyInterpreter::call(const std::string& handler, const std::string& func, CPyObject& args)
{
    auto it = m_objs.find(handler);

    if(it == m_objs.end())
        return CPyObject();

    CPyObject pyFunc = PyObject_GetAttrString(*(it->second), func.c_str());
    if (pyFunc && PyCallable_Check(*pyFunc))
    {
        CPyObject pyResult = PyObject_CallObject(*pyFunc, *args);
        if (!*pyResult)
        {
            PyErr_Print();
            return CPyObject();
        }

        return pyResult;
    }
    else
        PyErr_Print();
    
    return CPyObject();
}

std::string PyInterpreter::create(const std::string& handler, const std::string& name, CPyObject& args)
{
    auto obj = call(handler, name, args);

    if (obj)
    {
        auto uuid = getUUID(false, obj);
        m_objs[uuid] = obj;
        return uuid;
    }

    return std::string();
}

void PyInterpreter::release(const std::string& handler)
{
    m_objs.erase(handler);
}
