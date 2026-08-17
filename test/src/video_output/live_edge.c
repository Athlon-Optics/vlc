/*****************************************************************************
 * live_edge.c: test for bounded video live-edge policy
 *****************************************************************************
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <vlc_common.h>

#include "../../../src/video_output/live_edge.h"

static void CheckDecision(bool expected, vlc_tick_t max_delay,
                          vlc_tick_t now, vlc_tick_t pts, bool forced,
                          size_t newer_queued, vlc_tick_t expected_age)
{
    vlc_tick_t source_age = 0;
    bool drop = vout_live_edge_ShouldDrop(max_delay, now, pts, forced,
                                          newer_queued, &source_age);
    assert(drop == expected);
    assert(source_age == expected_age);
}

static size_t CountDrops(vlc_tick_t max_delay, vlc_tick_t now,
                         const vlc_tick_t *dates, size_t count)
{
    size_t dropped = 0;
    for (size_t i = 0; i < count; ++i)
    {
        vlc_tick_t source_age;
        if (!vout_live_edge_ShouldDrop(max_delay, now, dates[i], false,
                                       count - i - 1, &source_age))
            break;
        dropped++;
    }
    return dropped;
}

int main(void)
{
    const vlc_tick_t now = VLC_TICK_FROM_SEC(10);
    const vlc_tick_t max_delay = VLC_TICK_FROM_MS(250);

    CheckDecision(false, 0, now, now - VLC_TICK_FROM_SEC(1), false, 2,
                  VLC_TICK_INVALID);
    CheckDecision(true, max_delay, now, now - VLC_TICK_FROM_MS(500), false, 2,
                  VLC_TICK_FROM_MS(500));
    CheckDecision(false, max_delay, now, now - VLC_TICK_FROM_MS(500), false, 0,
                  VLC_TICK_INVALID);
    CheckDecision(false, max_delay, now, now - VLC_TICK_FROM_MS(500), true, 2,
                  VLC_TICK_INVALID);
    CheckDecision(false, max_delay, now, VLC_TICK_INVALID, false, 2,
                  VLC_TICK_INVALID);
    CheckDecision(false, max_delay, now, VLC_TICK_0, false, 2,
                  VLC_TICK_INVALID);
    CheckDecision(false, max_delay, now, now + VLC_TICK_FROM_MS(50), false, 2,
                  VLC_TICK_INVALID);
    CheckDecision(false, max_delay, now, now - max_delay, false, 2, max_delay);

    const vlc_tick_t queued_dates[] = {
        now - VLC_TICK_FROM_MS(900),
        now - VLC_TICK_FROM_MS(600),
        now - VLC_TICK_FROM_MS(100),
    };
    assert(CountDrops(max_delay, now, queued_dates, 3) == 2);
    assert(CountDrops(max_delay, now, queued_dates, 1) == 0);

    vout_live_edge_state_t state;
    vout_live_edge_Init(&state);
    assert(state.last_drop_log == VLC_TICK_INVALID);
    assert(state.dropped_total == 0);
    assert(state.dropped_consecutive == 0);

    assert(vout_live_edge_NoteDrop(&state, now));
    assert(!vout_live_edge_NoteDrop(&state, now + VLC_TICK_FROM_MS(500)));
    assert(vout_live_edge_NoteDrop(&state, now + VLC_TICK_FROM_SEC(1)));
    assert(state.dropped_total == 3);
    assert(state.dropped_consecutive == 3);

    assert(vout_live_edge_EndCatchUp(&state) == 3);
    assert(state.dropped_total == 3);
    assert(state.dropped_consecutive == 0);
    assert(!vout_live_edge_NoteDrop(&state,
                                   now + VLC_TICK_FROM_MS(1500)));

    vout_live_edge_ResetCatchUp(&state);
    assert(state.last_drop_log == VLC_TICK_INVALID);
    assert(state.dropped_total == 4);
    assert(state.dropped_consecutive == 0);
    assert(vout_live_edge_NoteDrop(&state, now + VLC_TICK_FROM_MS(1600)));
    assert(state.dropped_total == 5);
    assert(state.dropped_consecutive == 1);

    return 0;
}
