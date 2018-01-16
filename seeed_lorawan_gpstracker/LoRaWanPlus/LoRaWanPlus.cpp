#include "LoRaWanPlus.h"


LoRaWanPlusClass::LoRaWanPlusClass(void)
{
    memset(_buffer, 0, 256);
}

void LoRaWanPlusClass::init(void)
{
    SerialLoRa.begin(9600);
}

void LoRaWanPlusClass::getVersion(char *buffer, short length, unsigned char timeout)
{
    if(buffer)
    {
        while(SerialLoRa.available())SerialLoRa.read();
        sendCommand("AT+VER=?\r\n");
        readBuffer(buffer, length, timeout);    
    }
}

void LoRaWanPlusClass::getId(char *buffer, short length, unsigned char timeout)
{
    if(buffer)
    {
        while(SerialLoRa.available())SerialLoRa.read();
        sendCommand("AT+ID=?\r\n");
        readBuffer(buffer, length, timeout);    
    }
}

void LoRaWanPlusClass::setId(char *DevAddr, char *DevEUI, char *AppEUI)
{
    char cmd[64];
    
    if(DevAddr)
    {
        memset(cmd, 0, 64);
        sprintf(cmd, "AT+ID=DevAddr,\"%s\"\r\n", DevAddr);
        sendCommand(cmd);
        delay(DEFAULT_TIMEWAIT);
    }
    
    if(DevEUI)
    {
        memset(cmd, 0, 64);
        sprintf(cmd, "AT+ID=DevEui,\"%s\"\r\n", DevEUI);
        sendCommand(cmd);
        delay(DEFAULT_TIMEWAIT);
    }
    
    if(AppEUI)
    {
        memset(cmd, 0, 64);
        sprintf(cmd, "AT+ID=AppEui,\"%s\"\r\n", AppEUI);
        sendCommand(cmd);
        delay(DEFAULT_TIMEWAIT);
    }
}

void LoRaWanPlusClass::setKey(char *NwkSKey, char *AppSKey, char *AppKey)
{
    char cmd[64];
    
    if(NwkSKey)
    {
        memset(cmd, 0, 64);
        sprintf(cmd, "AT+KEY=NWKSKEY,\"%s\"\r\n", NwkSKey);
        sendCommand(cmd);
#if _DEBUG_SERIAL_
        loraDebugPrint(DEFAULT_DEBUGTIME);
#endif
        delay(DEFAULT_TIMEWAIT);
    }

    if(AppSKey)
    {
        memset(cmd, 0, 64);
        sprintf(cmd, "AT+KEY=APPSKEY,\"%s\"\r\n", AppSKey);
        sendCommand(cmd);
#if _DEBUG_SERIAL_
        loraDebugPrint(DEFAULT_DEBUGTIME);
#endif
        delay(DEFAULT_TIMEWAIT);
    }

    if(AppKey)
    {
        memset(cmd, 0, 64);
        sprintf(cmd, "AT+KEY= APPKEY,\"%s\"\r\n", AppKey);
        sendCommand(cmd);
#if _DEBUG_SERIAL_
        loraDebugPrint(DEFAULT_DEBUGTIME);
#endif
        delay(DEFAULT_TIMEWAIT);
    }
}

void LoRaWanPlusClass::setDataRate(_data_rate_t dataRate, _physical_type_t physicalType)
{
    char cmd[32];
    
    if(physicalType == EU434)sendCommand("AT+DR=EU433\r\n");
    else if(physicalType == EU868)sendCommand("AT+DR=EU868\r\n");
    else if(physicalType == US915)sendCommand("AT+DR=US915\r\n");
    else if(physicalType == US915HYBRID)sendCommand("AT+DR=US915HYBRID\r\n");
    else if(physicalType == AU915)sendCommand("AT+DR=AU915\r\n");
	else if(physicalType == AU915OLD)sendCommand("AT+DR=AU915OLD\r\n");
    else if(physicalType == CN470)sendCommand("AT+DR=CN470\r\n");
    else if(physicalType == CN779)sendCommand("AT+DR=CN779\r\n");
    else if(physicalType == AS923)sendCommand("AT+DR=AS923\r\n");
    else if(physicalType == KR920)sendCommand("AT+DR=KR920\r\n");
    else if(physicalType == IN865)sendCommand("AT+DR=IN865\r\n");
	
#if _DEBUG_SERIAL_
    loraDebugPrint(DEFAULT_DEBUGTIME);
#endif
    delay(DEFAULT_TIMEWAIT);
    
    memset(cmd, 0, 32);
    sprintf(cmd, "AT+DR=%d\r\n", dataRate);
    sendCommand(cmd);
#if _DEBUG_SERIAL_
    loraDebugPrint(DEFAULT_DEBUGTIME);
#endif
    delay(DEFAULT_TIMEWAIT);
}
 
void LoRaWanPlusClass::setPower(short power)
{
    char cmd[32];
    
    memset(cmd, 0, 32);
    sprintf(cmd, "AT+POWER=%d\r\n", power);
    sendCommand(cmd);
#if _DEBUG_SERIAL_
    loraDebugPrint(DEFAULT_DEBUGTIME);
#endif
    delay(DEFAULT_TIMEWAIT);
}

void LoRaWanPlusClass::setPort(unsigned char port)
{
    char cmd[32];
    
    memset(cmd, 0, 32);
    sprintf(cmd, "AT+PORT=%d\r\n", port);
    sendCommand(cmd);
#if _DEBUG_SERIAL_
    loraDebugPrint(DEFAULT_DEBUGTIME);
#endif
    delay(DEFAULT_TIMEWAIT);
}

void LoRaWanPlusClass::setAdaptiveDataRate(bool command)
{
    if(command)sendCommand("AT+ADR=ON\r\n");
    else sendCommand("AT+ADR=OFF\r\n");
#if _DEBUG_SERIAL_
    loraDebugPrint(DEFAULT_DEBUGTIME);
#endif
    delay(DEFAULT_TIMEWAIT);
}

void LoRaWanPlusClass::setChannel(unsigned char channel, float frequency)
{
    char cmd[32];
    
    memset(cmd, 0, 32);
    sprintf(cmd, "AT+CH=%d,%d.%d\r\n", channel, (short)frequency, short(frequency * 10) % 10);
    sendCommand(cmd);
#if _DEBUG_SERIAL_
    loraDebugPrint(DEFAULT_DEBUGTIME);
#endif
    delay(DEFAULT_TIMEWAIT);
}

void LoRaWanPlusClass::setChannel(unsigned char channel, float frequency, _data_rate_t dataRata)
{
    char cmd[32];
    
    memset(cmd, 0, 32);
    sprintf(cmd, "AT+CH=%d,%d.%d,%d\r\n", channel, (short)frequency, short(frequency * 10) % 10, dataRata);
    sendCommand(cmd);
#if _DEBUG_SERIAL_
    loraDebugPrint(DEFAULT_DEBUGTIME);
#endif
    delay(DEFAULT_TIMEWAIT);
}

void LoRaWanPlusClass::setChannel(unsigned char channel, float frequency, _data_rate_t dataRataMin, _data_rate_t dataRataMax)
{
    char cmd[32];
    
    memset(cmd, 0, 32);
    sprintf(cmd, "AT+CH=%d,%d.%d,%d,%d\r\n", channel, (short)frequency, short(frequency * 10) % 10, dataRataMin, dataRataMax);
    sendCommand(cmd);
#if _DEBUG_SERIAL_
    loraDebugPrint(DEFAULT_DEBUGTIME);
#endif
    delay(DEFAULT_TIMEWAIT);
}

bool LoRaWanPlusClass::transferPacket(char *buffer, unsigned char timeout)
{
    unsigned char length = strlen(buffer);
    
    while(SerialLoRa.available())SerialLoRa.read();
    
    sendCommand("AT+MSG=\"");
    for(unsigned char i = 0; i < length; i ++)SerialLoRa.write(buffer[i]);
    sendCommand("\"\r\n");
    
    memset(_buffer, 0, BEFFER_LENGTH_MAX);
    readBuffer(_buffer, BEFFER_LENGTH_MAX, timeout);
#if _DEBUG_SERIAL_    
    SerialUSB.print(_buffer);
#endif    
    if(strstr(_buffer, "+MSG: Done"))return true;
    return false;
}

bool LoRaWanPlusClass::transferPacket(unsigned char *buffer, unsigned char length, unsigned char timeout)
{
    char temp[2] = {0};
    
    while(SerialLoRa.available())SerialLoRa.read();
    
    sendCommand("AT+MSGHEX=\"");
    for(unsigned char i = 0; i < length; i ++)
    {
        sprintf(temp,"%02x", buffer[i]);
        SerialLoRa.write(temp); 
    }
    sendCommand("\"\r\n");
    
    memset(_buffer, 0, BEFFER_LENGTH_MAX);
    readBuffer(_buffer, BEFFER_LENGTH_MAX, timeout);
#if _DEBUG_SERIAL_    
    SerialUSB.print(_buffer);
#endif    
    if(strstr(_buffer, "+MSGHEX: Done"))return true;
    return false;
}

bool LoRaWanPlusClass::transferPacketWithConfirmed(char *buffer, unsigned char timeout)
{
    unsigned char length = strlen(buffer);
    
    while(SerialLoRa.available())SerialLoRa.read();
    
    sendCommand("AT+CMSG=\"");
    for(unsigned char i = 0; i < length; i ++)SerialLoRa.write(buffer[i]);
    sendCommand("\"\r\n");
    
    memset(_buffer, 0, BEFFER_LENGTH_MAX);
    readBuffer(_buffer, BEFFER_LENGTH_MAX, timeout);
#if _DEBUG_SERIAL_    
    SerialUSB.print(_buffer);
#endif      
    if(strstr(_buffer, "+CMSG: ACK Received"))return true;
    return false;
}

bool LoRaWanPlusClass::transferPacketWithConfirmed(unsigned char *buffer, unsigned char length, unsigned char timeout)
{
    char temp[2] = {0};
    
    while(SerialLoRa.available())SerialLoRa.read();
    
    sendCommand("AT+CMSGHEX=\"");
    for(unsigned char i = 0; i < length; i ++)
    {
        sprintf(temp,"%02x", buffer[i]);
        SerialLoRa.write(temp); 
    }
    sendCommand("\"\r\n");
#if _DEBUG_SERIAL_    
    SerialUSB.print(_buffer);
#endif      
    memset(_buffer, 0, BEFFER_LENGTH_MAX);
    readBuffer(_buffer, BEFFER_LENGTH_MAX, timeout);
    
    if(strstr(_buffer, "+CMSGHEX: ACK Received"))return true;
    return false;
}

short LoRaWanPlusClass::receivePacket(char *buffer, short length, short *rssi)
{
    char *ptr;
    short number = 0;
    
    ptr = strstr(_buffer, "RSSI ");
    if(ptr)*rssi = atoi(ptr + 5);
    else *rssi = -255;
    
    ptr = strstr(_buffer, "RX: \"");
    if(ptr)
    {        
        ptr += 5;
        
        uint8_t bitStep = 0;
        if(*(ptr + 2) == ' ')bitStep = 3; // Firmware version 2.0.10
        else bitStep = 2;                   // Firmware version 2.1.15
        
        for(short i = 0; ; i ++)
        {
            char temp[2] = {0};
            unsigned char tmp, result = 0;
            
            temp[0] = *(ptr + i * bitStep);
            temp[1] = *(ptr + i * bitStep + 1);
           
            for(unsigned char j = 0; j < 2; j ++)
            {
                if((temp[j] >= '0') && (temp[j] <= '9'))
                tmp = temp[j] - '0';
                else if((temp[j] >= 'A') && (temp[j] <= 'F'))
                tmp = temp[j] - 'A' + 10;
                else if((temp[j] >= 'a') && (temp[j] <= 'f'))
                tmp = temp[j] - 'a' + 10;

                result = result * 16 + tmp;
            }
            
            if(i < length)buffer[i] = result;

            if(*(ptr + (i + 1) * bitStep) == '\"' && *(ptr + (i + 1) * bitStep + 1) == '\r' && *(ptr + (i + 1) * bitStep + 2) == '\n')
            {
                number = i + 1;
                break;
            }
        }        
    }
       
    memset(_buffer, 0, BEFFER_LENGTH_MAX);
    
    return number;
}

bool LoRaWanPlusClass::transferProprietaryPacket(char *buffer, unsigned char timeout)
{
    unsigned char length = strlen(buffer);
    
    while(SerialLoRa.available())SerialLoRa.read();
    
    sendCommand("AT+PMSG=\"");
    for(unsigned char i = 0; i < length; i ++)SerialLoRa.write(buffer[i]);
    sendCommand("\"\r\n");
    
    memset(_buffer, 0, BEFFER_LENGTH_MAX);
    readBuffer(_buffer, BEFFER_LENGTH_MAX, timeout);
#if _DEBUG_SERIAL_    
    SerialUSB.print(_buffer);
#endif    
    if(strstr(_buffer, "+PMSG: Done"))return true;
    return false;
}

bool LoRaWanPlusClass::transferProprietaryPacket(unsigned char *buffer, unsigned char length, unsigned char timeout)
{
    char temp[2] = {0};
    
    while(SerialLoRa.available())SerialLoRa.read();
    
    sendCommand("AT+PMSGHEX=\"");
    for(unsigned char i = 0; i < length; i ++)
    {
        sprintf(temp,"%02x", buffer[i]);
        SerialLoRa.write(temp); 
    }
    sendCommand("\"\r\n");
    
    memset(_buffer, 0, BEFFER_LENGTH_MAX);
    readBuffer(_buffer, BEFFER_LENGTH_MAX, timeout);
#if _DEBUG_SERIAL_    
    SerialUSB.print(_buffer);
#endif    
    if(strstr(_buffer, "+PMSGHEX: Done"))return true;
    return false;
}

        
void LoRaWanPlusClass::setUnconfirmedMessageRepeatTime(unsigned char time)
{
    char cmd[32];
    
    if(time > 15) time = 15;
    else if(time == 0) time = 1;
    
    memset(cmd, 0, 32);
    sprintf(cmd, "AT+REPT=%d\r\n", time);
    sendCommand(cmd);
#if _DEBUG_SERIAL_
    loraDebugPrint(DEFAULT_DEBUGTIME);
#endif
    delay(DEFAULT_TIMEWAIT);
}

void LoRaWanPlusClass::setConfirmedMessageRetryTime(unsigned char time)
{
    char cmd[32];
    
    if(time > 15) time = 15;
    else if(time == 0) time = 1;
    
    memset(cmd, 0, 32);
    sprintf(cmd, "AT+RETRY=%d\r\n", time);
    sendCommand(cmd);
#if _DEBUG_SERIAL_
    loraDebugPrint(DEFAULT_DEBUGTIME);
#endif
    delay(DEFAULT_TIMEWAIT);    
}

void LoRaWanPlusClass::setReceiceWindowFirst(bool command)
{
    if(command)sendCommand("AT+RXWIN1=ON\r\n");
    else sendCommand("AT+RXWIN1=OFF\r\n");
#if _DEBUG_SERIAL_
    loraDebugPrint(DEFAULT_DEBUGTIME);
#endif
    delay(DEFAULT_TIMEWAIT);
}

void LoRaWanPlusClass::setReceiceWindowFirst(unsigned char channel, float frequency)
{
    char cmd[32];
    
    memset(cmd, 0, 32);
    sprintf(cmd, "AT+RXWIN1=%d,%d.%d\r\n", channel, (short)frequency, short(frequency * 10) % 10);
    sendCommand(cmd);
#if _DEBUG_SERIAL_
    loraDebugPrint(DEFAULT_DEBUGTIME);
#endif
    delay(DEFAULT_TIMEWAIT);
}

void LoRaWanPlusClass::setReceiceWindowSecond(float frequency, _data_rate_t dataRate)
{
    char cmd[32];
    
    memset(cmd, 0, 32);
    sprintf(cmd, "AT+RXWIN2=%d.%d,%d\r\n", (short)frequency, short(frequency * 10) % 10, dataRate);
    sendCommand(cmd);
#if _DEBUG_SERIAL_
    loraDebugPrint(DEFAULT_DEBUGTIME);
#endif
    delay(DEFAULT_TIMEWAIT);
}

void LoRaWanPlusClass::setReceiceWindowSecond(float frequency, _spreading_factor_t spreadingFactor, _band_width_t bandwidth)
{
    char cmd[32];
    
    memset(cmd, 0, 32);
    sprintf(cmd, "AT+RXWIN2=%d.%d,%d,%d\r\n", (short)frequency, short(frequency * 10) % 10, spreadingFactor, bandwidth);
    sendCommand(cmd);
#if _DEBUG_SERIAL_
    loraDebugPrint(DEFAULT_DEBUGTIME);
#endif
    delay(DEFAULT_TIMEWAIT);
}

void LoRaWanPlusClass::setDutyCycle(bool command)
{
    if(command)sendCommand("AT+LW=DC, ON\r\n");
    else sendCommand("AT+LW=DC, OFF\r\n");  
#if _DEBUG_SERIAL_
    loraDebugPrint(DEFAULT_DEBUGTIME);
#endif
    delay(DEFAULT_TIMEWAIT);
}

void LoRaWanPlusClass::setJoinDutyCycle(bool command)
{
    if(command)sendCommand("AT+LW=JDC,ON\r\n");
    else sendCommand("AT+LW=JDC,OFF\r\n");  
#if _DEBUG_SERIAL_
    loraDebugPrint(DEFAULT_DEBUGTIME);
#endif
    delay(DEFAULT_TIMEWAIT);
}

void LoRaWanPlusClass::setReceiceWindowDelay(_window_delay_t command, unsigned short _delay)
{
    char cmd[32];
    
    memset(cmd, 0, 32);
    if(command == RECEIVE_DELAY1) sprintf(cmd, "AT+DELAY=RX1,%d\r\n", _delay);
    else if(command == RECEIVE_DELAY2) sprintf(cmd, "AT+DELAY=RX2,%d\r\n", _delay);
    else if(command == JOIN_ACCEPT_DELAY1) sprintf(cmd, "AT+DELAY=JRX1,%d\r\n", _delay);
    else if(command == JOIN_ACCEPT_DELAY2) sprintf(cmd, "AT+DELAY=JRX2,%d\r\n", _delay); 
    sendCommand(cmd);
#if _DEBUG_SERIAL_
    loraDebugPrint(DEFAULT_DEBUGTIME);
#endif
    delay(DEFAULT_TIMEWAIT);
}

void LoRaWanPlusClass::setClassType(_class_type_t type)
{
    if(type == CLASS_A)sendCommand("AT+CLASS=A\r\n");
    else if(type == CLASS_C)sendCommand("AT+CLASS=C\r\n");
#if _DEBUG_SERIAL_
    loraDebugPrint(DEFAULT_DEBUGTIME);
#endif
    delay(DEFAULT_TIMEWAIT);
}

void LoRaWanPlusClass::setDeciveMode(_device_mode_t mode)
{
    if(mode == LWABP)sendCommand("AT+MODE=LWABP\r\n");
    else if(mode == LWOTAA)sendCommand("AT+MODE=LWOTAA\r\n");
#if _DEBUG_SERIAL_
    loraDebugPrint(DEFAULT_DEBUGTIME);
#endif
    delay(DEFAULT_TIMEWAIT);
}

bool LoRaWanPlusClass::setOTAAJoin(_otaa_join_cmd_t command, unsigned char timeout)
{
    char *ptr;
    
    if(command == JOIN)sendCommand("AT+JOIN\r\n");
    else if(command == FORCE)sendCommand("AT+JOIN=FORCE\r\n"); 
 
#if _DEBUG_SERIAL_
    loraDebugPrint(DEFAULT_DEBUGTIME);
#endif
    delay(DEFAULT_TIMEWAIT);
    
    memset(_buffer, 0, BEFFER_LENGTH_MAX);
    readBuffer(_buffer, BEFFER_LENGTH_MAX, timeout);
#if _DEBUG_SERIAL_    
    SerialUSB.print(_buffer);
#endif  

    ptr = strstr(_buffer, "+JOIN: Join failed");
    if(ptr)return false;
    ptr = strstr(_buffer, "+JOIN: LoRaWAN modem is busy");
    if(ptr)return false;
    
    return true;
}

void LoRaWanPlusClass::setDeviceLowPower(void)
{
    sendCommand("AT+LOWPOWER\r\n");
#if _DEBUG_SERIAL_
    loraDebugPrint(DEFAULT_DEBUGTIME);
#endif
    delay(DEFAULT_TIMEWAIT);
}

void LoRaWanPlusClass::setDeviceReset(void)
{
    sendCommand("AT+RESET\r\n");
#if _DEBUG_SERIAL_
    loraDebugPrint(DEFAULT_DEBUGTIME);
#endif
    delay(DEFAULT_TIMEWAIT);
}

void LoRaWanPlusClass::setDeviceDefault(void)
{
    sendCommand("AT+FDEFAULT=RISINGHF\r\n");
#if _DEBUG_SERIAL_
    loraDebugPrint(DEFAULT_DEBUGTIME);
#endif
    delay(DEFAULT_TIMEWAIT);
}

void LoRaWanPlusClass::initP2PMode(unsigned short frequency, _spreading_factor_t spreadingFactor, _band_width_t bandwidth, 
                                unsigned char txPreamble, unsigned char rxPreamble, short power)
{
    char cmd[64] = {0,};
    sprintf(cmd, "AT+TEST=RFCFG,%d,%d,%d,%d,%d,%d\r\n", frequency, spreadingFactor, bandwidth, txPreamble, rxPreamble, power);
    
    sendCommand("AT+MODE=TEST\r\n");
    delay(DEFAULT_TIMEWAIT);
    sendCommand(cmd);
    delay(DEFAULT_TIMEWAIT);
    sendCommand("AT+TEST=RXLRPKT\r\n");
    delay(DEFAULT_TIMEWAIT);
}

bool LoRaWanPlusClass::transferPacketP2PMode(char *buffer, unsigned char timeout)
{
    unsigned char length = strlen(buffer);
    
    sendCommand("AT+TEST=TXLRSTR,\"");
    for(unsigned char i = 0; i < length; i ++)SerialLoRa.write(buffer[i]);
    sendCommand("\"\r\n");
    
    // memset(_buffer, 0, BEFFER_LENGTH_MAX);
    // readBuffer(_buffer, BEFFER_LENGTH_MAX, timeout);

    // if(strstr(_buffer, "+TEST: TX DONE"))return true;
    // return false;
    
    return waitForResponse("+TEST: TX DONE", timeout);
}

bool LoRaWanPlusClass::transferPacketP2PMode(unsigned char *buffer, unsigned char length, unsigned char timeout)
{
    char temp[2] = {0};
    
    sendCommand("AT+TEST=TXLRPKT,\"");
    for(unsigned char i = 0; i < length; i ++)
    {
        sprintf(temp,"%02x", buffer[i]);
        SerialLoRa.write(temp);    
    }
    sendCommand("\"\r\n");
    
    // memset(_buffer, 0, BEFFER_LENGTH_MAX);
    // readBuffer(_buffer, BEFFER_LENGTH_MAX, timeout);
    
    // if(strstr(_buffer, "+TEST: TX DONE"))return true;
    // return false;
    
    return waitForResponse("+TEST: TX DONE", timeout);
}

short LoRaWanPlusClass::receivePacketP2PMode(unsigned char *buffer, short length, short *rssi, unsigned char timeout)
{
    char *ptr;
    short number;
    
    sendCommandAndWaitForResponse("AT+TEST=RXLRPKT\r\n", "+TEST: RXLRPKT", 2);
    
    while(SerialLoRa.available())SerialLoRa.read();
    memset(_buffer, 0, BEFFER_LENGTH_MAX);
    readBuffer(_buffer, BEFFER_LENGTH_MAX, timeout);
    
    ptr = strstr(_buffer, "LEN");
    if(ptr)number = atoi(ptr + 4);
    else number = 0;
    
    if(number <= 0)return 0;
    
    ptr = strstr(_buffer, "RSSI:");
    if(ptr)*rssi = atoi(ptr + 5);
    else *rssi = -255;
    
    ptr = strstr(_buffer, "RX \"");
    if(ptr)
    {
        ptr += 4;
        
        uint8_t bitStep = 0;
        if(*(ptr + 2) == ' ')bitStep = 3; // Firmware version 2.0.10
        else bitStep = 2;                   // Firmware version 2.1.15
        
        for(short i = 0; i < number; i ++)
        {
            char temp[2] = {0};
            unsigned char tmp, result = 0;
            
            temp[0] = *(ptr + i * bitStep);
            temp[1] = *(ptr + i * bitStep + 1);
           
            for(unsigned char j = 0; j < 2; j ++)
            {
                if((temp[j] >= '0') && (temp[j] <= '9'))
                tmp = temp[j] - '0';
                else if((temp[j] >= 'A') && (temp[j] <= 'F'))
                tmp = temp[j] - 'A' + 10;
                else if((temp[j] >= 'a') && (temp[j] <= 'f'))
                tmp = temp[j] - 'a' + 10;

                result = result * 16 + tmp;
            }
            
            if(i < length)buffer[i] = result;
        }
    }
    
    memset(_buffer, 0, BEFFER_LENGTH_MAX);
    
    return number;
}

short LoRaWanPlusClass::getBatteryVoltage(void)
{
    int battery;
    
    pinMode(CHARGE_STATUS_PIN, OUTPUT);
    digitalWrite(CHARGE_STATUS_PIN, LOW);
    delay(DEFAULT_TIMEWAIT);
    battery = (analogRead(BATTERY_POWER_PIN) * 3300 * 11) >> 10;
    pinMode(CHARGE_STATUS_PIN, INPUT);
    
    return battery;
}

void LoRaWanPlusClass::loraDebug(void)
{
    if(SerialUSB.available())SerialLoRa.write(SerialUSB.read());
    if(SerialLoRa.available())SerialUSB.write(SerialLoRa.read());
}

#if _DEBUG_SERIAL_
void LoRaWanPlusClass::loraDebugPrint(unsigned char timeout)
{
    unsigned long timerStart, timerEnd;

    timerStart = millis();
    
    while(1)
    {
        while(SerialLoRa.available()) SerialUSB.write(SerialLoRa.read());  
        
        timerEnd = millis();
        if(timerEnd - timerStart > 1000 * timeout)break;
    }
}
#endif

void LoRaWanPlusClass::sendCommand(char *command)
{
    SerialLoRa.print(command);
}

short LoRaWanPlusClass::readBuffer(char *buffer, short length, unsigned char timeout)
{
    short i = 0;
    unsigned long timerStart, timerEnd;

    timerStart = millis();

    while(1)
    {
        if(i < length)
        {
            while(SerialLoRa.available())
            {
                char c = SerialLoRa.read();  
                buffer[i ++] = c;
            }  
        }
        
        timerEnd = millis();
        if(timerEnd - timerStart > 1000 * timeout)break;
    }
    
    return i;
}

bool LoRaWanPlusClass::waitForResponse(char* response, unsigned char timeout)
{
    short len = strlen(response);
    short sum = 0;
    unsigned long timerStart,timerEnd;
    
    timerStart = millis();

    while(1)
    {
        if(SerialLoRa.available())
        {
            char c = SerialLoRa.read();

            sum = (c == response[sum]) ? sum + 1 : 0;
            if(sum == len)break;
        }
        
        timerEnd = millis();
        if(timerEnd - timerStart > 1000 * timeout)return false;
    }

    return true;
}

bool LoRaWanPlusClass::sendCommandAndWaitForResponse(char* command, char *response, unsigned char timeout)
{
    sendCommand(command);
    
    return waitForResponse(response, timeout);
}


LoRaWanPlusClass lora;
