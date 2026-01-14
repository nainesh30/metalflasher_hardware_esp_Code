This project turns an ESP32 into a BLE iBeacon scanner that filters a specific target UUID, extracts Major, Minor, RSSI, and publishes the data to an MQTT broker over Wi-Fi.
It is suitable for indoor positioning, attendance, student tracking, or room-level presence detection.

**Features**

Scans BLE advertisements
Filters only iBeacon packets
Matches a specific target UUID
Extracts:
Major
Minor
RSSI
Applies RSSI threshold filtering
Publishes data as JSON over MQTT
Designed for ESP32 (Arduino framework)

**How It Works**

ESP32 scans for BLE advertisements
Manufacturer data is checked for iBeacon signature
UUID is matched against TARGET_UUID
RSSI threshold is applied
Major, Minor, RSSI are extracted
Data is sent to MQTT broker
Only your beacon is accepted — all others are ignored.

iBeacon Packet Structure
Offset  	Length	    Field
0–1	      2	      Apple Company ID (0x004C)
2	        1	      iBeacon Type (0x02)
3	        1	      Length (0x15)
4–19	   16	      Proximity UUID
20–21	   2	      Major
22–23	   2	      Minor
24	     1	      TX Power
