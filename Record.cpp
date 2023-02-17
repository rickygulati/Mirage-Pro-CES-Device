#include "Record.h"

Record::Record(Session* s, int d, int il) {
    session = s;
    duration = d;
    intensityLevel = il;
}

Session* Record::getSession() {return session;}
int Record::getDuration() {return duration;}
int Record::getIntensity() {return intensityLevel;}

ostream& operator<<(ostream& os, const Record& r){
    return os << *r.session << "Duration: " << r.duration << "\nIntensity: " << r.intensityLevel << endl;
}
