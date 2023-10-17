const nodecallspython = require("../");
const path = require("path");
const { Worker } = require('worker_threads');

let py = nodecallspython.interpreter;
let pyfile = path.join(__dirname, "nodetest.py");

let pymodule = py.importSync(pyfile);

it("nodecallspython tests", async () => {
    await py.call(pymodule, "hello");
    await py.call(pymodule, "dump", "a", "b");

    await expect(py.call(pymodule, "calc", true, 2, 3)).resolves.toEqual(5);
    await expect(py.call(pymodule, "calc", false, 2, 3)).resolves.toEqual(6);

    expect(py.callSync(pymodule, "calc", true, 2, 3)).toEqual(5);
    expect(py.callSync(pymodule, "calc", false, 2, 3)).toEqual(6);

    await expect(py.call(pymodule, "concatenate", "aaa", "bbb")).resolves.toEqual("aaabbb");

    expect(py.callSync(pymodule, "concatenate", "aaa", "bbb")).toEqual("aaabbb");

    await expect(py.call(pymodule, "check", 42)).resolves.toEqual(true);
    await expect(py.call(pymodule, "check", 43)).resolves.toEqual(false);

    expect(py.callSync(pymodule, "check", 42)).toEqual(true);
    expect(py.callSync(pymodule, "check", 43)).toEqual(false);

    await expect(py.call(pymodule, "multiple", [1, 2, 3, 4], [2, 3, 4, 5])).resolves.toEqual([2, 6, 12, 20]);

    expect(py.callSync(pymodule, "multiple", [1, 2, 3, 4], [2, 3, 4, 5])).toEqual([2, 6, 12, 20]);

    await expect(py.call(pymodule, "multipleNp", [1, 2, 3, 4], [2, 3, 4, 5])).resolves.toEqual([2, 6, 12, 20]);

    expect(py.callSync(pymodule, "multipleNp", [1, 2, 3, 4], [2, 3, 4, 5])).toEqual([2, 6, 12, 20]);

    await expect(py.call(pymodule, "multiple2D", [[1, 2], [3, 4]], [[2, 3], [4, 5]])).resolves.toEqual([[10, 13], [22, 29]]);

    expect(py.callSync(pymodule, "multiple2D", [[1, 2], [3, 4]], [[2, 3], [4, 5]])).toEqual([[10, 13], [22, 29]]);

    await expect(py.call(pymodule, "numpyArray")).resolves.toEqual([[1.3, 2.1, 2], [1, 1, 2]]);

    expect(py.callSync(pymodule, "numpyArray")).toEqual([[1.3, 2.1, 2], [1, 1, 2]]);

    await expect(py.call(pymodule, "createtuple")).resolves.toEqual(["aaa", 1, 2.3 ]);

    expect(py.callSync(pymodule, "createtuple")).toEqual(["aaa", 1, 2.3 ]);

    let res = await py.call(pymodule, "undefined", undefined, null)
    expect(res[0]).toEqual(undefined);
    expect(res[1]).toEqual(undefined);
    expect(res[2].length).toEqual(3);
    expect(res[2].includes(1)).toEqual(true);
    expect(res[2].includes(2)).toEqual(true);
    expect(res[2].includes("www")).toEqual(true);

    let obj1 = {
        a: 1,
        b: [1, 2, "rrr"],
        c: {
            test: 34,
            array: ["a", "b"]
        }
    };

    let obj2 = {
        aa: {
            "test": 56,
            4: ["a", {a: 3}]
        }
    };

    for (var i=0;i<10000;++i)
        res = await py.call(pymodule, "mergedict", obj1, obj2);

    expect(i).toEqual(10000);

    obj1["aa"] = obj2["aa"];

    expect(JSON.stringify(res)).toEqual(JSON.stringify(obj1));

    for (var i=0;i<10000;++i)
        res = py.callSync(pymodule, "mergedict", obj1, obj2);

    expect(i).toEqual(10000);

    expect(JSON.stringify(res)).toEqual(JSON.stringify(obj1));

    let pyobj;
    for (let i=0;i<1000;++i)
        pyobj = await py.create(pymodule, "Calculator", [1.4, 5.5, 1.2, 4.4]);
    await expect(py.call(pyobj, "multiply", 2, [10.4, 50.5, 10.2, 40.4])).resolves.toEqual([13.2, 61.5, 12.6, 49.2]);
    expect(py.callSync(pyobj, "multiply", 2, [10.4, 50.5, 10.2, 40.4])).toEqual([13.2, 61.5, 12.6, 49.2]);

    for (let i=0;i<1000;++i)
        pyobj = await py.create(pymodule, "Calculator", [1.4, 5.5, 1.2, 4.4], { "value": 2, "__kwargs": true });
    await expect(py.call(pyobj, "multiply", 2, [10.4, 50.5, 10.2, 40.4])).resolves.toEqual([16, 72.5, 15, 58]);
    expect(py.callSync(pyobj, "multiply", 2, [10.4, 50.5, 10.2, 40.4])).toEqual([16, 72.5, 15, 58]);

    for (let i=0;i<1000;++i)
        pyobj = py.createSync(pymodule, "Calculator", [1.4, 5.5, 1.2, 4.4]);
    expect(py.callSync(pyobj, "multiply", 2, [10.4, 50.5, 10.2, 40.4])).toEqual([13.2, 61.5, 12.6, 49.2]);
    await expect(py.call(pyobj, "multiply", 2, [10.4, 50.5, 10.2, 40.4])).resolves.toEqual([13.2, 61.5, 12.6, 49.2]);

    for (let i=0;i<1000;++i)
        pyobj = py.createSync(pymodule, "Calculator", [1.4, 5.5, 1.2, 4.4], { "value": 2, "__kwargs": true });
    expect(py.callSync(pyobj, "multiply", 2, [10.4, 50.5, 10.2, 40.4])).toEqual([16, 72.5, 15, 58]);
    await expect(py.call(pyobj, "multiply", 2, [10.4, 50.5, 10.2, 40.4])).resolves.toEqual([16, 72.5, 15, 58]);

    await expect(py.exec(pymodule, "concatenate(\"aaa\", \"bbb\")")).resolves.toEqual(undefined);
    expect(py.execSync(pymodule, "concatenate(\"aaa\", \"bbb\")")).toEqual(undefined);

    await expect(py.eval(pymodule, "concatenate(\"aaa\", \"bbb\")")).resolves.toEqual("aaabbb");
    expect(py.evalSync(pymodule, "concatenate(\"aaa\", \"bbb\")")).toEqual("aaabbb");

    expect(py.callSync(pymodule, "kwargstest", { "obj1": obj1, "__kwargs": true })).toEqual({"obj1": obj1, "test": 1234});
    await expect(py.call(pymodule, "kwargstest", { "obj1": obj1, "__kwargs": true })).resolves.toEqual({"obj1": obj1, "test": 1234});

    expect(py.callSync(pymodule, "kwargstestvalue", 54321, { "obj1": obj1, "__kwargs": true })).toEqual({"obj1": obj1, "test": 54321});
    await expect(py.call(pymodule, "kwargstestvalue", 54321, {"obj1": obj1, "__kwargs": true })).resolves.toEqual({"obj1": obj1, "test": 54321});
});

it("nodecallspython async import", () => {
    expect(py.import(pyfile)).resolves.toMatchObject({handler: expect.stringMatching("@nodecallspython-")});
});

it("nodecallspython errors", async () => {
    await expect(py.call(pymodule, "error")).rejects.toEqual("module 'nodetest' has no attribute 'error'");
    expect(() => py.callSync(pymodule, "error")).toThrow("module 'nodetest' has no attribute 'error'");

    expect(() => py.callSync(pymodule, function(){})).toThrow("Wrong type of arguments");
    expect(() => py.callSync(pymodule, "dump", function(){})).toThrow("Invalid parameter: unknown type");
    await expect(py.call(pymodule, "dump", function(){})).rejects.toThrow("Invalid parameter: unknown type");

    await expect(py.call(pymodule, "dump")).rejects.toEqual("dump() missing 2 required positional arguments: 'a' and 'b'");
    expect(() => py.callSync(pymodule, "dump")).toThrow("dump() missing 2 required positional arguments: 'a' and 'b'");

    await expect(py.call(pymodule, "dump", "a")).rejects.toEqual("dump() missing 1 required positional argument: 'b'");
    expect(() => py.callSync(pymodule, "dump", "a")).toThrow("dump() missing 1 required positional argument: 'b'");

    await expect(py.call(pymodule, "testException")).rejects.toEqual("test");
    expect(() => py.callSync(pymodule, "testException")).toThrow("test");

    await expect(py.create(pymodule, "Calculator2")).rejects.toEqual("module 'nodetest' has no attribute 'Calculator2'");
    expect(() => py.createSync(pymodule, "Calculator2")).toThrow("module 'nodetest' has no attribute 'Calculator2'");
    expect(() => py.createSync(pymodule, "Calculator2", function(){})).toThrow("Invalid parameter: unknown type");
    await expect(py.create(pymodule, "Calculator2", function(){})).rejects.toThrow("Invalid parameter: unknown type");

    await expect(py.import(path.join(__dirname, "error.py"))).rejects.toEqual("No module named 'error'");
    expect(() => py.importSync(path.join(__dirname, "error.py"))).toThrow("No module named 'error'");

    await expect(py.exec(pymodule, "dump(12)")).rejects.toEqual("dump() missing 1 required positional argument: 'b'");
    expect(() => py.execSync(pymodule, "dump(12)")).toThrow("dump() missing 1 required positional argument: 'b'");
    await expect(py.exec(pymodule, function(){})).rejects.toThrow("Wrong type of arguments");
    expect(() => py.execSync(pymodule, function(){})).toThrow("Wrong type of arguments");

    await expect(py.eval(pymodule, "dump(12)")).rejects.toEqual("dump() missing 1 required positional argument: 'b'");
    expect(() => py.evalSync(pymodule, "dump(12)")).toThrow("dump() missing 1 required positional argument: 'b'");
    await expect(py.eval(pymodule, function(){})).rejects.toThrow("Wrong type of arguments");
    expect(() => py.evalSync(pymodule, function(){})).toThrow("Wrong type of arguments");

    async function testError(func, end = 1000)
    {
        let count = 0;
        for (let i=0;i<end;++i)
        {            
            try
            {
                await func();
            }
            catch(e)
            {
                ++count;
            }
        }
        expect(count).toEqual(end);
    }

    await testError(() => { return py.call(pymodule, "error"); });

    await testError(() => { return py.call(pymodule, "testException"); });

    await testError(() => { return py.create(pymodule, "Calculator2"); });

    await testError(() => { return py.import(path.join(__dirname, "error.py")); }, 10);

    function testErrorSync(func, end = 1000)
    {
        let count = 0;
        for (let i=0;i<end;++i)
        {            
            try
            {
                func();
            }
            catch(e)
            {
                ++count;
            }
        }
        expect(count).toEqual(end);
    }

    testErrorSync(() => { return py.callSync(pymodule, "error"); });

    testErrorSync(() => { return py.callSync(pymodule, "testException"); });

    testErrorSync(() => { return py.createSync(pymodule, "Calculator2"); });

    testErrorSync(() => { return py.importSync(path.join(__dirname, "error.py")); }, 10);
});

it("nodecallspython worker", () => {
    const worker1 = new Worker(path.join(__dirname, "worker.js"));
    const worker2 = new Worker(path.join(__dirname, "worker.js"));
});

it("nodecallspython import", async () => {
    let pymodule = py.importSync(pyfile);
    expect(py.importSync(pyfile, false)).not.toEqual(pymodule);
    expect(py.importSync(pyfile, true)).not.toEqual(pymodule);

    await expect(py.import(pyfile, false)).resolves.not.toEqual(pymodule);
    await expect(py.import(pyfile, true)).resolves.not.toEqual(pymodule);
});
