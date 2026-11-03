# Yocto v2.074 — what's new since 2.003beta

*Version française : [new-features.fr.md](new-features.fr.md).*

This complements the original *Yocto v2.002 user manual*. It does not replace it:
everything the original manual describes still works as written, except where this
document says otherwise.

Your stored patterns and tracks are not touched by the update. The new per-step settings
used memory that already existed in every pattern without ever being used, so existing
patterns play exactly as before.

---

## 1. The step settings screen

Three new features are set **per step and per instrument**: trig conditions, retrig and
micro-timing. They share one screen, available in **PATT EDIT**.

**Opening it**: hold **SHIFT** and press a **step button**.

| Gesture | Result |
| --- | --- |
| SHIFT + step | opens page 1 for that step |
| SHIFT + the same step again | goes to page 2 |
| SHIFT + the same step a third time | closes the screen |
| SHIFT + another step | jumps to that step, page 1 |

There is no exit button to learn: you leave the way you came in. The gesture is ignored
while LAST STEP, INST or BANK is held, and in keyboard mode, because those already give
SHIFT + step a meaning.

**The two pages**:

```
pas cnd prb ins          pas rtg utm ins
5   1:4  50  SD          5   x3  .   SD
```

| Label | Meaning |
| --- | --- |
| `pas` | the step (1–16) |
| `cnd` | trig condition |
| `prb` | probability |
| `rtg` | retrig |
| `utm` | micro-timing |
| `ins` | the instrument |

Step and instrument stay in the same columns on both pages.

**Navigating**: **press the encoder** to move to the next field, then **turn** it to
change the value. The cursor arrives on the condition (or on retrig on page 2), not on
the step you just chose. You can change `pas` and `ins` without leaving the screen. `ins`
skips the global accent slot, which has no steps to set.

While the screen is open, the sixteen step LEDs show which steps of the instrument carry
a setting, together with the playhead.

**Saving**: like any pattern edit, ENTER blinks and ENTER saves.

**Clearing**: clearing a step also clears its condition, retrig and micro-timing.
CLEAR clears them all. Shifting a pattern left or right takes the settings along with
their steps.

---

## 2. Trig conditions — `cnd` and `prb`

A step can be told to play only on some bars, or only some of the time.

**Cycle condition** (`cnd`): `X:Y` means *play on bar X of every Y bars*.

| Value | Plays |
| --- | --- |
| `---` | every bar (the default) |
| `1:2` · `2:2` | on the first or the second of every 2 bars |
| `1:3` · `2:3` · `3:3` | on the first, second or third of every 3 bars |
| `1:4` … `4:4` | on bar 1, 2, 3 or 4 of every 4 bars |
| `1:8` · `5:8` | on the first or the fifth of every 8 bars |

**Probability** (`prb`): from 6 % to 93 % in sixteenths, then 100 %, which means
always. The wheel reads in the direction of density: turning up makes the note fall more
often.

**They combine.** `1:4` at `50` is a note that is *eligible* one bar in four, and plays
on half of those bars.

A step that a condition silences stays on the grid. The EXT track keeps its note stack in
step with the grid, exactly as under a mute.

---

## 3. Retrig — `rtg`

A step can sound **2, 3, 4, 6 or 8 times** within its own length. Use it for hi-hat rolls,
snare buzzes and kick stutters. `x3` and `x6` give triplets.

| Value | Hits per step |
| --- | --- |
| `x1` | 1 (normal) |
| `x2` `x3` `x4` `x6` `x8` | that many, evenly spaced |

- All repeats sound **at the same level** as the main hit.
- The next step cuts the roll.
- A condition or a mute governs the whole step, repeats included.
- On page 2 the step LEDs show which steps roll.
- Timing does not depend on how many voices roll at the same time.

---

## 4. Micro-timing — `utm`

Each step can be **delayed by 0 to 23 ticks**. At 120 BPM one tick is about 5 ms. Use it
to push a voice behind the beat without changing the grid.

- `.` means *on the grid* (no delay).
- **To play a hit *before* a step**, write it on the previous step and delay it almost all
  the way. A flam-like clap just ahead of the snare is done this way.
- **Shuffle uses up part of the range.** A swung step is already late, so the room left
  before the next hit is shorter. The setting stops there instead of making the hit
  vanish.
- A roll (retrig) moves with its step: the whole group is delayed.
- The EXT track is not affected.

---

## 5. Mute and solo

In **MUTE** mode, which is now announced in large letters on the screen:

| Gesture | Result |
| --- | --- |
| pad | mute or unmute the voice, as before |
| **SHIFT + pad** | **solo** the voice, or un-solo it |
| encoder push | clears all mutes **and** all solos |

- **Several voices can be soloed together.** Removing the last one exits solo.
- **Solo is a layer, not a rewrite.** When you leave solo, the mutes you had set before
  come back.
- During a solo a bare pad does nothing, so you cannot change a mute you cannot see.
- **The global accent, TR1 and TR2 are never silenced by a solo.** The accent is a level
  added to a voice, not a sound. TR1 and TR2 clock external gear: soloing the kick must
  not stop the sequencer it drives. The EXT track makes sound, so a solo silences it like
  the other voices.

---

## 6. Tempo and shuffle

- **TEMPO held + wheel** sets the tempo from any edit screen, and shows it while you turn.
  The bare wheel only sets the tempo in **PATT PLAY**, the only screen that shows it.
- **SHUFFLE held** sets the shuffle of the **current instrument**. The title names it:
  `Shuffle  SD`.
- **SHIFT + SHUFFLE held** sets **all sixteen** at once: `Shuffle  GLOBAL`. The bar graph
  shows the value being set.
- Shuffle ranges from 50 % to 70.8 %. Each notch is the same percentage at every scale,
  and **notch 9 is an exact triplet shuffle**.
- **Changing an instrument's scale while running re-phases it** onto the bar. SHIFT
  re-phases all sixteen.
- All 221 BPM values are exact.

---

## 7. Banks and patterns

- **Holding BANK** lights the **eight banks** dimly, with the current one at full
  brightness. Only steps 1–8 are banks.
- **Choosing a bank no longer changes what is playing.** BANK + a step *arms* the bank,
  and the next step button chooses the pattern in it. The screen changes letter only
  when the pattern really changes.
- The bank letter, the bank LED and the step buttons always agree with the pattern that
  is playing.
- The **direction change** works in PATT PLAY too, as the original manual describes, and
  it is saved with the pattern.

---

## 8. Smaller changes you will notice

- **The encoder moves one step per detent** (it counted four), and it only changes what
  the screen shows.
- The machine starts in **PATT PLAY**.
- **Switching between TRACK and PATT requires the transport to be stopped.** Switching
  between play and edit within a family stays free.
- **TAP mode, steps 13–16** play the four tracks that have no voice: global accent,
  external instrument, TRIG-1 and TR2. They used to all hit the kick.
- **The EXT track has accents.** Its steps cycle off / normal / accent like the twelve
  voices, and the MIDI note-on carries the step's velocity.
- The NUM LED lights while NUM is held.
- **In slave sync, the master's tempo is displayed**, averaged over four beats. The three
  full blocks only remain until a clock arrives.
- **Leaving config mode without ENTER** shows `[NOT SAVED!]`: the settings are active
  but will not survive power-off.
- **A failed save says so**: `[SAVE FAILED n]` replaces `[SAVED!]`.

  | n | Meaning |
  | --- | --- |
  | 2 | the memory does not answer |
  | 3 | the memory refuses a byte |
  | 4, 5 | I2C bus fault |
  | 6 | the write never completes |

Every fix is listed in [CHANGELOG.en.md](../CHANGELOG.en.md) (French: [CHANGELOG.md](../CHANGELOG.md)).
