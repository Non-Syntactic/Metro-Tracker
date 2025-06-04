# Metro Tracker

The Seaford Metro Tracker is an IOT (Internet of Things) device powered by a tiny esp32 that processes publicly available GPS Adelaide metro data from the internet and plots it into RGB lights. It does this with it’s built in Wi-Fi antenna, connecting to an access point where it can query the API (application programming interface). It shows train stations between Hallet Cove beach, to the Adelaide City station. 

Each train picked up by the tracker is given a different colour to help identify them on the canvas. As the trains move along the railway, the brightness for the LEDs around it change based on the distance. The indicator light will turn green when pulling down data, which can take up to 10 seconds because of its primitive Wi-Fi power. The Metro Tracker is a really cool installation, where art meets technology.  It makes a great feature to hang on any wall if you don’t mind the bandwidth it uses. 

## Info
### Quick Information
Here's a link to the API route: http://gtfs.adelaidemetro.com.au/v1/realtime/vehicle_positions/debug

|  |  |
|--|--|
|Platform|Arduino|
|Language|C++|
|Board|ESP32-S3 Supermini|
|IDE|Platform IO|

### Code Breakdown
To draw the trains on the LED lights is a 2 step process:
1. Fetch the Adelaide Metro API, and parse it character by character extracting usefull data
2. Map the coordinates from the API into the LED lights on the canvas.

Fetching data from the internet is simple on an ESP32: Contact the server -> Store the output in a `String` variable. Unfortunately, `HTTPClient` struggles when writing the entire output from the api into a string. Its not strictly that the ESP doesn't have enough memory space to store the output, but http client doesn't like it when you try to store 218236 characters into a single variable..
So instead of writing the output to a variable, the API is parsed character by character to extract the data from it.

So what does the reply from the server look like?
```
header { // Header that occurs only at the start
  gtfs_realtime_version: "1.0"
  incrementality: FULL_DATASET
  timestamp: 1743992197
}
entity { // Entity contains data about the vehicle
  id: "V10233231002"
  vehicle {
    trip {
      trip_id: "1023323"
      start_date: "20250407"
      schedule_relationship: SCHEDULED
      route_id: "167C" // This is what we are interested in
      direction_id: 1
    }
    position {
      latitude: -34.9487 // And these ones
      longitude: 138.5626 // ^^
      bearing: 291.24
      speed: 2.7
    }
    timestamp: 1743992185
    vehicle {
      id: "1002"
      label: "1002"
      [transit_realtime.tfnsw_vehicle_descriptor] {
        air_conditioned: true
        wheelchair_accessible: 1
      }
    }
  }
}
entity { etc...
```
Umm, what? For some reason, the API is not any sort of *well known* markup language like JSON or XML but instead, although looking quite similar to JSON. Attempting to parse it as JSON doesn't work from several inconsistencies in the data. What the ESP has to do is check each character for token seperating symbols (`"`,`:`) and grab the data that lies next to them. Not too hard but kind of a pain.

To display the parsed long and lat, the tracker has a list of stops (one for each light) that it uses to calculate a brightness factor for each. Stops are defined in a struct:
```cpp
struct Stops {
  double lon;
  double lat;
  int index;
  String stop_name;

  Stops(double a, double b, double c, String d) : lon(a), lat(b), index(c), stop_name(d) {}
};
```
..and defined in an array called `STOPS`:
```cpp
const Stops STOPS[] = {
  Stops(-35.0795155,138.5020983,0,"Hallet Cove Beach"),
  Stops(-35.0659680,138.5056141,1,"Hallet Cove"),
  Stops(-35.0462356,138.5126078,2,"Marino Rocks"),
  etc..
```
Once fresh data has been extracted, the tracker runs through it's list of stops, and then through a list of found trains and calculates an LED brightness for each light in the track based on the distance. Distance is determined using standard distance formula:
```cpp
dist = sqrt(pow((train.lon-stop.lon),2)+pow((train.lat-stop.lat),2))*DISTANCE_MUL;
```
..Multiplied by an additional variable to adjust the range.
### Other
- 3D models are made in OpenSCAD, a programing CAD software.

## Todo
- Clean up `trackerTool.py`
- Add images to the repo
- Better logging for `code/src/main.cpp`
