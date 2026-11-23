import { fresh, load } from "./h.mjs";
const s = await fresh(); load(s, process.argv[2]);
const d = s._machine.processor.disassembler;
for (const r of process.argv.slice(3)) { let [a,b]=r.split("-").map(x=>parseInt(x,16)); 
  while (a<b) { const [t,n]=d.disassemble(a,true); console.log(a.toString(16), t); a=n; } console.log("---"); }
