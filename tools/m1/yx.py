import sys
# Rider x, jump counter, speed and height from a rec.mjs trace.
# Height is the screen line of the sprite's top: MODE 2 at &3000, 640 bytes per character row.
d=open(sys.argv[1],'rb').read(); N=0x3000; F=len(d)//N
prev=None
for i in range(F):
    f=d[i*N:(i+1)*N]
    x=f[0xc10]; addr=f[0xc17]*256+f[0xc16]; ln=f[0xc18]
    y=((addr-0x3000)//640)*8+ln
    t=(x,f[0xc12],f[0xc13],y)
    if t!=prev: print(i,*t); prev=t
