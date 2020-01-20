#include <node_api.h>
#include <memory>
#include <sstream>
#include <iostream>
#ifndef WIN32
#include <dlfcn.h>
#endif
#include "cpyobject.h"
#include "pyinterpreter.h"

#define DECLARE_NAPI_METHOD(name, func) { name, 0, func, 0, 0, 0, napi_default, 0 }
#define CHECK(func) { if (func != napi_ok) { napi_throw_error(env, "error", #func); return; } }
#define CHECKNULL(func) { if (func != napi_ok) { napi_throw_error(env, "error", #func); return nullptr; } }

namespace nodecallspython
{
    struct BaseTask
    {
        napi_async_work m_work;
        napi_ref m_callback;
        PyInterpreter* m_py;
        napi_env m_env;

        ~BaseTask()
        {
            napi_delete_reference(m_env, m_callback);
            napi_delete_async_work(m_env, m_work);
        }
    };

    struct ImportTask : public BaseTask
    {
        std::string m_name;
        std::string m_handler;
    };

    struct CallTask : public BaseTask
    {
        std::string m_handler;
        std::string m_func;
        bool m_isfunc;
        CPyObject m_args;

        CPyObject m_result;

        ~CallTask()
        {
            GIL gil;
            m_args = CPyObject();
            m_result = CPyObject();
        }
    };

    class Handler
    {
        PyInterpreter* m_py;
        std::string m_handler;
    
    public:
        Handler(PyInterpreter* py, const std::string& handler) : m_py(py), m_handler(handler) {}

        ~Handler()
        {
            GIL gil;
            m_py->release(m_handler);
        }

        static void Destructor(napi_env env, void* nativeObject, void* finalize_hint)
        {
            delete reinterpret_cast<Handler*>(nativeObject);
        }
    };

    napi_value createHandler(napi_env env, PyInterpreter* py, const std::string& stringhandler)
    {
        napi_value key;
        CHECKNULL(napi_create_string_utf8(env, "handler", NAPI_AUTO_LENGTH, &key));

        napi_value handler;
        CHECKNULL(napi_create_string_utf8(env, stringhandler.c_str(), NAPI_AUTO_LENGTH, &handler));

        napi_value result;
        CHECKNULL(napi_create_object(env, &result));

        CHECKNULL(napi_set_property(env, result, key, handler));

        CHECKNULL(napi_add_finalizer(env, result, new Handler(py, stringhandler), Handler::Destructor, nullptr, nullptr));

        return result;
    }

    template<class T>
    napi_value createHandler(napi_env env, T* task)
    {
        return createHandler(env, task->m_py, task->m_handler);
    }

    static void ImportAsync(napi_env env, void* data)
    {
        auto task = static_cast<ImportTask*>(data);
        GIL gil;
        task->m_handler = task->m_py->import(task->m_name);
    }

    static void ImportComplete(napi_env env, napi_status status, void* data)
    {
        std::unique_ptr<ImportTask> task(static_cast<ImportTask*>(data));
        task->m_env = env;

        napi_value global;
        CHECK(napi_get_global(env, &global));

        if (task->m_handler.empty())
        {
            napi_value undefined;
            CHECK(napi_get_undefined(env, &undefined));

            napi_value callback;
            CHECK(napi_get_reference_value(env, task->m_callback, &callback));

            napi_value result;
            CHECK(napi_call_function(env, global, callback, 1, &undefined, &result));
        }
        else
        {
            auto handler = createHandler(env, task.get());

            napi_value callback;
            CHECK(napi_get_reference_value(env, task->m_callback, &callback));

            napi_value result;
            CHECK(napi_call_function(env, global, callback, 1, &handler, &result));
        }
    }

    static void CallComplete(napi_env env, napi_status status, void* data)
    {
        std::unique_ptr<CallTask> task(static_cast<CallTask*>(data));
        task->m_env = env;

        napi_value global;
        CHECK(napi_get_global(env, &global));

        if (task->m_result || (!task->m_isfunc && !task->m_handler.empty()))
        {
            napi_value args;
            if (task->m_isfunc)
            {
                GIL gil;
                args = task->m_py->convert(env, *task->m_result);
            }
            else
            {
                args = createHandler(env, task.get());
            }

            napi_value callback;
            CHECK(napi_get_reference_value(env, task->m_callback, &callback));

            napi_value result;
            CHECK(napi_call_function(env, global, callback, 1, &args, &result));
        }
        else
        {
            napi_value undefined;
            CHECK(napi_get_undefined(env, &undefined));

            napi_value error;
            CHECK(napi_create_string_utf8(env, "Cannot call function", NAPI_AUTO_LENGTH, &error));

            napi_value args[] = {undefined, error};

            napi_value callback;
            CHECK(napi_get_reference_value(env, task->m_callback, &callback));

            napi_value result;
            CHECK(napi_call_function(env, global, callback, 2, args, &result));
        }
    }

    static void CallAsync(napi_env env, void* data)
    {
        auto task = static_cast<CallTask*>(data);
        GIL gil;
        if (task->m_isfunc)
            task->m_result = task->m_py->call(task->m_handler, task->m_func, task->m_args);
        else
            task->m_handler = task->m_py->create(task->m_handler, task->m_func, task->m_args);
    }

    class Python
    {
        napi_env m_env;
        napi_ref m_wrapper;
        static napi_ref constructor;

    public:
        static napi_value Init(napi_env env, napi_value exports)
        {
            napi_property_descriptor properties[] = 
            {
                DECLARE_NAPI_METHOD("import", import),
                DECLARE_NAPI_METHOD("importSync", importSync),
                DECLARE_NAPI_METHOD("call", call),
                DECLARE_NAPI_METHOD("callSync", callSync),
                DECLARE_NAPI_METHOD("create", newclass),
                DECLARE_NAPI_METHOD("createSync", newclassSync),
                DECLARE_NAPI_METHOD("fixlink", fixlink)
            };

            napi_value cons;
            CHECKNULL(napi_define_class(env, "PyInterpreter", NAPI_AUTO_LENGTH, create, nullptr, 7, properties, &cons));

            CHECKNULL(napi_create_reference(env, cons, 1, &constructor));

            CHECKNULL(napi_set_named_property(env, exports, "PyInterpreter", cons));

            return exports;
        }

        static void Destructor(napi_env env, void* nativeObject, void* finalize_hint)
        {
           delete reinterpret_cast<Python*>(nativeObject);
        }

        static napi_value callImpl(napi_env env, napi_callback_info info, bool func) 
        {
            napi_value jsthis;
            size_t argc = 100;
            napi_value args[100];
            CHECKNULL(napi_get_cb_info(env, info, &argc, &args[0], &jsthis, nullptr));

            if (argc < 3)
            {
                napi_throw_error(env, "args", "Must have at least 3 arguments");
                return nullptr;
            }

            Python* obj;
            CHECKNULL(napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj)));

            napi_valuetype handlerT;
            CHECKNULL(napi_typeof(env, args[0], &handlerT));

            napi_valuetype funcT;
            CHECKNULL(napi_typeof(env, args[1], &funcT));

            napi_valuetype callbackT;
            CHECKNULL(napi_typeof(env, args[argc - 1], &callbackT));

            if (handlerT == napi_object && funcT == napi_string && callbackT == napi_function)
            {
                napi_value optname;
                napi_create_string_utf8(env, "Python::import", NAPI_AUTO_LENGTH, &optname);

                CallTask* task = new CallTask;
                task->m_py = &(obj->getInterpreter());

                napi_value key;
                CHECKNULL(napi_create_string_utf8(env, "handler", NAPI_AUTO_LENGTH, &key));

                napi_value value;
                CHECKNULL(napi_get_property(env, args[0], key, &value));

                task->m_handler = convertString(env, value);
                task->m_func = convertString(env, args[1]);
                task->m_isfunc = func;

                std::vector<napi_value> napiargs;
                napiargs.reserve(argc - 3);
                for (auto i=2u;i<argc - 1;++i)
                    napiargs.push_back(args[i]);

                {
                    GIL gil;
                    task->m_args = obj->getInterpreter().convert(env, napiargs);
                }

                CHECKNULL(napi_create_reference(env, args[argc - 1], 1, &task->m_callback));

                CHECKNULL(napi_create_async_work(env, args[1], optname, CallAsync, CallComplete, task, &task->m_work));
                CHECKNULL(napi_queue_async_work(env, task->m_work));
            }
            else
            {
                napi_throw_error(env, "args", "Wrong type of arguments");
            }

            return nullptr;
        }

        static napi_value callImplSync(napi_env env, napi_callback_info info, bool isfunc) 
        {
            napi_value jsthis;
            size_t argc = 100;
            napi_value args[100];
            CHECKNULL(napi_get_cb_info(env, info, &argc, &args[0], &jsthis, nullptr));

            if (argc < 2)
            {
                napi_throw_error(env, "args", "Must have at least 2 arguments");
                return nullptr;
            }

            Python* obj;
            CHECKNULL(napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj)));

            napi_valuetype handlerT;
            CHECKNULL(napi_typeof(env, args[0], &handlerT));

            napi_valuetype funcT;
            CHECKNULL(napi_typeof(env, args[1], &funcT));

            if (handlerT == napi_object && funcT == napi_string)
            {
                napi_value key;
                CHECKNULL(napi_create_string_utf8(env, "handler", NAPI_AUTO_LENGTH, &key));

                napi_value value;
                CHECKNULL(napi_get_property(env, args[0], key, &value));

                auto handler = convertString(env, value);
                auto func = convertString(env, args[1]);

                std::vector<napi_value> napiargs;
                napiargs.reserve(argc - 2);
                for (auto i=2u;i<argc;++i)
                    napiargs.push_back(args[i]);

                GIL gil;
                auto& py = obj->getInterpreter();
                auto args = py.convert(env, napiargs);

                napi_value result;
                if (isfunc)
                {
                    auto pyres = py.call(handler, func, args);
                    if (pyres)
                        result = py.convert(env, *pyres);
                    else
                        CHECKNULL(napi_get_undefined(env, &result));
                }
                else
                {
                    auto newhandler = py.create(handler, func, args);
                    result = createHandler(env, &py, newhandler);
                }

                return result;
            }
            else
            {
                napi_throw_error(env, "args", "Wrong type of arguments");
            }

            return nullptr;
        }
    private:
        std::unique_ptr<PyInterpreter> m_py;
        
        Python(napi_env env) : m_env(env), m_wrapper(nullptr)
        {
            m_py = std::make_unique<PyInterpreter>();
        }

        ~Python()
        {
            napi_delete_reference(m_env, m_wrapper);
        }

        PyInterpreter& getInterpreter() { return *m_py; }
        
        static napi_value create(napi_env env, napi_callback_info info) 
        {
            napi_value target;
            CHECKNULL(napi_get_new_target(env, info, &target));
            auto isConstructor = target != nullptr;

            if (isConstructor) 
            {
                napi_value jsthis;
                CHECKNULL(napi_get_cb_info(env, info, nullptr, 0, &jsthis, nullptr));

                Python* obj = new Python(env);

                CHECKNULL(napi_wrap(env, jsthis, reinterpret_cast<void*>(obj), Python::Destructor, nullptr, &obj->m_wrapper));

                return jsthis;
            } 
            else 
            {
                napi_value jsthis;
                CHECKNULL(napi_get_cb_info(env, info, nullptr, 0, &jsthis, nullptr));

                napi_value cons;
                CHECKNULL(napi_get_reference_value(env, constructor, &cons));

                napi_value instance;
                CHECKNULL(napi_new_instance(env, cons, 0, nullptr, &instance));

                return instance;
            }
        }

        static napi_value import(napi_env env, napi_callback_info info) 
        {
            napi_value jsthis;
            size_t argc = 2;
            napi_value args[2];
            CHECKNULL(napi_get_cb_info(env, info, &argc, &args[0], &jsthis, nullptr));

            if (argc != 2)
            {
                napi_throw_error(env, "args", "Must have 2 arguments");
                return nullptr;
            }

            Python* obj;
            CHECKNULL(napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj)));

            napi_valuetype valuetype1;
            CHECKNULL(napi_typeof(env, args[0], &valuetype1));

            napi_valuetype valuetype2;
            CHECKNULL(napi_typeof(env, args[1], &valuetype2));

            if (valuetype1 == napi_string && valuetype2 == napi_function)
            {
                napi_value optname;
                napi_create_string_utf8(env, "Python::import", NAPI_AUTO_LENGTH, &optname);

                ImportTask* task = new ImportTask;
                task->m_py = &(obj->getInterpreter());
                task->m_name = convertString(env, args[0]);
                CHECKNULL(napi_create_reference(env, args[1], 1, &task->m_callback));

                CHECKNULL(napi_create_async_work(env, args[1], optname, ImportAsync, ImportComplete, task, &task->m_work));
                CHECKNULL(napi_queue_async_work(env, task->m_work));
            }
            else
            {
                napi_throw_error(env, "args", "Wrong type of arguments");
            }

            return nullptr;
        }

        static napi_value importSync(napi_env env, napi_callback_info info) 
        {
            napi_value jsthis;
            size_t argc = 1;
            napi_value args[1];
            CHECKNULL(napi_get_cb_info(env, info, &argc, &args[0], &jsthis, nullptr));

            if (argc != 1)
            {
                napi_throw_error(env, "args", "Must have 1 arguments");
                return nullptr;
            }

            Python* obj;
            CHECKNULL(napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj)));

            napi_valuetype valuetype;
            CHECKNULL(napi_typeof(env, args[0], &valuetype));

            if (valuetype == napi_string)
            {
                auto name = convertString(env, args[0]);
                auto& py = obj->getInterpreter();

                std::string handler;
                {
                    GIL gil;
                    handler = py.import(name);
                }

                napi_value result;
                if (handler.empty())
                    CHECKNULL(napi_get_undefined(env, &result))
                else
                    result = createHandler(env, &py, handler);
                
                return result;
            }
            else
            {
                napi_throw_error(env, "args", "Wrong type of arguments");
            }

            return nullptr;
        }

        static std::string convertString(napi_env env, napi_value value)
        {
            size_t length = 0;
            napi_get_value_string_utf8(env, value, NULL, 0, &length);
            std::string result(length, ' ');
            napi_get_value_string_utf8(env, value, &result[0], length + 1, &length);
            return result;
        }

        static napi_value call(napi_env env, napi_callback_info info) 
        {
            return callImpl(env, info, true); 
        }

        static napi_value callSync(napi_env env, napi_callback_info info) 
        {
            return callImplSync(env, info, true); 
        }

        static napi_value newclass(napi_env env, napi_callback_info info) 
        {
            return callImpl(env, info, false); 
        }

        static napi_value newclassSync(napi_env env, napi_callback_info info) 
        {
            return callImplSync(env, info, false); 
        }

        static napi_value fixlink(napi_env env, napi_callback_info info)
        {
#ifdef WIN32
            return nullptr;
#else
            napi_value jsthis;
            size_t argc = 1;
            napi_value args[1];
            CHECKNULL(napi_get_cb_info(env, info, &argc, &args[0], &jsthis, nullptr));

            if (argc != 1)
            {
                napi_throw_error(env, "args", "Must have 1 arguments");
                return nullptr;
            }

            Python* obj;
            CHECKNULL(napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj)));

            napi_valuetype valuetype;
            CHECKNULL(napi_typeof(env, args[0], &valuetype));

            if (valuetype == napi_string)
            {
                auto filename = convertString(env, args[0]);
                dlopen(filename.c_str(), RTLD_LAZY | RTLD_GLOBAL);
                
                return nullptr;
            }
            else
            {
                napi_throw_error(env, "args", "Wrong type of arguments");
            }

            return nullptr;
#endif
        }
    };

    napi_ref Python::constructor;
}

NAPI_MODULE_INIT()
{
    return nodecallspython::Python::Init(env, exports);
}
