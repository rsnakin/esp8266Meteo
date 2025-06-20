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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define TREND_TRACKER_MAX_POINTS 20 // Maximum frame size
#define UPWARD   "&#x2191;"
#define DOWNWARD "&#x2193;"
#define STEADY   " "

typedef struct {
    float buffer[TREND_TRACKER_MAX_POINTS];
    int maxPoints;
    int count;
    int head;
    float threshold;
} TrendTracker;

void TrendTracker_init(TrendTracker* t, float threshold);
void TrendTracker_add(TrendTracker* t, float value);
const char* TrendTracker_getArrow(const TrendTracker* t);
float TrendTracker_getSlope(const TrendTracker* t);

#ifdef __cplusplus
}
#endif

