from __future__ import division

from math import log
from scipy.special import binom
import numpy as np

def C(p, p0):
    p1 = 1 - p0
    return -p0*log(p0, 2) + p0*p*log(p0*p, 2) - (p1+p0*p)*log(p1+p0*p, 2)

def P(c, p0, eps=0.00001):
    left = 0
    right = 1
    while right - left > eps:
        p = (left + right) / 2
        cp = C(p, p0)
        if cp > c:
            left = p
        else:
            right = p
    return left

def coef(i, p):
    return binom(N, i) * p**i*(1-p)**(N-i) 

def A(c, N, M):
    points = (np.array(xrange(M)) + 1) / (M + 1)
    A = np.matrix([np.array([coef(i, p) for i in xrange(N)]) for p in points])
    b = np.array([P(c, p) for p in points])
    a, _, _, _ = np.linalg.lstsq(A, b)
    return a

if __name__ == '__main__':
    N = 10 # Buffer size
    M = 100 # Num of points
    c = 0.15
    
    a = A(c, N)
    p0 = 0.7
    x = np.array([coef(i, p0) for i in xrange(N)])
    print(np.dot(a, x))
    print(P(c, p0))
    print(a)
