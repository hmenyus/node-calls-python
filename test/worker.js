const nodecallspython = require("../");
const path = require("path");

let py = nodecallspython.interpreter;
let pyfile = path.join(__dirname, "nodetest.py");

let pymodule = py.importSync(pyfile);

console.log(py.evalSync(pymodule, "concatenate(\"aaa\", \"bbb\")"));

