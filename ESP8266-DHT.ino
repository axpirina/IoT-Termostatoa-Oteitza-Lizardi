/* ESP8266 Programmable Thermostat IoT

   Version 1.0  1/7/2017  Version 1.0   Damon Borgnino
   Modified by Axpi 2022
*/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <FS.h> // FOR SPIFFS
#include <ctype.h> // for isNumber check
#include <DHT.h>
#define DHTTYPE DHT11
#define DHTPIN  2 // D4
#define RELAYPIN 4 // D2

const char* ssid     = "Zure Wifia";
const char* password = "Zure Pasahitza";

int coolOff = 2;
int coolOn = 6;
String relayState = "OFF";
const static String fName = "prop.txt";
const static String dfName = "data.txt";
int dataLines = 0;
int maxFileData = 20;


ESP8266WebServer server(80);

// Initialize DHT sensor

// This is for the ESP8266 processor on ESP-01
DHT dht(DHTPIN, DHTTYPE, 11); // 11 works fine for ESP8266

float humidity, temp_f;  // Values read from sensor
String webString = "";   // String to display
String webMessage = "";
// Generally, you should use "unsigned long" for variables that hold time to handle rollover
unsigned long previousMillis = 0;        // will store last temp was read
long interval = 20000;              // interval at which to read sensor


// reads the temp and humidity from the DHT sensor and sets the global variables for both
void gettemperature() {

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
  humidity = dht.readHumidity();          // Read humidity (percent)
  temp_f = dht.readTemperature(false);     // Read temperature as Celsius
  // Check if any reads failed and exit
  if (isnan(humidity) || isnan(temp_f)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // turn the relay switch Off or On depending on the temperature reading
  if (temp_f <= coolOff)
  {
    digitalWrite(RELAYPIN,HIGH);
    relayState = "OFF";
    Serial.println("Relay OFF");
  }
  else if (temp_f >= coolOn)
  {
    digitalWrite(RELAYPIN,LOW);
    relayState = "ON";
    Serial.println("Relay ON");
  }
}

/////////////////////////////////////////////////

void clearDataFile() // deletes all the stored data
{
  File f = SPIFFS.open(dfName, "w");
  if (!f) {
    Serial.println("data file open to clear failed");
  }
  else
  {
    Serial.println("-- Data file cleared =========");
    f.close();
  }
}

///////////////////////////////////////////////////////////////////////////////////
// removes the first line of a file by writing all data except the first line
// to a new file. The old file is deleted and new file is renamed.
///////////////////////////////////////////////////////////////////////////////////

void removeFileLine(String fi)
{
  File original = SPIFFS.open(fi, "r");
  if (!original) {

    Serial.println("original data file open failed");
  }

  File temp = SPIFFS.open("tempfile.txt", "w");
  if (!temp) {

    Serial.println("temp data file open failed");
  }

  Serial.println("------ Removing Data Lines ------");

  //Lets read line by line from the file
  for (int i = 0; i < maxFileData; i++) {

    String str = original.readStringUntil('\n'); // read a line
    if (i > 0) { // skip writing first line to the temp file

      temp.println(str);
      //  Serial.println(str); // uncomment to view the file data in the serial console
    }
  }

  int origSize = original.size();
  int tempSize = temp.size();
  temp.close();
  original.close();

  Serial.print("size orig: "); Serial.print(origSize); Serial.println(" bytes");
  Serial.print("size temp: "); Serial.print(tempSize); Serial.println(" bytes");

  if (! SPIFFS.remove(dfName))
    Serial.println("Remove file failed");


  if (! SPIFFS.rename("tempfile.txt", dfName))
    Serial.println("Rename file failed");
  // dataLines--;
}

//////////////////////////////////////////////////////////////////////
// writes the most recent variable values to the data file          //
//////////////////////////////////////////////////////////////////////

void updateDataFile()
{
  Serial.println("dataLines: ");
  Serial.println(dataLines);
  if (dataLines >= maxFileData)
  {
    removeFileLine(dfName);
  }
  ///////
  File f = SPIFFS.open(dfName, "a");
  if (!f) {

    Serial.println("data file open failed");
  }
  else
  {
    Serial.println("====== Writing to data file =========");

    f.print(relayState); f.print(":");
    f.print(temp_f); f.print( ","); f.println(humidity);

    Serial.println("Data file updated");
    f.close();
  }

  Serial.print("millis: ");
  Serial.println(millis());

}

//////////////////////////////////////////////////////////////////////////////
// reads the data and formats it so that it can be used by google charts    //
//////////////////////////////////////////////////////////////////////////////

String readDataFile()
{
  String returnStr = "";
  File f = SPIFFS.open(dfName, "r");

  if (!f)
  {
    Serial.println("Data File Open for read failed.");

  }
  else
  {
    Serial.println("----Reading Data File-----");
    dataLines = 0;

    while (f.available()) {

      //Lets read line by line from the file
      dataLines++;
      String str = f.readStringUntil('\n');
      String switchState =  str.substring(0, str.indexOf(":")  );
      /*
          String tempF = str.substring(str.indexOf(":") + 1, str.indexOf(",")  );
        //  String humid = str.substring(str.indexOf(",") + 1 );
        //    String milliSecs = str.substring(str.indexOf("~") + 1 , str.indexOf("~"));
        //   Serial.println(tempF);
        //   Serial.println(humid);
        //  Serial.println(str);
      */

      returnStr += ",['" + switchState + "'," + str.substring(str.indexOf(":") + 1) + "]";
    }
    f.close();
  }

  return returnStr;

}

////////////////////////////////////////////////////////////////////////////////////
//      creates the HTML string to be sent to the client                          //
////////////////////////////////////////////////////////////////////////////////////

void setHTML()
{
  webString = "<html><head>\n";
  webString += "<meta http-equiv=\"refresh\" content=\"60;url=http://" + WiFi.localIP().toString() + "\"> \n";

  ///////////// google charts script
  webString += "<script type=\"text/javascript\" src=\"https://www.gstatic.com/charts/loader.js\"></script> \n";
  webString += "   <script type=\"text/javascript\"> \n";
  webString += "    google.charts.load('current', {'packages':['corechart','gauge']}); \n";
  webString += "    google.charts.setOnLoadCallback(drawTempChart); \n";
  webString += "    google.charts.setOnLoadCallback(drawHumidChart); \n";
  webString += "    google.charts.setOnLoadCallback(drawChart); \n ";

  // draw temp guage
  webString += "   function drawTempChart() { \n";
  webString += "      var data = google.visualization.arrayToDataTable([ \n";
  webString += "        ['Label', 'Value'], ";
  webString += "        ['Tenp\xB0',  ";
  webString += temp_f;
  webString += " ], ";
  webString += "       ]); \n";
  // setup the google chart options here
  webString += "    var options = {";
  webString += "      width: 250, height: 150,";
  webString += "      min: -10, max: 50,";
  webString += "      yellowFrom: -10, yellowTo: " + String(coolOff) + ",";
  webString += "      greenFrom: " + String(coolOff) + ", greenTo: " + String(coolOn) + ",";
  webString += "      redFrom: " + String(coolOn) + ", redTo: 50,";
  webString += "       minorTicks: 5";
  webString += "    }; \n";
  webString += "   var chart = new google.visualization.Gauge(document.getElementById('chart_divTemp')); \n";
  webString += "  chart.draw(data, options); \n";
  webString += "  } \n";

  // draw humidity guage
  webString += "   function drawHumidChart() { \n";
  webString += "      var data = google.visualization.arrayToDataTable([ \n";
  webString += "        ['Label', 'Value'], ";
  webString += "        ['Hezetasuna',  ";
  webString += humidity;
  webString += " ], ";
  webString += "       ]); \n";
  // setup the google chart options here
  webString += "    var options = {";
  webString += "      width: 250, height: 150,";
  webString += "      min: 0, max: 100,";
  webString += "      yellowFrom: 0, yellowTo: 25,";
  webString += "      greenFrom: 25, greenTo: 75,";
  webString += "      redFrom: 75, redTo: 100,";
  webString += "       minorTicks: 5";
  webString += "    }; \n";
  webString += "   var chart = new google.visualization.Gauge(document.getElementById('chart_divHumid')); \n";
  webString += "  chart.draw(data, options); \n";
  webString += "  } \n";


  // draw main graph
  webString += "    function drawChart() { \n";

  webString += "     var data = google.visualization.arrayToDataTable([ \n";
  webString += "       ['Hit', 'Temp F', 'Humidity'] ";


  webString += readDataFile();
  // open file for reading

  webString += "     ]); \n";
  webString += "     var options = { \n";
  webString += "        title: 'Temp/Humidity Activity', ";
  webString += "        curveType: 'function', \n";

  webString += "  series: { \n";
  webString += "         0: {targetAxisIndex: 0}, \n";
  webString += "         1: {targetAxisIndex: 1} \n";
  webString += "       }, \n";

  webString += "  vAxes: { \n";
  webString += "         // Adds titles to each axis. \n";
  webString += "         0: {title: 'Temp Fahrenheit'}, \n";
  webString += "         1: {title: 'Humidity %'} \n";
  webString += "       }, \n";

  webString += "  hAxes: { \n";
  webString += "         // Adds titles to each axis. \n";
  webString += "         0: {title: 'Heat On/Off'}, \n";
  webString += "         1: {title: ''} \n";
  webString += "       }, \n";

  webString += "         legend: { position: 'bottom' } \n";
  webString += "       }; \n";

  webString += "       var chart = new google.visualization.LineChart(document.getElementById('curve_chart')); \n";

  webString += "       chart.draw(data, options); \n";
  webString += "      } \n";

  ////////////
  webString += "    </script> \n";
  webString += "</head><body>\n";
  webString += "<form action=\"http://" + WiFi.localIP().toString() + "/submit\" method=\"POST\">";
  webString += "<h1>Lizardi-Oteitza IoT Termostatoa 1 </h1>\n";
  webString += "<div style=\"color:red\">" + webMessage + "</div>\n";

  webString += "<table style=\"width:800px;\"><tr><td>";
  webString += "<div>Hozgailua OFF: &le;" + String((int)coolOff) + "&deg;C</div>\n";
  webString += "<div>Hozgailua ON:  &ge;" + String((int)coolOn) + "&deg;C</div>\n";
  webString += "</td><td style=\"text-align:right\">";
  webString += "T&deg min: <input type='text' value='" + String((int)coolOff) + "' name='coolOff' maxlength='3' size='2'><br>\n";
  webString += "T&deg max: <input type='text' value='" + String((int)coolOn) + "' name='coolOn' maxlength='3' size='2'><br>\n";
  webString += "</td><td style=\"text-align:right\">";
  webString += "Laginketa maiztasuna (s): <input type='text' value='" + String((long)interval / 1000) + "' name='sRate' maxlength='3' size='2'><br>\n";
  webString += "Gehienezko Datu Sektorea: <input type='text' value='" + String((long)maxFileData) + "' name='maxData' maxlength='3' size='2'><br>\n";

  webString += "</td><td>";
  webString += "<input type='submit' value='Onartu' >";
  webString += "</td></tr></table>\n";

  webString += "<table style=\"width:800px;\"><tr><td>";
  webString += "<div style=\"width:300px\"><b>Azken Irakurketak</b></div>\n";
  webString += "<div>Tenperatura: " + String(temp_f, 2) + "&deg; C</div>\n";
  webString += "<div>Hezetasuna: " + String((int)humidity) + "%</div> \n";

  if (digitalRead(RELAYPIN) == LOW)
    webString += "<div>Hozgailua ON dago</div>";
  else
    webString += "<div>Hozgailua OFF dago</div>";

 // webString += "<div>Data Lerroak: " + String((int)dataLines) + "</div> \n";
  //  webString += "<div>Total On Time: " + String( ((onCount * interval) / 1000) / 60 ) + " minutes</div> \n";
 // webString += "<div>Laginketa Maiztasuna: " + String(interval / 1000) + " seconds</div> \n";

  webString += "<div><a href=\"http://" + WiFi.localIP().toString() + "\">Eguneratu</a></div>\n";
  webString += "</td><td>";
  webString += "<div id=\"chart_divTemp\" style=\"width: 250px;\"></div>\n";
  webString += "</td><td>";
  webString += "<div id=\"chart_divHumid\" style=\"width: 250px;\"></div>\n";
  webString += "</td></tr></table>\n";
  webString += "<div id=\"curve_chart\" style=\"width: 1000px; height: 500px\"></div> \n";
  webString += "<div><a href=\"/clear\">Clear Data</a></div> \n";

  webString += "</body></html>\n";
}

//////////////////////////////////
///  used for error checking   ///
//////////////////////////////////

boolean isValidNumber(String str) {
  for (byte i = 0; i < str.length(); i++)
  {
    if (isDigit(str.charAt(i))) return true;
  }
  return false;
}

////////////////////////////////////////////////////////////////////
// handler for web server request: http://IpAddress/submit        //
////////////////////////////////////////////////////////////////////

void handle_submit() {

  webMessage = "";
  int tempON = 0;
  int tempOFF = 0;
  long sRate = 0;
  int maxData = 0;

  if (server.args() > 0 ) {
    for ( uint8_t i = 0; i < server.args(); i++ ) {

      if (server.argName(i) == "coolOff") {

        if (server.arg(i) != "")
        {
          if (isValidNumber(server.arg(i)) )
            tempON = server.arg(i).toInt();
          else
            webMessage += "Tenperatura min zenbakia izan behar du<br>";
        }
        else
          webMessage += "Tenperatura max behar da<br>";
      }

      if (server.argName(i) == "coolOn") {

        if (server.arg(i) != "")
        {
          if (isValidNumber(server.arg(i)) )
            tempOFF = server.arg(i).toInt();
          else
            webMessage += "Tenperatura max zenbakia izan behar du<br>";

        }
        else
          webMessage += "Tenperatura max behar da<br>";
      }


      if (server.argName(i) == "sRate") {

        if (server.arg(i) != "")
        {
          if (isValidNumber(server.arg(i)) )
            sRate = server.arg(i).toInt();
          else
            webMessage += "Laginketak zenbakia izan behar du<br>";
        }
        else
          webMessage += "Laginketa sartu behar da<br>";
      }

      if (server.argName(i) == "maxData") {

        if (server.arg(i) != "")
        {
          if (isValidNumber(server.arg(i)) )
            maxData = server.arg(i).toInt();
          else
            webMessage += "Datu segmentuak zenbakia izan behar du<br>";
        }
        else
          webMessage += "Datu segmentua behar da<br>";
      }

    }

    if (tempOFF <= tempON)
      webMessage += "Min Tenp max tenp baina txikiagoa izan behar du <br>";

    if (sRate < 10)
      webMessage += "Laginketak 10 baino haundiagoa izan behar du<br>";

    if (maxData < 10 || maxData > 300)
      webMessage += "MData segmentua 10 eta 300 bitartekoa<br>";

    if (webMessage == "")
    {
      coolOff = tempON;
      coolOn = tempOFF;
      interval = sRate * 1000;
      maxFileData = maxData;
      ///////
      File f = SPIFFS.open(fName, "w");
      if (!f) {

        Serial.println("file open for properties failed");
      }
      else
      {
        Serial.println("====== Writing to config file =========");

        f.print(coolOff); f.print( ","); f.print(coolOn);
        f.print("~"); f.print(sRate);
        f.print(":"); f.println(maxData);
        Serial.println("Properties file updated");
        f.close();
      }

    }
  }


  if (webMessage == "") {
    webMessage = "Parametroak Eguneratuta";
  }


  gettemperature();

  setHTML();
  server.send(200, "text/html", webString);            // send to someones browser when asked

  delay(100);
}


////////////////////////////////////////////////////////////
// handler for web server request: http://IpAddress/      //
////////////////////////////////////////////////////////////

void handle_root() {

  gettemperature();       // read sensor
  webMessage = "";
  setHTML();
  server.send(200, "text/html", webString);            // send to someones browser when asked

  delay(100);
}

///////////////////////////////////////
// first funtion to run at power up //
//////////////////////////////////////

void setup(void)
{
  // You can open the Arduino IDE Serial Monitor window to see what the code is doing
  Serial.begin(115200);  // Serial connection from ESP-01 via 3.3v console cable

  pinMode(RELAYPIN, OUTPUT);
  digitalWrite(RELAYPIN, HIGH);
  // Connect to WiFi network
  WiFi.begin(ssid, password);
  WiFi.config(IPAddress(192, 168, 0, 201), IPAddress(192, 168, 0, 1), IPAddress(255, 255, 255, 0));

  Serial.print("\n\r \n\rWorking to connect");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("DHT Server");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  dht.begin();           // initialize temperature sensor
  delay(10);


  SPIFFS.begin();
  delay(10);
  ///////////////////
  // SPIFFS.format(); // uncomment to completely clear data


  File f = SPIFFS.open(fName, "r");

  if (!f) {
    // no file exists so lets format and create a properties file
    Serial.println("Please wait 30 secs for SPIFFS to be formatted");

    SPIFFS.format();

    Serial.println("Spiffs formatted");

    f = SPIFFS.open(fName, "w");
    if (!f) {

      Serial.println("properties file open failed");
    }
    else
    {
      // write the defaults to the properties file
      Serial.println("====== Writing to properties file =========");

      f.print(coolOff); f.print( ","); f.print(coolOn);
      f.print("~"); f.print(interval / 1000);
      f.print(":"); f.println(maxFileData);
      Serial.println("Properties file created");
      dataLines = 1;
      f.close();
    }

  }
  else
  {
    // if the properties file exists on startup,  read it and set the defaults
    Serial.println("Properties file exists. Reading.");

    while (f.available()) {

      //Lets read line by line from the file
      String str = f.readStringUntil('\n');

      String loSet = str.substring(0, str.indexOf(",")  );
      String hiSet = str.substring(str.indexOf(",") + 1, str.indexOf("~") );
      String sampleRate = str.substring(str.indexOf("~") + 1, str.indexOf(":") );
      String maxData = str.substring(str.indexOf(":") + 1 );

      Serial.println(loSet);
      Serial.println(hiSet);
      Serial.println(sampleRate);
      Serial.println(maxData);

      coolOff = loSet.toInt();
      coolOn = hiSet.toInt();
      interval = sampleRate.toInt() * 1000;
      maxFileData = maxData.toInt();
    }

    f.close();
  }

  // now lets read the data file mainly to set the number of lines
  readDataFile();
  // now read the DHT and set the temp and humidity variables
  gettemperature();
  // update the datafile to start a new session
  updateDataFile();

  // web client handlers
  server.on("/", handle_root);

  server.on("/submit", handle_submit);

  server.on("/clear", []() {
    // handler for http://iPaddress/clear

    // deletes all the stored data for temp and humidity
    clearDataFile();

    webMessage = "Data Cleared";

    // read the DHT and use new values to start new file data
    gettemperature();
    updateDataFile();
    setHTML(); // this will set the webString varialbe with HTML
    server.send(200, "text/html", webString);            // send to someones browser when asked
    delay(100);

  });

  // start the web server
  server.begin();
  Serial.println("HTTP server started");

}

////////////////////////////////////////////////////////////////////////////////

void loop(void)
{

  // check timer to see if it's time to update the data file with another DHT reading
  unsigned long currentMillis = millis();

  // cast to unsigned long to handle rollover
  if ((unsigned long)(currentMillis - previousMillis) >= interval )
  {
    // save the last time you read the sensor
    previousMillis = currentMillis;

    gettemperature();

    Serial.print("Temp: ");
    Serial.println(temp_f);
    Serial.print("Humidity: ");
    Serial.println(humidity);

    updateDataFile();
    readDataFile();
  }

  server.handleClient();

}



