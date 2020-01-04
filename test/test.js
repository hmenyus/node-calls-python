const nodecallspython = require("../");
const path = require("path");

let py = nodecallspython.interpreter;
let pyfile = path.join(__dirname, "nodetest.py");

it("nodecallspython tests", () => {
    py.import(pyfile).then(async function(pymodule) {
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

        await expect(py.call(pymodule, "multiple2D", [[1, 2], [3, 4]], [[2, 3], [4, 5]])).resolves.toEqual([[10, 13], [22, 29]]);

        expect(py.callSync(pymodule, "multiple2D", [[1, 2], [3, 4]], [[2, 3], [4, 5]])).toEqual([[10, 13], [22, 29]]);

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

        await expect(py.call(pymodule, "error")).rejects.toEqual("Cannot call function");
        expect(py.callSync(pymodule, "error")).toEqual(undefined);

        await expect(py.call(pymodule, "dump")).rejects.toEqual("Cannot call function");
        expect(py.callSync(pymodule, "dump")).toEqual(undefined);
        
        await expect(py.call(pymodule, "dump", "a")).rejects.toEqual("Cannot call function");
        expect(py.callSync(pymodule, "dump", "a")).toEqual(undefined);

        let pyobj;
        for (var i=0;i<1000;++i)
            pyobj = await py.create(pymodule, "Calculator", [1.4, 5.5, 1.2, 4.4]);
        await expect(py.call(pyobj, "multiply", 2, [10.4, 50.5, 10.2, 40.4])).resolves.toEqual([13.2, 61.5, 12.6, 49.2]);
        expect(py.callSync(pyobj, "multiply", 2, [10.4, 50.5, 10.2, 40.4])).toEqual([13.2, 61.5, 12.6, 49.2]);

        for (var i=0;i<1000;++i)
            pyobj = py.createSync(pymodule, "Calculator", [1.4, 5.5, 1.2, 4.4]);
        expect(py.callSync(pyobj, "multiply", 2, [10.4, 50.5, 10.2, 40.4])).toEqual([13.2, 61.5, 12.6, 49.2]);
        await expect(py.call(pyobj, "multiply", 2, [10.4, 50.5, 10.2, 40.4])).resolves.toEqual([13.2, 61.5, 12.6, 49.2]);

        await expect(py.create(pymodule, "Calculator2")).rejects.toEqual("Cannot call function");

        await expect(py.import(path.join(__dirname, "error.py"))).rejects.toEqual("Cannot load module");
        await expect(py.call(pymodule, "error")).rejects.toEqual("Cannot call function");
        await expect(py.create(pymodule, "error")).rejects.toEqual("Cannot call function");
    });

});

it("nodecallspython sync test", () => {
    let module = py.importSync(pyfile);
    expect(py.callSync(module, "concatenate", "aaa", "bbb")).toEqual("aaabbb");
});