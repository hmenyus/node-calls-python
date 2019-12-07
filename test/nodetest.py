import numpy as np

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

def undefined(un, n):    
    return (un, n, {1, 2, "www"})

class Calculator:
    vector = []

    def __init__(self, vector):
        self.vector = vector

    def multiply(self, scalar, vector):
        return np.add(np.multiply(scalar, self.vector), vector).tolist()
