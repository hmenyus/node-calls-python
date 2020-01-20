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
        static bool inited;
    public:
        PyInterpreter() : m_state(nullptr)
        {
            if (!inited)
            {
                Py_Initialize();

                if (!PyEval_ThreadsInitialized())
                    PyEval_InitThreads();

                Py_DECREF(PyImport_ImportModule("threading"));

                m_state = PyEval_SaveThread();

                inited = true;
            }
        }

        ~PyInterpreter()
        {
            if (m_state)
            {
                PyEval_RestoreThread(m_state);
                m_objs = {};
                Py_Finalize();
            }
        }

        static CPyObject convert(napi_env env, const std::vector<napi_value>& args);

        static napi_value convert(napi_env env, PyObject* obj);

        std::string import(const std::string& modulename);

        std::string create(const std::string& handler, const std::string& name, CPyObject& args);

        void release(const std::string& handler);
        
        CPyObject call(const std::string& handler, const std::string& func, CPyObject& args);
    };
}
