# DeathAxe — user guide

## 1. How it works

DeathAxe is not a sampler. Every note plays a modelled guitar string (a Karplus-Strong loop with a pick, a pickup and string damping),
and that signal goes through the amp chain. That is why palm muting, tremolo picking, pinch harmonics and legato glides can be done
live from MIDI, and why every knob changes the sound of notes that are already playing.

Signal path: **strings → double-track width → noise gate → boost pedal → preamp stages (with tone stack) → power amp (sag) → cab/mic → low/high cut → delay → reverb → output limiter**.
The nonlinear amp section runs at 4x oversampling to keep high-gain distortion free of aliasing.

## 2. Playing it

* **Track type:** put DeathAxe on an instrument/MIDI-driven track.
* **Pitch:** MIDI note 40 is the guitar's low E (E2). Standard 6-string range is notes 40–88. For a lower tuning use **Transpose**, or just play lower notes (down to about note 12).
* **Velocity:** soft notes are quieter and duller; hard notes are louder and brighter. **Velocity 120 or higher triggers a pinch-harmonic squeal** if *Pinch Harmonic* is above 0.
* **Mod wheel (CC1):** adds palm mute. Fully up means fully muted. Combined with the *Palm Mute* knob, so you can automate it per note.
* **Pitch bend:** range set by *Bend Range* (default 2 semitones).
* **Note length matters.** Releasing a key lifts the "finger": the string stops quickly. Short notes give tight chugs, long notes ring.
* **Poly vs mono:** *Poly* lets notes overlap like a normal synth. *Mono Legato* plays one note at a time; overlapping notes slur (hammer-on / pull-off) with an optional *Glide*.

## 3. Controls

### String and performance
* **Transpose / Bend Range** — global pitch shift and pitch-bend range.
* **Voice Mode / Glide** — poly or mono legato, and the slide time between legato notes.
* **Chord** — each key plays extra strings: *Power 5th* (+7), *Power + Octave* (+7, +12), *Octave*, *Power 4th* (+5), *Tritone* (+6), *Minor* (+3, +7), *Sub Power* (-12, 0, +7).
* **Strum** — time spread between the strings of a chord (up to 60 ms), low to high.
* **Sub Octave** — adds a string one octave below the note, at this level.
* **Double Track** — two independently picked strings (slightly detuned, different timing) are panned left/right. 0 = mono, 100 % = full wide double-tracked sound.
* **Pick Attack** — brightness and snap of the pick. **Pick Noise** — pick click/scrape. **Pickup Pos** — bridge (bright, bitey) to neck (warm, round).
* **Sustain** — how long the string rings (0.2–20 s). **String Damping** — darkens the string as it decays.
* **Palm Mute** — shortens and darkens the string. **Tremolo Pick** — while a key is held the note is re-picked automatically, alternating hard/soft. Rate in Hz = 3 + 19 × value. Example: 16th notes at 190 BPM is 12.7 Hz, so the value is (12.7 − 3) / 19 = 0.51.
* **Pinch Harmonic** — amount of squeal on notes with velocity 120+.

### Gate and boost
* **Gate Threshold / Release** — chops the signal before the amp. A fast release and higher threshold gives the tight, staccato death-metal chop. Lower the threshold for long sustained notes.
* **Boost / Drive / Tight / Level** — an overdrive pedal in front of the amp. *Tight* cuts bass before distortion (higher is tighter and less flabby). *Level* pushes the amp harder.

### Amp
* **Amp Voicing** — five different preamps: *Modern Scoop* (tight, scooped), *Brutal Mid* (aggressive mids), *Old School Tampa* (dynamic, looser), *Swedish Chainsaw* (hard-clipped buzzsaw), *Tech Tight* (very clear and tight).
* **Gain, Bright, Bias** — distortion amount, high-end bite before the clipping, and clipping asymmetry (more even harmonics).
* **Sag** — power-amp compression: higher feels spongier.
* **Bass / Mid / Mid Freq / Treble** — tone stack placed inside the preamp, so it also changes how the distortion reacts. **Presence** (highs) and **Resonance** (lows) shape the power-amp stage.

### Cabinet
* **Cab Type** — 4x12 V30, 4x12 Greenback, 2x12 Tight, 1x12 Open, 8x10 Beast, or *Direct* (no cab, only a high-frequency filter).
* **Mic Position** — centre (bright) to edge (dark). **Mic Distance** — close (more bass, drier) to distant (room reflections).
* **Low Cut / High Cut** — final filters.

### Effects and output
* **Delay** (ping-pong, time in ms, feedback), **Reverb** (size, damping), **Output Level**.
* There is a soft limiter at the end, so the output never exceeds 0 dBFS.

## 4. Factory presets (40)

| # | Preset | What it is |
|---|---|---|
| 1 | **Death Metal Standard** | All-rounder: tight modern scooped rhythm tone, V30 cab. |
| 2 | **Stockholm Buzzsaw** | Hard-clipped pedal-into-amp chainsaw: bass-heavy, razor top, scooped mids. |
| 3 | **Tampa Rot** | Dynamic old-school crunch with a boost in front: rotten, mid-heavy, alive under the pick. |
| 4 | **Gothenburg Twin** | Melodic-death lead/rhythm tone: singing mids, neck pickup, legato glide, dotted delay. |
| 5 | **Brutal Slam** | Huge, mid-scooped slam rhythm: power-chord dyads, sub support, 8x10 weight. |
| 6 | **Technical Precision** | Surgical tech-death rhythm: very tight low end, fast gate, clear note definition. |
| 7 | **Chug Machine** | Palm-muted machine-gun chugs on power chords. Stays tight at any tempo. |
| 8 | **Tremolo Torrent** | Built for tremolo picking: hold a note and the strings re-pick for you. |
| 9 | **Dropped Abyss** | Seven semitones down plus a sub-octave string: seismic low end, dark top. |
| 10 | **Pinch Squeal Lead** | Mono legato lead. Hit a note at velocity 120+ for a pinch-harmonic squeal. |
| 11 | **Doom Funeral** | Slow, huge death-doom: strummed octave power chords, endless sustain, cavernous space. |
| 12 | **Blackened Frost** | Icy blackened-death wall: tremolo-picked power chords, thin bass, huge reverb. |
| 13 | **Cavernous Gore** | Low, wet and rotten: dropped tuning, sub string and a very distant mic in a big room. |
| 14 | **Industrial Grind** | Cabless, digital-sounding grind: fizzy, upper-mid forward, almost mono. |
| 15 | **Old Flesh Crunch** | Early-days rhythm tone: low gain, big dynamics, loose and open 1x12 sound. |
| 16 | **Wall of Twin Tracks** | Maximum width: octave power chords, fully double-tracked, strummed. |
| 17 | **Death'n'Roll Swagger** | Mid-tempo groove tone: bright, bitey, a little slap-back for attitude. |
| 18 | **Goregrind Mud** | Twelve semitones down, saturated to the limit, gated to a chop. |
| 19 | **Slam Djent-Death** | Staccato, tightly gated, trebly power-chord stabs on a tight 2x12. |
| 20 | **Necro Basement** | Lo-fi demo-tape rot: no bottom, hissy top, cheap reverb, distant cab. |
| 21 | **Sweep Lead** | Clear, sustaining solo tone with a touch of delay. Mono legato, no glide. |
| 22 | **Crypt Clean** | Near-clean, eerie intro tone: neck pickup, long delay and reverb. Play it soft. |
| 23 | **Thrash Pit** | Death-thrash crossover: pushed mids, tremolo-picked power chords, snappy gate. |
| 24 | **Atmospheric Dirge** | Slow ambient death: 18-second strings, huge delay and reverb, soft octave chords. |
| 25 | **Cathedral of Bones** | Melodic death-doom cathedral: neck-pickup octave chords, long dotted delay, huge hall. |
| 26 | **Minor Key Dirge** | Every key plays a full minor triad, slowly strummed. Dark, sad, enormous. |
| 27 | **Tritone Terror** | The devil's interval on every note: tremolo-picked horror-movie dissonance. |
| 28 | **Bowel Shaker** | Two octaves down with a sub string. Mostly felt, not heard: clear the room. |
| 29 | **Machine Gun Blast** | Maximum-speed tremolo picking on power chords, razor gate, every pick audible. |
| 30 | **Haunted Music Box** | Almost-clean octave-up shimmer with long echoes. Creepy intros and outros. |
| 31 | **Megaphone Mortuary** | Telephone-through-a-bullhorn distortion: no lows, no highs, all attitude. |
| 32 | **Throat Cut** | Nasal, vocal-like mid peak. Sounds like the guitar is screaming along with the singer. |
| 33 | **Frostbitten Cathedral** | Blackened-death tremolo power chords washed in an icy hall of reverb. |
| 34 | **Slide Guitar From Hell** | Mono slides between notes with feedback echoes and a squeal. Overlap notes to slur. |
| 35 | **Pit of Fuzz** | Broken, spitting asymmetric fuzz: maximum bias, heavy sag, boost cooked to the limit. |
| 36 | **Crushed Cassette** | Worn-out four-track demo tape: thin, dark, noisy picks, everything pressed together. |
| 37 | **Fourths of Doom** | Parallel fourths instead of fifths: open, ominous, slightly Eastern. Wide and strummed. |
| 38 | **Ritual Drone** | Twenty-second strings, endless echo and a held sub: hold one chord and let it breathe. |
| 39 | **Shattered Glass** | Ice-thin, bridge-pickup lead with a razor edge. Cuts through any mix. |
| 40 | **Grave Robber Slapback** | Short slap-back echo on a loose old-school crunch: the sound of digging by lantern. |

Presets are only starting points: every knob stays editable. If you move a knob the editor shows "(modified)" next to the preset name. Your DAW saves all knob values with the project.

## 5. The editor and your own presets

DeathAxe opens its own window. It is laid out in the order the signal flows: **Playing → String → Gate + Boost → Amp → Cabinet → Delay → Reverb → Output**.

**Using the controls**
* **Knobs:** drag up or down. Hold **Shift** for fine control. **Double-click** (or Ctrl-click) resets to the default. The mouse wheel also works.
* **Selectors** (Chord, Amp voicing, Cabinet): click the arrows to step, or click the name to see the list.
* **POLY / MONO** switches voice mode. The **Glide** knob is greyed out in Poly because it only applies to Mono. The Boost knobs grey out when the Boost switch (top right of that panel) is off.
* Hover over anything and the bar at the bottom explains it in plain words.

**The preset bar (top)**
* Click the **preset name** to open the list: factory presets first, then **MY PRESETS**. Click one to load it. **< >** step through the list.
* If you move a knob after loading a preset, the name shows **(modified)** and the SAVE button lights up.
* **SAVE** opens a normal Save dialog, already pointing at your preset folder. Type a name and press Save. Pick an existing name to overwrite it (Windows asks first).
* **LOAD** (or "Load preset from file..." at the bottom of the list) opens a file dialog for any `.daxe` preset file, e.g. one a friend sent you.
* **Delete** one of your presets: open the list, hover over it and click the **x** on the right. It asks before removing the file.

**Good to know**
* Your presets are plain-text `.daxe` files: share or back them up by copying them. Anything you put in the folder shows up in MY PRESETS the next time the list is opened.
* Your DAW always stores the current knob values inside the project, whatever the preset name shows.
* Your DAW's own preset menu lists the factory presets too. Your own presets live in the editor's list.

**Where the files live**

| System | Folder |
|---|---|
| Windows | `%APPDATA%\DeathAxe\Presets` (usually `C:\Users\<you>\AppData\Roaming\DeathAxe\Presets`) |
| macOS | `~/Library/Application Support/DeathAxe/Presets` |
| Linux | `~/.config/DeathAxe/Presets` |

To store them somewhere else (for example a portable setup or a synced folder), set the environment variable `DEATHAXE_PRESET_DIR` to the folder you want before starting your DAW.

## 6. Worked example: a death metal chug riff in five minutes

Goal: a tight, palm-muted, double-tracked riff with open accents, a tremolo-picked run and a pinch-harmonic ending.

1. Create an instrument track, load **DeathAxe**, and choose the preset **Chug Machine**.
2. Set the project tempo to 190 BPM and draw a 2-bar clip.
3. **Chugs:** in the piano roll draw 16th notes on **E1 (MIDI 28)** — or any note — each a bit shorter than the grid step (about 80 % length). Velocity 100–110.
4. **Open accents:** make the first note of beat 2 and the first note of beat 4 a full 8th long, and draw the mod wheel (CC1) lane at **0** for those notes and at **127** for all the chugs. You will hear the open notes ring and the chugs stay tight.
5. **Tremolo run:** on bar 2, hold one 1-bar note on **A1** and set *Tremolo Pick* to **0.51** (this equals 16ths at 190 BPM). Automate it back to 0 afterwards.
6. **Pinch harmonic ending:** set *Pinch Harmonic* to **0.8**, then add a long note on **E3** with **velocity 124**. Raise *Delay Mix* to ~0.2 for the tail.
7. **Tune the tone:**
   * Too flubby in the low end → raise *Boost Tight* (try 260) or *Low Cut*.
   * Notes ring into each other → lower *Sustain* or raise the *Gate Threshold* to -45 dB and shorten *Gate Release*.
   * Too harsh → lower *Treble*, move *Mic Position* up toward the edge, or lower *High Cut*.
   * Want it heavier → set *Chord* to *Power 5th*, add some *Sub Octave*, or switch the cab to *8x10 Beast*.
8. **Wider guitars:** copy the track, set the copy's *Double Track* to 100 %, and pan the two tracks hard left and hard right, with slightly different timing or presets, for the classic four-guitar wall.

## 7. Tips and troubleshooting

* **No sound:** make sure the track is an instrument track receiving MIDI, and check *Output Level* and the *Gate Threshold* (very high thresholds mute quiet notes).
* **Notes cut off too fast:** raise *Sustain*, lower *Palm Mute* and the mod wheel, and check that the gate threshold isn't too high.
* **Click or clipping:** lower *Output Level*; the built-in limiter only rounds off peaks.
* **CPU:** about one amp chain per instance. Chords, *Sub Octave* and *Double Track* multiply the number of strings, which is cheap compared with the amp. Idle (no notes ringing) uses almost no CPU.
* **Latency:** the oversampling adds under 1 ms; the plugin reports zero latency.
* **Presets and automation:** all sound parameters are automatable. The factory presets are also the standard VST3 program list; some hosts show it as a dropdown, others in a "Factory Presets" menu.
* **Saved a preset but it isn't in the list?** If you saved it outside the preset folder it is still loaded, but only files in the folder appear in MY PRESETS. If saving fails, DeathAxe shows a message; check the folder is writable.
* **No window / only a generic panel?** The custom editor is Windows-only in this release; other hosts/systems show their generic parameter panel.

## 8. Parameter reference

| Parameter | Range | Default |
|---|---|---|
| Transpose | -24–24 st | 0 st |
| Bend Range | 0–12 st | 2 st |
| Voice Mode | Poly / Mono Legato | Poly |
| Glide | 0–500 ms | 0 ms |
| Chord | Single / Power 5th / Power + Octave / Octave / Power 4th / Tritone / Minor / Sub Power | Single |
| Strum | 0–100 % | 15 % |
| Sub Octave | 0–100 % | 0 % |
| Double Track | 0–100 % | 80 % |
| Pick Attack | 0–100 % | 70 % |
| Pickup Pos | 0–100 % | 30 % |
| Pick Noise | 0–100 % | 25 % |
| Sustain | 0.2–20 s (log) | 4 s |
| String Damping | 0–100 % | 35 % |
| Palm Mute | 0–100 % | 0 % |
| Tremolo Pick | 0–100 % | 0 % |
| Pinch Harmonic | 0–100 % | 0 % |
| Gate Threshold | -80–-10 dB | -55 dB |
| Gate Release | 5–500 ms (log) | 60 ms |
| Boost | Off / On | On |
| Boost Drive | 0–100 % | 35 % |
| Boost Tight | 60–500 Hz (log) | 180 Hz |
| Boost Level | -12–12 dB | 0 dB |
| Amp Voicing | Modern Scoop / Brutal Mid / Old School Tampa / Swedish Chainsaw / Tech Tight | Modern Scoop |
| Gain | 0–100 % | 70 % |
| Bright | 0–100 % | 30 % |
| Bias | 0–100 % | 50 % |
| Sag | 0–100 % | 30 % |
| Bass | -12–12 dB | 0 dB |
| Mid | -15–15 dB | 0 dB |
| Mid Freq | 200–2500 Hz (log) | 650 Hz |
| Treble | -12–12 dB | 0 dB |
| Presence | -10–10 dB | 0 dB |
| Resonance | -8–10 dB | 0 dB |
| Cab Type | 4x12 V30 / 4x12 Greenback / 2x12 Tight / 1x12 Open / 8x10 Beast / Direct (no cab) | 4x12 V30 |
| Mic Position | 0–100 % | 35 % |
| Mic Distance | 0–100 % | 25 % |
| Low Cut | 20–400 Hz (log) | 45 Hz |
| High Cut | 2000–20000 Hz (log) | 12000 Hz |
| Delay Mix | 0–100 % | 0 % |
| Delay Time | 20–1000 ms | 320 ms |
| Delay Feedback | 0–95 % | 30 % |
| Reverb Mix | 0–100 % | 8 % |
| Reverb Size | 0–100 % | 40 % |
| Reverb Damping | 0–100 % | 50 % |
| Output Level | -24–12 dB | -6 dB |

Plus: **Preset** (the factory program list; your own presets are managed in the editor, see section 5). Pitch bend and mod wheel are mapped automatically.
