#include "HKServer.h"
#include "HKConnection.h"
#include "HKLog.h"

#ifndef PARTICLE_COMPAT
#include "spark_wiring_tcpclient.h"
#include "spark_wiring_tcpserver.h"
#include "spark_wiring_network.h"
#include "spark_wiring_thread.h"
#endif

HKServer::HKServer(int deviceType, const char* hapName,const char *passcode) {
    this->hapName = hapName;
    this->deviceType = deviceType;
    this->passcode = passcode;
    persistor = new HKPersistor();
    persistor->loadRecordStorage();
    char *deviceIdentity = new char[12+5];
    const unsigned char *deviceId = persistor->getDeviceId();
    sprintf(deviceIdentity, "%02X:%02X:%02X:%02X:%02X:%02X",deviceId[0],deviceId[1],deviceId[2],deviceId[3],deviceId[4],deviceId[5]);
    this->deviceIdentity = deviceIdentity;
}
void HKServer::setup () {

    server.begin();
    HKLogger.printf("Server started at port %d", TCP_SERVER_PORT);


    bonjour.setUDP( &udp );
    bonjour.begin(hapName);
    setPaired(false);

}

void HKServer::setPaired(bool p) {
    paired = p;
    bonjour.removeAllServiceRecords();

    char* deviceTypeStr = new char[6];
    memset(deviceTypeStr, 0, 6);
    sprintf(deviceTypeStr, "%d",deviceType);

    char* recordTxt = new char[512];
    memset(recordTxt, 0, 512);
    int len = sprintf(recordTxt, "%csf=1%cid=%s%cpv=1.0%cc#=2%cs#=1%cff=0%cmd=%s%cci=%s",4,(char)strlen(deviceIdentity)+3,deviceIdentity,6,4,4,4,(char)(strlen(hapName) + 3),hapName,3 + strlen(deviceTypeStr),deviceTypeStr);

    char* bonjourName = new char[128];
    memset(bonjourName, 0, 128);

    sprintf(bonjourName, "%s._hap",hapName);

    bonjour.addServiceRecord(bonjourName,
                             TCP_SERVER_PORT,
                             MDNSServiceTCP,
                             recordTxt);
    free(deviceTypeStr);
    free(recordTxt);
    free(bonjourName);
}

void HKServer::handle() {
    bonjour.run();

    if(clients.size() < MAX_CONNECTIONS) {
        TCPClient newClient = server.available();
        if(newClient) {
            Serial.println("Client connected.");
            clients.insert(clients.begin(),new HKConnection(this,newClient));
        }
    }

    int i = clients.size() - 1;
    while(i >= 0) {
        HKConnection *conn = clients.at(i);

        conn->handleConnection();
        if(!conn->isConnected()) {
            conn->close();
            Serial.println("Client removed.");
            clients.erase(clients.begin() + i);

            free(conn);
        }

        i--;
    }


}
