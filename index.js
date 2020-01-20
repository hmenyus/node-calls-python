const nodecallspython = require("./build/Release/nodecallspython");

class Interpreter
{
    constructor()
    {
        this.py = new nodecallspython.PyInterpreter();
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
