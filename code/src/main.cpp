/*
Use the following bash to get the length of the return data:
`wget http://gtfs.adelaidemetro.com.au/v1/realtime/vehicle_positions/debug -o /dev/null -O /tmp/apilen;a=$(cat /tmp/apilen);echo ${#a};rm /tmp/apilen`
*/

// TODO: Change this
#ifdef DEVELOPMENT_MODE
  #define DEBUG true
#else
  #define DEBUG true
#endif

// Include packages
#include <Arduino.h>
#include <vector>
#include <iomanip>
#include <FastLED.h>
#include <WiFi.h>
#include <HTTPClient.h> // For API requesting

/// The API route to get train data
const char API[] = "http://gtfs.adelaidemetro.com.au/v1/realtime/vehicle_positions/debug";

const int LAT_RANGE = 1000;
const int LON_RANGE = 1000;

HTTPClient http; // Create the http client object

//// Setup LEDS
#define NUM_LEDS 19
#define DATA_PIN 3
#define DEBUG_LED 18
CRGB leds[NUM_LEDS];

struct Stops {
  double lon;
  double lat;
  int index;
  String stop_name;

  Stops(double a, double b, double c, String d) : lon(a), lat(b), index(c), stop_name(d) {}
};

/* ---- CONFIG ---- */

// Train stops for each LED
const Stops STOPS[] = {
  Stops(-35.0795155,138.5020983,0,"Hallet Cove Beach"),
  Stops(-35.0659680,138.5056141,1,"Hallet Cove"),
  Stops(-35.0462356,138.5126078,2,"Marino Rocks"),
  Stops(-35.0422496,138.5172564,3,"Marino Railway Station"),
  Stops(-35.0324786,138.5212542,4,"Seacliff"),
  Stops(-35.0216526,138.5192003,5,"Brighton"),
  Stops(-35.0122889,138.5236356,6,"Hove"),
  Stops(-35.0091272,138.5408492,7,"Oaklands"),
  Stops(-34.9990117,138.5526242,8,"Marion"),
  Stops(-34.9906526,138.5604201,9,"Ascot Park"),
  /*INTERSECTION*/
  Stops(-34.9821925,138.5673640,10,"Woodlands Park"),
  Stops(-34.9720174,138.5712942,11,"Edwardstown"),
  Stops(-34.9667332,138.5734255,12,"Emerson"),
  Stops(-34.9720174,138.5712942,13,"Clarence Park"),
  Stops(-34.9513205,138.5850539,14,"Goodwood"),
  Stops(-34.9439191,138.5831178,15,"Adelaide Showground"),
  Stops(-34.9251956,138.5800757,16,"Mile End"),
};

// The Route IDs to be tracked
const String TRACKED_ROUTES[] = {"SEAFRD","FLNDRS"};

#define REFRESH_RATE 20000 // Delay in milliseconds until the board refreashes
#define DISTANCE_MUL 20000 // Controls the *spread* of brightness for LEDs around the train

#define CORE_DEBUG_LEVEL = 5

// Configuring WiFi auth
const char WIFI_SSID[] = "WIFI_SSID";
const char WIFI_PASSWORD[] = "WIFI_PASSWORD";

/* ---- END CONFIG ---- */


void setup() {
  // Attach LEDS
  FastLED.addLeds<WS2811, DATA_PIN>(leds, NUM_LEDS);
  
  Serial.begin(115200);
  pinMode(LED_BUILTIN,OUTPUT);

  for (int i=0;i<NUM_LEDS;i++) leds[i].setRGB(25,25,25);

  // Connect to WiFi
  Serial.println("[INFO] : (setup) Connecting to AP...");  

  leds[DEBUG_LED] = CRGB::Yellow; FastLED.show();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  Serial.print("[INFO] : (setup) Connected to AP! IP is: ");
  Serial.println(WiFi.localIP());
  leds[DEBUG_LED] = CRGB::Cyan; FastLED.show();
}

struct Coords {
  double lon;
  double lat;
  String route;

  Coords(double a, double b, String c) : lon(a), lat(b), route(c) {}
};


// Converts lon and lat into 0-500 & 0-1000 range
void convertCoords(Coords &coords) {
  coords.lon = (coords.lon+180)/360*LON_RANGE;
  coords.lat = (coords.lat+180)/360*LAT_RANGE;
}

void printLog(uint8_t indent, String type, String origin, String message) {
  if (type == "DEBUG" && !DEBUG) return;

  String indentString;
  for (int i=0;i<indent*2;i++) indentString += " ";

  Serial.println(indentString+"["+type+"] : ("+origin+")  "+message);
}

// Method to get the position of the trains
std::vector<Coords> getTrainData() {
  printLog(0,"DEBUG","getTrainData","Fetching API...");

  std::vector<Coords> locations;
  
  http.begin(API);
  int status_code = http.GET();

  if (status_code > 0 && status_code == HTTP_CODE_OK) {
    printLog(0,"DEBUG","getTrainData","Request 200 OK. Passing data");
    leds[DEBUG_LED] = CRGB::Green; FastLED.show();


    WiFiClient *stream_pointer = http.getStreamPtr();
    
    //int locations_set = 0; // How many locations have been set in `locations[][]`

    String route_id = "";
    double route_lon = 0;
    double route_lat = 0;

    String data_key;
    String data_value;
    char c;
    int j=0;

    //stream_pointer->available()
    delay(1000);
    int response_size = http.getSize();
    while (stream_pointer->available() > 30) {
      while (stream_pointer->available() < 3000 && response_size-j > 3000) {
        delay(1);
      }

      //// Get the data key
      while (c != ':') {
        j++;
        c = stream_pointer->read();
        if (c != ' ') {
          data_key += c;
        }
      }

      //// Get the data value
      while (c != '\n') {
        j++;
        c = stream_pointer->read();
         if (c != ' ') {
          data_value += c;
        }
      }

      //Serial.print(stream_pointer->available());
      //Serial.print(" ");
      //Serial.println(j);

      //// Clean the data and remove any yucky characters
      /// NOTE: The parsing isnt perfect but its good enough (:
      int clean_index = data_key.lastIndexOf("{\n");
      if (clean_index >= 0) {
        data_key = data_key.substring(clean_index+2,-1);
      }

      /// Remove the newline
      data_value.remove(data_value.length()-1,1);

      /// Remove `"` from a string value if present
      if (data_value.indexOf('"') >= 0) {
        data_value.remove(0,1);
        data_value.remove(data_value.length()-1,1);
      }

      //// Now check if the data is actully needed and set the variables if so
      /// Set the route_id if it is in the lists
      if (data_key == "route_id:") {
        for (String route : TRACKED_ROUTES) {
          if (data_value == route) {
            route_id = data_value;
            printLog(1,"DEBUG","getTrainData","Found route "+route_id);
          }
        }
      }
      else if (data_key == "latitude:" && route_id != "") {
        route_lat = std::stod(data_value.c_str());
        printLog(1,"DEBUG","getTrainData","  Found lat "+String(route_lat));
      }
      else if (data_key == "longitude:" && route_id != "") {
        route_lon = std::stod(data_value.c_str());
        printLog(1,"DEBUG","getTrainData","  Found lon "+String(route_lon));
      }

      //// Add to locations if all variables have value
      if (route_lon && route_lat && route_id) {
        printLog(1,"DEBUG","getTrainData","  Appended to locations");
        locations.push_back(Coords(route_lat,route_lon,route_id));

        /// Reset route variables
        route_id = "";
        route_lon = 0;
        route_lat = 0;
      }

      /// Reset data variables
      data_key = "";
      data_value = "";
    }
    http.end();

        
    printLog(0,"DEBUG","getTrainData","Parsing done!");

    return locations;
  }
  else {
    printLog(0,"ERROR","getTrainData","Could not fetch data successfully! Error code: "+String(status_code));

    leds[DEBUG_LED] = CRGB::Red; FastLED.show();

    return locations;
  }
}

void showLights(std::vector<Coords> trains) {
  double latDist, lonDist;
  long stopBrightness;
  printLog(0,"DEBUG","showLights","Showing lights");

  for (int i=0;i<NUM_LEDS;i++) leds[i].setRGB(0,0,0);
  leds[DEBUG_LED].setRGB(0,0,0);

  int trainNo = 0;
  double dist = 0;

  for (Stops stop : STOPS) {
    for (Coords train : trains) {
      dist = sqrt(pow((train.lon-stop.lon),2)+pow((train.lat-stop.lat),2))*DISTANCE_MUL;

      stopBrightness = 255 - dist;
      
      if (stopBrightness > 0) {
        CRGB oldStop = leds[stop.index];
        CRGB newStop;

        newStop.setHSV(trainNo*2,255,stopBrightness);
        //newStop.setHSV(map(trainNo,0,trains.size(),0,85),255,stopBrightness);

        // Colour mixing
        leds[stop.index].setRGB(
          (newStop.r+oldStop.r)/2,
          (newStop.g+oldStop.g)/2,
          (newStop.b+oldStop.b)/2
        );
        
        ESP_LOGD("showLights", "Writing to led: %d => %d",stop.index,stopBrightness);
      }

      trainNo++;
    }
  }
  FastLED.show();
}


void loop() {
  std::vector<Coords> trains = getTrainData();

  showLights(trains);

  delay(REFRESH_RATE);
}
