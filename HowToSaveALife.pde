import processing.serial.*;

Serial myPort;  // Create object from Serial class
String val;     // Data received from the serial port

String a1 = "Compression rate too low";
String a2 = "Compression rate too high";
String a3 = "Too few compressions";
String a4 = "Too many compressions";
String b1 = "Too fast rescue breaths";
String b2 = "Too slow rescue breaths";
String b3 = "Too few rescue breaths";
String b4 = "Too many rescue breaths";
String b5 = "Don't forget to tilt the chin";

PFont f;
PFont f_bold;

String feedback = "";

boolean update_draw = true;

void setup() {
  size(1650, 950);
  f = createFont("TimesNewRomanPSMT", 64, true);
  f_bold = createFont("TimesNewRomanPS-BoldMT", 64, true);
  myPort = new Serial(this, Serial.list()[9], 9600);
  myPort.bufferUntil('\n');
  noLoop();
}

void draw() {
  background(0);

  if (feedback.equals("call112Reminder")) {
    textFont(f_bold, 40);
    fill(255);
    textAlign(CENTER, CENTER);
    text("Call 112 before starting CPR", 825, 380);
  } else if (feedback.equals("ambulanceArrived")) {
    textFont(f_bold, 40);
    fill(255);
    textAlign(CENTER, CENTER);
    text("The ambulance is here and the person is now breathing.\nFor more information about how to save a life visit: \nhttps://www.nhs.uk/conditions/first-aid/cpr/", 825, 380);
  } else if (feedback.equals("s")) {
    textFont(f_bold, 60);
    fill(255);
    textAlign(CENTER, CENTER);
    text("You just saved a life. Well done.\nFor more information about how to save a life visit: \nhttps://www.nhs.uk/conditions/first-aid/cpr/", 825, 380);
  } else if (feedback.equals("clearReminder")) {   // display nothing
  } else {
    textFont(f_bold, 90);
    fill(255);
    textAlign(CENTER, CENTER);
    stroke(255);
    line(450, 260, 1218, 260);
    text("Feedback", 850, 200);

    textFont(f, 70);
    fill(200);
    textAlign(CENTER, CENTER);
    text(feedback, 850, 450);
  }
  feedback = "";
}

void serialEvent(Serial myPort) {
  val = myPort.readStringUntil('\n');
  String[ ] list = split(val, ',');
  update_draw = false;

  for (int i = 0; i < list.length; i++) {
    if (list[i] != null) {
      val = trim(list[i]);
      if (val.equals("a1")) {
        feedback += a1 + "\n";
        update_draw = true;
      } else if (val.equals("a2")) {
        feedback += a2 + "\n";
        update_draw = true;
      } else if (val.equals("a3")) {
        feedback += a3 + "\n";
        update_draw = true;
      } else if (val.equals("a4")) {
        feedback += a4 + "\n";
        update_draw = true;
      } else if (val.equals("b1")) {
        feedback += b1 + "\n";
        update_draw = true;
      } else if (val.equals("b2")) {
        feedback += b2 + "\n";
        update_draw = true;
      } else if (val.equals("b3")) {
        feedback += b3 + "\n";
        update_draw = true;
      } else if (val.equals("b4") ) {
        feedback += b4 + "\n";
        update_draw = true;
      } else if (val.equals("b5")) {
        feedback += b5 + "\n";
        update_draw = true;
      } else if (val.equals("ambulanceArrived")) {
        feedback = "ambulanceArrived";
        update_draw = true;
      } else if (val.equals("breathing")) {
        feedback = "breathing";
        update_draw = true;
      } else if (val.equals("call112Reminder")) {
        feedback = "call112Reminder";
        update_draw = true;
      } else if (val.equals("clearReminder")) {
        feedback = "clearReminder";
        update_draw = true;
      }
    }
  }
  if (update_draw) {
    redraw();
  }
}
