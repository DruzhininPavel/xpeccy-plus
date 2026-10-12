// Redrawing the picture of a stopped machine, for the debugger: a layer it
// hides should be gone from what is on screen at once, not from the next frame.
//
// The picture is history - the lines were drawn as the beam passed them, from
// memory as it was then - so it cannot be drawn again from the machine as it
// stands. Instead the machine keeps a snapshot (xstate) at the start of each of
// the last two frames and a log of every byte IN read since. A redraw loads the
// older snapshot and runs it on to the T the machine stopped at, the INs fed from
// the log, so the beam goes over the frame before and this one up to where it
// is, exactly as the first time. Then the machine as it stood is put back; only
// the pixels stay.
//
// The run is checked, not trusted: it has to end on the same T with the log used
// up, and the machine it leaves has to be the one that was put aside, byte for
// byte (xstate_diff). Anything that changed the machine from outside in between
// - a reset, a load, a poke from the debugger, the tape trap - fails that, and
// then the picture is left as it was.
//
// A snapshot a frame is a few MB copied: it is only taken once armed, which the
// gui does the first time the debugger is shown on TSConf.

#pragma once

#include "spectrum.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
	XRP_OFF = 0,
	XRP_LOG,		// logging the INs
	XRP_FEED		// a redraw is feeding them back
};

// armed: keep the snapshots and the log (xreplay_arm)
extern int xreplay_on;
extern int xreplay_mode;
// set by the owner while it runs the machine fast: no snapshots, the ones held dropped
extern int xreplay_hold;

// start or stop keeping the snapshots; stopping drops them
void xreplay_arm(int on);
// a frame has just started (comp_step_end)
void xreplay_frame(Computer*);
// what an IN read: logged, or during a redraw swapped for what it read the first time
int xreplay_in(int val);
// drop the snapshots and the log
void xreplay_forget(void);
// draw the picture again with the machine as it is now set to draw; 1 when done.
// The machine must be stopped and owned by the caller.
int xreplay_redraw(Computer*);
// what came of the last one, in a few words
const char* xreplay_why(void);

#ifdef __cplusplus
}
#endif
