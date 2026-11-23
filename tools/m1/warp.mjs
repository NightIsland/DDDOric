import { fresh, shot, save, load } from "./h.mjs";
const [snap, out, n="1"] = process.argv.slice(2);
const s = await fresh(); load(s, snap);
for (let k=0;k<+n;k++){
  s.writeMemory(0xc10,[74]); s.writeMemory(0xc11,[0xe0]); s.writeMemory(0xc13,[60]);
  for (let i=0;i<40;i++){ await s.runFrames(5); if (s.readMemory(0xc11,1)[0]==0x38 && s.readMemory(0xc10,1)[0]<3) break; }
}
await s.runFrames(5);
console.log("screen", s.readMemory(0x2fe3,1)[0], "x", s.readMemory(0xc10,1)[0], "y", s.readMemory(0xc11,1)[0], "spd", s.readMemory(0xc13,1)[0]);
await shot(s, out); save(s, out);
