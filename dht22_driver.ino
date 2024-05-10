/* Simple driver for the DHT22 temperature and relative humidity sensor.
 * Not production-ready, just a little project to refresh my knowledge.
 *
 * Copyright 2024 Fabian Krause
 */
#define PIN 4

void setup()
{
    Serial.begin(9600);
    pinMode(PIN, OUTPUT);
    digitalWrite(PIN, HIGH); // Make sure voltage is high from the beginning
    delay(1000);             // Wait for sensor to be available
}

void sendStartSignal()
{
    // Send start signal
    pinMode(PIN, OUTPUT);
    digitalWrite(PIN, LOW);
    delayMicroseconds(1200);
    // Release bus
    digitalWrite(PIN, HIGH);
    delayMicroseconds(30);
}

unsigned long waitForLow()
{
    auto time = micros();
    while (digitalRead(PIN) == HIGH)
    {
    }
    return micros() - time;
}

unsigned long waitForHigh()
{
    auto time = micros();
    while (digitalRead(PIN) == LOW)
    {
    }
    return micros() - time;
}

void readStartResponse()
{
    pinMode(PIN, INPUT);
    auto startTime = micros();

    while (digitalRead(PIN) == HIGH)
    {
        if (micros() - startTime > 1000)
        {
            Serial.println("Sensor timeout!");
            return;
        }
    };

    auto lowDuration = waitForHigh();
    auto highDuration = waitForLow();
}

byte readBit()
{
    waitForHigh(); // should be between 48-55 microsecs

    auto duration = waitForLow();
    if (duration < 30) // 22-30 microsecs is '0'
        return 0;
    else // 68-75 microsecs is '1'
        return 1;
}

word readWord()
{
    word result = 0;
    for (int i = 0; i < 16; ++i)
    {
        auto value = readBit();
        result |= value << (15 - i);
    }
    return result;
}

byte readByte()
{
    byte result = 0;
    for (int i = 0; i < 8; ++i)
    {
        auto value = readBit();
        result |= value << (7 - i);
    }
    return result;
}

bool verifyParity(word humidity, word temperature, byte parity)
{
    byte humidityHigh = humidity >> 8;
    byte humidityLow = humidity;
    byte temperatureHigh = temperature >> 8;
    byte temperatureLow = temperature;

    byte sum = humidityHigh + humidityLow + temperatureHigh + temperatureLow;
    if (sum != parity)
        return false;
    return true;
}

void setTemperatureString(word temperature, char *result)
{
    bool temperatureNegative = (temperature) >> 16;  // Top bit determines sign
    auto temperatureValue = (temperature << 1) >> 1; // Get rid of top bit, which is the sign

    sprintf(result, "%c%d.%d C", temperatureNegative ? '-' : ' ', temperatureValue / 10, temperatureValue % 10);
}

void setHumidityString(word humidity, char *result)
{
    sprintf(result, "%d.%d %%", humidity / 10, humidity % 10);
}

void loop()
{
    sendStartSignal();
    readStartResponse();

    auto humidity = readWord();
    auto temperature = readWord();
    auto parity = readByte();

    auto correct = verifyParity(humidity, temperature, parity);
    if (!correct)
    {
        Serial.println("Parity incorrect, possible transmission error!");
    }
    else
    {
        char temperatureString[10];
        char humidityString[10];

        setTemperatureString(temperature, temperatureString);
        setHumidityString(humidity, humidityString);

        Serial.print("Humidity: ");
        Serial.print(humidityString);
        Serial.print(", Temperature: ");
        Serial.print(temperatureString);
        Serial.println();
    }

    // Wait 2 seconds until next measurement, as per data sheet
    delay(2000);
}