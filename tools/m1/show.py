import sys
d=open(sys.argv[1],'rb').read(); N=0x3000; F=len(d)//N
addrs=[int(a,16) for a in sys.argv[2].split(',')]
step=int(sys.argv[3]) if len(sys.argv)>3 else 1
prev=None
for i in range(0,F):
    f=d[i*N:(i+1)*N]; row=tuple(f[a] for a in addrs)
    if row!=prev and (step==0 or i%step==0 or True):
        print(i, ' '.join('%3d'%x for x in row)); prev=row
