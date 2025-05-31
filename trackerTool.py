import requests
import csv
from time import sleep
import argparse
from termcolor import cprint, colored as col
import math


# Set up the parser
parser = argparse.ArgumentParser()

parser.add_argument("-f","--track",action="store_true",help="Run a loop to append to the csv")
parser.add_argument("-c","--compass",action="store_true",help="Show the bearings of a vehical")
parser.add_argument("-i","--first",action="store_true",help="Just pick the first result from --target")
parser.add_argument("-t","--target",help="Choose a route ID to track")
parser.add_argument("-s","--csv",help="File location to a csv file containing coords to track")

args = parser.parse_args()


FILE_PATH = 'stops.csv'
STOP_DETECT_RADIUS = 100
SAMPLE_RATE = 5


# The route used to get metro data (don't touch)
api = 'https://gtfs.adelaidemetro.com.au/v1/realtime/vehicle_positions/debug'


def getStopsFromCSV():
    stops = []
    try:
        with open(FILE_PATH,'r',newline='') as f:
            csvreader = csv.reader(f)
            data = [i for i in csvreader]

        # Convert coords to floats
        for i in range(len(data)):
            data[i][0] = float(data[i][0])
            data[i][1] = float(data[i][1])
            
        return tuple(data)
    except Exception as e:
        cprint(f'Error while reading file! {FILE_PATH}\n{e}','red')

def printTrackHeader():
    print(f'Tracking {col(args.target,"yellow")}...\n')
    print('| ROUTE  |  ID     | BEARING  |  SPD  |    LAT   |  LON   |')
    return True

# Source: GeeksForGeeks (https://www.geeksforgeeks.org/haversine-formula-to-find-distance-between-two-points-on-a-sphere/)
def haversine(lat1, lon1, lat2, lon2):
    
    # distance between latitudes
    # and longitudes
    dLat = (lat2 - lat1) * math.pi / 180.0
    dLon = (lon2 - lon1) * math.pi / 180.0
 
    # convert to radians
    lat1 = (lat1) * math.pi / 180.0
    lat2 = (lat2) * math.pi / 180.0
 
    # apply formulae
    a = (pow(math.sin(dLat / 2), 2) +
         pow(math.sin(dLon / 2), 2) *
             math.cos(lat1) * math.cos(lat2));
    rad = 6371
    c = 2 * math.asin(math.sqrt(a))
    return round((rad * c)*1000)
    
def getData(target: str):
    # Get the api and error handling
    for i in range(50):
        try:
            res = requests.get(api)
            
            if i > 0:
                cprint('Connection Restored!\n','green')
                printTrackHeader()
            
            break
        except Exception as e:
            cprint(e,'red')
            print('Something went wrong requesting the API. Internet dropout?')
            print(f'({i}/50) Trying again in {SAMPLE_RATE}...')
            
            sleep(SAMPLE_RATE)
            
    # Do a bit of passing and split it into each entity
    res = res.text.replace('\n',',').replace(' ','').split('entity{')
    
    for i in res[1:-1]:
        #route = str(i.split('route_id:')[1].split(',')[0].replace('"',''))
        #if route == target:
        if target in i:
            route = str(i.split('route_id:')[1].split(',')[0].replace('"',''))
            lat = float(i.split('latitude:')[1].split(',')[0])
            lon = float(i.split('longitude:')[1].split(',')[0])
            bearing = float(i.split('bearing:')[1].split(',')[0])
            speed = round(float(i.split('speed:')[1].split(',')[0])*(18/5),2)
            id = str(i.split('id:')[1].split(',')[0].replace('"',''))
            break
            
    try:
        vehical = {
            'id': id,
            'lat': lat,
            'lon': lon,
            'bearing': bearing,
            'speed': speed,
            'route': route
        }
        return vehical
    except:
        return False

def getData2(key: str, vindex=None):
    # Get the api and error handling
    for i in range(50):
        try:
            res = requests.get(api)
            
            if i > 0:
                cprint('Connection Restored!\n','green')
                global update_count
                update_count = 0            
            break
        except Exception as e:
            cprint(e,'red')
            print('Something went wrong requesting the API. Internet dropout?')
            print(f'({i}/50) Trying again in {SAMPLE_RATE}...')
            
            sleep(SAMPLE_RATE)
            
    # Do a bit of passing and split it into each entity
    res = res.text.replace('\n',',').replace(' ','').split('entity{')
    
    tracked = []
    for i in res[1:-1]:

        if '"'+key in i:
            tracked.append({
                'route': str(i.split('route_id:')[1].split(',')[0].replace('"','')),
                'lat': float(i.split('latitude:')[1].split(',')[0]),
                'lon': float(i.split('longitude:')[1].split(',')[0]),
                'bearing': float(i.split('bearing:')[1].split(',')[0]),
                'speed': round(float(i.split('speed:')[1].split(',')[0])*(18/5),2),
                'id': str(i.split('id:')[1].split(',')[0].replace('"',''))
            })
            
    while 1:
        if args.first:
            return tracked[0]
            
        elif vindex == None:
            if len(tracked) == 1:
                return 0
            print(f'Found {col(len(tracked),"blue")} Results: {col([i["route"] for i in tracked],"yellow")}')
                # Returning different data types is kinda wack 
            res = int(input(f'  Select A vehicle to track (0-{len(tracked)-1}): '))

            if res >= 0 and res < len(tracked):
                return res
            else:
                print(col('Invalid Selection!','red'))
                continue
                
        else:
            return tracked[vindex]



if args.csv:
    FILE_PATH = args.csv.replace('\\','/')

if not args.target:
    cprint('Please add a search key with the --target (-t) flag.\nExample: -ft G10','red')
    exit()

STOPS = getStopsFromCSV()
route_id = args.target

if args.track:
    vehicle_selection = getData2(args.target)
    
    global update_count
    update_count = 0
    while 1:
        tracked = getData2(args.target,vehicle_selection)
        
        if not tracked:
            print(f'Could not find target {args.target}. Trying again in 5...')
            sleep(5)
            continue
        
        if update_count == 0:
            printTrackHeader()
        else:
            print('\033[F'*5)
            try:
                print('\033[F'*len(STOPS))
            except:
                pass
            

        # Print data
        print(f"  {col(tracked['route'],'yellow')} ({col(tracked['id'],'yellow')}) {col(str(tracked['bearing'])+'°','blue')}   {col(str(tracked['speed'])+'kph','cyan')}  {col(str(tracked['lat'])+' '+str(tracked['lon']),'red')}    ")
        
        cprint(f'  https://www.google.com/maps/place/{tracked["lat"]},{tracked["lon"]}/@-34.9593971,138.5439935,12.17z     ','green')

        # Hightlights the tonsley line in google maps
        #cprint(f'  https://www.google.com/maps/place/Flinders+Railway+Station/@{tracked["lon"]},{tracked["lat"]},11.54z/data=!4m6!3m5!1s0x6ab0d088ac2d3897:0xcda9ee205727e349!8m2!3d-35.018913!4d138.569226!16s%2Fg%2F11fmmb3zmr?entry=ttu&g_ep=EgoyMDI1MDQyMi4wIKXMDSoJLDEwMjExNjQwSAFQAw%3D%3D     ','green')
        
       
        if STOPS:
            print(f'DISTANCE ({FILE_PATH.split("/")[-1]}):')
            # Check if the vehical is at one of the tracked stops
            for i in range(len(STOPS)):
                meterDistance = haversine(tracked['lat'],tracked['lon'], STOPS[i][0],STOPS[i][1])
                if meterDistance < STOP_DETECT_RADIUS:
                    print(f"  {col(STOPS[i][2],'red' )}: {'.'*(25-len(STOPS[i][2]))} {haversine(tracked['lat'],tracked['lon'], STOPS[i][0],STOPS[i][1])}m    ")
                else:
                    print(f"  {col(STOPS[i][2],'cyan')}: {'.'*(25-len(STOPS[i][2]))} {haversine(tracked['lat'],tracked['lon'], STOPS[i][0],STOPS[i][1])}m    ")
                    
        try:
            sleep(SAMPLE_RATE)

        except KeyboardInterrupt:
            print("Stopping...")
            exit()
            
            
        update_count += 1
        
elif args.compass:
    print(f'Tracking {args.target_route}')

    while 1:
        # Sample some new data
        tracked = getData(args.target_route)
        
        bearing = tracked['bearing']
        if bearing >= 315 or bearing <= 45:
            print(col('NORTH :','red'),bearing,'↑',end="")
        elif bearing > 45 and bearing < 135:
            print(col('EAST  :','green'),bearing,'→',end="")
        elif bearing > 135 and bearing < 225:
            print(col('SOUTH :','blue'),bearing,'↓',end="")
        else:
            print(col('WEST  :','yellow'),bearing,'←',end="")

        print(f'  | {tracked["id"]}\r',end="")

        sleep(SAMPLE_RATE)
