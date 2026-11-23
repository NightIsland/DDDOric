import { fresh, load } from "./h.mjs";
const [snap, ...addrs] = process.argv.slice(2);
const s = await fresh(); load(s, snap);
console.log(snap, addrs.map(a => { const [x,n=1]=a.split(":"); return a+"="+[...s.readMemory(parseInt(x,16), +n)].map(v=>v.toString(16)).join(" "); }).join("  "));
