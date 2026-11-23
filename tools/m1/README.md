# M1 measuring tools

Scripts used to measure the BBC Micro original (Daredevil Dennis, Visions 1984) for
`docs/M1-MEASUREMENTS.md`. They drive a headless JSBeeb in Node, step it one video frame (20 ms)
at a time, press keys on exact frames and record RAM every frame. Nothing from the original
(disc, code, graphics, layouts) is stored in this repository.

## Setup (cloud shell)

```sh
git clone --depth 1 https://github.com/mattgodbolt/jsbeeb ~/jsbeeb
(cd ~/jsbeeb && npm install --ignore-scripts)
# The disc: www.bbcmicro.co.uk game id 79, Disc005-DareDevilDenis.ssd
export JSBEEB=~/jsbeeb DD_SSD=~/disc/dd.ssd M1_OUT=~/m1work
mkdir -p $M1_OUT && cd $M1_OUT
```

## Scripts

| Script | What it does |
| --- | --- |
| `start.mjs <A-F>` | Boots, pages through the title, picks the skill level, saves `play0_<L>.snap` as play starts |
| `rec.mjs <snap> <frames> <out> "<keys>" [shotEvery] [saveAt]` | Runs from a snapshot, keys as `frame:Key+` / `frame:Key-` (`ShiftLeft`, `Space`, `Enter`), writes one 12 KB RAM dump per frame to `<out>.bin`; `saveAt` is `frame:name` |
| `show.py <bin> <addr,addr,..>` | Prints the chosen bytes on every frame where one changes |
| `yx.py <bin>` | Prints rider x, jump counter, speed and height (screen line) |
| `warp.mjs <snap> <out>` | Puts the rider at the end of track 4 so the game moves to the next screen (used to reach sea and snow) |
| `peek.mjs <snap> <addr[:n]> ..` | Reads memory from a snapshot |
| `dis.mjs <snap> <from-to> ..` | Disassembles a range (to read the rule constants) |
| `grid.py <out.png> <png> ..` | Tiles screenshots |

## Variables found (RAM)

| Address | Meaning |
| --- | --- |
| `&0C10` | Rider x, in screen bytes (1 byte = 2 MODE 2 pixels); the track wraps at 75 to 77 |
| `&0C11` | Rider ground line: 56, 112, 168, 224 for tracks 1 to 4 |
| `&0C12` | Jump counter (0 on the ground), crash timer after a crash |
| `&0C13` | Speed, 0 to 250 |
| `&0C14` | Fraction of x (1/256 byte) |
| `&0C09` | Stop available (1) or used (0) |
| `&0C15` | Take number (game over at 4) |
| `&2FE3` | Screen number, 1 to 36 |
| `&2F3A` | Wages, ASCII digits |
| `&2FA4` | First runner's x |
| `&1670` | First dangler's line length |
