// Boot the disc, page through the title and instructions, pick a skill level
// (A Novice .. F Ace) and save a snapshot just as play starts: play0_<level>.snap
import { boot, tap, shot, save } from "./h.mjs";
const level = (process.argv[2] || "A").toUpperCase();
const s = await boot();
await s.runFrames(200);                          // loader, instruction page 1
await tap(s, "Space"); await s.runFrames(100);   // instruction page 2
await tap(s, "Space"); await s.runFrames(300);   // part two loads, title page
await tap(s, "Space"); await s.runFrames(300);   // story and controls
await tap(s, "Space"); await s.runFrames(200);   // skill menu
await tap(s, `Key${level}`, 2);
await s.runFrames(350);            // level loads, "TAKE 1" card, screen drawn
save(s, `play0_${level}`);
await shot(s, `play0_${level}`);
console.log(`saved play0_${level}.snap`);
