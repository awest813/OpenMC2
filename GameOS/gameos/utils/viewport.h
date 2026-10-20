#ifndef VIEWPORT_H
#define VIEWPORT_H

// Where the game's logical screen (the resolution it renders at) ends up in the window, and how
// window (mouse) coordinates map back to it. Kept free of SDL/GL so it can be unit tested.

namespace presentation {

struct Rect {
    int x, y, w, h;
};

// Fits a logical_w x logical_h screen into the drawable (in pixels): centred with black bars on
// two sides when the aspect ratios differ, or filling everything when stretching.
inline Rect compute_viewport(int logical_w, int logical_h, int drawable_w, int drawable_h, bool stretch)
{
    Rect r = { 0, 0, drawable_w, drawable_h };
    if (stretch || logical_w <= 0 || logical_h <= 0 || drawable_w <= 0 || drawable_h <= 0)
        return r;

    // compare aspect ratios without floating point rounding: lw/lh vs dw/dh
    const long long lhs = (long long)logical_w * drawable_h;
    const long long rhs = (long long)drawable_w * logical_h;
    if (lhs > rhs) {
        // logical screen is wider: full width, bars at the top and bottom
        r.h = (int)(rhs / logical_w);
        r.y = (drawable_h - r.h) / 2;
    } else if (lhs < rhs) {
        // logical screen is narrower: full height, bars on the left and right
        r.w = (int)(lhs / logical_h);
        r.x = (drawable_w - r.w) / 2;
    }
    return r;
}

// Window coordinates (as SDL reports mouse positions; they differ from drawable pixels on HiDPI
// displays) to a position relative to the viewport: 0..1 inside it, clamped at the edges.
inline void window_to_viewport(float wx, float wy, int window_w, int window_h, int drawable_w, int drawable_h,
                               const Rect& vp, float* out_x, float* out_y)
{
    const float sx = window_w > 0 ? (float)drawable_w / (float)window_w : 1.0f;
    const float sy = window_h > 0 ? (float)drawable_h / (float)window_h : 1.0f;
    float x = vp.w > 0 ? (wx * sx - (float)vp.x) / (float)vp.w : 0.0f;
    float y = vp.h > 0 ? (wy * sy - (float)vp.y) / (float)vp.h : 0.0f;
    *out_x = x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x);
    *out_y = y < 0.0f ? 0.0f : (y > 1.0f ? 1.0f : y);
}

// How many logical pixels one window coordinate unit covers (for mouse deltas).
inline void window_to_logical_scale(int window_w, int window_h, int drawable_w, int drawable_h,
                                    const Rect& vp, int logical_w, int logical_h, float* scale_x, float* scale_y)
{
    const float sx = window_w > 0 ? (float)drawable_w / (float)window_w : 1.0f;
    const float sy = window_h > 0 ? (float)drawable_h / (float)window_h : 1.0f;
    *scale_x = (vp.w > 0 && logical_w > 0) ? sx * (float)logical_w / (float)vp.w : sx;
    *scale_y = (vp.h > 0 && logical_h > 0) ? sy * (float)logical_h / (float)vp.h : sy;
}

} // namespace presentation

#endif // VIEWPORT_H
