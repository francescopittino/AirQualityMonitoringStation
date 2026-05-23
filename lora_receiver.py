#!/usr/bin/python3
import board
import busio
import digitalio
import datetime
import adafruit_rfm9x
import time
import json
from influxdb_client_3 import InfluxDBClient3, write_client_options, SYNCHRONOUS

#extract influx token from local json file
with open('cred.json', 'r') as file:
	data = json.load(file)

cred_token = data['token']


#Define radio parameters
RADIO_FREQ_MHZ = 868.0 #Frequency - European

#Define Pins connected
CS = digitalio.DigitalInOut(board.CE1)
RESET = digitalio.DigitalInOut(board.D25)

#Initialize SPI bus
spi = busio.SPI(board.SCK, MOSI = board.MOSI, MISO = board.MISO)

#Initialize RFM Radio
rfm9x = adafruit_rfm9x.RFM9x(spi, CS, RESET, RADIO_FREQ_MHZ, baudrate = 50000)
#NOTE:
#Radio in configured in LoRa mode, so syncword, frequency deviation
# and encryption are not available

#Optional settings
#rfm9x.signal_bandwidth = 125000
#rfm9x.coding_rate = 5
#rfm9x.spreading_factor = 8
#rfm9x.enable_crc = True
#rfm9x.tx_power = 23 #not needed as this platform will just be receving data

#setup influxdb write client connection

#write client options
wco = write_client_options(write_options = SYNCHRONOUS)

client = InfluxDBClient3(host = f"eu-central-1-1.aws.cloud2.influxdata.com",
	database = f"AQMeasurements",
	token = cred_token,
	write_client_options = wco,
	flight_client_options = None)

#Upload data to influx cloud
def uploadMeasurement(source, lat, lon, alt, type, value):
	point = "Measurements,source={source},lat={lat},lon={lon},alt={alt},type={type} value={value}".format(source=source, lat=lat, lon=lon, alt=alt, type=type, value=value)
	#DEBUG print(point)
	print("Data uploaded\n\n")
	client.write(record=point, write_precision = "s")

#elaborate the message received
def elaborateMessage(message, rssi):
	message_sections = message.split("#")
	for section in message_sections:
		section_fields = section.split("/")
		if(section_fields[0] == "SNGeo"):
			#elaborate geographic data
			for field in section_fields:
				item = field.split(":")
				if(item[0] == "Name"):
					geo_name = item[1]
				elif(item[0] == "Lat"):
					geo_lat = item[1]
				elif(item[0] == "Lon"):
					geo_lon = item[1]
				elif(item[0] == "Alt"):
					geo_alt = item[1]
			#data memorized for later use
		elif(section_fields[0] == "DHT11"):
			#elaborate data from temperature and humidity sensor
			for field in section_fields:
				item = field.split(":")
				if(item[0] == "Temperature_Celsius"):
					temp_c = item[1]
				elif(item[0] == "Temperature_Fahrenheit"):
					temp_f = item[1]
				elif(item[0] == "Temperature_Feels_like"):
					temp_fl = item[1]
				elif(item[0] == "Humidity"):
					temp_h = item[1]
			#upload data
			uploadMeasurement(geo_name, geo_lat, geo_lon, geo_alt, "temp_c", temp_c)
			uploadMeasurement(geo_name, geo_lat, geo_lon, geo_alt, "temp_f", temp_f)
			uploadMeasurement(geo_name, geo_lat, geo_lon, geo_alt, "temp_fl", temp_fl)
			uploadMeasurement(geo_name, geo_lat, geo_lon, geo_alt, "temp_h", temp_h)
		elif(section_fields[0] == "MQ135"):
			#elaborate data from mixed measurement sensor MQ135
			for field in section_fields:
				item = field.split(":")
				if(item[0] == "MQ"):
					mq_value= item[1]
			#upload data
			uploadMeasurement(geo_name, geo_lat, geo_lon, geo_alt, "mq_value", mq_value)
		elif(section_fields[0] == "PMS5003"):
			#elaborate data from particulate matter sensor
			for field in section_fields:
				item = field.split(":")
				if(item[0] == "PM1.0"):
					pm1_0 = item[1]
				elif(item[0] == "PM2.5"):
					pm2_5 = item[1]
				elif(item[0] == "PM10"):
					pm10 = item[1]
			#upload data
			uploadMeasurement(geo_name, geo_lat, geo_lon, geo_alt, "pm1.0", pm1_0)
			uploadMeasurement(geo_name, geo_lat, geo_lon, geo_alt, "pm2.5", pm2_5)
			uploadMeasurement(geo_name, geo_lat, geo_lon, geo_alt, "pm10", pm10)
		try:
			uploadMeasurement(geo_name, geo_lat, geo_lon, geo_alt, "rssi", rssi)
		except:
			print("Impossibile caricare rssi del pacchetto")
		#print received data
		tempo = datetime.datetime.now()
		tempo = tempo.strftime('%a %d %b %Y, %I:%M%p')
	try:
		print("Ricevuto messaggio da: " + geo_name + "alle ore: " + tempo + " con RSSI: " + str(rssi) + "\n")
		print("Coordinate: " + str(geo_lat) + "/" + str(geo_lon) + "@" + str(geo_alt) + "\n\n")
		print("Dati DHT11: \n\tTemperatura (Celsius): " + str(temp_c) + "\n\tTemperatura (Fahrenheit): " + str(temp_f) + "\n\tTemperatura (Percepita): " + str(temp_fl) + "\n\tUmidita (%): " + str(temp_h) + "\n")
		print("Dati MQ135: \n\tValore MQ: " + str(mq_value) + "\n")
		print("Dati PMS5003: \n\tPM1.0 (ppm): " + str(pm1_0) + "\n\tPM2.5 (ppm): " + str(pm2_5) + "\n\tPM10 (ppm): " + str(pm10) + "\n")
		print("\n---------\n")
	except:
		print("Error: possible incomplete message @ " + tempo + ", RSSI: " + str(rssi))

#main execution loop
while True:
	packet = rfm9x.receive(with_header = True)
	if packet is None:
		pass
		#keep listening
	else:
		#received a packet
		try:
			packet_text = str(packet, "ascii")
			rssi = rfm9x.last_rssi
			print("Received a packet")
			elaborateMessage(packet_text, rssi)
		except Exception as e:
			print("Message elaboration error, exception: \n\t ")
			print(e)
			print("\n---------\n")

