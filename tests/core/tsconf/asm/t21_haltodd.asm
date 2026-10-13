; A HALT at an odd address with the cache on: the idle fetches of a halted cpu are at
; pc+1, in the next cached word, so the first one misses and writes its tag - at any
; clock, not only at 14 MHz where it waits. Fast mode's HALT run-on (QUIET=1) has to
; leave the cache as the steps do (CACHE=1). Runs to the frame limit, halted.
	include "common.inc"
	ifndef CLK
CLK = 0
	endif
	org #8000
	di
	TSOUT SYSCONF, CLK
	TSOUT #2BAF, #0F		; CacheConfig: every window cached
	if ($ & 1) == 0
	nop
	endif
	halt
