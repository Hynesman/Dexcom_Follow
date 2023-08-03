#include "Arduino.h"
#include "Dexcom_follow.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char *DEXCOM_BASE_URL = "https://shareous1.dexcom.com/ShareWebServices/Services";
const char *DEXCOM_BASE_URL_OUS = "https://shareous1.dexcom.com/ShareWebServices/Services";

// Trend directions mapping
const char *DEXCOM_TREND_DIRECTIONS[] = {
    "None", // unconfirmed
    "DoubleUp",
    "SingleUp",
    "FortyFiveUp",
    "Flat",
    "FortyFiveDown",
    "SingleDown",
    "DoubleDown",
    "NotComputable", // unconfirmed
    "RateOutOfRange" // unconfirmed
};

// Trend arrows
const char *DEXCOM_TREND_ARROWS[] = {
    "",
    "↑↑",
    "↑",
    "↗",
    "→",
    "↘",
    "↓",
    "↓↓",
    "?",
    "-"};

String applicationId = "d8665ade-9673-4e27-9ff6-92db4ce13d13";

Follower::Follower(bool ous, String user, String pass, String sessionID)
{
    Username = user;
    Password = pass;
    SessionID = sessionID;
    update_json_string();
    if (ous)
    {
        DexcomServer = DEXCOM_BASE_URL_OUS;
    }
    else
    {
        DexcomServer = DEXCOM_BASE_URL;
    }
};

void Follower::update_json_string()
{
    String jsonString = "{";
    jsonString += "\"accountName\": \"" + Username + "\",";
    jsonString += "\"applicationId\": \"" + applicationId + "\",";
    jsonString += "\"password\": \"" + Password + "\"";
    jsonString += "}";
    jsonStr = jsonString;
};

void Follower::Set_user_pass(String user, String pass)
{
    Username = user;
    Password = pass;
    update_json_string();
};

void Follower::Set_sessionID(String sessionID)
{
    SessionID = sessionID;
};

String Follower::getNewSessionID()
{
    String response;
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;

        http.begin(DexcomServer + "/General/LoginPublisherAccountByName");

        http.addHeader("Content-Type", "application/json");

        int httpCode = http.POST(jsonStr);

        if (httpCode > 0)
        {
            if (httpCode == HTTP_CODE_OK)
            {
                char characterToRemove = '\"';

                response = removeCharacterFromString(http.getString(), characterToRemove);
                // response = http.getString();
                Serial.println("HTTP POST was successful!");
                Serial.println("Response:");
                Serial.println(response);
            }
            else
            {
                Serial.print("HTTP error code: ");
                Serial.println(httpCode);
                response = "error";
            }
        }
        else
        {
            Serial.println("HTTP connection failed!");
            response = "no connection";
        }

        http.end();
    }
    SessionID = response;
    return response;
};

void Follower::GlucoseLevelsNow()
{
    HTTPClient http;

    String url = DexcomServer + "/Publisher/ReadPublisherLatestGlucoseValues?sessionId=";
    url += SessionID;
    url += "&minutes=1440&maxCount=1";

    http.begin(url);

    int httpResponseCode = http.GET();
    if (httpResponseCode == HTTP_CODE_OK)
    {
        String response = http.getString();

        // Parse the JSON response
        StaticJsonDocument<2000> doc;
        DeserializationError error = deserializeJson(doc, response);
        if (error)
        {
            Serial.print("Error parsing JSON response: ");
            Serial.println(error.c_str());
            return;
        }

        // Extract the blood glucose value
        GlucoseNow.mg_dl = doc[0]["Value"];
        GlucoseNow.mmol_l = convertToMmol(GlucoseNow.mg_dl);
        Serial.print("Blood Glucose Level: ");
        Serial.println(GlucoseNow.mmol_l);

        GlucoseNow.trend_description = doc[0]["Trend"].as<const char *>();
        GlucoseNow.trend_Symbol = getTrendSymbol(GlucoseNow.trend_description);
        Serial.print("current trend: ");
        Serial.println(GlucoseNow.trend_Symbol);

        GlucoseNow.timestamp = convertToUnixTimestamp(doc[0]["DT"].as<const char *>());
        Serial.println(GlucoseNow.timestamp);
    }
    else
    {
        Serial.print("HTTP GET request failed, error code: ");
        Serial.println(httpResponseCode);
        getNewSessionID();
    }

    http.end();
};

double Follower::convertToMmol(int mgdl)
{
    return mgdl / 18.01559;
};

String Follower::removeCharacterFromString(String input, char characterToRemove)
{
    String result;
    for (char c : input)
    {
        if (c != characterToRemove)
        {
            result += c;
        }
    }
    return result;
};

unsigned long Follower::convertToUnixTimestamp(const char *dtValue)
{
    // Extract the numerical part of the string
    const char *numericalPart = dtValue + 5; // Skip "Date("

    // Find the position of the first 10 digits
    const char *offset =  numericalPart + 10;

    if (offset)
    {
        // Convert the timestamp part to a long integer
        char timestampStr[11]; // Assuming timestamp will always have 13 digits
        strncpy(timestampStr, numericalPart, offset - numericalPart);
        timestampStr[offset - numericalPart] = '\0';

        unsigned long timestamp = atol(timestampStr);

        // Get the time zone offset in hours and minutes
        int tzHours, tzMinutes;
        sscanf(offset+3, "+%2d%2d", &tzHours, &tzMinutes);
        

        // Convert the offset to seconds and adjust the timestamp
        unsigned long offsetSeconds = tzHours * 60*60  + tzMinutes * 60;
        timestamp += offsetSeconds;

        return timestamp; // Convert microseconds to seconds
    }
    else
    {
        return 0; // Invalid DT value format
    }
}

const char *Follower::getTrendSymbol(const char *trendDescription)
{
    if (strcmp(trendDescription, "None") == 0)
    {
        return "";
    }
    else if (strcmp(trendDescription, "DoubleUp") == 0)
    {
        return "^^";
    }
    else if (strcmp(trendDescription, "SingleUp") == 0)
    {
        return "^";
    }
    else if (strcmp(trendDescription, "FortyFiveUp") == 0)
    {
        return "/^";
    }
    else if (strcmp(trendDescription, "Flat") == 0)
    {
        return "->";
    }
    else if (strcmp(trendDescription, "FortyFiveDown") == 0)
    {
        return "\\v";
    }
    else if (strcmp(trendDescription, "SingleDown") == 0)
    {
        return "v";
    }
    else if (strcmp(trendDescription, "DoubleDown") == 0)
    {
        return "vv";
    }
    else if (strcmp(trendDescription, "NotComputable") == 0)
    {
        return "?";
    }
    else if (strcmp(trendDescription, "RateOutOfRange") == 0)
    {
        return "-";
    }
    else
    {
        return ""; // Default to an empty string for unknown trends
    }
};
