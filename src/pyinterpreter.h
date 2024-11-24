#pragma once
#include <node_api.h>
#include "cpyobject.h"
#include <vector>
#include <string>
#include <mutex>
#include <unordered_map>
#include <iostream>

namespace nodecallspython
{
    class GIL
    {
        PyGILState_STATE m_gstate;
        static std::mutex m_mutex;
    public:
        GIL()
        {
            m_mutex.lock();
            m_gstate = PyGILState_Ensure();
        }

        ~GIL()
        {
            PyGILState_Release(m_gstate);
            m_mutex.unlock();
        }
    };

    using PyFunctionData = std::pair<CPyObject, std::unique_ptr<napi_threadsafe_function> >;

    struct PyParameters
    {
        CPyObject args;
        CPyObject kwargs;
        std::vector<PyFunctionData> functions;

        PyParameters() = default;

        PyParameters(CPyObject&& args, CPyObject&& kwargs, std::vector<PyFunctionData>&& functions) : 
            args(std::move(args)), kwargs(std::move(kwargs)), functions(std::move(functions))
        {
        }

        ~PyParameters()
        {
            args = CPyObject();
            kwargs = CPyObject();
            for (auto& func : functions)
            {
                func.first = CPyObject();
                napi_release_threadsafe_function(*func.second, napi_tsfn_abort);
            }
            functions.clear();
        }

        void operator=(PyParameters&& other)
        {
            if (this != &other)
            {
                args = std::move(other.args);
                kwargs = std::move(other.kwargs);
                functions = std::move(other.functions);
            }
        }
    };

    class PyInterpreter
    {
        PyThreadState* m_state;
        std::unordered_map<std::string, CPyObject> m_objs;
        std::unordered_map<PyObject*, std::string> m_imports;
        static std::mutex m_mutex;
        static bool m_inited;
    public:
        PyInterpreter();

        ~PyInterpreter();

        static PyParameters convert(napi_env env, const std::vector<napi_value>& args);

        static napi_value convert(napi_env env, PyObject* obj);

        std::string import(const std::string& modulename, bool allowReimport);

        std::string create(const std::string& handler, const std::string& name, PyParameters& params);

        void release(const std::string& handler);
        
        CPyObject call(const std::string& handler, const std::string& func, PyParameters& params);

        CPyObject exec(const std::string& handler, const std::string& code, bool eval);

        void addImportPath(const std::string& path);

        void reimport(const std::string& directory);
    };
}
