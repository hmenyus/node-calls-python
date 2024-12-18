import numpy as np
import nodetestre
import multiprocessing

def hello():
    print("hello world")

def dump(a, b):
    print(a + b)

def calc(add, a, b):
    if add:
        return a + b
    else:
        return a * b

def concatenate(s1, s2):
    return s1 + s2

def check(a):
    return a == 42

def multipleNp(a, b):
    return np.multiply(a, b);

def numpyArray():
    return np.array([[1.3, 2.1, 2], [1, 1, 2]])

def multiple(a, b):
    return np.multiply(a, b).tolist()

def multiple2D(a, b):
    return np.matmul(a, b).tolist()

def createtuple():
    return ("aaa", 1, 2.3)

def mergedict(a, b):
    res = {}
    for k in a:
        res[k] = a[k]

    for k in b:
        res[k] = b[k]

    return res

def testException():
    raise RuntimeError("test")

def undefined(un, n):    
    return (un, n, {1, 2, "www"})

def kwargstest(**kwargs):
    return mergedict(kwargs, {"test": 1234});

def kwargstestvalue(value, **kwargs):
    return mergedict(kwargs, {"test": value});

class Calculator:
    vector = []
    value = 1

    def __init__(self, vector, **kwargs):
        self.vector = vector
        if "value" in kwargs:
            self.value = kwargs["value"]

    def multiply(self, scalar, vector):
        return np.add(np.multiply(scalar * self.value, self.vector), vector).tolist()

def testReimport():
    return nodetestre.getVar()

def testBuffer(input, asBA = False):
    a = np.frombuffer(input, dtype=np.single)
    if asBA:
        return bytearray((a * 2).tobytes())
    else:
        return (a * 2).tobytes()

def testBufferEmpty(asBA = False):
    if asBA:
        return bytearray()
    else:
        return bytes()

def testFunction(type, function):
    if type == 0:
        function()
        return 2
    else:
        function(123, [1, 2, 4], {"a": 1, "b": 2})
        function(125)
        return 22
    
def testFunctionPromise(type, function):
    if type == 0:
        res = function()
        return res
    else:
        res = function(123, [1, 2, 4], {"a": 1, "b": 2})
        function(res * 125)
        return res * 22

def compute(i):
    return 2 * i

def testMultiProcessing(len):
    numbers = [(i + 1) for i in range(len)]
    with multiprocessing.Pool(processes=3) as pool:
        results = pool.map(compute, numbers)
    return sum(results)
