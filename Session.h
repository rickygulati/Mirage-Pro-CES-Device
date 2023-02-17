#ifndef SESSION
#define SESSION

#include <string>
#include <iostream>

using namespace std;

class Session {
    friend ostream& operator<<(ostream& os, const Session& r);
    private:
        int id; // Unique identifier for each Session
        string frequencyRange; // Session Frequency Range (Page 12 Manual) - 4 available based on specs
        string cesMode; // CES Mode (page 11 Manual) - 2 available

    public:
        static int baseID; // starting id which is incremented by 1 for each instance

        Session(string fr, string cm);
        string getFrequency();
        string getCesMode();
};

#endif // SESSION
