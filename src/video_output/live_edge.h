/*****************************************************************************
 * live_edge.h: bounded live-edge policy for video output
 *****************************************************************************
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *****************************************************************************/

#ifndef VLC_VOUT_LIVE_EDGE_H
#define VLC_VOUT_LIVE_EDGE_H 1

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <vlc_config.h>
#include <vlc_mtime.h>

typedef struct
{
    vlc_tick_t last_drop_log;
    uint64_t dropped_total;
    unsigned dropped_consecutive;
} vout_live_edge_state_t;

static inline void
vout_live_edge_Init(vout_live_edge_state_t *state)
{
    state->last_drop_log = VLC_TICK_INVALID;
    state->dropped_total = 0;
    state->dropped_consecutive = 0;
}

static inline void
vout_live_edge_ResetCatchUp(vout_live_edge_state_t *state)
{
    state->last_drop_log = VLC_TICK_INVALID;
    state->dropped_consecutive = 0;
}

static inline bool
vout_live_edge_NoteDrop(vout_live_edge_state_t *state, vlc_tick_t system_now)
{
    state->dropped_total++;
    state->dropped_consecutive++;

    if (state->last_drop_log != VLC_TICK_INVALID &&
        system_now - state->last_drop_log < VLC_TICK_FROM_SEC(1))
        return false;

    state->last_drop_log = system_now;
    return true;
}

static inline unsigned
vout_live_edge_EndCatchUp(vout_live_edge_state_t *state)
{
    const unsigned dropped = state->dropped_consecutive;
    state->dropped_consecutive = 0;
    return dropped;
}

static inline bool
vout_live_edge_ShouldDrop(vlc_tick_t max_delay, vlc_tick_t system_now,
                          vlc_tick_t arrival_pts, bool forced,
                          size_t newer_queued, vlc_tick_t *source_age)
{
    *source_age = VLC_TICK_INVALID;

    if (max_delay == 0 || forced || arrival_pts == VLC_TICK_INVALID ||
        arrival_pts <= VLC_TICK_0 || arrival_pts > system_now ||
        newer_queued == 0)
        return false;

    *source_age = system_now - arrival_pts;
    return *source_age > max_delay;
}

#endif
