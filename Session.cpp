#include "Session.h"

int Session::baseID = 1000;

Session::Session(string fr, string cm) {
    baseID++;
    id = baseID;
    frequencyRange = fr;
    cesMode = cm;
}

string Session::getFrequency() {
    return frequencyRange;
}

string Session::getCesMode(){
    return cesMode;
}

ostream& operator<<(ostream& os, const Session& s){
    return os << "Frequency: " << s.frequencyRange << "\nCES mode: " << s.cesMode << endl;
}
