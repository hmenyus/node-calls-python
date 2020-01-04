
![node-calls-python](https://github.com/hmenyus/node-calls-python/blob/master/logo.png)

# node-calls-python - call Python from NodeJS directly in-process without spawning processes

## Suitable for running your ML or deep learning models from Node directly

[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.me/hmenyus)

## Installation
```
npm install node-calls-python
```
## Installation FAQ
Sometimes you have to install prerequisites to make it work.
### **Linux**: install node, npm, node-gyp, python3, python3-dev, g++ and make

#### Install Node
```
sudo apt install curl
curl -sL https://deb.nodesource.com/setup_13.x | sudo -E bash -
sudo apt install nodejs
```

#### Install Python
```
sudo apt install python3
sudo apt install python3-dev
```

#### Install Node-gyp
```
sudo apt install make
sudo apt install g++
sudo npm install -g node-gyp
```

### **Windows**: install [NodeJS](https://nodejs.org/en/download/) and [Python](https://www.python.org/downloads/)
  
#### Install Node-gyp if missing
```
npm install --global --production windows-build-tools
npm install -g node-gyp
```
  
## Examples

### Calling a simple python function
Let's say you have the following python code in **test.py**
```python
import numpy as np

def multiple(a, b):
    return np.multiply(a, b).tolist()
```

Then to call this function directly you can do this in Node
```javascript
const nodecallspython = require("node-calls-python");

let py = nodecallspython.interpreter;

py.import("test.py").then(async function(pymodule) {
    let result = await py.call(pymodule, "multiple", [1, 2, 3, 4], [2, 3, 4, 5]);
    console.log(result);
});
```

Or to call this function by using the synchronous version
```javascript
const nodecallspython = require("node-calls-python");

let py = nodecallspython.interpreter;

py.import("test.py").then(async function(pymodule) {
    let result = py.callSync(pymodule, "multiple", [1, 2, 3, 4], [2, 3, 4, 5]);
    console.log(result);
});
```

### Creating python objects
Let's say you have the following python code in **test.py**
```python
import numpy as np

class Calculator:
    vector = []

    def __init__(self, vector):
        self.vector = vector

    def multiply(self, scalar, vector):
        return np.add(np.multiply(scalar, self.vector), vector).tolist()
```

Then to instance the class directly in Node
```javascript
const nodecallspython = require("node-calls-python");

let py = nodecallspython.interpreter;

py.import("test.py").then(async function(pymodule) {
    let pyobj = await py.create(pymodule, "Calculator", [1.4, 5.5, 1.2, 4.4]);
    let result = await py.call(pyobj, "multiply", 2, [10.4, 50.5, 10.2, 40.4]);
});
```

Or to instance the class synchronously and directly in Node
```javascript
const nodecallspython = require("node-calls-python");

let py = nodecallspython.interpreter;

py.import("test.py").then(async function(pymodule) {
    let pyobj = py.createSync(pymodule, "Calculator", [1.4, 5.5, 1.2, 4.4]);
    let result = await py.callSync(pyobj, "multiply", 2, [10.4, 50.5, 10.2, 40.4]); // you can use ayns version (call) as well
});
```

### Doing some ML with Python and Node
Let's say you have the following python code in **logreg.py**
```python
from sklearn.datasets import load_iris, load_digits
from sklearn.linear_model import LogisticRegression

class LogReg:
    logreg = None

    def __init__(self, dataset):
        if (dataset == "iris"):
            X, y = load_iris(return_X_y=True)
        else:
            X, y = load_digits(return_X_y=True)

        self.logreg = LogisticRegression(random_state=42, solver='lbfgs', multi_class='multinomial')
        self.logreg.fit(X, y)

    def predict(self, X):
        return self.logreg.predict_proba(X).tolist()
```

Then you can do this in Node
```javascript
const nodecallspython = require("node-calls-python");

let py = nodecallspython.interpreter;

py.import("logreg.py")).then(async function(pymodule) { // import the python module
    let logreg = await py.create(pymodule, "LogReg", "iris"); // create the instance of the classifier

    let predict = await py.call(logreg, "predict", [[1.4, 5.5, 1.2, 4.4]]); // call predict
    console.log(predict);
});
```

### [See mode examples here](https://github.com/hmenyus/node-calls-python/tree/master/test)

## Supported data mapping

### From Node to Python
```
  - undefined to None
  - null to None
  - boolean to boolean
  - number to double
  - int32 to long
  - uint32 to long
  - int64 to long
  - string to unicode (string)
  - array to list
  - object to dictionary
```

### From Python to Node
```
  - None to undefined
  - boolean to boolean
  - double to number
  - long to int64
  - unicode (string) to string
  - list to array
  - tuple to array
  - set to array
  - dictionary to object
```
