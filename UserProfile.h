#ifndef USER_PROFILE
#define USER_PROFILE

#include <string>
#include <vector>
#include "Record.h"

using namespace std;

class UserProfile {
    private:
        string username;
        vector<Record*> savedRecords;

    public:
        // Constructor to create a UserProfile, requiring only a username for the user
        UserProfile(string un);
        // Add a new record to the savedRecords list
        void addRecord(Record* r);
        vector<Record*> getRecords();

        //getters
        string getUsername();
};

#endif // USER_PROFILE
