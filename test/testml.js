const nodecallspython = require("../");
const path = require("path");

let py = nodecallspython.interpreter;

py.import(path.join(__dirname, "logreg.py")).then(async function(pymodule) { // import the python module
    let logreg = await py.create(pymodule, "LogReg", "iris"); // create the instance of the classifier

    let predict = await py.call(logreg, "predict", [[1.4, 5.5, 1.2, 4.4]]); // call predict
    console.log(predict);
});
