/*
MIT License

Copyright (c) 2025 Richard Snakin

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "TrendTracker.h"

void TrendTracker_init(TrendTracker* t, int maxPoints, float threshold) {
    if(maxPoints > TREND_TRACKER_MAX_POINTS) maxPoints = TREND_TRACKER_MAX_POINTS;

    t->maxPoints = maxPoints;
    t->threshold = threshold;
    t->count = 0;
    t->head = 0;

    for(int i = 0; i < maxPoints; ++i)
        t->buffer[i] = 0.0f;
}

void TrendTracker_add(TrendTracker* t, float value) {
    t->buffer[t->head] = value;
    t->head = (t->head + 1) % t->maxPoints;
    if(t->count < t->maxPoints) t->count ++;
}

const char* TrendTracker_getArrow(const TrendTracker* t) {

    if(t->count < 2) return STEADY;

    float sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;

    for(int i = 0; i < t->count; ++i) {
        int index = (t->head + i) % t->count;
        float x = (float)i;
        float y = t->buffer[index];

        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_x2 += x * x;
    }

    float denom = t->count * sum_x2 - sum_x * sum_x;

    if(denom == 0) return STEADY;

    float slope = (t->count * sum_xy - sum_x * sum_y) / denom;

    if(slope > t->threshold)
        return UPWARD;
    else if(slope < -t->threshold)
        return DOWNWARD;
    else
        return STEADY;

}
