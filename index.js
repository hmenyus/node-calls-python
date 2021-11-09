const { execSync } = require("child_process");
const fs = require("fs");
const path = require("path");
const nodecallspython = require("./build/Release/nodecallspython");

class Interpreter
{
    constructor()
    {
        this.py = new nodecallspython.PyInterpreter();
        if (process.platform === "linux")
        {
            const stdout = execSync("python3-config --configdir");
            if (stdout)
            {
                const dir = stdout.toString().trim();
                fs.readdirSync(dir).forEach(file => {
                    if (file.match(/libpython.*\.so/))
                        this.fixlink(path.join(dir, file));
                });
            }
        }
    }

    import(filename)
    {
        return new Promise(function(resolve, reject) {
            this.py.import(filename, function(handler) {
                if (handler)
                    resolve(handler);
                else
                    reject("Cannot load module");
            });
        }.bind(this));
    }

    importSync(filename)
    {
        return this.py.importSync(filename);
    }

    call(handler, func, ...args)
    {
        return new Promise(function(resolve, reject) {
            this.py.call(handler, func, ...args, function(result, error) {
                if (error)
                    reject(error);
                else
                    resolve(result);
            });
        }.bind(this));
    }

    callSync(handler, func, ...args)
    {
        return this.py.callSync(handler, func, ...args);
    }

    create(handler, func, ...args)
    {
        return new Promise(function(resolve, reject) {
            this.py.create(handler, func, ...args, function(result, error) {
                if (error)
                    reject(error);
                else
                    resolve(result);
            });
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
}

let py = new Interpreter();

module.exports = {
    interpreter: py
}
