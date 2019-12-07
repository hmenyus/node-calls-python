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
