// Shared helpers for the M1 measurements: a headless JSBeeb session with the
// Daredevil Dennis disc, snapshots, screenshots and key taps.
// Paths come from the environment (see README.md):
//   JSBEEB  path to a jsbeeb checkout with `npm install` done
//   DD_SSD  path to the Daredevil Dennis .ssd (not in this repository)
//   M1_OUT  working folder for snapshots, traces and screenshots
import fs from "fs";
import path from "path";
import v8 from "v8";

const JSBEEB = process.env.JSBEEB || "/home/user/jsbeeb";
const DISC = process.env.DD_SSD || "/home/user/disc/dd.ssd";
export const OUT = process.env.M1_OUT || process.cwd();
const { MachineSession } = await import(path.join(JSBEEB, "src/machine-session.js"));

fs.mkdirSync(path.join(OUT, "shots"), { recursive: true });

export async function boot() {
  const s = new MachineSession("B-DFS1.2");
  await s.initialise();
  await s.bootDisc("file://" + path.resolve(DISC));
  return s;
}
export async function fresh() {
  const s = new MachineSession("B-DFS1.2");
  await s.initialise();
  await s.loadDiscImage("file://" + path.resolve(DISC));
  return s;
}
export async function shot(s, name) {
  fs.writeFileSync(path.join(OUT, "shots", `${name}.png`), await s.screenshotActive({ scale: 1 }));
}
export async function tap(s, k, f = 4) { s.keyDown(k); await s.runFrames(f); s.keyUp(k); await s.runFrames(f); }
export function save(s, n) { fs.writeFileSync(path.join(OUT, `${n}.snap`), v8.serialize(s.snapshot({ includeRoms: false }))); }
export function load(s, n) { s.restore(v8.deserialize(fs.readFileSync(path.join(OUT, `${n}.snap`)))); }
