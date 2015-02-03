
# coding: utf-8

# In[55]:

import random
A = [random.random() for x in range(16)]


# In[3]:

print A


# In[56]:

def prefixSum(A):
    B = A[:]
    for i in range(1, len(A)):
        B[i] = B[i-1] + B[i]
    return B
print prefixSum(A)


# In[58]:

import math
def prefixSum_parallel1(A):
    steps = int(math.log(len(A), 2))
    B = A[:]
    
    for i in range(steps):
        stride = 2**i
        nblocks = len(A)/stride
        # broadcast the value at B[j*stride-1] to next block
        for j in range(nblocks):
            bidx = (j+1) * stride
            if j % 2 == 0:
                for k in range(stride):
                    B[bidx+k] = B[bidx+k] + B[bidx-1]
    return B
print prefixSum_parallel1(A)
print prefixSum(A)


# In[62]:

def prefixSum_parallel2(A):
    steps = int(math.log(len(A), 2))
    B = A[:]                    
                
    for i in range(steps):
        stride = 2**(i+1)
        gap = 2**i
        nblocks = len(A)/stride
        for j in range(nblocks):
            B[(j+1)*stride-1] = B[(j+1)*stride-1] + B[(j+1)*stride-1-gap]
    
    C = B[:]
    B[len(A)-1] = 0
    for i in range(steps):
        stride = 2**(steps-i)
        gap = stride/2
        nblocks = len(A)/stride
        for j in range(nblocks):
            tmp = B[(j+1)*stride-1]
            B[(j+1)*stride-1] = tmp + B[(j+1)*stride-1-gap]
            # assign it
            B[(j+1)*stride-1-gap] = tmp
    for i in range(len(A)-1):
        C[i] = B[i+1]
    return C
print prefixSum_parallel2(A)
print prefixSum(A)
        
        


# In[81]:

def prefixSum_nprocs(A, n):
    blockSize = int(math.ceil(len(A) / n))
    S = []
    Sis = []
    for i in range(n+1):
        bstart = blockSize * i
        bend = min(blockSize * (i+1), len(A))
        print bstart, bend
        Si = prefixSum(A[bstart:bend])
        print Si
        Sis.append(Si[-1])
        S.append(Si)
        
    S2 = [0]
    S2.extend(prefixSum(Sis))
    Sfinal = []
    for i in range(n+1):
        for j in range(len(S[i])):
            Sfinal.append(S[i][j] + S2[i])
    return Sfinal

print prefixSum_nprocs(A, 3)
print prefixSum(A)


# In[ ]:



