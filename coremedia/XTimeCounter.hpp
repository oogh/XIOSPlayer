//
//  XTimeCounter.h
//  XExporter
//
//  Created by Oogh on 2020/3/19.
//  Copyright © 2020 Oogh. All rights reserved.
//

#ifndef XEXPORTER_XTIMECOUNTER_H
#define XEXPORTER_XTIMECOUNTER_H

#include <ctime>
#include <chrono>

class XTimeCounter {
public:
    XTimeCounter() = default;

    ~XTimeCounter() = default;

    inline void markStart() {
        mStart = std::chrono::system_clock::now();
    }

    inline void markEnd() {
        mEnd = std::chrono::system_clock::now();
    }

    long getRunDuration() {
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(mEnd - mStart);
        return static_cast<long>(duration.count());
    }

private:
    std::chrono::system_clock::time_point mStart;
    std::chrono::system_clock::time_point mEnd;
};

#endif //XEXPORTER_XTIMECOUNTER_H
