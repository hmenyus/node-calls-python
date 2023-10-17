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

        static std::pair<CPyObject, CPyObject> convert(napi_env env, const std::vector<napi_value>& args);

        static napi_value convert(napi_env env, PyObject* obj);

        std::string import(const std::string& modulename, bool allowReimport);

        std::string create(const std::string& handler, const std::string& name, CPyObject& args, CPyObject& kwargs);

        void release(const std::string& handler);
        
        CPyObject call(const std::string& handler, const std::string& func, CPyObject& args, CPyObject& kwargs);

        CPyObject exec(const std::string& handler, const std::string& code, bool eval);

        void addImportPath(const std::string& path);
    };
}
