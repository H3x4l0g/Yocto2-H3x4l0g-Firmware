# Yocto v2 — changelog

From e-licktronic's **v2.003beta**, the last published version, to **v2.074**. Only
what changes the machine's behaviour is listed.

*Version française : [CHANGELOG.md](CHANGELOG.md).*

## Encoder

- One detent of the wheel moves one step. It used to count **four**.
- On the pattern edit screens, the bare wheel no longer touches the **tempo**, which is
  not even displayed there. You no longer move it while thinking you are setting
  something else.
- Holding TEMPO is still the way to the tempo, and it shows the value while you turn,
  from any edit screen. **That includes the ACC slot and keyboard mode**, where the
  screen did show the tempo while the wheel was actually editing the global accent or
  the note under the cursor.
- **And the tempo follows your hand.** While TEMPO was held, the screen was redrawn on
  every pass of the main loop: the machine spent its time talking to the LCD instead of
  reading the wheel, and every other detent was lost as soon as you turned a little
  fast. The screen is now redrawn only when the value changes.
- Holding SHUFFLE sets the shuffle in PATT TAP too. It used to move the tempo silently.
- "Total Accent" is now called **Global Accent**: every parameter that applies to the
  whole pattern is a "global" on this machine.
- **On page 2 of config mode, the wheel no longer writes the page 1 settings.** The three
  positions marked `nc.`, the ones showing `xxx`, actually set the start-up BPM and the
  two MIDI channels. You could leave config mode with a receive channel you never chose,
  with nothing on screen to say so, and ENTER would store it. They now do nothing.
- **The wheel only touches what is displayed.** That is now the rule, and it fits in one
  sentence. In MUTE it silently set the tempo while the screen showed an edit page, a
  shuffle bar graph or the global accent: it now goes back to the mode playing
  underneath. In TRACK PLAY, where the tempo is not displayed either, it is inert. PATT
  PLAY remains the only screen where the bare wheel sets the tempo, because it is the
  only one where you can read it.

## Micro-timing — new

- **Each step can be delayed by 0 to 23 ticks**, about 5 ms per tick at 120 BPM. Enough
  to push a voice behind the beat without changing the grid.
- **To play a hit BEFORE a step, write it on the previous step and delay it almost all
  the way.** That is how you place a clap just ahead of the snare.
- **Shuffle uses up part of the range.** A swung step is already late, so the room left
  before the next hit is shorter, and the setting stops there instead of making the hit
  disappear.
- The setting is the third field of page 2: `pas rtg utm ins`.
- A roll moves with its hit: the offset takes the whole group along.
- The external track is not affected.

## Retrig — new

- **Holding SHUFFLE shows its bar graph again** when a step settings screen is open. The
  wheel did set the shuffle, but the screen showed something else: you were setting it
  blind.

- **Each step can sound 2, 3, 4, 6 or 8 times within its own interval.** Hi-hat rolls,
  snare buzzes, kick stutters — and triplets are reachable, which decided the shape of
  the setting.
- **`SHIFT` + a step button, twice**: the first press opens that step's conditions, the
  second its roll, the third closes. Like the pages of config mode.
- The sixteen LEDs show which steps roll, as they show conditioned steps.
- Where a hit lands no longer depends on how many voices roll at the same time.
- All repeats sound **at the same level** as the main hit, and the next step cuts the
  roll.
- A condition or a mute governs **the whole step**, repeats included.
- **Patterns already stored do not change.**

## Trig conditions — new

- **Shifting a pattern takes its conditions along.** The shift arrows moved trigs and
  velocities but left the conditions in place: each step inherited its neighbour's
  condition.

- **Each step can play only one bar in X, or every other time.** Two settings per step
  and per instrument: a cycle condition — `1:2` one bar in two, `3:4` the third of four,
  up to `5:8` — and a probability. The two combine: `1:4` at 50 % is a note that falls
  one bar in four, and only half of those times.
- **SHIFT + a step button** opens that step's screen in pattern edit. The same gesture on
  the same step closes it; on another step, it jumps there. Meanwhile the sixteen LEDs
  show which steps of the voice carry a condition.
- A conditioned step **goes silent without leaving the grid**: the external track keeps
  the relationship between its grid and its note stack, exactly as under a mute.
- **Patterns already stored do not change.** The room existed in memory from the start
  and was never used; a marker tells patterns that carry conditions from those that
  never had any.
- The probability wheel reads in the direction of density, from the first detent to the
  last: 6 %, 12 %, … 93 %, then 100 %, which means always.
- The instrument field skips the global accent slot, which has no steps to condition.
- **Clearing a step clears its condition.** It used to stay: writing a trig at the same
  place again made it conditioned with nothing to say so.
- **The sixteen LEDs show the playhead while you set a condition.** They only showed the
  conditioned steps: setting "one bar in four" without seeing the bar go round was
  setting it blind.
- CLEAR clears the conditions along with the rest.

## Mute and solo

- **Leaving solo restores the mutes you had set before.** Solo overwrote them, and you
  then had to undo them pad by pad. It is now a layer, not a rewrite.
- **Soloing a voice no longer takes its accents away**, and no longer stops the two trig
  outputs. The global accent is not a sound but a level added to a voice, and TR1/TR2
  clock external gear: soloing the kick must not stop the sequencer it drives. The EXT
  track does go silent like the other voices — it makes sound.
- **Several voices can be soloed together**; SHIFT + the pad of a soloed voice removes
  it, and removing the last one exits solo. Pressing the encoder brings everything back,
  solo included.
- During a solo, a bare pad does nothing anymore: it used to change an invisible mute
  that you only discovered when leaving solo.
- **Holding the encoder down no longer prevents muting.** It reset the mutes
  continuously, so every pad press was undone immediately.

## Config mode

- **Leaving config mode without ENTER says so: `[NOT SAVED!]`.** The settings stay
  active but did not survive power-off, and the warning — the blinking ENTER LED — went
  out with the mode, at the very moment it mattered.
- **The cursor no longer stops on the three empty positions of page 2.** It vanished
  there three times out of four, which looked like a broken page.

## Modes

- Start-up on **PATT PLAY**, no longer on the edit screen.
- Switching between the TRACK and PATT families requires the transport to be **stopped**:
  the two do not play the same thing. The switch then keeps the play/edit character,
  instead of landing on the edit screen whatever you leave.
- Switching between play and edit **within** a family stays free, even while running: it
  only changes what the buttons do, never what plays.
- Leaving TRACK with the PATT button no longer silently discards track changes.
- **MUTE announces itself in large letters on the screen.** Nothing said you were in it:
  you noticed when trying to change pattern, too late.

## Panel readability

- **The NUM LED lights while NUM is held.** It never lit — the firmware reserved the lamp
  and never wrote to it. NUM shows the track number, acts as a modifier and enters
  keyboard mode: it was the only held button the panel said nothing about.
- Holding BANK lights the **eight banks** dimly, the current one at full brightness.
- Buttons 9 to 16 no longer select bank H: they are not banks.
- The playhead follows the bar, no longer the length of the selected instrument.
- The screen no longer freezes in MUTE.
- Centred welcome screen, with a sweep of the step LEDs.

## Panel buttons

- **The buttons finally have a real debounce.** What was called that was not one: the
  machine compared two readings taken a few tenths of a millisecond apart. They are now
  truly 5 ms apart.
- **And the panel has 5 ms more latency** — a press used to be seen 0 to 5 ms after the
  finger, it is now seen 5 to 10 ms after. It is a **constant** delay, not jitter: the
  hand adjusts to it without knowing.
- What it buys: the machine spent about **5 % of its time with interrupts disabled**
  re-reading the buttons in a loop, and disabled interrupts are what shift hits in time.
  That has dropped to 0.4 %.

## Step buttons

- A button could become **permanently dead**: leaving keyboard mode while holding it
  killed it until the next visit to that mode. Fixed.
- **In TAP mode, buttons 13 to 16 play the four tracks that have no voice** — global
  accent, external instrument, TRIG-1 and TR2. They all hit a kick, and recorded it: four
  buttons for twelve written instruments, the last four landing on slot zero.
- **A hit in TAP no longer releases a held MIDI note.** The velocity converter sweep,
  copied by hand into that mode, rewrote twelve times per hit the port that also carries
  the trig outputs: the gate of a note 60 dropped on the way.
- **TAP hits stay in the pattern where you played them.** They only enter the pattern at
  the end of the bar, so as not to sound twice. But leaving TAP or stopping before the
  bar left them pending, with nothing on screen: the next time you entered TAP, they
  landed in whatever pattern was current then, while their velocity had stayed in the
  first one. Leaving the mode or stopping now writes them immediately, and a save made at
  the bar no longer makes them miss the merge. *(v2.072)*
- **A chord tapped in TAP lands on a single step, at full velocity.** Each hit read the
  step twice, once for the velocity and once for the trig: right on the rounding
  threshold, the full velocity went to one step and the trig to the next, and the hit was
  recorded at half level. The voices of a chord could also be split across two steps.
  *(v2.073)*

## Tempo and swing

- **Turning the tempo while running no longer shifts the current beat.** The detent
  arrived while a period was already under way, and the next one restarted from zero:
  the current beat grew by a fraction of a step with each detent. It now fires at the
  instant it was due.

- All 221 BPM values are exact. Only **seven** were.
- No more 32 ms hiccup when turning the tempo.
- Shuffle range from 50 % to **70.8 %**, against 62.5 % at best before.
- A shuffle detent means a **percentage**, the same at all four resolutions. Exact triplet
  shuffle at detent 9.
- Swing stays on the odd steps. It drifted onto the even ones, and its phase was
  arbitrary as soon as you changed pattern or length.
- No more permanent phase shift after changing an instrument's length.
- **SHUFFLE and SHIFT+SHUFFLE finally have two screens.** Both gestures drew the same
  one, title included, with a bar graph that always read the current voice's value: the
  global setting therefore displayed something other than what it set, and the first
  detent made the cursor jump without warning. The title now names what you are setting,
  and the bar graph shows its value.
- After a global shuffle setting, **a wheel detent no longer randomly does nothing**: no
  redraw, no blinking ENTER, on the value the global gesture had just set everywhere.
- **Changing a scale while running no longer shifts step 1.** The instrument's counter
  stayed where it was and started advancing at another rate: the grid no longer fell on
  the beat, and the only remedy was to stop and press PLAY again. The gesture now brings
  the instrument back in phase — the current instrument alone, all sixteen with SHIFT.
- **And re-phasing after a length edit finally takes the scale into account.** Hats in
  1/32 under a 1/16 reference were sent half a pattern back with each length change.
  Position is now counted in ticks elapsed since the bar, the only measure that
  instruments at different rates share.

## Patterns and chains

- Selecting a chain while playing no longer plays the last pattern first.
- Switching edit ↔ play neither repeats nor skips a pattern.
- MUTE no longer freezes chain progress.
- The current pattern is no longer replayed when it happens to be the first of the chain.
- **The bank follows the pattern.** Arriving on D13 through a track displayed `A61`, lit
  bank LED A, and step button 3 sent you back to A3. The screen, the LED and the button
  now say the same thing.
- **Choosing a bank no longer changes the pattern that plays.** Coming from A3, choosing
  bank D loaded **D3**, discarding the running chain on the way. The gesture arms the
  bank, nothing more: the step button that follows chooses the pattern. The arming waits
  for that button — a chain advancing in between no longer clears it — and the screen
  changes letter only when the pattern actually changes.
- **The stumbles at pattern changes at high tempo are gone.** Loading from EEPROM blocked
  for 115 ms and started *at* the bar: above 230 BPM the first steps of the new pattern
  were replayed. The bus now runs at its rated speed and loading takes 30 ms — from
  nearly two steps late to half a step.
- A chain can no longer read **outside the pattern area**. On a chain whose stored
  position was inconsistent, the calculation wrapped around and the sequencer read from
  the track area: a pattern of noise. Nothing was destroyed — it was a read — but you
  could hear it.
- **CLEAR also puts the bar back in order.** Clearing a pattern rewrote the sixteen
  lengths and divisions but left the **global** length, division and shuffle at their
  old values. Clearing an eight-step pattern gave sixteen steps per instrument and a bar
  that still ended on the eighth: chains and tracks advanced mid-pattern. And since CLEAR
  arms the save, the wrong value went to EEPROM on the first ENTER.
- **On a pattern change, counters align with the bar** instead of being reset to zero.
  Zero says which index, never when: an instrument faster than the bar hit once too often
  before the bar, played its index 0 there, and its whole pattern stayed one step off for
  its entire duration.
- **The bar falls on the longest instrument in duration, not in number of steps.**
  Sixteen 1/32 steps last two beats, twelve 1/16 steps last three: the former won, and
  the bar fell in the middle of the latter's phrase. Chained with an ordinary pattern, it
  looped in 6 beats instead of 7. A pattern whose instruments all share the same
  resolution does not move by a single tick.
- **The metronome no longer overwrites the pattern's velocities.** The click put its own
  values in the rimshot track: a half-level rimshot on a beat came back at full level
  after a round trip through GUIDE, without ENTER blinking as a warning, and the data went
  to EEPROM with the first save made for another reason. The click is a machine function,
  it no longer goes through the pattern.

## Playback direction

- **Changing direction is available in PATT PLAY**, as the manual describes, and no longer
  only in edit. Its twin — the left/right shift — was already there: the two gestures are
  finally symmetrical.
- **And it is saved.** Changing direction did not blink the ENTER LED, holding ENTER
  stored nothing, and the next load of the pattern overwrote the change. The data did go
  to EEPROM; it was the flag that was missing.

## Tracks

- The length no longer gets corrupted at 999.
- BACK and FWD move one step, no longer two.
- Inserting works on an empty track.
- **ENTER no longer truncates the track.** Rewriting a pattern at position 4 of a 20-long
  track brought it down to 4 bars. The length now only grows; shortening it is still
  possible, deliberately, with the wheel on the `len` field.
- After a power cycle, TRACK EDIT shows **the pattern the track contains**, not the one
  loaded at start-up. The data was not lost, the screen lied until you moved.
- Position is counted from **1** in play and in edit alike. The first bar was called 0 on
  one side and 1 on the other.
- **No more gap at the end of the track.** On an 8-long track, advancing to position 12
  then pressing ENTER brought the length to 12 and left positions 9 to 11 full of
  whatever was lying in memory. FWD and the wheel now stop at the append slot, the one
  `SHIFT+NUM` already takes you to.
- **Emptying a track from the end no longer takes every other press.** After each
  deletion the cursor fell back on the append slot, where there is nothing to delete; you
  needed a BACK between each. Four presses now empty a four-long track. In return, ENTER
  right after a deletion **overwrites** the last position instead of appending —
  `SHIFT+NUM` takes you back to the append slot.
- **TRACK PLAY redrawn**: the track on the left, the position on the right, the title
  alone at the top. The engaged pattern is gone from it — in track mode, what you follow
  is the track and where you are in it; the pattern is an execution detail, and TRACK EDIT
  shows it for those who want it.
- **TRACK WRITE's `num` field is now called `trk`**: it is the track number, and saying
  so saves guessing. Above all, **it finally answers the wheel** — the screen promised a
  parameter the wheel did not serve.
- **The ENTER LED no longer blinks on a track never touched.** A missing brace made
  execution fall through into the neighbouring fields: after a track load, the length
  witness stayed stale and re-armed the save by itself. Holding ENTER then rewrote the
  track to EEPROM for nothing.
- **Holding a track button no longer reloads the track in a loop** in TRACK PLAY. The
  test was on a level, not an edge: one second of pressing was worth some forty re-reads
  of eight EEPROM pages.
- **A track position never written no longer plays noise.** A byte of blank memory is
  255 where patterns stop at 128: the computed address left the pattern area and read
  from the track area.

## External instrument

- The note stack no longer gets truncated. Going back to note 3 of a twenty-note stack
  and replaying one cut it at 4.
- **No more hanging notes.** BACK and FWD sound each note as you walk the stack; holding
  one then pressing the other cut the wrong note, and the first one hung indefinitely.
- The top twelve notes no longer wrap into the bottom octave. Above a certain pitch, the
  note sent overflowed and wrapped around; the wheel now stops before.
- **No more hanging notes from the keyboard.** Holding a key, changing octave, releasing:
  the note-off went to the new pitch and the held note sounded indefinitely.
- **The stack no longer gets holes.** BACK and FWD stop at the append slot, as on tracks.
  You could advance to index 40 of a five-note stack and place a note there: the slots in
  between kept whatever was lying in memory.
- **The note stack is finally saved.** Its length was neither saved nor read back: the
  notes came back in full, but without their loop point, which kept the value left by the
  previously loaded pattern. The EXT track had never been storable since 2018 — copy and
  paste did carry it, which betrayed an oversight rather than a choice.
- **Copy and paste no longer loses the last note of the stack** — the one that closes the
  phrase. It kept whatever the destination pattern had in that slot.
- **Accents reach the EXT track.** Its steps had two states, present or absent, and their
  velocity was stored but never read back. They join the three-state cycle of the twelve
  voices — off, normal, accent — and the note-on carries the step's velocity: an x0xb0x
  reads it as accent. Steps already stored sound as before, they are only displayed at
  half instead of full. Trade-off: clearing a step takes two more presses, as on the
  twelve voices.
- **The cursor no longer leaves the stack.** You could take it well past the last note,
  edit a note there that would never be played, and wonder why you could not hear it.
- **Editing a note with the wheel marks the pattern**: ENTER blinks, and the work no
  longer disappears silently at the next load. The same note placed with a step button
  was marked — it is the asymmetry that betrayed the oversight.
- **Muting EXT no longer freezes the stack, and no longer leaves a hanging note.** The grid
  says when, the stack says what, and their relationship is the feature: muting stopped
  the stack, so on unmute the phrase resumed shifted by every step that passed under the
  mute. Muting everything was enough to freeze it without touching EXT. And a note held at
  the moment of the mute stayed held until STOP.
- **The stack restarts from the beginning on a pattern change.** It was the only state of
  the EXT track to survive one: going from pattern 1 to 2 and back left pattern 1's stack
  shifted by what pattern 2 had consumed — a pattern did not sound the same from one time
  to the next. The drift against the grid, on the other hand, does not move: that is the
  feature.
- **PLAY no longer leaves a stuck note.** The note-off rebuilt the pitch instead of
  remembering the one it had sent; since PLAY resets the stack counter, the note-off went
  to another note and the one sounding never received one. A few PLAYs in a row were
  enough to pile up stuck notes on the synth on the other end.

## Voices

- **PC2's half level goes up from 20 to 40**, like the other eight voices. Inherited as is
  from a TR-909 table, it gave a 1-to-4 gap between half and full, where the other eight
  are at 1 to 2 and the hi-hats at 1 to 1.3. Noticed by ear, confirmed by the table.

## Sync and outputs

- TRIG-1: no more truncated pulses, and MIDI note 60 drives the output again.
- Incoming MIDI clock: 2 ms of busy waiting per tick removed, i.e. 48 ms per second at
  120 BPM given back to the main loop.
- Drift over four minutes brought down from 0.75 s to 13 ms.
- **A MIDI chord goes out in one hit.** Kick, snare and cymbal on the same step arrived as
  three messages, handled three loop passes apart — a soft attack instead of a clean hit.
  They now go out on the same edge, like a sequencer step.
- **A MIDI note no longer cuts the current step.** It replaced the sequencer's trigger
  instead of adding to it: the pattern's hit was truncated.
- **And it no longer zeroes the other voices.** Each MIDI note reset all twelve velocities
  at once, with zero values for any voice never hit over MIDI. With the sequencer running
  it caught up on the next step; stopped, it did not.
- **Shuffle works in slave sync.** Incoming MIDI clock beats four times slower than the
  sequencer, and the four count steps it triggered all fired at the same instant: the
  swing delay had only 20.8 ms of resolution at 120 BPM, so several shuffle detents
  sounded identical. The four are now spread across the interval measured between two
  pulses. Micro-timing, if it ever came, would have had exactly the same problem.
- **In slave sync, changing pattern no longer makes the machine fall behind.** Loading a
  pattern held the machine long enough for it to miss count steps, and it never caught
  up: the delay settled in, and the next change added to it. Memory transfers now let the
  incoming clock through, and the clock can catch up if it still falls behind.
- **In slave sync, the downbeats are straight.** The first of the four count steps each
  clock pulse produces was placed by the main loop, and that is where every unswung step
  falls: downbeats trembled by a fraction of a millisecond while the offbeats were
  perfectly regular. The crystal now places all four.
- **The master's tempo is displayed in slave sync.** The screen showed three full blocks
  instead of the tempo. It is measured over four quarter notes of incoming clock, which
  makes it stable enough to read — a single interval carries too much jitter to show
  anything but a flickering number. The three blocks only remain while there is no clock
  to measure.
- **The DIN output gives a real signal in slave mode.** It only produced a spike of a few
  tens of microseconds there, where master mode gives a square wave. Both modes now
  output the same signal.
- **In slave sync, the clock is no longer doubled on the output.** The MIDI library echoed
  everything arriving at the input to the output, clock included: a device chained behind
  received 48 pulses per quarter note instead of 24.
- No more DIN-sync or trigger pulse lost under dense MIDI playing. The velocity
  converter's address bits and the real-time outputs share the same port, and a received
  note could overwrite a pulse being written.
- **A voice no longer goes permanently deaf to MIDI.** The lock that stops a repeated note
  from re-triggering a voice before its release was only cleared by the note-off of that
  exact note: one lost message — a cable unplugged mid-note, a keyboard reset, a chatty
  hub — and the voice stopped answering MIDI until power-off, with nothing on screen to
  say so. STOP, and a received Stop or Continue, now release all sixteen locks.
- **A MIDI note no longer comes out weak or silent.** Between raising the bits and arming
  the pulse, the release interrupt could undo everything: the voice got one microsecond of
  high state instead of the hundred the edge needs. Around 0.2 % of notes in dense
  playing, and nothing to tell it from a slip of the hand on the keyboard.
- **A real note-off instead of CC 123 alone** when everything stops. Many machines ignore
  All Notes Off — an x0xb0x among them.
- **Changing the MIDI receive channel finally works.** The setting was written but never
  passed on to the receiver: after changing it, the machine answered no note at all, on
  any channel, until the next power-up.
- **Switching back from slave to master works first time.** On a machine saved in slave
  mode, the first wheel detent towards "master" did nothing at all: neither the screen nor
  the clock moved, and you first had to turn the other way — where there is nothing — to
  unstick the setting. Same for pattern change sync.

## Reliability

- **Saving is three times faster, and the machine no longer freezes during it.** After
  each page written, the firmware waited 10 ms blindly — twice the maximum the EEPROM
  datasheet gives, and three times what it actually takes. A track save meant 80 ms
  during which nothing responded: not the LEDs, not the TRIG-1 output, not the incoming
  MIDI clock in slave sync. The chip now says by itself when it has finished.
- **A failed save says so.** The screen showed `[SAVED!]` whatever happened: a busy,
  badly connected or missing memory lost its page silently, and you only found out on
  reload. It now shows `[SAVE FAILED n]`, where `n` is the error code — 2: the memory
  does not answer; 3: it refuses a byte; 4 or 5: I2C bus fault; 6: it never finishes its
  write. Chains stored step by step and the memory initialisation at first start-up,
  which displayed nothing, also report their failure. *(v2.072)*
- **No more freezes at start-up.** It happened one time in two or three: two wrong fuses
  since PCB assembly — brown-out detection disabled, oscillator misconfigured — and two
  unbounded waits in the code.
- No more random LEDs or trigger outputs at power-up.
- The encoder had always read a 16 MHz square wave on its channel B: the system clock was
  output on that pin. It was the very first symptom of the project.
- The clock interrupt did thirty-six divisions per tick, on a processor that cannot do
  them in hardware. **One** remains, checked in the disassembly. Two of them even ran while
  stopped, on a machine playing nothing. The time given back to the main loop shows in
  everything that responds to your hand.

---

Built for ATmega1284P with MightyCore, zero warnings. Patterns and tracks live in the
external EEPROMs: no update touches them.
