# SampleRealm: METER

A meter for the output of a track or a mix. It shows the spectrum, the waveform, the stereo field, and the
loudness and true peak. It only listens: the sound passes through unchanged. It works in mono or stereo.

## Spectrum

The curve shows how loud each frequency is, from 20 Hz at the left to 20 kHz at the right.
The scale tilts up 4.5 dB an octave, so most music looks about level and a mix that is heavy at one end stands out.
The thin line above the curve holds the highest level each frequency has reached.
Move the mouse over the curve to read the frequency under it and the curve's level there, on the tilted scale.

## Waveform

The last 8 seconds of the sound scroll by from right to left, as DJ software draws a track.
The lows, the mids and the highs are drawn one over the other in three colours, so a kick, a voice and a hi-hat each stand out.

## Stereo Field

The scope draws the left channel against the right. A mono sound is an upright line, a wide one fills the
circle, and a sound that is out of phase lies flat. Quiet sounds are drawn as large as loud ones.

- Correlation is +1 when both channels are the same and falls as they differ. Below 0, parts of the sound cancel in mono.
- Balance shows which channel is louder.

## Loudness

Loudness is measured in LUFS, as ITU-R BS.1770 and EBU R 128 define it. LUFS is a level scale weighted to
match how loud things sound to people. One LU is the same step as 1 dB.

- Momentary covers the last 0.4 seconds, and Short-term the last 3 seconds. The MOM and ST bars show them.
- Integrated is the loudness of everything since the last reset, as one number. The line across the MOM and ST bars marks it.
- Range is how far the loudness moves between the quiet and loud parts, in LU.
- True Peak Max is the highest peak so far in dBTP, including peaks between samples. The L and R bars show each channel's peak now.
- Clips counts how many times the signal has gone over 0 dBTP. Both turn red once it has.

## Integrated LUFS

Integrated is the number streaming services and broadcasters mean by a loudness target, such as -14 LUFS.

- Momentary and Short-term keep moving with the music. Integrated averages the whole track so far, and settles as it plays.
- Silence and quiet passages are left out, so a long intro or a gap doesn't pull the number down. Anything under -70 LUFS is ignored, and so is anything more than 10 LU below the average.
- To measure a track, start playback at its beginning and read the number at its end.

## Play and Reset

Starting playback starts the integrated loudness, the range, the held peaks and the clip count again.
Once the host has played, they wait while it is stopped, and the integrated loudness shows PAUSED. In the Standalone, and in a host with no play button, they always run.
RESET in the header starts them again by hand, and drops the spectrum's held line to the current levels.

## Views and Size

Click the corner button of a panel, or double-click the panel, to expand it over the whole window. Do the same to go back.
VIEW in the header lists the panels. Untick one to hide it, and the others fill the space.
Drag the gap between two panels to resize them. Double-click the gap to put them back.
Drag the corner of the window to resize it. Made short, the window becomes one row of panels, and the loudness panel shows fewer readings.
The size is saved with your project.

## Look

- The theme button lists the colour themes.
- Edit custom... in that list opens your own theme. Choose the theme it starts from, then set its three colours: click one, and drag in the picker or type a hex colour.
- VIEW also sets how much the display moves with the beat: Full, Subtle or Off.
- Backdrop in VIEW puts the meter on flat black, green or blue, to key it out or blend it over video. The curves keep the theme's colours.
- CAPTURE hides the header and the small labels, for a clean screen recording. Move the mouse and press EXIT CAPTURE to bring them back.

## Presets

A preset stores the look: the theme and your custom theme, the reactivity, the backdrop, the view, which panels show and their sizes.

- The arrows step through the presets, and the name opens the full list.
- A * after the name means the look has changed since the preset was loaded.
- Save overwrites the current user preset. For a factory preset it asks for a name.
- The ... menu has Init Preset, Save As, Rename and Delete. In the plugin it also has Show Preset Folder.
- Factory presets can't be renamed, deleted or overwritten.
