// usage: node rec.mjs <snap> <frames> <out> "<keys>" [shotEvery] [saveAt]; keys: "frame:Key+,frame:Key-,.."
import { fresh, shot, save, load } from "./h.mjs";
import fs from "fs";
const [snap, nf, out, script="", shotEvery="0", saveAt=""] = process.argv.slice(2);
const s = await fresh(); load(s, snap);
const ev = {};
for (const e of script.split(",").filter(Boolean)) { const [f, k] = e.split(":"); (ev[f] ||= []).push(k); }
const rows=[]; const saves = Object.fromEntries(saveAt.split(",").filter(Boolean).map(x=>x.split(":")));
for (let i=0;i<+nf;i++){
  for (const k of ev[i]||[]) { const n=k.slice(0,-1); if (k.endsWith("+")) s.keyDown(n); else s.keyUp(n); }
  if (saves[i]) save(s, saves[i]);
  await s.runFrames(1);
  const m = s.readMemory(0, 0x3000);
  rows.push(Buffer.from(m));
  if (+shotEvery && i % +shotEvery == 0) await shot(s, `${out}_${String(i).padStart(4,"0")}`);
}
fs.writeFileSync(out+".bin", Buffer.concat(rows));
await shot(s, out+"_end");
