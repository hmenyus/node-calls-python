#include "pyinterpreter.h"
#include <sstream>
#include <iostream>
#include <csignal>
#include <cstdlib>

using namespace nodecallspython;

std::mutex nodecallspython::GIL::m_mutex;
bool nodecallspython::PyInterpreter::m_inited = false;
std::mutex nodecallspython::PyInterpreter::m_mutex;

namespace
{
    void signal_handler_int(int)
    {
        std::exit(130);
    }
}

PyInterpreter::PyInterpreter() : m_state(nullptr)
{
    std::lock_guard<std::mutex> l(m_mutex);

    if (!m_inited)
    {
        Py_InitializeEx(0);

#if PY_MINOR_VERSION < 9
        if (!PyEval_ThreadsInitialized())
            PyEval_InitThreads();
#endif

        Py_DECREF(PyImport_ImportModule("threading"));

        m_state = PyEval_SaveThread();

        if (!std::getenv("NODE_CALLS_PYTHON_IGNORE_SIGINT"))
            PyOS_setsig(SIGINT, ::signal_handler_int);

        m_inited = true;
    }
}

PyInterpreter::~PyInterpreter()
{
    {
        GIL gil;
        m_objs = {};
    }

    if (m_state)
    {
        PyEval_RestoreThread(m_state);
        Py_Finalize();
    }
}

#define CHECK(func) { auto res = func; if (res != napi_ok) { throw std::runtime_error(std::string(#func) + " returned with an error: " + std::to_string(static_cast<int>(func))); } }

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
                    throw std::runtime_error("Invalid parameter: unknown type");
            }
            else
                return PyLong_FromLong(i);
        }
        else
            return PyLong_FromLong(i);
    }

    std::pair<PyObject*, bool> convert(napi_env env, napi_value arg)
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
                PyList_SetItem(list, i, ::convert(env, value).first);
            }

            return { list, false };
        }

        bool isarraybuffer = false;
        CHECK(napi_is_arraybuffer(env, arg, &isarraybuffer));
        if (isarraybuffer)
        {
            void* data = nullptr;
            size_t len = 0;
            CHECK(napi_get_arraybuffer_info(env, arg, &data, &len));
            auto* bytes = PyBytes_FromStringAndSize((const char*)data, len);
            return { bytes, false };
        }

        bool isbuffer = false;
        CHECK(napi_is_buffer(env, arg, &isbuffer));
        if (isbuffer)
        {
            void* data = nullptr;
            size_t len = 0;
            CHECK(napi_get_buffer_info(env, arg, &data, &len));
            auto* bytes = PyBytes_FromStringAndSize((const char*)data, len);
            return { bytes, false };
        }

        bool istypedarray = false;
        CHECK(napi_is_typedarray(env, arg, &istypedarray));
        if (istypedarray)
        {
            napi_typedarray_type type;
            void* data = nullptr;
            size_t len = 0;
            CHECK(napi_get_typedarray_info(env, arg, &type, &len, &data, nullptr, nullptr));

            switch (type)
            {
            case napi_int16_array:
            case napi_uint16_array:
                len *= 2;
                break;
            case napi_int32_array:
            case napi_uint32_array:
            case napi_float32_array:
                len *= 4;
                break;
            case napi_float64_array:
            case napi_bigint64_array:
            case napi_biguint64_array:
                len *= 8;
                break;
            default:
                break;
            }

            auto* bytes = PyBytes_FromStringAndSize((const char*)data, len);
            return { bytes, false };
        }

        bool isdataview = false;
        CHECK(napi_is_dataview(env, arg, &isdataview));
        if (isdataview)
        {
            void* data = nullptr;
            size_t len = 0;
            CHECK(napi_get_dataview_info(env, arg, &len, &data, nullptr, nullptr));
            auto* bytes = PyBytes_FromStringAndSize((const char*)data, len);
            return { bytes, false };
        }

        else if (type == napi_undefined || type == napi_null)
        {
            Py_INCREF(Py_None);
            return { Py_None, false };
        }
        else if (type == napi_string)
        {
            size_t length = 0;
            CHECK(napi_get_value_string_utf8(env, arg, NULL, 0, &length));
            std::string s(length, ' ');
            CHECK(napi_get_value_string_utf8(env, arg, &s[0], length + 1, &length));

            return { PyUnicode_FromString(s.c_str()), false };
        }
        else if (type == napi_number)
        {
            double d = 0.0;
            CHECK(napi_get_value_double(env, arg, &d));
            if ((double) ((int) d) == d)
                return { handleInteger(env, arg), false };
            else
                return { PyFloat_FromDouble(d), false };
        }
        else if (type == napi_boolean)
        {
            bool b = false;
            CHECK(napi_get_value_bool(env, arg, &b));
            return { b ? PyBool_FromLong(1) : PyBool_FromLong(0), false };
        }
        else if (type == napi_object)
        {
            napi_value properties;
            CHECK(napi_get_property_names(env, arg, &properties));

            uint32_t length = 0;
            CHECK(napi_get_array_length(env, properties, &length));

            auto kwargs = false;

            auto* dict = PyDict_New();
            for (auto i = 0u; i < length; ++i)
            {
                napi_value key;
                CHECK(napi_get_element(env, properties, i, &key));
                
                napi_value value;
                CHECK(napi_get_property(env, arg, key, &value));

                napi_valuetype keyType;
                CHECK(napi_typeof(env, key, &keyType));
                auto thisKwargs = false;
                if (keyType == napi_string)
                {
                    napi_valuetype valueType;
                    CHECK(napi_typeof(env, value, &valueType));
                    if (valueType == napi_boolean)
                    {
                        size_t length = 0;
                        CHECK(napi_get_value_string_utf8(env, key, NULL, 0, &length));
                        std::string s(length, ' ');
                        CHECK(napi_get_value_string_utf8(env, key, &s[0], length + 1, &length));
                        if (s == "__kwargs")
                        {
                            CHECK(napi_get_value_bool(env, value, &thisKwargs));
                        }
                    }
                }

                if (thisKwargs)
                    kwargs = true;
                else
                {
                    CPyObject pykey = ::convert(env, key).first;

                    CPyObject pyvalue = ::convert(env, value).first;

                    PyDict_SetItem(dict, *pykey, *pyvalue);
                }
            }

            return { dict, kwargs };
        }
        else
            return { handleInteger(env, arg), false };

        throw std::runtime_error("Invalid parameter: unknown type");
    }
}

std::pair<CPyObject, CPyObject> PyInterpreter::convert(napi_env env, const std::vector<napi_value>& args)
{
    std::vector<PyObject*> paramsVect;
    paramsVect.reserve(args.size());
    CPyObject kwparams;
    for (auto i=0u;i<args.size();++i)
    {
        auto cparams = ::convert(env, args[i]);
        if (!cparams.first)
            throw std::runtime_error("Cannot convert #" + std::to_string(i + 1) + " argument");

        if (cparams.second)
            kwparams = cparams.first;
        else
            paramsVect.push_back(cparams.first);
    }

    CPyObject params = PyTuple_New(paramsVect.size());

    for (auto i=0u;i<paramsVect.size();++i)
        PyTuple_SetItem(*params, i, paramsVect[i]);

    return { params, kwparams };
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

    napi_value createArrayBuffer(napi_env env, size_t size, const char* ptr)
    {
        void* data = nullptr;
        napi_value buffer;
        CHECK(napi_create_arraybuffer(env, size, &data, &buffer));
        if (size && ptr)
            memcpy(data, ptr, size);
        return buffer;
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
    else if (PyBytes_Check(obj))
    {
        auto size = PyBytes_Size(obj);
        auto ptr = PyBytes_AsString(obj);

        return createArrayBuffer(env, size, ptr);
    }
    else if (PyByteArray_Check(obj))
    {
        auto size = PyByteArray_Size(obj);
        auto ptr = PyByteArray_AsString(obj);

        return createArrayBuffer(env, size, ptr);
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
            // attempt to force convert to support numpy arrays
            PyErr_Clear();

            // cannot decide between int and double if we do not know the type here, so cast everything to double
            auto value = PyFloat_AsDouble(obj);
            if (!PyErr_Occurred())
            {
                napi_value result;
                CHECK(napi_create_double(env, value, &result));
                return result;
            }
            else
            {
                PyErr_Clear();
                Py_ssize_t size;
                auto str = PyUnicode_AsUTF8AndSize(obj, &size);
                if (str)
                {
                    napi_value result;
                    CHECK(napi_create_string_utf8(env, str, size, &result));
                    return result;
                }
                PyErr_Clear();
            }

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

    void handleException()
    {
        if (PyErr_Occurred())
        {
            PyObject *type, *value, *traceback;
            PyErr_Fetch(&type, &value, &traceback);

            CPyObject error = PyObject_Str(value);

            std::runtime_error err("Unknown python error");
            if (error)
            {
                Py_ssize_t size;
                auto str = PyUnicode_AsUTF8AndSize(*error, &size);
                err = std::runtime_error(str ? str : "Unknown python error");
            }

            PyErr_Restore(type, value, traceback);

            throw err;
        }
    }
}

std::string PyInterpreter::import(const std::string& modulename, bool allowReimport)
{
    auto name = modulename;
    auto pos = name.find_last_of("\\");
    if (pos == std::string::npos)
        pos =  name.find_last_of("/");

    if (pos != std::string::npos)
    {
        auto dir = name.substr(0, pos);
        name = name.substr(pos + 1);

        addImportPath(dir);
    }

    auto len = name.length();
    if (len > 2 && name[len - 3] == '.' && name[len - 2] == 'p' && name[len - 1] == 'y')
        name = name.substr(0, len - 3);
    
    PyErr_Clear();
    CPyObject pyModule = PyImport_ImportModule(name.c_str());
    if (!pyModule)
    {
        handleException();
        return {};
    }

    auto it = m_imports.find(*pyModule);
    if (it != m_imports.end() && allowReimport)
    {
        pyModule = PyImport_ReloadModule(*pyModule);
        if (!pyModule)
        {
            handleException();
            return {};
        }
    }

    auto uuid = getUUID(true, pyModule);
    m_objs[uuid] = pyModule;
    m_imports[*pyModule] = uuid;
    return uuid;
}

CPyObject PyInterpreter::call(const std::string& handler, const std::string& func, CPyObject& args, CPyObject& kwargs)
{
    auto it = m_objs.find(handler);

    if(it == m_objs.end())
        throw std::runtime_error("Cannot find handler: " + handler);

    PyErr_Clear();
    CPyObject pyFunc = PyObject_GetAttrString(*(it->second), func.c_str());
    if (pyFunc && PyCallable_Check(*pyFunc))
    {
        CPyObject pyResult = PyObject_Call(*pyFunc, *args, *kwargs);
        if (!*pyResult)
        {
            handleException();
            throw std::runtime_error("Unknown python error");
        }

        return pyResult;
    }
    else
        handleException();
    
    throw std::runtime_error("Unknown python error");
}

CPyObject PyInterpreter::exec(const std::string& handler, const std::string& code, bool eval)
{
    auto it = m_objs.find(handler);

    if(it == m_objs.end())
        throw std::runtime_error("Cannot find handler: " + handler);

    auto localsPtr = PyModule_GetDict(*(it->second));
    auto globals = CPyObject{PyDict_New()};

    PyErr_Clear();
    CPyObject pyResult = PyRun_String(code.c_str(), eval ? Py_eval_input : Py_file_input, *globals, localsPtr);
    if (!*pyResult)
    {
        handleException();
        throw std::runtime_error("Unknown python error");
    }

    return pyResult;
}

std::string PyInterpreter::create(const std::string& handler, const std::string& name, CPyObject& args, CPyObject& kwargs)
{
    auto obj = call(handler, name, args, kwargs);

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
    if (m_objs.count(handler))
    {
        m_imports.erase(*m_objs[handler]);
        m_objs.erase(handler);
    }
}

void PyInterpreter::addImportPath(const std::string& path)
{
    auto sysPath = PySys_GetObject("path");
    CPyObject dirName = PyUnicode_FromString(path.c_str());
    PyList_Insert(sysPath, 0, *dirName);
}

namespace
{
    std::string normalize(const std::string& s)
    {
        std::string result;
        char lastCh = 'A';
        for (auto& ch : s)
        {
            if (ch == '\\' || ch == '/')
            {
                if (lastCh != '\\' && lastCh != '/')
                    result.push_back('/');
            }
            else
                result.push_back(ch);

            lastCh = ch;
        }

        return result;
    }
}

void PyInterpreter::reimport(const std::string& input)
{
    PyErr_Clear();

    std::vector<std::pair<PyObject*, std::string> > reloadThese;
    auto directory = ::normalize(input);

    auto sysModules = PySys_GetObject("modules");
    if (!sysModules)
    {
        handleException();
        throw std::runtime_error("Unknown python error");
    }

    PyObject *key, *pymodule;
    Py_ssize_t pos = 0;
    while (PyDict_Next(sysModules, &pos, &key, &pymodule))
    {
        if (PyModule_Check(pymodule))
        {
            CPyObject fileName = PyModule_GetFilenameObject(pymodule);
            if (fileName)
            {
                Py_ssize_t size = 0;
                auto str = PyUnicode_AsUTF8AndSize(*fileName, &size);
                auto normalized = ::normalize(str);
                if (normalized.find(directory) != std::string::npos)
                {
                    auto it = m_imports.find(pymodule);
                    if (it != m_imports.end())
                        reloadThese.push_back({ pymodule, it->second });
                    else
                        reloadThese.push_back({ pymodule, std::string() });
                }
            }
        }
    }
    
    PyErr_Clear();
    for (auto& reloadThis : reloadThese)
    {
        CPyObject reloaded = PyImport_ReloadModule(reloadThis.first);
        if (!reloaded)
        {
            handleException();
            return;
        }
        if (!reloadThis.second.empty())
        {
            m_imports[*reloaded] = reloadThis.second;
            m_objs[reloadThis.second] = reloaded;
        }
    }
}
