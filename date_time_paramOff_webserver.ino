```cpp
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <time.h>
#include <vector>

const char* ssid = "TrapHouse";
const char* password = "13371337";

ESP8266WebServer server(80);

IPAddress staticIP(192, 168, 0, 109);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

struct Schedule {
  time_t time;
  int intervalDays;
  int intervalHours;
  int duration;
  bool active;
};

std::vector<Schedule> schedules;

void handleRoot() {
  String html = "<html>\
  <head>\
  <title>ESP8266 Web Server</title>\
  </head>\
  <body>\
  <h1>ESP8266 Web Server</h1>\
  <p><a href=\"/on\"><button>Turn On</button></a></p>\
  <p><a href=\"/off\"><button>Turn Off</button></a></p>\
  <p><a href=\"/addevent\"><button>Add Event</button></a></p>\
  <p><a href=\"/viewevents\"><button>View Events</button></a></p>\
  <p><a href=\"/clearevents\"><button>Clear All Events</button></a></p>\
  </body>\
  </html>";
  
  server.send(200, "text/html", html);
}

void handleOn() {
  digitalWrite(12, HIGH);
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleOff() {
  digitalWrite(12, LOW);
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleAddEvent() {
  String html = "<html>\
  <head>\
  <title>Add Event</title>\
  </head>\
  <body>\
  <h1>Add Event</h1>\
  <form action=\"/schedule\" method=\"get\">\
    <label for=\"date\">Select Date:</label>\
    <input type=\"date\" id=\"date\" name=\"date\"><br><br>\
    <label for=\"time\">Select Time:</label>\
    <input type=\"time\" id=\"time\" name=\"time\"><br><br>\
    <label for=\"intervalDays\">Repeat every (days):</label>\
    <input type=\"number\" id=\"intervalDays\" name=\"intervalDays\"><br><br>\
    <label for=\"intervalHours\">Repeat every (hours):</label>\
    <input type=\"number\" id=\"intervalHours\" name=\"intervalHours\"><br><br>\
    <label for=\"duration\">Duration (seconds):</label>\
    <input type=\"number\" id=\"duration\" name=\"duration\"><br><br>\
    <input type=\"submit\" value=\"Add Event\">\
  </form>\
  <p><a href=\"/\"><button>Back</button></a></p>\
  </body>\
  </html>";
  
  server.send(200, "text/html", html);
}

void handleSchedule() {
  String date = server.arg("date");
  String time = server.arg("time");
  String intervalDaysStr = server.arg("intervalDays");
  String intervalHoursStr = server.arg("intervalHours");
  String durationStr = server.arg("duration");

  if (date.length() > 0 && time.length() > 0) {
    String dateTime = date + " " + time + ":00";
    struct tm tm;
    strptime(dateTime.c_str(), "%Y-%m-%d %H:%M:%S", &tm);
    time_t scheduledTime = mktime(&tm);
    int intervalDays = intervalDaysStr.length() > 0 ? intervalDaysStr.toInt() : 0;
    int intervalHours = intervalHoursStr.length() > 0 ? intervalHoursStr.toInt() : 0;
    int duration = durationStr.length() > 0 ? durationStr.toInt() : 0;

    Schedule schedule = {scheduledTime, intervalDays, intervalHours, duration, true};
    schedules.push_back(schedule);

    Serial.print("Scheduled time (epoch): ");
    Serial.println(scheduledTime);
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleViewEvents() {
  String html = "<html>\
  <head>\
  <title>View Events</title>\
  </head>\
  <body>\
  <h1>Planned Events</h1>";

  for (size_t i = 0; i < schedules.size(); i++) {
    html += "<p>Event " + String(i+1) + ": ";
    html += "Time: " + String(schedules[i].time);
    html += ", Interval (days): " + String(schedules[i].intervalDays);
    html += ", Interval (hours): " + String(schedules[i].intervalHours);
    html += ", Duration: " + String(schedules[i].duration) + " seconds";
    html += ", Active: " + String(schedules[i].active ? "Yes" : "No");
    html += " <a href=\"/deleteevent?id=" + String(i) + "\"><button>Delete</button></a></p>";
  }
  
  html += "<p><a href=\"/\"><button>Back</button></a></p>\
  </body>\
  </html>";

  server.send(200, "text/html", html);
}

void handleDeleteEvent() {
  int id = server.arg("id").toInt();
  if (id >= 0 && id < schedules.size()) {
    schedules.erase(schedules.begin() + id);
  }
  server.sendHeader("Location", "/viewevents");
  server.send(303);
}

void handleClearEvents() {
  schedules.clear();
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  Serial.begin(115200);
  pinMode(12, OUTPUT);
  digitalWrite(12, LOW);
  
  if (!WiFi.config(staticIP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/addevent", handleAddEvent);
  server.on("/schedule", handleSchedule);
  server.on("/viewevents", handleViewEvents);
  server.on("/deleteevent", handleDeleteEvent);
  server.on("/clearevents", handleClearEvents);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();

  time_t now = time(nullptr) + 10800-2;

  for (auto &schedule : schedules) {
    Serial.println("Now sced");
    Serial.println(now);
    Serial.println(schedule.time);
    if (schedule.active && schedule.time > 0 && now >= schedule.time) {
      digitalWrite(12, HIGH);

      if (schedule.duration > 0) {
        delay(schedule.duration * 1000);
        digitalWrite(12, LOW);
      }

      if (schedule.intervalDays > 0) {
        schedule.time += schedule.intervalDays * 86400;
      } else if (schedule.intervalHours > 0) {
        schedule.time += schedule.intervalHours * 3600;
      } else {
        schedule.active = false;
      }
    }
  }

  schedules.erase(
    std::remove_if(schedules.begin(), schedules.end(), [](Schedule &s) { return !s.active; }),
    schedules.end()
  );
}
```
}


