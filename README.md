# Click Assist

A Geode mod that widens the timing window for frame-perfect clicks.

Click up to a few frames early and the click is held, then fired on the exact
frame it needs to land. A jump that normally gives you 1 frame gives you 1 plus
the window (4 by default). It only acts on a real press: if you don't click,
nothing happens.

Targets Geometry Dash 2.2081 with Geode 5.11.

## Settings

| Setting | Default | What it does |
| --- | --- | --- |
| Enabled | on | Master switch |
| Early-click window (frames) | 4 | How many frames early a click still counts (1 frame = 1/240 s) |
| Assist orbs and rings | on | Replay the click when you touch an orb or ring |
| Assist ground jumps | on | Replay the click when you land |

## Building

### Option A: GitHub Actions (no local setup)

1. Create a new GitHub repo and push this folder to it.
2. Open the Actions tab and wait for "Build Binary" to finish.
3. Download the "Build Output" artifact and unzip it.
4. Put `gavin.clickassist.geode` into your Geometry Dash `geode/mods` folder.

### Option B: Build locally

With the Geode CLI and SDK already set up:

```
geode build
```

The `.geode` file appears in the `build` folder. Copy it into `geode/mods`.

## Notes

This is a gameplay-assist mod. Don't use it for runs you plan to submit as
legit completions or for leaderboards.
