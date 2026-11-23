import sys
from PIL import Image
out=sys.argv[1]; files=sys.argv[2:]
ims=[Image.open(f).convert('RGB') for f in files]
w,h=ims[0].size; s=0.5; cols=2
tw,th=int(w*s),int(h*s); rows=(len(ims)+cols-1)//cols
G=Image.new('RGB',(tw*cols,th*rows),(40,40,40))
for i,im in enumerate(ims): G.paste(im.resize((tw,th)),((i%cols)*tw,(i//cols)*th))
G.save(out)
