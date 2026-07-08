const { interpreter: py } = require("node-calls-python");
const { join } = require("path");

test("calls python", () => {
  const pyfile = join(__dirname, "nodetest.py");
  const pymodule = py.importSync(pyfile);

  console.log(py.callSync(pymodule, "hello"));
  console.log(py.callSync(pymodule, "concatenate", "aaa", "bbb"));

  expect(py.callSync(pymodule, "concatenate", "aaa", "bbb")).toBe("aaabbb");
});