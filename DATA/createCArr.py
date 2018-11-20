import sys

datadir = sys.argv[1]
fnames = ['m1', 'm2', 'm3', 'm4', 'm5', 'b1', 'b2', 'b3', 'b4', 'b5'] 
uniques = []
for fname in fnames:
   with open('%s/%s.txt' % (datadir, fname)) as f: lines = f.read().strip()
   tokens = lines.split(",")
   tokens = tokens[:-1]
   s = "static int %s[%d] = {" % (fname, len(tokens))
   for o in tokens: 
      if not o in uniques: uniques.append(o)
      s+= ("%d,"  % int(o))
   s = s[:-1] + "};"
   print(s)

print("static const int M = %d;" % len(uniques))
