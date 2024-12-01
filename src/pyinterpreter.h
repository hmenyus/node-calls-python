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
    public:
        GIL()
        {
            m_gstate = PyGILState_Ensure();
        }

        ~GIL()
        {
            PyGILState_Release(m_gstate);
        }

        GIL(const GIL&) = delete;
        GIL& operator=(const GIL&) = delete;

        GIL(GIL&&) = delete;
        GIL& operator=(GIL&&) = delete;
    };

    class PyInterpreter
    {
        PyThreadState* m_state;
        std::unordered_map<std::string, CPyObject> m_objs;
        std::unordered_map<PyObject*, std::string> m_imports;
        bool m_syncJsAndPy;
        static std::mutex m_mutex;
        static bool m_inited;
    public:
        PyInterpreter();

        ~PyInterpreter();

        std::pair<CPyObject, CPyObject> convert(napi_env env, const std::vector<napi_value>& args, bool isSync);

        napi_value convert(napi_env env, PyObject* obj);

        std::string import(const std::string& modulename, bool allowReimport);

        std::string create(const std::string& handler, const std::string& name, CPyObject& args, CPyObject& kwargs);

        void release(const std::string& handler);
        
        CPyObject call(const std::string& handler, const std::string& func, CPyObject& args, CPyObject& kwargs);

        CPyObject exec(const std::string& handler, const std::string& code, bool eval);

        void addImportPath(const std::string& path);

        void reimport(const std::string& directory);

        void setSyncJsAndPyInCallback(bool syncJsAndPy);
    };
}
