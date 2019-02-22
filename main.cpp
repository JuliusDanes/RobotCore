#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <vector>
#include <thread>
#include <pthread.h>
#include <mutex>
#include <map>
#include <sstream>
#include <vector>
#include <regex>
#include <math.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <fstream>

using namespace std;

string useAs, myIP = "0.0.0.0", ballOn = "", serialPort = "/dev/ttyUSB0";
bool ball = false, transpose = false;
int listening, bufSize = 4096;
vector <int> PosXYZ {0, 0, 0}, tempPosXYZ {0, 0, 0}, shift {1, 1, 1};
map<string, thread> gotoDict, th_Receiveds;
map<string, int> socketDict;
sockaddr_in client;
mutex m;
FILE* connectArduinoW = fopen(serialPort.c_str(), "w"); //Opening device file;

inline bool isBlank(const string &s)
{
    return std::all_of(s.cbegin(), s.cend(), [](char c) { return std::isspace(c); });
}

string trim_control(const string &str)
{
    string s = "";
    for (auto c : str)
        if (!iscntrl(c))
            s += c;
    return s;
}

string trim_left(const string &str)
{
    const string pattern = " \f\n\r\t\v";
    return str.substr(str.find_first_not_of(pattern));
}

string trim_right(const string &str)
{
    const string pattern = " \f\n\r\t\v";
    return str.substr(0, str.find_last_not_of(pattern) + 1);
}

string trim(const string &str)
{
    return trim_control(trim_left(trim_right(str)));
}

string toLowers(string &s)
{
    for (int i =0; i< s.size(); i++)
        s[i] = tolower(s[i], locale());
    return s;
}

void swapS(string &a, string &b)
{
    auto _temp = a;
    a = b;
    b = _temp;
}

void swapI(int &a, int &b)
{
    auto _temp = a;
    a = b;
    b = _temp;
}

void splitS(string &text, char delimiter, vector<string> &vec)
{
    string item;
    for (stringstream ss(text); getline(ss, item, delimiter); (vec.push_back(item)));
}

void splitI(string &text, char delimiter, vector<int> &vec)
{
    string item;
    for (stringstream ss(text); getline(ss, item, delimiter); (vec.push_back(stoi(item))));
}

template<typename K, typename V>
string keyByValue(map<K, V> m, V value)
{
    for (auto &i : m)
        if (i.second == value)
            return i.first;
    return 0;
}

int kbhit(void)
{
    struct termios oldt, newt;
    int ch;
    int oldf;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1; }
    return 0;
}

string getMyIP(){
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;
    int ctr = 0;
    myIP.clear();

    getifaddrs(&ifAddrStruct);
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr)
            continue;

        if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            if (trim(ifa->ifa_name) != "lo") {
                if (ctr > 0)
                    myIP += '|';
            myIP += addressBuffer; ctr++; }
        } else if (ifa->ifa_addr->sa_family == AF_INET6) { // check it is IP6
            // is a valid IP6 Address
            tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
            // printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
        }
    }
    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
    return myIP;
}

void checkConnection()
{
    try {
        // for (m.lock(); true; m.unlock())
        {
            for (auto &i : socketDict) {
                string key = i.first;
                if (i.second == 0) {
                    // close(i.second);
                    th_Receiveds[key].detach();
                    th_Receiveds.erase(key);
                    socketDict.erase(key); } }
        // usleep(1000000);
        }
    }
    catch (exception e) {
        cout << "# Check Connection error \n~\n" << e.what() << endl; }
}

void sendCallBack(int clientSocket, string message)
{
    try {
        if ((!isBlank(trim(message))) /*&& (toLowers(message) != "quit")*/) {
            char buf[bufSize];

            if (clientSocket != 0)
            {
                //	Send to server
                int sendRes = send(clientSocket, trim(message).c_str(), message.size() + 1, 0);
                if (sendRes == -1) {
                    cout << "Could not send to server! Whoops!\r\n";
                    clientSocket = 0;
                    return; }
                else {
                    cout << "@ " << keyByValue(socketDict, clientSocket) << " : " << trim(message) << endl; }

                // //		Wait for response
                // memset(buf, 0, bufSize);
                // int bytesReceived = recv(clientSocket, buf, bufSize, 0);
                // if (bytesReceived == -1) {
                //     cout << "There was an error getting response from server\r\n"; }
                // else {
                //     //		Display response
                //     cout << "SERVER> " << string(buf, bytesReceived) << "\r\n"; }
            }
            else
                printf("!...Not Connected...!"); }
    }
    catch (exception e) {
        cout << "# Send Callback error \n~\n" << e.what() << endl; }
}

void sendPosXYZ()
{
    if (socketDict.count("BaseStation")) {
        string message = "E" + to_string(PosXYZ[0]) + "," + to_string(PosXYZ[1]) + "," + to_string(PosXYZ[2]);
        sendCallBack(socketDict["BaseStation"], message); }
}

void GotoLoc (string Robot, int endX, int endY, int endAngle, int shiftX, int shiftY, int shiftAngle)
{
    try {
        cout << "# " << Robot << " : Goto >> " << "X:" << endX << " Y:" << endY << " ∠:" << endAngle << "°" << endl;
        bool chk[] = { true, true, true };
        while (chk[0] |= chk[1] |= chk[2]) {
            if (tempPosXYZ[0] > 12000)
                tempPosXYZ[0] = stoi (to_string(tempPosXYZ[0]).substr (0, 4));
            if (tempPosXYZ[1] > 9000)
                tempPosXYZ[1] = stoi (to_string(tempPosXYZ[1]).substr (0, 4));
            if (tempPosXYZ[2] > 360)
                tempPosXYZ[2] = stoi (to_string(tempPosXYZ[2]).substr (0, 2));

            if ((tempPosXYZ[0] > endX) && (shiftX > 0))
                shiftX *= -1;
            else if ((tempPosXYZ[0] < endX) && (shiftX < 0))
                shiftX *= -1;
            if ((tempPosXYZ[1] > endY) && (shiftY > 0))
                shiftY *= -1;
            else if ((tempPosXYZ[1] < endY) && (shiftY < 0))
                shiftY *= -1;
            if ((tempPosXYZ[2] > endAngle) && (shiftAngle > 0))
                shiftAngle *= -1;
            else if ((tempPosXYZ[2] < endAngle) && (shiftAngle < 0))
                shiftAngle *= -1;

            if (tempPosXYZ[0] != endX) {
                if (abs (endX - tempPosXYZ[0]) < abs (shiftX)) // Shift not corresponding
                    shiftX = (endX - tempPosXYZ[0]);
                tempPosXYZ[0] += shiftX; // On process
            } else
                chk[0] = false; // Done
            if (tempPosXYZ[1] != endY) {
                if (abs (endY - tempPosXYZ[1]) < abs (shiftY)) // Shift not corresponding
                    shiftY = (endY - tempPosXYZ[1]);
                tempPosXYZ[1] += shiftY; // On process
            } else
                chk[1] = false; // Done
            if (tempPosXYZ[2] != endAngle) {
                if (abs (endAngle - tempPosXYZ[2]) < abs (shiftAngle)) // Shift not corresponding
                    shiftAngle = (endAngle - tempPosXYZ[2]);
                tempPosXYZ[2] += shiftAngle; // On process
            } else
                chk[2] = false; // Done

            sendPosXYZ();
            usleep (100000); // time per limit (microsecond)
        }
    } catch (exception e)  {
        cout << "% Error GotoLoc \n\n" << e.what() << endl; }
}

void threadGoto (string keyName, string message)
{
    string item;
    vector<int> dtXYZ;
    if (gotoDict.count(keyName)) {
        gotoDict[keyName].detach();
        gotoDict.erase (keyName);
    }
    for (stringstream ss(message); (getline(ss, item, ',')); (dtXYZ.push_back(stoi(item))));
    gotoDict[keyName] = thread( GotoLoc, useAs, dtXYZ[0], dtXYZ[1], dtXYZ[2], 20, 20, 1);
}

void fromArduino()
{
    char buffer[1024];
    int numBytesRead;
    FILE *connectArduinoR = fopen(serialPort.c_str(), "r");

    if (connectArduinoR == NULL) {
        printf("ERROR_CONNECT_ARDUINO");
        return; }

    // for (m.lock(); true; m.unlock())
    while ((true) && (connectArduinoR != NULL))
    {
        memset(buffer, 0, 1024);
        fread(buffer, sizeof(char), 1024, connectArduinoR);
        string message = string(buffer);
        if (!isBlank(message)) {
            message = trim(message);
            if (regex_match(message.begin(), message.end(), regex("^[-]{0,1}[0-9]{1,5},[-]{0,1}[0-9]{1,5},[-]{0,1}[0-9]{1,5}E$"))) {
                vector<string> dataVec1;
                vector<int> dataVec2;
                splitS(message, 'E', dataVec1);
                splitI(dataVec1[0], ',', dataVec2);
                if (PosXYZ != dataVec2) {
                    PosXYZ = dataVec2;
                    sendPosXYZ(); } }
        }
        // usleep(50000);
    }
}

void toArduino(string message)
{
    if (!isBlank(message))
        if (connectArduinoW == NULL)
            printf("ERROR_CONNECT_ARDUINO");
        else {
            fprintf(connectArduinoW, "%s", (message + "\n").c_str());    //Writing to the Arduino
            printf("@@ Arduino : %s \n", message.c_str()); }
}

string ResponeSendCallback(int clientSocket, string message)
{
    string respone = "", text = "", item;
    vector<string> _dtMessage;
    for (stringstream ss(message); (getline(ss, item, '|')); (_dtMessage.push_back(item)));

    if ((_dtMessage[0].find("!") == 0) && (_dtMessage[0].size() > 1)) {
        _dtMessage.clear();
        _dtMessage.push_back(_dtMessage[0].substr(1)); _dtMessage.push_back("Robot1,Robot2,Robot3"); }
    if ((_dtMessage[0].find("**") == 0) && (_dtMessage[0].size() > 2)) {
        respone = _dtMessage[0];
        goto broadcast; }
    else if ((_dtMessage[0].find("*") == 0) && (_dtMessage[0].size() > 1)) {
        goto multicast; }

    if (toLowers(_dtMessage[0]) == "myip") {
        text = "MyIP: "+ getMyIP();
        respone = getMyIP();
        if (clientSocket != 0)
            goto multicast;
        goto end; }
    else if (toLowers(_dtMessage[0]) == "as") {
        text = "UseAs: "+ useAs;
        goto end; }
    // else if ((_socketDict.ContainsKey("BaseStation")) && (socket.Client.RemoteEndPoint.ToString().Contains(_socketDict["BaseStation"].Client.RemoteEndPoint.ToString())))
    else if (clientSocket != 0)
    {
        // If to send Base Station socket
        /// LOCATION ///
        if (regex_match(_dtMessage[0].begin(), _dtMessage[0].end(), regex("^(go|Go|gO|GO)[-]{0,1}[0-9]{1,5},[-]{0,1}[0-9]{1,5},[-]{0,1}[0-9]{1,5}$")))
        { //Goto Location
            if (_dtMessage.size() > 1)
                goto multicast;
            else
                threadGoto(useAs, _dtMessage[0].substr(2));
            goto end;
        }

        /// INFORMATION ///
        else if (_dtMessage[0] == "B")
        { //Get the Ball
            ball = true;
            respone = "B_";
            goto broadcast;
        }
        else if (_dtMessage[0] == "b")
        { //Lose the Ball
            ball = false;
            respone = "b_";
            goto broadcast;
        }

        /// OTHERS ///
        else if (_dtMessage[0] == ";")
        { //PING
            respone = "ping";
            goto multicast;
        }
    }
    goto multicast;

broadcast:
    sendCallBack(clientSocket, respone + "|" + "Robot1,Robot2,Robot3");
    // sendByHostList("BaseStation", respone + "|" + "Robot1,Robot2,Robot3");
    goto end;

multicast:
    if (isBlank(respone))
        respone = _dtMessage[0];
    if (_dtMessage.size() > 1)
        sendCallBack(clientSocket, respone + "|" + _dtMessage[1]);
    else
        sendCallBack(clientSocket, respone);
    // sendByHostList("BaseStation", respone + "|" + chkRobotCollect);
    goto end;

end:
    if (!isBlank(text))
        cout << "# " << text << endl;
    return respone;
}

string ResponeReceivedCallback(int clientSocket, string message)
{
    string respone = "", text = "", item;
    vector<string> _dtMessage, msgXYZs, msgXYZ;
    for (stringstream ss(message); (getline(ss, item, '|')); (_dtMessage.push_back(item)));
    if ((_dtMessage[0].find("!") == 0) && (_dtMessage[0].size() > 1)) {         // Broadcast message
        _dtMessage.clear();
        _dtMessage.push_back(_dtMessage[0].substr(1)); _dtMessage.push_back("Robot1,Robot2,Robot3"); }
    if ((_dtMessage[0].find("**") == 0) && (_dtMessage[0].size() > 2)) {        // Forward & Broadcast message
        respone = _dtMessage[0].substr(2);
        goto broadcast; }
    else if ((_dtMessage[0].find("*") == 0) && (_dtMessage[0].size() > 1)) {    // Forward & Multicast essage
        respone = _dtMessage[0].substr(1);
        goto multicast; }

    if (regex_match(_dtMessage[0].begin(), _dtMessage[0].end(), regex("^E[-]{0,1}[0-9]{1,5},[-]{0,1}[0-9]{1,5},[-]{0,1}[0-9]{1,5}$")))
    {
        // If message is data X, Y, Z
        toArduino(_dtMessage[0]);

        // for (stringstream ss(_dtMessage[0]); (getline(ss, item, 'E')); (msgXYZs.push_back(item)));
        // for (stringstream ss(msgXYZs.back()); (getline(ss, item, ',')); (msgXYZ.push_back(item)));

        // for (int i = 0; i < msgXYZ.size(); i++)
        //     tempPosXYZ[i] = stoi(msgXYZ[i]);
        // sendPosXYZ();
        // text = "X:" + to_string(tempPosXYZ[0]) + " Y:" + to_string(tempPosXYZ[1]) + " ∠:" + to_string(tempPosXYZ[2]) + "°";
    }
    else if (regex_match(_dtMessage[0].begin(), _dtMessage[0].end(), regex("^(go|Go|gO|GO)[-]{0,1}[0-9]{1,5},[-]{0,1}[0-9]{1,5},[-]{0,1}[0-9]{1,5}$")))
    { //Goto Location
        if (_dtMessage.size() > 1)
            goto multicast;
        else
            threadGoto(useAs, _dtMessage[0].substr(2));
        goto end;
    }
    // else if ((_socketDict.ContainsKey("BaseStation")) && (socket.Client.RemoteEndPoint.ToString().Contains(_socketDict["BaseStation"].Client.RemoteEndPoint.ToString())))
    // else if ((clientSocket != 0) && (socketDict.count("BaseStation")) && (keyByValue(socketDict, clientSocket) == "BaseStation"))
    else if (clientSocket != 0)
    // else if (true)
    {
        // If socket is Base Station socket
        ////    REFEREE BOX COMMANDSt	////
        ///{
        /// 1. DEFAULT COMMANDS ///
            if (_dtMessage[0] == "S") { //STOP
                text = "STOP"; }
            else if (_dtMessage[0] == "s") { //START
                text = "START"; }
            else if (_dtMessage[0] == "W") { //WELCOME (welcome message)
                text = "WELCOME"; }
            else if (_dtMessage[0] == "Z") { //RESET (Reset Game)
                text = "RESET"; }
            else if (_dtMessage[0] == "U") { //TESTMODE_ON (TestMode On)
                text = "TESTMODE_ON"; }
            else if (_dtMessage[0] == "u") { //TESTMODE_OFF (TestMode Off)
                text = "TESTMODE_OFF"; }

        /// 3. GAME FLOW COMMANDS ///
            else if (_dtMessage[0] == "1") { //FIRST_HALF
                text = "FIRST_HALF"; }
            else if (_dtMessage[0] == "2") { //SECOND_HALF
                text = "SECOND_HALF"; }
            else if (_dtMessage[0] == "3") { //FIRST_HALF_OVERTIME
                text = "FIRST_HALF_OVERTIME"; }
            else if (_dtMessage[0] == "4") { //SECOND_HALF_OVERTIME
                text = "SECOND_HALF_OVERTIME"; }
            else if (_dtMessage[0] == "h") { //HALF_TIME
                text = "HALF_TIME"; }
            else if (_dtMessage[0] == "e") { //END_GAME (ends 2nd part, may go into overtime)
                text = "END_GAME"; }
            else if (_dtMessage[0] == "z") { //GAMEOVER (Game Over)
                text = "GAMEOVER"; }
            else if (_dtMessage[0] == "L") { //PARKING
                text = "PARKING"; }
            else if (_dtMessage[0] == "N") { //DROP_BALL
                text = "DROP_BALL"; }

        /// 2. PENALTY COMMANDS ///
            else if (_dtMessage[0] == "Y") { //YELLOW_CARD_CYAN
                text = "YELLOW_CARD_CYAN"; }
            else if (_dtMessage[0] == "R") { //RED_CARD_CYAN
                text = "RED_CARD_CYAN"; }
            else if (_dtMessage[0] == "B") { //DOUBLE_YELLOW_CYAN
                text = "DOUBLE_YELLOW_CYAN"; }

        /// 4. GOAL STATUS ///
            else if (_dtMessage[0] == "A") { //GOAL_CYAN
                text = "GOAL_CYAN"; }
            else if (_dtMessage[0] == "D") { //SUBGOAL_CYAN
                text = "SUBGOAL_CYAN"; }

        /// 5. GAME FLOW COMMANDS ///
            else if (_dtMessage[0] == "K") { //KICKOFF_CYAN
                text = "KICKOFF_CYAN"; }
            else if (_dtMessage[0] == "F") { //FREEKICK_CYAN
                text = "FREEKICK_CYAN"; }
            else if (_dtMessage[0] == "G") { //GOALKICK_CYAN
                text = "GOALKICK_CYAN"; }
            else if (_dtMessage[0] == "T") { //THROWN_CYAN
                text = "THROWN_CYAN"; }
            else if (_dtMessage[0] == "C") { //CORNER_CYAN
                text = "CORNER_CYAN"; }
            else if (_dtMessage[0] == "P") { //PENALTY_CYAN
                text = "PENALTY_CYAN"; }
            else if (_dtMessage[0] == "O") { //REPAIR_CYAN
                text = "REPAIR_CYAN"; }

        /// 2. PENALTY COMMANDS ///
            else if (_dtMessage[0] == "y") { //YELLOW_CARD_MAGENTA
                text = "YELLOW_CARD_MAGENTA"; }
            else if (_dtMessage[0] == "r") { //RED_CARD_MAGENTA
                text = "RED_CARD_MAGENTA"; }
            else if (_dtMessage[0] == "b") { //DOUBLE_YELLOW_MAGENTA
                text = "DOUBLE_YELLOW_MAGENTA"; }

        /// 4. GOAL STATUS ///
            else if (_dtMessage[0] == "a") { //GOAL_MAGENTA
                text = "GOAL_MAGENTA"; }
            else if (_dtMessage[0] == "d") { //SUBGOAL_MAGENTA
                text = "SUBGOAL_MAGENTA"; }

        /// 5. GAME FLOW COMMANDS ///
            else if (_dtMessage[0] == "k") { //KICKOFF_MAGENTA
                text = "KICKOFF_MAGENTA"; }
            else if (_dtMessage[0] == "f") { //FREEKICK_MAGENTA
                text = "FREEKICK_MAGENTA"; }
            else if (_dtMessage[0] == "g") { //GOALKICK_MAGENTA
                text = "GOALKICK_MAGENTA"; }
            else if (_dtMessage[0] == "t") { //THROWN_MAGENTA
                text = "THROWN_MAGENTA"; }
            else if (_dtMessage[0] == "c") { //CORNER_MAGENTA
                text = "CORNER_MAGENTA"; }
            else if (_dtMessage[0] == "p") { //PENALTY_MAGENTA
                text = "PENALTY_MAGENTA"; }
            else if (_dtMessage[0] == "o") { //REPAIR_MAGENTA
                text = "REPAIR_MAGENTA"; }
        ///}

        /// INFORMATION ///
        else if (_dtMessage[0].find("B_Robot") == 0)
        { //Get the ball
            text = "Ball on " + (ballOn = _dtMessage[0].substr(2));
            if (ballOn == useAs)
                ball = true;
            else
                ball = false;
        }
        else if (_dtMessage[0] == "b_")
        { //Lose the ball
            ballOn.clear();
            text = "Lose the ball  ";
        }
        else if (_dtMessage[0] == "B?")
        { //Ball Status
            if (ball)
                respone = "B_";
            else if ((!ball) && (isBlank(ballOn)))
                respone = "b_";
            else {
                respone = "0";
                goto multicast; }
            goto broadcast;
        }

        /// COMMAND ///
        else if (regex_match(_dtMessage[0].begin(), _dtMessage[0].end(), regex("^(x[+]|x-|y[+]|y-|z[+]|z-)$")))
        { //Go by Arrow
            toArduino(_dtMessage[0]);
        }

        /// OTHERS ///
        else if (toLowers(_dtMessage[0]) == "basestation")
        { //Info BS
            text = "BS";
            socketDict["BaseStation"] = clientSocket;
            for (auto &i : socketDict)
                if (i.first != "BaseStation") {
                    // close(i.second);
                    socketDict[i.first] = 0;
                }
            checkConnection();
        }
        else if (toLowers(_dtMessage[0]) == "ping")
        { //PING-REPLY
            respone = "Reply " + useAs;
            goto multicast;
        }
        else if (toLowers(_dtMessage[0]) == "ip")
        { //IP Address Info
            respone = getMyIP();
            goto multicast;
        }
        else if (toLowers(_dtMessage[0]) == "os")
        { //Outside
            text = "Outside";
        }
        else if (toLowers(_dtMessage[0]) == "get_time")
        { //TIME NOW
            time_t ct = time(0);
            respone = ctime(&ct);
            goto multicast;
        }
        // else
        //     respone = text = "# Invalid Command :<";
    }
    if ((isBlank(respone)) && (_dtMessage.size() > 1))
        sendCallBack(clientSocket, _dtMessage[0] + "|" + _dtMessage[1]);
    goto end;

broadcast:
    sendCallBack(clientSocket, respone + "|" + "Robot1,Robot2,Robot3");
    // sendByHostList("BaseStation", respone + "|" + "Robot1,Robot2,Robot3");
    goto end;

multicast:
    if (_dtMessage.size() > 1)
        sendCallBack(clientSocket, respone + "|" + _dtMessage[1]);
    else
        sendCallBack(clientSocket, respone);
    // sendByHostList("BaseStation", respone + "|" + chkRobotCollect);
    goto end;

end:
    if (!isBlank(text))
        cout << "# " << text << endl;
   return respone;
}

void receivedCallBack(int clientSocket)
{
    try {
        string message = "";
        // for (m.lock(); (true) && (toLowers(message) != "quit"); message.clear(), m.unlock())
        while ((true) && (toLowers(message) != "quit"))
        {
            /// While loop: accept and echo message back to client user Input
            char buf[bufSize];
            memset(buf, 0, bufSize);
            /// Wait for client to send data
            int bytesReceived = recv(clientSocket, buf, bufSize, 0);
            if (bytesReceived == -1) {
                sendCallBack(clientSocket, "quit");
                cerr << "Error in receivedCallBack(). Quitting" << endl;
                break;
                }

            if ((bytesReceived == 0) || (clientSocket == 0)) {
                sendCallBack(clientSocket, "quit");
                cout << "Client disconnected " << endl;
                break;
                }
            message = trim(string(buf, 0, bytesReceived));
            if (!isBlank(message)) {
                cout << "> " << keyByValue(socketDict, clientSocket) << " : " << message << endl;
                ResponeReceivedCallback(clientSocket, message);
                }

            // Echo message back to client
            // send(clientSocket, buf, bytesReceived + 1, 0);
        }
        /// Close the socket
        close(clientSocket);
        clientSocket = 0;
    }
    catch (exception e) {
        cout << "# Received error \n~\n" << e.what() << endl; }
}

void listenClient(int listening)
{
    try
    {
        for (m.lock(); true; m.unlock())
        // while (true)
        {
            /// Wait for a connection
            socklen_t clientSize = sizeof(client);
            int clientSocket = accept(listening, (sockaddr*)&client, &clientSize);

            char host[NI_MAXHOST];      // Client's remote name
            char service[NI_MAXSERV];   // Service (i.e. port) the client is connect on

            memset(host, 0, NI_MAXHOST); // same as memset(host, 0, NI_MAXHOST);
            memset(service, 0, NI_MAXSERV);

            string IPAdd = inet_ntoa(client.sin_addr);
            // printf("IP address is: %s\n", inet_ntoa(client.sin_addr));
            // printf("port is: %d\n", (int) ntohs(client.sin_port));

            if (getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0) {
                cout << "C" << clientSocket << " >> " << host << " connected on port " << service << endl; }
            else {
                inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
                cout << "C" << clientSocket << " >> "  << host << " connected on port " << ntohs(client.sin_port) << endl; }

            socketDict[IPAdd+":"+to_string(clientSocket)] = clientSocket;

            /// Close listening socket
            // close(listening);

            /// Start received message
            th_Receiveds[IPAdd+":"+to_string(clientSocket)] = thread(receivedCallBack, clientSocket);
            sendCallBack(clientSocket, useAs);
            sendPosXYZ();
            checkConnection();
            // thread th_chkCon(checkConnection);
            // th_chkCon.join();
        }
    }
    catch (exception e) {
        cout << "# Listening error \n~\n" << e.what() << endl; }
}

int setupServer(int port)
{
    try {
        cout << "Server Starting..." << endl;
        // Create a socket
        listening = socket(AF_INET, SOCK_STREAM, 0);
        if (listening == -1) {
            cerr << "Can't create a socket! Quitting" << endl;
            return -1; }

        // Bind the ip address and port to a socket
        sockaddr_in hint;
        hint.sin_family = AF_INET;
        hint.sin_port = htons(port);
        inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr);
        bind(listening, (sockaddr*)&hint, sizeof(hint));

        // Tell Winsock the socket is for listening
        listen(listening, SOMAXCONN);
        cout  << "Listening to TCP clients at " << getMyIP() << " : " << port << endl;
        listenClient(listening); }
    catch (exception e) {
        cout << "# Setup Server error \n~\n" << e.what() << endl; }
}



bool changeTranspose()
{
    if (transpose == false) {
        transpose = true;
        cout << "Transpose mode ON" << endl; }
    else {
        transpose = false;
        cout << "Transpose mode OFF" << endl; }
    int _temp = tempPosXYZ[0];      // Swap data X & Y
    tempPosXYZ[0] = tempPosXYZ[1];
    tempPosXYZ[1] = _temp;
    if (socketDict.count("BaseStation"))
        sendPosXYZ();
    return transpose;
}

// void keyEvent(string key)    ///For Arduino is NOT available
// {
//     vector<int> _temp = tempPosXYZ;
//     int x = 0, y = 1;
//     if (transpose == true) {
//         x = 1; y = 0; }

//     if (key == "[C")            //Right
//         tempPosXYZ[x] += 1;
//     else if (key == "[D")       //Left
//         tempPosXYZ[x] -= 1;
//     else if (key == "[A")       //Up
//         tempPosXYZ[y] -= 1;
//     else if (key == "[B")       //Down
//         tempPosXYZ[y] += 1;
//     else if (key == "[5")       //PageUp
//         tempPosXYZ[2] += 1;
//     else if (key == "[6")       //PageDown
//         tempPosXYZ[2] -= 1;
//     else if (key == ".")        //Dot (.)
//         changeTranspose();

//     for (int i = 0; i < 3; i++)
//         if ((tempPosXYZ[i] != _temp[i]) && socketDict.count("BaseStation") /*&& (_socketDict.ContainsKey("BaseStation"))*/){
//             sendPosXYZ();
//             break; }
//     // cout << "# X:" << tempPosXYZ[0] << " Y:" << tempPosXYZ[1] << " ∠:" << tempPosXYZ[2] << "°" << endl;
// }

void keyEvent(string key)   ///For Arduino is available
{
    vector<int> _temp = tempPosXYZ;
    string x = "x", y = "y", z = "z";
    if (transpose == true)
        swapS(x, y);

    if (key == "[C")            //Right
        toArduino(x+"+");
    else if (key == "[D")       //Left
        toArduino(x+"-");
    else if (key == "[A")       //Up
        toArduino(y+"-");
    else if (key == "[B")       //Down
        toArduino(y+"+");
    else if (key == "[5")       //PageUp
        toArduino(z+"+");
    else if (key == "[6")       //PageDown
        toArduino(z+"-");
    else if (key == ".")        //Dot (.)
        changeTranspose();
    // cout << "# X:" << tempPosXYZ[0] << " Y:" << tempPosXYZ[1] << " ∠:" << tempPosXYZ[2] << "°" << endl;
}

void keyPress()
{
    string s = "";
    int i = 0;
    printf("# Set Location Mode \n");
    for (m.lock(); 1; m.unlock()){
        if (kbhit())
            s += getchar();
        if ((s.rfind("^") != (s.size() - 1)) && (s.rfind("[") != (s.size() - 1))) {
            if (s == "") {
                if (i == 1)
                    break;
                i++; }
            else {
                keyEvent(s);
                i = 0; }
            s = ""; } }
    printf("# Set Command Mode \n");
}

void setCommand()
{
    try {
        string Command;
        for (m.lock(); (true) && (toLowers(Command) != "quit"); m.unlock()) {
            getline(cin, Command);
            if (!isBlank(Command))
                Command = trim(Command);

            if (Command == "/") {           //Set Location Mode
                thread th_keyPress(keyPress);
                th_keyPress.join();
            }
            else if (Command == ".") {      //Change Transpose
                changeTranspose();
            }
            else if (Command == ",") {      //Check Connection
                cout << to_string(socketDict.size()) << endl;
                checkConnection();
            }
            else if (!isBlank(Command))
                ResponeSendCallback(socketDict["BaseStation"], Command); }
        sendCallBack(socketDict["BaseStation"],"quit");
        close(listening);
        close(socketDict["BaseStation"]);
        cout << "# Close App" << endl;
    }
    catch (exception e) {
        cout << "% setCommand error \n~\n" << e.what() << endl; }
}

int main()
{
    for (m.lock(); isBlank(useAs); m.unlock()){
        system("clear");
        printf("~ Welcome to Robot Core ~ \n");
        printf("Use as Robot: "); cin >> useAs;
        useAs = trim(useAs);
        if ((useAs == "1") ^ (toLowers(useAs) == "r1") ^ (toLowers(useAs) == "robot1")) useAs = "Robot1";
        else if ((useAs == "2") ^ (toLowers(useAs) == "r2") ^ (toLowers(useAs) == "robot2")) useAs = "Robot2";
        else if (((useAs == "3") ^ toLowers(useAs) == "r3") ^ (toLowers(useAs) == "robot3")) useAs = "Robot3";
        else useAs.clear(); }
    getMyIP();
    thread th_setupServer (setupServer, 8686);
    thread th_fromArduino (fromArduino);
    thread th_setCommand (setCommand);
    th_setCommand.join();
    return 0;
}
