#ifndef RECORD
#define RECORD

#include <string>
#include <iostream>
#include "Session.h"

using namespace std;

class Record {
    friend ostream& operator<<(ostream& os, const Record& r);

    private:
        Session *session;
        int duration;
        int intensityLevel; // Intensity level, 1 - 8

    public:
        Record(Session* s, int d, int il);
        Session* getSession();
        int getDuration();
        int getIntensity();
};

#endif // RECORD
