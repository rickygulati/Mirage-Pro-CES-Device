#include "mainwindow.h"


using namespace std;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    // Setting initial state of UI
    powerStatus = false;
    ui->setupUi(this);
    ui->pwrButton->setEnabled(true);
    ui->chargeButton->setEnabled(true);
    toggleButtonState(powerStatus);


    // Connect buttons to slot functions
    connect(ui->pwrButton, SIGNAL(released()), this, SLOT (handlePowerPress()));
    connect(ui->upButton, SIGNAL(released()), this, SLOT (handleUpPress()));
    connect(ui->downButton, SIGNAL(released()), this, SLOT (handleDownPress()));
    connect(ui->selectButton, SIGNAL(released()), this, SLOT (handleSelectPress()));
    connect(ui->addProfileButton, SIGNAL(released()), this, SLOT (handleAddProfilePress()));
    connect(ui->modeButton, SIGNAL(released()), this, SLOT (handleModePress()));
    connect(ui->pwrButton, SIGNAL(pressed()), this, SLOT (handlePowerHold()));
    connect(ui->selectButton, SIGNAL(pressed()), this, SLOT (handleSelectHold()));
    connect(ui->selectSavedButton, SIGNAL(released()), this, SLOT(handleSelectSavedPress()));
    connect(ui->signInButton, SIGNAL(released()), this, SLOT(promptSignIn()));
    connect(ui->chargeButton, SIGNAL(released()), this, SLOT(handleChargePress()));
    connect(ui->incDurationButton, SIGNAL(released()), this, SLOT(handleIncreaseDurationPress()));
    connect(ui->decDurationButton, SIGNAL(released()), this, SLOT(handleDecreaseDurationPress()));


    // connect session timer to its handler
    connect(&sessionTimer, SIGNAL(timeout()), this, SLOT (promptToRecord()));

    // connect per second timer to its handler
    connect(&perSecondTimer, SIGNAL(timeout()), this, SLOT (updatePerSecond()));

    // starting per second timer with one second intervals
    perSecondTimer.start(1000);

    // setting session time label to 00:00 as default on start-up
    ui->remainTimeN->setText("00:00");

    //Adding Default Guest Profile
    UserProfile* guest;
    QString g = "Guest";
    //Create user
    createUser(g.toStdString(), &guest);
    users.push_back(guest);
    curUser = guest;

    cout <<"Profile: " << guest->getUsername() << " Added." <<endl;

    //Add profile to profileSelector
    updateView();

    // Hover feature
    ui->pwrButton->installEventFilter(this);
    ui->upButton->installEventFilter(this);
    ui->downButton->installEventFilter(this);
    ui->selectButton->installEventFilter(this);
    ui->selectSavedButton->installEventFilter(this);


    // All available Session Frequency Ranges (can be used when the user wants to create a new Session in the User Designated group)
    sessionFreqRanges = {"MET", "Sub-Delta", "Delta", "Theta"};

    // All available CES Modes (can be used when the user wants to create a new Session in the User Designated group)
    cesModes = {"Short-Pulse", "50% Duty Cycle"};

    // All available Session Durations ('0' can be overriden when the user wants to create a new Session in the User Designated group)
    // Since the only difference between the session groups is duration, cycling through the durations will be equivalent to cycling
    // through the session groups
    durations = {20, 45, 0};

    // Create the 4 pre-defined sessions, storing them inside a Session Group
    for (int i = 0; i < 4; i++) {
        if (i == 1) {
            Session* session = new Session(sessionFreqRanges[i], cesModes[1]);
            sessionTypes.push_back(session);
        }
        else {
            Session* session = new Session(sessionFreqRanges[i], cesModes[0]);
            sessionTypes.push_back(session);
        }
    }

    // initalizing battery level to 60. After this, battery will have to be charged manually using the battery charge button
    batteryLevel = 60;

    init();

    // Updating mode UI as set for default value
    updateModeUI();
}

void MainWindow::init() {
    curSessionGroupIndex = 0; // Default duration is 20 mins

    curSessionIndex = 0; // Current session number (1-8) is 1

    curFrequencyIndex = 0; // Current selected frequency (if session group is user-defined) is "MET" (index 0)

    curModeIndex = 0; // Default mode for user-defined sessions

    signedInStatus = false; // Default is not signed in

    curDuration = durations[curSessionGroupIndex]; // setting default duration

    curIntensity = 4; // Setting default intensity on start up
    inSessionStatus = false;

    // setting session time label to 00:00 as default on start-up
    ui->remainTimeN->setText("00:00");

    // setting user defined duration label to 00:00 as default on start-up
    ui->setDurationNLabel->setText("00:00");
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow:: startConnectionTest() {

    //Display connection, blink lights, continue session
    int randInt = QRandomGenerator::global()->bounded(0,100);

    if ( 0 <= randInt && randInt <= 80) { //80% chance of good connection

        displayConnection(2);
        cout << "Connection 'Excellent'" <<endl;
        return true;
    } else if (80 < randInt && randInt <= 95) { //15% chance of ok conneciton

        displayConnection(1); // Ok connection
        cout << "Connection 'Okay'" <<endl;
        return true;
    } else {
        displayConnection(0);//bad connection
        // lights go from 8 to 1 for power off indication (Softoff)
        for (int i=8; i>=1; i--) {
            pwrLightOffAll();
            pwrLightOn(i);
            delay(300);
        }
        cout << "No Connection." << endl;
        cout << "Returning voltage to safe testing level. Try again." << endl;
        return false;
    }

}

void MainWindow:: displayConnection(int connectionQuality) {
    //call a helper function based on the given connection

    if (connectionQuality == 0 ) { //No connection
        noConnection(); //Will blink and return to safe mode, and then restart test with new connection

    } else if(connectionQuality == 1) { //"Okay" Connection
        //display connection and end connection test
        okayConnection();

    } else if (connectionQuality == 2) { // "Solid" connection
        //display connection and end
        solidConnection();
    }


}

void MainWindow::okayConnection(){

    for (int i =0; i < 4 ; ++i) {
        pwrLightOn(4);
        pwrLightOn(5);
        pwrLightOn(6);
        delay(300); //Sleeps for two seconds
        pwrLightOff(4);
        pwrLightOff(5);
        pwrLightOff(6);
        delay(300); //Sleeps for two seconds
    }
}

void MainWindow::noConnection(){

    for (int i =0; i < 4 ; ++i) {
        pwrLightOn(7);
        pwrLightOn(8);
        delay(300); //Sleeps for two seconds
        pwrLightOff(7);
        pwrLightOff(8);
        delay(300); //Sleeps for two seconds
    }
    //Scroll graph up and down to show unit returning to normal voltage (15-20 seconds)
    //to be continued
}

void MainWindow::solidConnection(){
    for (int i =0; i < 4 ; ++i) {
        pwrLightOn(1);
        pwrLightOn(2);
        pwrLightOn(3);
        delay(300); //Sleeps for two seconds
        pwrLightOff(1);
        pwrLightOff(2);
        pwrLightOff(3);
        delay(300); //Sleeps for two seconds
    }
}


// Triggered after startSession - when a session have just finished
void MainWindow:: promptToRecord() {
    // after session end, set timer to zero
    ui->remainTimeN->setText("00:00");
    // testing timer
    cout << "Timer finished now!" << endl;

    // Display the battery level at the end of the session
    displayBatteryLevel();

    inSessionStatus = false;
    // Enable the Select button after session is done
    ui->selectButton->setEnabled(true);
    // Turn off the light for the Current Intensity Level
    pwrLightOff(curIntensity);
    // Turn on the light for the current session number (except when the User Designated group is chosen)
    if (curSessionGroupIndex != 2) {
        pwrLightOn(curSessionIndex + 1);
    }
}

void MainWindow:: recordTherapy() {
    //Don't save to guest profile
    if (curUser->getUsername() == "Guest"){
        cout << "Please Sign Into a Profile to Save Session. (Cannot save session as 'Guest')" << endl;
        return;
    }

    Session* curSession;
    // if curSessionGroupIndex is not 2 (user-defined), use the current pre-defined session with the curDuration
    if(curSessionGroupIndex != 2) {
        curSession = sessionTypes[curSessionIndex];
    }

    // user-selected frequency and mode chosen, along with user-selected duration
    else {
        curSession = new Session(sessionFreqRanges[curFrequencyIndex], cesModes[curModeIndex]);
    }
    Record* r = new Record(curSession, curDuration, curIntensity);
    curUser->addRecord(r);
    cout << *r << endl;
    updateView();
}

void MainWindow:: createUser(string un, UserProfile** p) {
    //Create a profile to the profile selector
    UserProfile* newProfile = new UserProfile(un);
    *p = newProfile;
}

void MainWindow:: selectUser() {

    if(ui->profileSelector->currentIndex() >= 0){
        int idx = ui->profileSelector->currentIndex();
        curUser = users.at(idx);
        cout << "Selecting user: " << curUser->getUsername() << endl;
    }

}

void MainWindow::pwrLightOn(int index){
    switch(index){
        case 1: ui->oneLabel->setStyleSheet("color: " GREEN ";");break;
        case 2: ui->twoLabel->setStyleSheet("color: " GREEN ";");break;
        case 3: ui->threeLabel->setStyleSheet("color: " GREEN ";");break;
        case 4: ui->fourLabel->setStyleSheet("color: " YELLOW ";");break;
        case 5: ui->fiveLabel->setStyleSheet("color: " YELLOW ";");break;
        case 6: ui->sixLabel->setStyleSheet("color: " YELLOW ";");break;
        case 7: ui->sevenLabel->setStyleSheet("color: " RED ";");break;
        case 8: ui->eightLabel->setStyleSheet("color: " RED ";");break;
    }
    ui->pwrLvlSplitter->repaint();
}

void MainWindow::pwrLightOff(int index){
    switch(index){
        case 1: ui->oneLabel->setStyleSheet("color: grey");break;
        case 2: ui->twoLabel->setStyleSheet("color: grey");break;
        case 3: ui->threeLabel->setStyleSheet("color: grey");break;
        case 4: ui->fourLabel->setStyleSheet("color: grey");break;
        case 5: ui->fiveLabel->setStyleSheet("color: grey");break;
        case 6: ui->sixLabel->setStyleSheet("color: grey");break;
        case 7: ui->sevenLabel->setStyleSheet("color: grey");break;
        case 8: ui->eightLabel->setStyleSheet("color: grey");break;
    }
    ui->pwrLvlSplitter->repaint();
}

void MainWindow::pwrLightOffAll() {
    for (int i=1; i<9; i++) {
             pwrLightOff(i);
         }
}

void MainWindow::togglePowerStatus() {
    powerStatus = !powerStatus;
    toggleButtonState(powerStatus);
    if (powerStatus) {

        init();
        idleTimer.restart();

        greenLightOn();
        displayBatteryLevel();

        // buttons disabled if battery less than 15. To enable them, user needs to press the charge battery button
        if (batteryLevel < 15) {
            toggleButtonState(false);
            ui->chargeButton->setEnabled(true);
        }

        else {
            updateSessionsMenu(false);
        }
    }
    else {

        greenLightOff();
        // Turn off all session groups and session numbers icons when turned off
        allSessionGroupLightOff();
        allFrequencyLightOff();
        // stop ongoing session
        if (inSessionStatus) {
            inSessionStatus = false;
            sessionTimer.stop();

            // Set remaining time to 00:00
            ui->remainTimeN->setText("00:00");

        }
        ui->setDurationNLabel->setText("00:00");
        pwrLightOffAll();
    }
}



void MainWindow:: updateBatteryLevel(int duration) {
    // Compute the battery drain based on the current Intensity level duration
    int batteryDrain = (duration / 5) + (curIntensity / 2);
    // Update the current Battery Level
    batteryLevel -= floor(batteryDrain);
}

void MainWindow:: startSession(bool saved) {


    // Use curSessionGroupIndex for the duration vector to get the chosen Duration
    // Use curSessionIndex for the sessionTypes vector to get the chosen Session object, which will contain the chosen
    // session frequency type and CES mode

    inSessionStatus = true;

    // Turn off all number lights, and turn on the light for the current Intensity level
    updateIntensityUI();
    // Set the Previous Remaining Time to be equal to the chosen duration
    prevRt = durations[curSessionGroupIndex];

    Session *curSession;
    sessionTimer.setSingleShot(true);

    curDuration = durations[curSessionGroupIndex];//Set current duration accordingly

    if (saved){ //If session is already saved

        int idx = ui->sessionSelector->currentIndex();
        Record* r = curUser->getRecords().at(idx);
        curSession = r->getSession();
        string frequency = curSession->getFrequency();
        string cesMode = curSession->getCesMode();
        curIntensity = r->getIntensity();
        curDuration = r->getDuration();

        if (curDuration == 20){ curSessionGroupIndex = 0;} //Retrieving records and setting them as session variables.
        else if (curDuration == 45) {curSessionGroupIndex = 1;}

        if (frequency == "MET"){ curFrequencyIndex = 0;}
        else if (frequency == "Sub-Delta"){ curFrequencyIndex = 1;}
        else if (frequency == "Delta"){ curFrequencyIndex = 2;}
        else if (frequency == "Theta"){ curFrequencyIndex = 3;}

        if (cesMode == "Short-Pulse"){ curModeIndex = 0;}
        else if (cesMode == "50% Duty Cycle"){ curModeIndex = 1;}

        updateIntensityUI();
        updateSessionsMenu(true);
        updateModeUI();
    }

    // created a QTimer object in mainWindow on which you can call the QTimer functions
    // once this timer ends, the promptToRecord() function is called (connected by timeout() signal)
    //session time = minutes but in seconds hence the 1000 to convert ms to s.

    sessionTimer.start(curDuration*1000);


    //when session is started, will do a timer and on the end of timer, user will be prompted to record.
//    QTimer::singleShot(curDuration*1000,this,&MainWindow::promptToRecord);
}

// stops/ends the current session
 void MainWindow::softOff() {
     inSessionStatus = false;
     sessionTimer.stop();

     // Set remaining time to 00:00
     ui->remainTimeN->setText("00:00");

     // lights go from 8 to 1 for power off indication
     for (int i=8; i>=1; i--) {
         pwrLightOffAll();
         pwrLightOn(i);
         delay(300);
     }

     togglePowerStatus();
 }


void MainWindow:: delay(int millisecondsWait) {
    QEventLoop loop;
    QTimer t;
    t.connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
    t.start(millisecondsWait);
    loop.exec();
}

void MainWindow:: displayBatteryLevel() {
    // Turn off all other numbers in the bar graph
    pwrLightOffAll();

    if (batteryLevel >= 50) {

        for (int i =0; i < 2 ; ++i) {
            pwrLightOn(1);
            pwrLightOn(2);
            pwrLightOn(3);
            delay(300); //Sleeps for two seconds
            pwrLightOff(1);
            pwrLightOff(2);
            pwrLightOff(3);
            delay(300); //Sleeps for two seconds
        }
    }
    else if (batteryLevel < 50 && batteryLevel >= 15) {

        for (int i =0; i < 2 ; ++i) {
            ui->oneLabel->setStyleSheet("color: " YELLOW ";");
            ui->twoLabel->setStyleSheet("color: " YELLOW ";");
            delay(300); //Sleeps for two seconds
            pwrLightOff(1);
            pwrLightOff(2);
            delay(300); //Sleeps for two seconds
        }
    }
    else if (batteryLevel < 15) {
        for (int i =0; i < 2 ; ++i) {
            ui->oneLabel->setStyleSheet("color: " RED ";");
            delay(300); //Sleeps for two seconds
            pwrLightOff(1);
            delay(300); //Sleeps for two seconds
        }
    }
    cout << "Battery Level is at: "<< batteryLevel << endl;
}

/* --------------------------------START OF SLOTS--------------------------------- */

void MainWindow:: promptSignIn() {

    //Force users to make or select profile. Or select Guest

    if (ui->profileSelector->currentIndex() == 0) {
        cout << "Select a profile to sign in. ('Guest' is not a profile.)" <<endl;
        signedInStatus = false;
    } else {
        selectUser();
        signedInStatus = true;
        updateView();
    }
}

void MainWindow:: handlePowerPress() {
    // turn on device
    if (!powerStatus) {
        int te = powerHoldTimer.elapsed();
        if (te > 500) {
            cout << "Turning on device" << endl;
            togglePowerStatus();
            return;
        }
    }

    else {
        int te = powerHoldTimer.elapsed();
        if (te > 500) {
            cout << "Turning off device" << endl;
            togglePowerStatus();
            return;
        }
        // Cycle through the session groups, since the only difference between the groups is duration, only the durations vector will be cycled through
        // If the end of the array is reached, set index back to 0
        if (!inSessionStatus) {
            if (curSessionGroupIndex == 2) {
                curSessionGroupIndex = 0;

                // disable mode change button
                ui->modeButton->setEnabled(false);
            }
            // Else, increment the index to reach the next duration, which indicates the next session group
            else {
                curSessionGroupIndex += 1;
                // disable mode change button
                ui->modeButton->setEnabled(false);
            }
            // If the User Designated Group is chosen, allow the user to choose a custom duration, session frequency type and ces mode
            if (curSessionGroupIndex == 2) {
               // enable mode change button
               ui->modeButton->setEnabled(true);
               ui->incDurationButton->setEnabled(true);
               ui->decDurationButton->setEnabled(true);

            }
        }

        else {
            softOff();
            return;
        }

        curDuration = durations[curSessionGroupIndex]; // Set current duration accordingly

        // Update the UI to reflect changes
        updateSessionsMenu(false);

//        qInfo("Session Group: %i", curSessionGroupIndex);
    }
}

void MainWindow:: handleDownPress() {
    // If NOT currently in a session:
    if (!inSessionStatus)
        // Cycle through the sessions/session types
        // If index = 0 is reached, reset index back to 3
        // if current duration is user-defined, the options to change frequency available
        if (curSessionGroupIndex == 2) {
            if (curFrequencyIndex == 0) {
                curFrequencyIndex = 3;
            }
            // Else, decrement the index to reach the next session
            else {
                curFrequencyIndex -= 1;
            }
            // Update the UI to reflect changes
            updateSessionsMenu(false);

            qInfo("Frequency: %i", curFrequencyIndex);
        }

        // if current duration is not user-defined
        else {
            if (curSessionIndex == 0) {
                curSessionIndex = 3;
            }
            // Else, decrement the index to reach the next session
            else {
                curSessionIndex -= 1;
            }
            // Update the UI to reflect changes
            updateSessionsMenu(false);

            qInfo("Pre-defined session: %i", curSessionIndex);
        }
    // Else, the Up/Down press will be used to adjust the Intensity Level
    else {
        if (curIntensity == 1) {
            curIntensity = 8;
        }
        else {
            curIntensity -= 1;
        }
        // Update the UI to reflect the new Intensity level
        updateIntensityUI();
    }
}

void MainWindow:: handleUpPress() {
    // If NOT currently in a session
    if (!inSessionStatus) {
        // Cycle through the sessions, since sessions and session frequency types are one and the same, we will cycle through the session
        // frequency types icons
        // If index = 3 is reached, reset index back to 0
        // if current duration is user-defined, the options to change frequency available
        if (curSessionGroupIndex == 2) {
            if (curFrequencyIndex == 3) {
                curFrequencyIndex = 0;
            }
            // Else, decrement the index to reach the next session
            else {
                curFrequencyIndex += 1;
            }
            // Update the UI to reflect changes
            updateSessionsMenu(false);

            qInfo("Frequency: %i", curFrequencyIndex);
        }

        // if current duration is not user-defined
        else {
            if (curSessionIndex == 3) {
                curSessionIndex = 0;
            }
            // Else, decrement the index to reach the next session
            else {
                curSessionIndex += 1;
            }
            // Update the UI to reflect changes
            updateSessionsMenu(false);

            qInfo("Pre-defined session: %i", curSessionIndex);
        }
    }
    // Else, the Up/Down press will be used to adjust the Intensity Level
    else {
        if (curIntensity == 8) {
            curIntensity = 1;
        }
        else {
            curIntensity += 1;
        }
        // Update the UI to reflect the new Intensity level
        updateIntensityUI();
    }
}

void MainWindow:: handleSelectPress() {
    //Either start session or choose behaviour depending on mode
    if(!inSessionStatus){
        // according to manual, session begins after a 2.5 sec delay
        cout << "Session will begin in 2.5 seconds. Please be patient." << endl;
        ui->selectButton->setDisabled(true);
        delay(2500);
        ui->selectButton->setDisabled(false);
        if (startConnectionTest()) {
            startSession(false);
        }
    }
    else{
        int te = selectHoldTimer.elapsed();
        if (te >= 500){
            if (!signedInStatus){
                cout << "Please Sign in Before Attempting to Record Therapies." <<endl;
            } else {
                recordTherapy();
            }
        }
    }
}

void MainWindow:: handleSelectSavedPress() {
    //Either start session or choose behaviour depending on mode
    if (signedInStatus && curUser->getRecords().size() > 0) {
        if(!inSessionStatus){
            // according to manual, session begins after a 2.5 sec delay
            cout << "Session will begin in 2.5 seconds. Please be patient." << endl;
            ui->selectSavedButton->setDisabled(true);
            delay(2500);
            ui->selectSavedButton->setDisabled(false);
            if (startConnectionTest()) {
                startSession(true);
            }
        }
    }else {
        cout << "Make Sure to Sign In and/or Record a Session Before Starting a Saved Sessions." <<endl;
    }

}

void MainWindow:: handleBatteryLow() {
    // End session if session running, if during a session continuously keep calling the function
    // Continue blinking as well while ending session
    if (batteryLevel < 15) {
        // Pause the session
        sessionTimer.stop();
        // Simulate the bar blinking continuously for a short time to indicate low battery level
        for (int i = 0; i < 2; i++) {
            displayBatteryLevel();
        }
        // Emit a timeout signal to end the session early
        if(batteryLevel <= 0){
            cout << "Battery is depleted, device powering off..." << endl;
            handlePowerPress();
        }
        sessionTimer.QTimer::qt_metacall(QMetaObject::InvokeMetaMethod, 5, {});
    }
}

void MainWindow::handleAddProfilePress()
{
    UserProfile* newUser;
    //Create user
    createUser(ui->nameInput->text().toStdString(), &newUser);
    users.push_back(newUser);

    cout <<"Profile: " << newUser->getUsername() << " Added." <<endl;
    //Add profile to profileSelector
    updateView();
}

void MainWindow::handleModePress() {
    // if current duration/session group is user-defined
    if (curSessionGroupIndex == 2) {
        switch (curModeIndex) {
        case 0: curModeIndex = 1; break;
        case 1: curModeIndex = 0; break;
        }
    }

    updateModeUI();
}

void MainWindow::updatePerSecond() {

    // if device is idle for more than 120 secs, then it is turned off
    if (powerStatus && idleTimer.hasExpired(120 * 1000) && !inSessionStatus) {
        togglePowerStatus();
        return;
    }

    if(inSessionStatus) {
        if (sessionTimer.isActive()) {
            int rt = sessionTimer.remainingTime() / 1000;

            QString rtStr = QString::number(rt) + ":00";
            ui->remainTimeN->setText(rtStr);

            // Update and Display the battery level periodically while session is running
            // and when the session is done
            // prevRt is the rt (remaining time) when the battery level was last updated
            if ((prevRt != rt) && (rt % 10 == 0)) {
                updateBatteryLevel(prevRt - rt);
                // If there is remaining time left:
                if (rt > 0) {
                    // UI updates to display battery level, then show intensity level again
                    displayBatteryLevel();
                    updateIntensityUI();
                }
                prevRt = rt;
                // Check if battery is low and handle it accordingly
                handleBatteryLow(); //End session if running, and continue blinking for a short period while ending session
            }
        }
    }
}

void MainWindow::handlePowerHold() {
    powerHoldTimer.start();
}

void MainWindow::handleSelectHold() {
    selectHoldTimer.start();
}

void MainWindow::handleChargePress() {
    // if battery < 15, then the buttons are enabled, then battery is recharged
    if (batteryLevel < 15) {
        toggleButtonState(powerStatus);
    }
    batteryLevel = 100;
    if (powerStatus) {
        displayBatteryLevel();
        updateSessionsMenu(false);
    }
    cout << "Battery has been charged to: " << batteryLevel << endl;

 }

void MainWindow::handleIncreaseDurationPress() {
    if (curSessionGroupIndex == 2 && durations[2] <= MAX_USER_DUR) {
        durations[2] += 5;
        ui->setDurationNLabel->setText(QString::number(durations[2]) + ":00");
    }

}

void MainWindow::handleDecreaseDurationPress() {
    if (curSessionGroupIndex == 2 && durations[2] >= 5) {
        durations[2] -= 5;
        ui->setDurationNLabel->setText(QString::number(durations[2]) + ":00");
    }

}

/* ---------------------------------END OF SLOTS--------------------------- */






/* ---------------------------------UI UPDATES----------------------------- */

void MainWindow::updateView() {

    ui->profileSelector->clear(); //Clears Previous Values

    for (int i = 0; i < int(users.size()); ++i) {
        string temp = users.at(i)->getUsername();
        ui->profileSelector->addItem(QString::fromStdString(temp));
    }

    ui->sessionSelector->clear();
    vector<Record*> userRecords = curUser->getRecords();
    for(int i = 0; i < int(userRecords.size()); ++i){
        stringstream temp;
        temp << *userRecords.at(i);
        ui->sessionSelector->addItem(QString::fromStdString(temp.str()));
    }
}

void MainWindow::updateModeUI() {
    // user-defined session group, so they can change mode
    if (curSessionGroupIndex == 2) {
        if(curModeIndex == 0) {
            ui->shortPulse->setStyleSheet("image : url(:/pulses/Short_pulse_green.png)");
            ui->longPulse->setStyleSheet("image : url(:/pulses/Long_pulse.png)");
        }
        else {
            ui->longPulse->setStyleSheet("image : url(:/pulses/Long_pulse_green.png)");
            ui->shortPulse->setStyleSheet("image : url(:/pulses/Short_pulse.png)");
        }
    }

    // if session group is pre-defined
    else {
        switch (curSessionIndex) {
        case 0: {ui->shortPulse->setStyleSheet("image : url(:/pulses/Short_pulse_green.png)");
                ui->longPulse->setStyleSheet("image : url(:/pulses/Long_pulse.png)");} break;
        case 1: {ui->longPulse->setStyleSheet("image : url(:/pulses/Long_pulse_green.png)");
                ui->shortPulse->setStyleSheet("image : url(:/pulses/Short_pulse.png)");} break;
        case 2: {ui->shortPulse->setStyleSheet("image : url(:/pulses/Short_pulse_green.png)");
                ui->longPulse->setStyleSheet("image : url(:/pulses/Long_pulse.png)");} break;
        case 3: {ui->shortPulse->setStyleSheet("image : url(:/pulses/Short_pulse_green.png)");
                ui->longPulse->setStyleSheet("image : url(:/pulses/Long_pulse.png)");} break;
        }
    }

    ui->shortPulse->repaint();
    ui->longPulse->repaint();
}

void MainWindow:: updateSessionsMenu(bool fromSaved=false) {
    // Update UI when cycling through session groups
    allSessionGroupLightOff();
    switch (curSessionGroupIndex) {
        case 0: groupTwentyMinLightOn(); break;
        case 1: groupFortyFiveMinLightOn(); break;
        case 2: groupUserDesignatedLightOn(); break;
    }

    // Update UI when cycling through sessions (i.e. session frequency types)
    allFrequencyLightOff();

    // if current duration is user-defined, change light according to user-selected frequency and turn off number light of current session type
    if (curSessionGroupIndex == 2 || fromSaved) {
        pwrLightOff(curSessionIndex+1);
        switch (curFrequencyIndex) {
            case 0: sessionMETLightOn(); break;
            case 1: sessionSDeltaLightOn(); break;
            case 2: sessionDeltaLightOn(); break;
            case 3: sessionThetaLightOn(); break;
        }
    }

    // if current duration is pre-defined, change light according to pre-defined session frequency
    else {
        switch (curSessionIndex) {
        case 0: {pwrLightOn(1); pwrLightOff(2); pwrLightOff(3); pwrLightOff(4); sessionMETLightOn();} break;
        case 1: {pwrLightOn(2); pwrLightOff(1); pwrLightOff(3); pwrLightOff(4); sessionSDeltaLightOn();} break;
        case 2: {pwrLightOn(3); pwrLightOff(2); pwrLightOff(1); pwrLightOff(4); sessionDeltaLightOn();} break;
        case 3: {pwrLightOn(4); pwrLightOff(2); pwrLightOff(3); pwrLightOff(1); sessionThetaLightOn();} break;
        }
    }

    updateModeUI();
}

// Function to make UI changes based on the current Intensity level
void MainWindow::updateIntensityUI() {
    // Turn off light for all Intensity numbers
    pwrLightOffAll();
    // Turn on the light for the Current Intensity Level
    pwrLightOn(curIntensity);
}

// Functions for Session Group and Session Numbers UI changes:
void MainWindow::groupTwentyMinLightOn() {
    ui->twentyMinLabel->setStyleSheet("color: " YELLOW ";");
    ui->twentyMinLabel->repaint();
}

void MainWindow::groupFortyFiveMinLightOn() {
    ui->fortyFiveMinLabel->setStyleSheet("color: " YELLOW ";");
    ui->fortyFiveMinLabel->repaint();
}

void MainWindow::groupUserDesignatedLightOn() {
    ui->userDesignatedLabel->setStyleSheet("color: " YELLOW ";");
    ui->modeButton->setEnabled(true);
    ui->selectSavedButton->setEnabled(true);
    ui->modeButton->setStyleSheet("color: #e5e400;"
                                  "border: 0.2em solid #80c3bf;");
    ui->selectSavedButton->setStyleSheet("color: #e5e400;"
                                         "border-radius: 1.5em;"
                                         "border: 0.2em solid #80c3bf;");
    ui->centralwidget->repaint();
}
void MainWindow::groupUserDesignatedLightOff(){
    ui->userDesignatedLabel->setStyleSheet("color:grey;");
    ui->modeButton->setEnabled(false);
    ui->selectSavedButton->setEnabled(false);
    ui->modeButton->setStyleSheet("color: black;"
                                  "border: 0.2em solid black;");
    ui->selectSavedButton->setStyleSheet("color: black;"
                                         "border: 0.2em solid black;");
    ui->centralwidget->repaint();
}

void MainWindow::allSessionGroupLightOff() {
    ui->twentyMinLabel->setStyleSheet("color: grey");
    ui->twentyMinLabel->repaint();

    ui->fortyFiveMinLabel->setStyleSheet("color: grey");
    ui->fortyFiveMinLabel->repaint();

    groupUserDesignatedLightOff();
}

void MainWindow::sessionMETLightOn() {
    ui->METLabel->setStyleSheet("color: " GREEN ";");
    ui->METLabel->repaint();
}

void MainWindow::sessionSDeltaLightOn() {
    ui->sDeltaLabel->setStyleSheet("color: " GREEN ";");
    ui->sDeltaLabel->repaint();
}

void MainWindow::sessionDeltaLightOn() {
    ui->deltaLabel->setStyleSheet("color: " GREEN ";");
    ui->deltaLabel->repaint();
}

void MainWindow::sessionThetaLightOn() {
    ui->thetaLabel->setStyleSheet("color: " GREEN ";");
    ui->thetaLabel->repaint();
}

void MainWindow::allFrequencyLightOff() {
    ui->METLabel->setStyleSheet("color: grey");
    ui->METLabel->repaint();

    ui->sDeltaLabel->setStyleSheet("color: grey");
    ui->sDeltaLabel->repaint();

    ui->deltaLabel->setStyleSheet("color: grey");
    ui->deltaLabel->repaint();

    ui->thetaLabel->setStyleSheet("color: grey");
    ui->thetaLabel->repaint();
}

void MainWindow:: changeButtonStyles(QPushButton* btn, QEvent* event){
    if(powerStatus) {
        if(event->type() == QEvent::Enter){
            btn->setStyleSheet("color: " YELLOW ";"
                                 "border: 0.2em solid " LIGHTBLUE ";" ";"
                                 "min-height: 3em;"
                                 "max-height: 3em;"
                                 "min-width: 3em;"
                                 "max-width: 3em;"
                                 "border-radius: 1.5em;");

        }
        else if(event->type() == QEvent::Leave){
            btn->setStyleSheet("color: " YELLOW ";"
                                "border: 0.2em solid " BLUE ";" ";"
                                "min-height: 3em;"
                                "max-height: 3em;"
                                "min-width: 3em;"
                                "max-width: 3em;"
                                "border-radius: 1.5em;");

        }
    }

    else {
        if (btn == ui->pwrButton) {
            if(event->type() == QEvent::Enter){
                btn->setStyleSheet("color: " YELLOW ";"
                                     "border: 0.2em solid " LIGHTBLUE ";" ";"
                                     "min-height: 3em;"
                                     "max-height: 3em;"
                                     "min-width: 3em;"
                                     "max-width: 3em;"
                                     "border-radius: 1.5em;");

            }
            else if(event->type() == QEvent::Leave){
                btn->setStyleSheet("color: " YELLOW ";"
                                    "border: 0.2em solid " BLUE ";" ";"
                                    "min-height: 3em;"
                                    "max-height: 3em;"
                                    "min-width: 3em;"
                                    "max-width: 3em;"
                                    "border-radius: 1.5em;");

            }
        }
    }
}

void MainWindow::toggleButtonState(bool state) {
    ui->upButton->setEnabled(state);
    ui->downButton->setEnabled(state);
    ui->selectButton->setEnabled(state);
    ui->tabWidget->setEnabled(state);
    ui->modeButton->setEnabled(state);
    ui->selectSavedButton->setEnabled(state);
    ui->incDurationButton->setEnabled(state);
    ui->decDurationButton->setEnabled(state);

}

void MainWindow::greenLightOn(){
    ui->pwrLight->setStyleSheet("background-color: " GREEN ";");
    ui->pwrLight->repaint();
}

void MainWindow::greenLightOff(){
    ui->pwrLight->setStyleSheet("color: black");
    ui->pwrLight->repaint();
}

bool MainWindow:: eventFilter(QObject* obj, QEvent* event){
    if(obj == (QObject*)ui->pwrButton){
        changeButtonStyles(ui->pwrButton, event);
    }
    else if(obj == (QObject*)ui->upButton){
        changeButtonStyles(ui->upButton, event);
    }
    else if(obj == (QObject*)ui->downButton){
        changeButtonStyles(ui->downButton, event);
    }
    else if(obj == (QObject*)ui->selectButton){
        changeButtonStyles(ui->selectButton, event);
    }
    else if(obj == (QObject*)ui->selectSavedButton && ui->selectSavedButton->isEnabled()){
        changeButtonStyles(ui->selectSavedButton, event);
    }
    return QWidget::eventFilter(obj, event);
}

