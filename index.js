const { execSync } = require("child_process");
const fs = require("fs");
const path = require("path");
const nodecallspython = require("./build/Release/nodecallspython");
const chokidar = require("chokidar");

class Interpreter
{
    loadPython(dir)
    {
        const debug = process.env.NODECALLSPYTHON_DEBUG !== undefined;
        if (debug)
            console.log("Loading python from " + dir);

        let found = false;
        if (fs.existsSync(dir))
        {
            fs.readdirSync(dir).forEach(file => {
                if (file.match(/libpython3.*\.so/))
                {
                    try
                    {
                        const filename = path.join(dir, file);
                        if (debug)
                            console.log("Running fixlink on " + filename);

                        this.fixlink(filename);
                        found = true;
                    }
                    catch(e)
                    {
                        console.error(e);
                    }
                }
            });
        }

        if (!found && debug)
            console.log("Not found");

        return found;
    }

    getPythonExecutableImpl(command)
    {
        const stdout = execSync(command);
        if (stdout)
        {
            const result = stdout.toString().trim().split("\n");
            if (result.length > 0)
                return result[0].trim();
        }
        return undefined;
    }

    getPythonExecutable()
    {
        if (process.platform === "win32")
            return this.getPythonExecutableImpl("where python");
        else
            return this.getPythonExecutableImpl("which python3");
    }

    constructor()
    {
        this.py = new nodecallspython.PyInterpreter();
        if (process.platform === "linux")
        {
            const stdout = execSync("python3-config --configdir");
            let found = false;
            if (stdout)
            {
                const dir = stdout.toString().trim();
                const res = this.loadPython(dir);
                if (res)
                    found = true;
            }

            if (!found)
            {
                const stdout = execSync("python3-config --ldflags");
                if (stdout)
                {
                    const split = stdout.toString().trim().split(" ");
                    split.forEach(s => {
                        if (s.startsWith("-L"))
                            this.loadPython(s.substring(2));
                    });
                }
            }
        }

        try
        {
            this.setPythonExecutable(this.getPythonExecutable());
        }
        catch(e)
        {
        }
    }

    import(filename, allowReimport = false)
    {
        return new Promise(function(resolve, reject) {
            try
            {
                this.py.import(filename, allowReimport, function(handler, error) {
                    if (handler)
                        resolve(handler);
                    else
                        reject(error);
                });
            }
            catch(e)
            {
                reject(e);
            }
        }.bind(this));
    }

    importSync(filename, allowReimport = false)
    {
        return this.py.importSync(filename, allowReimport);
    }

    call(handler, func, ...args)
    {
        return new Promise(function(resolve, reject) {
            try
            {
                this.py.call(handler, func, ...args, function(result, error) {
                    if (error)
                        reject(error);
                    else
                        resolve(result);
                });
            }
            catch(e)
            {
                reject(e);
            }
        }.bind(this));
    }

    callSync(handler, func, ...args)
    {
        return this.py.callSync(handler, func, ...args);
    }

    create(handler, func, ...args)
    {
        return new Promise(function(resolve, reject) {
            try
            {
                this.py.create(handler, func, ...args, function(result, error) {
                    if (error)
                        reject(error);
                    else
                        resolve(result);
                });
            }
            catch(e)
            {
                reject(e);
            }
        }.bind(this));
    }

    createSync(handler, func, ...args)
    {
        return this.py.createSync(handler, func, ...args);
    }

    fixlink(filename)
    {
        return this.py.fixlink(filename);
    }

    reimport(directory)
    {
        return this.py.reimport(directory);
    }

    exec(handler, code)
    {
        return new Promise(function(resolve, reject) {
            try
            {
                this.py.exec(handler, code, function(result, error) {
                    if (error)
                        reject(error);
                    else
                        resolve(result);
                });
            }
            catch(e)
            {
                reject(e);
            }
        }.bind(this));
    }

    execSync(handler, code)
    {
        return this.py.execSync(handler, code);
    }

    eval(handler, code)
    {
        return new Promise(function(resolve, reject) {
            try
            {
                this.py.eval(handler, code, function(result, error) {
                    if (error)
                        reject(error);
                    else
                        resolve(result);
                });
            }
            catch(e)
            {
                reject(e);
            }
        }.bind(this));
    }

    evalSync(handler, code)
    {
        return this.py.evalSync(handler, code);
    }

    addImportPath(path)
    {
        return this.py.addImportPath(path);
    }

    setSyncJsAndPyInCallback(syncJsAndPy)
    {
        return this.py.setSyncJsAndPyInCallback(syncJsAndPy);
    }

    setPythonExecutable(executable)
    {
        const escaped = executable.trim().replace(/\\/g, '\\\\\\\\');
        this.py.execSync({}, 'import multiprocessing; multiprocessing.set_executable(\'' + escaped + '\');');
    }

    developmentMode(paths)
    {
        const watcher = chokidar.watch(paths, {
            persistent: true,
            ignoreInitial: true,
        });
        watcher.on("change", (fileName) => {
            const ext = path.extname(fileName);
            if (ext == ".py" || ext == "py")
                this.reimport(fileName);
        });
    }
}

let py = new Interpreter();

module.exports = {
    interpreter: py
}
